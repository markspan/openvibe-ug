///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmLuaStimulator.cpp
/// \author Yann Renard (Inria).
/// \version 1.1.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmLuaStimulator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

#define CB_ASSERT_(condition) \
    if(!(condition)) { \
        lua_pushstring(state, "Assertion failed : ["#condition"]"); \
        lua_error(state); \
        return 0; \
    }

namespace {
class CLuaThread
{
public:
    explicit CLuaThread(CBoxAlgorithmLuaStimulator* pBox) : m_box(pBox) { }
    CLuaThread(const CLuaThread& thread) : m_box(thread.m_box) { }
    void operator()() const { m_box->DoThread(); }

protected:
    CBoxAlgorithmLuaStimulator* m_box;
};
}  // namespace

static void LuaSetCB(lua_State* state, const char* name, int (*callback)(lua_State*), void* data)
{
    lua_getglobal(state, "__openvibe_box_context");
    lua_pushlightuserdata(state, data);
    lua_pushcclosure(state, callback, 1);
    lua_setfield(state, -2, name);
}

// Returns true on success, i.e. if code is 0 (no failure).
static bool LuaReport(Kernel::ILogManager& logManager, lua_State* state, const int code)
{
    if (code) {
        logManager << Kernel::LogLevel_ImportantWarning << "Lua error: " << lua_tostring(state, -1) << "\n";
        lua_pop(state, 1);
    }
    return (code == 0);
}

static bool LuaCheckArgumentCount(lua_State* state, const char* name, const int count1, const int count2 = -1)
{
    if (count2 < 0) {
        if (lua_gettop(state) != count1 + 1) {
            const std::string msg = "[" + std::string(name) + "] takes " + std::to_string(count1) + " parameter" + (count1 >= 2 ? "s" : "");
            lua_pushstring(state, msg.c_str());
            lua_error(state);
            return false;
        }
    }
    else {
        if (lua_gettop(state) != count1 + 1 && lua_gettop(state) != count2 + 1) {
            const std::string msg = "[" + std::string(name) + "] takes either " + std::to_string(count1) + " or " + std::to_string(count2) + " parameter"
                                    + (count2 >= 2 ? "s" : "");
            lua_pushstring(state, msg.c_str());
            lua_error(state);
            return false;
        }
    }
    return true;
}

static bool LuaCheckBoxStatus(lua_State* state, const char* name, const CBoxAlgorithmLuaStimulator::EStates curState)
{
    if (curState == CBoxAlgorithmLuaStimulator::EStates::Please_Quit) {
        const std::string msg = "[" + std::string(name) + "] This thread has been requested to quit...";
        lua_pushstring(state, msg.c_str());
        lua_error(state);
        return false;
    }
    if (curState != CBoxAlgorithmLuaStimulator::EStates::Processing) {
        const std::string msg = "[" + std::string(name) + "] should only be called in the [process] callback";
        lua_pushstring(state, msg.c_str());
        lua_error(state);
        return false;
    }
    return true;
}

static int LuaGetInputCountCB(lua_State* state)
{
    size_t nInput  = 0;
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "get_input_count", 0)) { return 0; }

    CB_ASSERT_(ptr->GetInputCountCB(nInput));
    lua_pushinteger(state, int(nInput));
    return 1;
}

static int LuaGetOutputCountCB(lua_State* state)
{
    size_t nOutput = 0;
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "get_output_count", 0)) { return 0; }

    CB_ASSERT_(ptr->GetOutputCountCB(nOutput));
    lua_pushinteger(state, int(nOutput));
    return 1;
}

static int LuaGetSettingCountCB(lua_State* state)
{
    size_t nSetting = 0;
    const auto ptr  = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "get_setting_count", 0)) { return 0; }

    CB_ASSERT_(ptr->GetSettingCountCB(nSetting));
    lua_pushinteger(state, int(nSetting));
    return 1;
}

static int LuaGetSettingCB(lua_State* state)
{
    CString setting;
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "get_setting", 1)) { return 0; }

    CB_ASSERT_(ptr->GetSettingCB(size_t(lua_tointeger(state, 2)-1), setting));
    lua_pushstring(state, setting.toASCIIString());
    return 1;
}

static int LuaGetConfigCB(lua_State* state)
{
    CString config;
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "get_config", 1)) { return 0; }

    CB_ASSERT_(ptr->GetConfigCB(lua_tostring(state, 2), config));
    lua_pushstring(state, config.toASCIIString());
    return 1;
}

