///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmLuaStimulator.hpp
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

#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/noncopyable.hpp>

#include <map>
#include <vector>
#include <cstdio>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
class CBoxAlgorithmLuaStimulator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>, public boost::noncopyable
{
public:
    CBoxAlgorithmLuaStimulator();
    ~CBoxAlgorithmLuaStimulator() override = default;

    void release() override { delete this; }

    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_LuaStimulator)

    bool WaitForStimulation(size_t inputIdx, size_t stimIdx);

    bool GetInputCountCB(size_t& count);
    bool GetOutputCountCB(size_t& count);
    bool GetSettingCountCB(size_t& count);
    bool GetSettingCB(size_t index, CString& setting);
    bool GetConfigCB(const CString& str, CString& config);

    bool GetCurrentTimeCB(uint64_t& time);
    bool SleepCB();
    bool GetStimulationCountCB(size_t index, size_t& count);
    bool GetStimulationCB(size_t inputIdx, size_t stimIdx, uint64_t& id, uint64_t& time, uint64_t& duration);
    bool RemoveStimulationCB(size_t inputIdx, size_t stimIdx);
    bool SendStimulationCB(size_t outputIdx, uint64_t id, uint64_t time, uint64_t duration);
    bool Log(Kernel::ELogLevel level, const CString& text);

    void DoThread();

    enum class EStates
    {
        Unstarted,
        Processing,
        Please_Quit,
        Finished,
    };

    static std::string ToString(const EStates state);

    EStates m_State          = EStates::Unstarted;
    bool m_LuaThreadHadError = false;       // Set to true by the lua thread if there were issues
    bool m_FilterMode        = false;            // Output chunk generation driven by inputs (true) or by clock (false)

protected:
    lua_State* m_luaState = nullptr;

    uint64_t m_lastTime = 0;
    std::vector<std::multimap<uint64_t, std::pair<uint64_t, uint64_t>>> m_oStimulations;
    std::vector<std::multimap<uint64_t, std::pair<uint64_t, uint64_t>>> m_iStimulations;

    boost::thread* m_luaThread = nullptr;
    boost::mutex m_mutex;
    boost::mutex::scoped_lock m_innerLock;
    boost::mutex::scoped_lock m_outerLock;
    boost::mutex::scoped_lock m_exitLock;
    boost::condition m_condition;
    boost::condition m_exitCondition;

    std::vector<Toolkit::TStimulationDecoder<CBoxAlgorithmLuaStimulator>*> m_decoders;
    std::vector<Toolkit::TStimulationEncoder<CBoxAlgorithmLuaStimulator>*> m_encoders;

private:
    bool runLuaThread();
    bool sendStimulations(uint64_t startTime, uint64_t endTime);
};

class CBoxAlgorithmLuaStimulatorListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_Stimulations);
        return true;
    }

    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputType(index, OV_TypeId_Stimulations);
        return true;
    }

    bool onSettingAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setSettingType(index, OV_TypeId_String);
        box.setSettingName(index, ("Setting " + std::to_string(index)).c_str());
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmLuaStimulatorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Lua Stimulator"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Generates some stimulations according to an Lua script"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Scripting"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-missing-image"; }

    CIdentifier getCreatedClass() const override { return Box_LuaStimulator; }
    IPluginObject* create() override { return new CBoxAlgorithmLuaStimulator; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmLuaStimulatorListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Lua Script", OV_TypeId_Script, "");

        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        prototype.addFlag(Kernel::BoxFlag_CanModifySetting);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_LuaStimulatorDesc)
};
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