static int LuaGetCurrentTimeCB(lua_State* state)
{
    uint64_t time  = 0;
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "get_current_time", 0)) { return 0; }

    CB_ASSERT_(ptr->GetCurrentTimeCB(time));
    lua_pushnumber(state, CTime(time).toSeconds());
    return 1;
}

static int LuaSleepCB(lua_State* state)
{
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckBoxStatus(state, "sleep", ptr->m_State)) { return 0; }
    if (!LuaCheckArgumentCount(state, "sleep", 0)) { return 0; }

    CB_ASSERT_(ptr->SleepCB());
    return 0;
}

static int LuaGetStimulationCountCB(lua_State* state)
{
    size_t count   = 0;
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckBoxStatus(state, "get_stimulation_count", ptr->m_State)) { return 0; }
    if (!LuaCheckArgumentCount(state, "get_stimulation_count", 1)) { return 0; }

    CB_ASSERT_(ptr->GetStimulationCountCB(size_t(lua_tointeger(state, 2)-1), count));
    lua_pushinteger(state, int(count));
    return 1;
}

static int LuaGetStimulationCB(lua_State* state)
{
    uint64_t id       = uint64_t(-1);
    uint64_t time     = uint64_t(-1);
    uint64_t duration = uint64_t(-1);
    const auto ptr    = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckBoxStatus(state, "get_stimulation", ptr->m_State)) { return 0; }
    if (!LuaCheckArgumentCount(state, "get_stimulation", 2)) { return 0; }

    CB_ASSERT_(ptr->GetStimulationCB(size_t(lua_tointeger(state, 2)-1), size_t(lua_tointeger(state, 3)-1), id, time, duration));
    lua_pushinteger(state, lua_Integer(id));
    lua_pushnumber(state, CTime(time).toSeconds());
    lua_pushnumber(state, CTime(duration).toSeconds());
    return 3;
}

static int LuaRemoveStimulationCB(lua_State* state)
{
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckBoxStatus(state, "remove_stimulation", ptr->m_State)) { return 0; }
    if (!LuaCheckArgumentCount(state, "remove_stimulation", 2)) { return 0; }

    CB_ASSERT_(ptr->RemoveStimulationCB(size_t(lua_tointeger(state, 2)-1), size_t(lua_tointeger(state, 3)-1)));
    return 0;
}

static int LuaSendStimulationCB(lua_State* state)
{
    const int iArguments = lua_gettop(state);
    const auto ptr       = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckBoxStatus(state, "send_stimulation", ptr->m_State)) { return 0; }
    if (!LuaCheckArgumentCount(state, "send_stimulation", 3, 4)) { return 0; }

    CB_ASSERT_(ptr->SendStimulationCB(size_t(lua_tointeger(state, 2)-1), size_t(lua_tointeger(state, 3)),
        CTime(double(lua_tonumber(state, 4))).time(),
        (iArguments == 5 ? CTime(double(lua_tonumber(state, 5))).time() : 0)));
    return 0;
}

// Has the side effect of setting m_bLuaThreadHadError to true if called with "Error" or "Fatal" loglevels.
static int LuaLogCB(lua_State* state)
{
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "log", 2)) { return 0; }

    Kernel::ELogLevel level = Kernel::LogLevel_Debug;
    const std::string str(lua_tostring(state, 2));
    if (str == "Trace") { level = Kernel::LogLevel_Trace; }
    else if (str == "Info") { level = Kernel::LogLevel_Info; }
    else if (str == "Warning") { level = Kernel::LogLevel_Warning; }
    else if (str == "ImportantWarning") { level = Kernel::LogLevel_ImportantWarning; }
    else if (str == "Error") {
        level                    = Kernel::LogLevel_Error;
        ptr->m_LuaThreadHadError = true;
    }
    else if (str == "Fatal") {
        level                    = Kernel::LogLevel_Fatal;
        ptr->m_LuaThreadHadError = true;
    }
    CB_ASSERT_(ptr->Log(level, lua_tostring(state, 3)));

    return 0;
}

static int LuaKeepProcessingCB(lua_State* state)
{
    const auto pThis = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(pThis != nullptr);
    if (pThis->m_State == CBoxAlgorithmLuaStimulator::EStates::Processing) { return 1; }
    return 0;
}

static int LuaSetFilterModeCB(lua_State* state)
{
    const auto ptr = static_cast<CBoxAlgorithmLuaStimulator*>(lua_touserdata(state, lua_upvalueindex(1)));
    CB_ASSERT_(ptr != nullptr);

    if (!LuaCheckArgumentCount(state, "set_filter_mode", 1)) { return 0; }

    ptr->m_FilterMode = (lua_tointeger(state, 2) == 0 ? false : true);

    return 0;
}

#if BOOST_VERSION >= 103500
CBoxAlgorithmLuaStimulator::CBoxAlgorithmLuaStimulator()
    : m_innerLock(m_mutex, boost::defer_lock), m_outerLock(m_mutex, boost::defer_lock), m_exitLock(m_mutex, boost::defer_lock) { }
#else
CBoxAlgorithmLuaStimulator::CBoxAlgorithmLuaStimulator()
    : m_innerLock(m_mutex, false), m_outerLock(m_mutex, false), m_exitLock(m_mutex, false) { }
#endif

uint64_t CBoxAlgorithmLuaStimulator::getClockFrequency()
{
    if (m_FilterMode) { return 0; }     // Only when input received
    return 128LL << 32;                 // the box clock frequency
}

bool CBoxAlgorithmLuaStimulator::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (!m_FilterMode) { this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }
    return true;
}

bool CBoxAlgorithmLuaStimulator::processInput(const size_t /*index*/)
{
    if (m_FilterMode) { getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }
    return true;
}

bool CBoxAlgorithmLuaStimulator::initialize()
{
    size_t i;
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    CString filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    filename         = this->getConfigurationManager().expand(filename);

    m_FilterMode = false;

    m_State    = EStates::Unstarted;
    m_luaState = luaL_newstate();

    luaL_openlibs(m_luaState);

    lua_newtable(m_luaState);
    lua_setglobal(m_luaState, "__openvibe_box_context");

    LuaSetCB(m_luaState, "get_input_count", LuaGetInputCountCB, this);
    LuaSetCB(m_luaState, "get_output_count", LuaGetOutputCountCB, this);
    LuaSetCB(m_luaState, "get_setting_count", LuaGetSettingCountCB, this);
    LuaSetCB(m_luaState, "get_setting", LuaGetSettingCB, this);
    LuaSetCB(m_luaState, "get_config", LuaGetConfigCB, this);
    LuaSetCB(m_luaState, "get_current_time", LuaGetCurrentTimeCB, this);
    LuaSetCB(m_luaState, "sleep", LuaSleepCB, this);
    LuaSetCB(m_luaState, "get_stimulation_count", LuaGetStimulationCountCB, this);
    LuaSetCB(m_luaState, "get_stimulation", LuaGetStimulationCB, this);
    LuaSetCB(m_luaState, "remove_stimulation", LuaRemoveStimulationCB, this);
    LuaSetCB(m_luaState, "send_stimulation", LuaSendStimulationCB, this);
    LuaSetCB(m_luaState, "log", LuaLogCB, this);
    LuaSetCB(m_luaState, "keep_processing", LuaKeepProcessingCB, this);
    LuaSetCB(m_luaState, "set_filter_mode", LuaSetFilterModeCB, this);

    if (!LuaReport(this->getLogManager(), m_luaState, luaL_dostring(m_luaState, "function initialize(box) end"))) { return false; }
    if (!LuaReport(this->getLogManager(), m_luaState, luaL_dostring(m_luaState, "function uninitialize(box) end"))) { return false; }
    if (!LuaReport(this->getLogManager(), m_luaState, luaL_dostring(m_luaState, "function process(box) end"))) { return false; }
    if (!LuaReport(this->getLogManager(), m_luaState, luaL_dofile(m_luaState, filename.toASCIIString()))) { return false; }
    if (!LuaReport(this->getLogManager(), m_luaState, luaL_dostring(m_luaState, "initialize(__openvibe_box_context)"))) { return false; }

    m_iStimulations.resize(boxContext.getInputCount());
    m_oStimulations.resize(boxContext.getOutputCount());

    for (i = 0; i < boxContext.getInputCount(); ++i) {
        Toolkit::TStimulationDecoder<CBoxAlgorithmLuaStimulator>* decoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmLuaStimulator>;
        decoder->initialize(*this, i);

        m_decoders.push_back(decoder);
    }

    for (i = 0; i < boxContext.getOutputCount(); ++i) {
        Toolkit::TStimulationEncoder<CBoxAlgorithmLuaStimulator>* encoder = new Toolkit::TStimulationEncoder<CBoxAlgorithmLuaStimulator>;
        encoder->initialize(*this, i);

        m_encoders.push_back(encoder);
    }

    m_lastTime          = 0;
    m_luaThread         = nullptr;
    m_LuaThreadHadError = false;

    return true;
}

bool CBoxAlgorithmLuaStimulator::uninitialize()
{
    if (m_luaThread) {
        m_outerLock.lock();

        // If Lua thread is still running, ask it to stop.
        if (m_State == EStates::Processing || m_State == EStates::Please_Quit) {
            this->getLogManager() << Kernel::LogLevel_Debug << "Requesting thread to quit, waiting max 5 secs ...\n";

            m_State = EStates::Please_Quit;
            m_condition.notify_one();

            m_outerLock.unlock();
            m_exitLock.lock();

            bool gotLock = false;
            for (int i = 0; i < 5; ++i) {
                // Wait for the thread to stop (in that case it notifies the lock)
                boost::system_time timeout = boost::get_system_time() + boost::posix_time::milliseconds(1000);
                if (m_exitCondition.timed_wait(m_exitLock, timeout)) {
                    gotLock = true;
                    break;
                }
                this->getLogManager() << Kernel::LogLevel_Info << "Waiting for thread to exit, " << i + 1 << "/5 ...\n";
            }
            if (gotLock) { this->getLogManager() << Kernel::LogLevel_Debug << "Ok, thread notified the exit lock\n"; }
            else { this->getLogManager() << Kernel::LogLevel_Debug << "Bad, thread did not notify the exit lock in 5s\n"; }

            m_exitLock.unlock();
            m_outerLock.lock();
        }

        if (m_State == EStates::Finished) {
            this->getLogManager() << Kernel::LogLevel_Debug << "Ok, thread reached Finished as expected ...\n";
            m_luaThread->join();
            delete m_luaThread;
            m_luaThread = nullptr;
        }
        else { this->getLogManager() << Kernel::LogLevel_Warning << "Bad, thread still in state " << ToString(m_State) << ", cannot delete. Memory leak.\n"; }

        m_outerLock.unlock();
    }

    if (m_luaState) {
        if (!LuaReport(this->getLogManager(), m_luaState, luaL_dostring(m_luaState, "uninitialize(__openvibe_box_context)"))) {
            this->getLogManager() << Kernel::LogLevel_Warning << "Lua script uninitialize failed\n";
            // Don't return here on false, still want to free the resources below
        }
        lua_close(m_luaState);
        m_luaState = nullptr;
    }
    /* TODO: Remove next comment*/
    /*IBox& staticBoxContext=*/
    this->getStaticBoxContext();

    for (size_t i = 0; i < m_decoders.size(); ++i) {
        m_decoders[i]->uninitialize();
        delete m_decoders[i];
    }
    m_decoders.clear();

    for (size_t i = 0; i < m_encoders.size(); ++i) {
        m_encoders[i]->uninitialize();
        delete m_encoders[i];
    }
    m_encoders.clear();


    return true;
}

bool CBoxAlgorithmLuaStimulator::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();
    const uint64_t time        = this->getPlayerContext().getCurrentTime();

    // Default time range for the generated stimulation chunk
    uint64_t start = (m_FilterMode ? std::numeric_limits<uint64_t>::max() : m_lastTime);
    uint64_t end   = (m_FilterMode ? 0 : time);

    if (time == 0) {
        // Send all headers
        for (size_t i = 0; i < m_encoders.size(); ++i) {
            m_encoders[i]->encodeHeader();
            boxContext.markOutputAsReadyToSend(size_t(i), 0, 0);
        }
    }

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            m_decoders[i]->decode(j);
            if (m_decoders[i]->isHeaderReceived()) { }
            if (m_decoders[i]->isBufferReceived()) {
                const CStimulationSet* stimSet = m_decoders[i]->getOutputStimulationSet();

                for (size_t k = 0; k < stimSet->size(); ++k) {
                    m_iStimulations[i].insert(std::make_pair(stimSet->getDate(k),
                                                             std::make_pair(stimSet->getId(k),
                                                                            stimSet->getDuration(k))));
                }

                if (m_FilterMode) {
                    // In this mode, the output chunk time range contains all the current input chunk time ranges
                    start = std::min<uint64_t>(start, boxContext.getInputChunkStartTime(i, j));
                    end   = std::max<uint64_t>(end, boxContext.getInputChunkEndTime(i, j));
                    if (start < m_lastTime) {
                        this->getLogManager() << Kernel::LogLevel_Warning << "Earliest current input chunk start time "
                                << CTime(start) << " is older than the last sent block end time " << CTime(m_lastTime) << "\n";
                    }
                }
            }
            if (m_decoders[i]->isEndReceived()) { }
            boxContext.markInputAsDeprecated(i, j);
        }
    }

    runLuaThread();

    // Send unless endtime=0 (then either we're at header time, or no chunks were received in filter mode)
    if (end > 0) { sendStimulations(start, end); }

    if (!m_luaThread && m_LuaThreadHadError) {
        // Lua thread has exit, so it was ok to check m_bLuaThreadHadError without a lock
        return false;
    }

    return true;
}

bool CBoxAlgorithmLuaStimulator::runLuaThread()
{
    m_outerLock.lock();

    // Executes one step of the thread
    switch (m_State) {
        case EStates::Unstarted:
            m_State = EStates::Processing;
            m_luaThread = new boost::thread(CLuaThread(this));
            m_condition.wait(m_outerLock);
            break;

        case EStates::Processing:
            m_condition.notify_one();
            m_condition.wait(m_outerLock);
            break;

        case EStates::Please_Quit:
            // Shouldn't happen as only uninitialize() sets this
            break;

        case EStates::Finished:
            break;

        default:
            break;
    }

    // Executes one step of the box once the thread has finished
    if (m_luaThread) {
        switch (m_State) {
            case EStates::Unstarted:
                break; // This should never happen

            case EStates::Processing:
                break;

            case EStates::Please_Quit:
                // Shouldn't happen as only uninitialize() sets this
                break;

            case EStates::Finished:
                if (m_luaThread) {
                    m_luaThread->join();
                    delete m_luaThread;
                    m_luaThread = nullptr;
                    this->getLogManager() << Kernel::LogLevel_Info << "Lua script terminated\n";
                }
                break;

            default:
                break;
        }
    }

    m_outerLock.unlock();

    return true;
}

bool CBoxAlgorithmLuaStimulator::sendStimulations(const uint64_t startTime, const uint64_t endTime)
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nOutput       = this->getStaticBoxContext().getOutputCount();

    for (size_t i = 0; i < nOutput; ++i) {
        const CStimulationSet* stimSet = m_encoders[i]->getInputStimulationSet();
        stimSet->clear();

        auto it = m_oStimulations[i].begin();
        while (it != m_oStimulations[i].end() && it->first < endTime) {
            const auto itStim = it;
            ++it;

            stimSet->push_back(itStim->second.first, itStim->first, itStim->second.second);
            this->getLogManager() << Kernel::LogLevel_Debug << "On output " << i << " - should send stimulation " << itStim->second.first << " at date "
                    << CTime(itStim->first) << " with duration " << itStim->second.second << "\n";

            m_oStimulations[i].erase(itStim);
        }

        m_encoders[i]->encodeBuffer();

        boxContext.markOutputAsReadyToSend(i, startTime, endTime);
    }

    m_lastTime = endTime;

    return true;
}

bool CBoxAlgorithmLuaStimulator::WaitForStimulation(const size_t inputIdx, const size_t stimIdx)
{
    if (inputIdx >= m_iStimulations.size()) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Input " << (inputIdx + 1) << " does not exist\n";
        return false;
    }

    while (m_iStimulations[inputIdx].size() <= stimIdx) {
        switch (m_State) {
            case EStates::Unstarted: return false; // this should never happen
            case EStates::Finished: return false;
            case EStates::Please_Quit: return false;
            case EStates::Processing: this->SleepCB();
                break;
            default: break;
        }
    }

    return true;
}

bool CBoxAlgorithmLuaStimulator::GetInputCountCB(size_t& count)
{
    count = this->getStaticBoxContext().getInputCount();
    return true;
}

bool CBoxAlgorithmLuaStimulator::GetOutputCountCB(size_t& count)
{
    count = this->getStaticBoxContext().getOutputCount();
    return true;
}

bool CBoxAlgorithmLuaStimulator::GetSettingCountCB(size_t& count)
{
    count = this->getStaticBoxContext().getSettingCount();
    return true;
}

bool CBoxAlgorithmLuaStimulator::GetSettingCB(const size_t index, CString& setting)
{
    if (index >= this->getStaticBoxContext().getSettingCount()) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Setting " << (index + 1) << " does not exist\n";
        setting = CString("");
        return true;
    }

    setting = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), index);

    return true;
}

bool CBoxAlgorithmLuaStimulator::GetConfigCB(const CString& str, CString& config)
{
    config = this->getConfigurationManager().expand(str);
    return true;
}

bool CBoxAlgorithmLuaStimulator::GetCurrentTimeCB(uint64_t& time)
{
    time = this->getPlayerContext().getCurrentTime();
    return true;
}

bool CBoxAlgorithmLuaStimulator::SleepCB()
{
    m_condition.notify_one();
    m_condition.wait(m_innerLock);
    return true;
}

bool CBoxAlgorithmLuaStimulator::GetStimulationCountCB(const size_t index, size_t& count)
{
    if (index >= m_iStimulations.size()) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Input " << (index + 1) << " does not exist\n";
        count = 0;
        return true;
    }

    count = size_t(m_iStimulations[index].size());
    return true;
}

bool CBoxAlgorithmLuaStimulator::GetStimulationCB(const size_t inputIdx, const size_t stimIdx, uint64_t& id, uint64_t& time, uint64_t& duration)
{
    if (!this->WaitForStimulation(inputIdx, stimIdx)) {
        id       = uint64_t(-1);
        time     = uint64_t(-1);
        duration = uint64_t(-1);
        return true;
    }

    auto it = m_iStimulations[inputIdx].begin();

    for (size_t i = 0; i < stimIdx; i++, ++it) {}

    id       = it->second.first;
    time     = it->first;
    duration = it->second.second;

    this->getLogManager() << Kernel::LogLevel_Debug << "getStimulationCB " << (inputIdx + 1) << " " << (stimIdx + 1) << " " << id << " " << time << " "
            << duration << "\n";

    return true;
}

bool CBoxAlgorithmLuaStimulator::RemoveStimulationCB(const size_t inputIdx, const size_t stimIdx)
{
    if (!this->WaitForStimulation(inputIdx, stimIdx)) { return true; }

    auto it = m_iStimulations[inputIdx].begin();

    for (size_t i = 0; i < stimIdx; i++, ++it) {}

    m_iStimulations[inputIdx].erase(it);

    this->getLogManager() << Kernel::LogLevel_Debug << "removeStimulationCB " << (inputIdx + 1) << " " << (stimIdx + 1) << "\n";

    return true;
}

bool CBoxAlgorithmLuaStimulator::SendStimulationCB(const size_t outputIdx, uint64_t id, uint64_t time, uint64_t duration)
{
    if (outputIdx < m_oStimulations.size()) {
        if (time >= m_lastTime) {
            this->getLogManager() << Kernel::LogLevel_Debug << "sendStimulationCB " << (outputIdx + 1) << " " << id << " " << time << " " << duration << "\n";
            m_oStimulations[outputIdx].insert(std::make_pair(time, std::make_pair(id, duration)));
        }
        else {
            this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Ignored outdated stimulation " << id << " " << CTime(time) << " " << CTime(duration)
                    << " sent on output " << (outputIdx + 1) << " - older than last chunk end time " << CTime(m_lastTime) << "\n";
        }
    }
    else {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Ignored stimulation " << id << " " << CTime(time) << " " << CTime(duration)
                << " sent on unexistant output " << (outputIdx + 1) << "\n";
    }
    return true;
}

bool CBoxAlgorithmLuaStimulator::Log(const Kernel::ELogLevel level, const CString& text)
{
    this->getLogManager() << level << "<" << Kernel::LogColor_PushStateBit << Kernel::LogColor_ForegroundGreen
            << "In Script" << Kernel::LogColor_PopStateBit << "> " << text.toASCIIString() << "\n";
    return true;
}

void CBoxAlgorithmLuaStimulator::DoThread()
{
    m_innerLock.lock();
    if (!LuaReport(this->getLogManager(), m_luaState, luaL_dostring(m_luaState, "process(__openvibe_box_context)"))) { m_LuaThreadHadError = true; }
    m_State = EStates::Finished;
    m_innerLock.unlock();

    m_condition.notify_one();
    m_exitCondition.notify_one();
}

std::string CBoxAlgorithmLuaStimulator::ToString(const EStates state)
{
    switch (state) {
        case EStates::Unstarted: return "Unstarted";
        case EStates::Processing: return "Processing";
        case EStates::Please_Quit: return "Please_Quit";
        case EStates::Finished: return "Finished";
        default: return "Invalid";
    }
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
