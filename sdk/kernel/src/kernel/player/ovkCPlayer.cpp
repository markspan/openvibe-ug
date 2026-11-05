#include "ovkCPlayer.h"
#include "ovkCSimulatedBox.h"
#include "ovkCScheduler.h"

#include "../configuration/ovkCConfigurationManager.h"
#include "../scenario/ovkCScenarioManager.h"
#include "../scenario/ovkCScenarioSettingKeywordParserCallback.h"

#include <system/ovCTime.h>

#include <xml/IReader.h>

#include <string>
#include <iostream>
#include <fstream>

namespace OpenViBE {
namespace Kernel {

const uint64_t SCHEDULER_DEFAULT_FREQUENCY      = 128;
const uint64_t SCHEDULER_MAXIMUM_LOOPS_DURATION = (100LL << 22); /* 100/1024 seconds, approx 100ms */

//___________________________________________________________________//
//                                                                   //

CPlayer::CPlayer(const IKernelContext& ctx)
    : TKernelObject<IPlayer>(ctx), m_kernelCtxBridge(ctx), m_scheduler(m_kernelCtxBridge, *this)
{
    uint64_t schedulerFrequency = this->TKernelObject<IPlayer>::getConfigurationManager().expandAsUInteger("${Kernel_PlayerFrequency}");
    if (schedulerFrequency == 0)
    {
        OV_WARNING_K(std::string("Invalid frequency configuration Kernel_PlayerFrequency = ")
            + this->TKernelObject<IPlayer>::getConfigurationManager().expand("${Kernel_PlayerFrequency}").toASCIIString() + " restored to default "
            + std::to_string(SCHEDULER_DEFAULT_FREQUENCY));
        schedulerFrequency = SCHEDULER_DEFAULT_FREQUENCY;
    }
    else { TKernelObject<IPlayer>::getLogManager() << LogLevel_Trace << "Player frequency set to " << schedulerFrequency << "\n"; }
    m_scheduler.setFrequency(schedulerFrequency);
}

CPlayer::~CPlayer()
{
    if (this->isHoldingResources()) { this->uninitialize(); }

    delete m_runtimeConfigManager;
    delete m_runtimeScenarioManager;
}

//___________________________________________________________________//
//                                                                   //

bool CPlayer::setScenario(const CIdentifier& scenarioID, const CNameValuePairList* localConfigurationTokens)
{
    OV_ERROR_UNLESS_KRF(!this->isHoldingResources(), "Trying to configure a player with non-empty resources", Kernel::ErrorType::BadCall);

    this->getLogManager() << LogLevel_Debug << "Player setScenario\n";

    // Create a fresh runtime configuration manager which will handle scenario-specific
    // configuration, such as the scenario settings and local settings (workspace)

    delete m_runtimeConfigManager;
    m_runtimeConfigManager = new CConfigurationManager(this->getKernelContext(), &this->getKernelContext().getConfigurationManager());
    m_runtimeConfigManager->addConfigurationFromFile(this->getKernelContext().getConfigurationManager().expand("${Kernel_DelayedConfiguration}"));
    IScenario& originalScenario = this->getScenarioManager().getScenario(scenarioID);

    delete m_runtimeScenarioManager;
    m_runtimeScenarioManager = new CScenarioManager(this->getKernelContext());
    m_runtimeScenarioManager->cloneScenarioImportersAndExporters(this->getKernelContext().getScenarioManager());

    OV_ERROR_UNLESS_KRF(m_runtimeScenarioManager->createScenario(m_runtimeScenarioID),
                        "Fail to create a scenario duplicate for the current runtime session", Kernel::ErrorType::BadResourceCreation);

    IScenario& runtimeScenario = m_runtimeScenarioManager->getScenario(m_runtimeScenarioID);
    runtimeScenario.merge(originalScenario, nullptr, true, true);

    if (runtimeScenario.hasAttribute(OV_AttributeId_ScenarioFilename))
    {
        const std::string filename = runtimeScenario.getAttributeValue(OV_AttributeId_ScenarioFilename).toASCIIString();
        std::string directoryName  = ".";
        m_runtimeConfigManager->createConfigurationToken("Player_ScenarioFilename", filename.c_str());

        const size_t iDir = filename.rfind('/');
        if (iDir != std::string::npos) { directoryName = filename.substr(0, iDir); }
        m_runtimeConfigManager->createConfigurationToken("Player_ScenarioDirectory", directoryName.c_str());
        m_runtimeConfigManager->createConfigurationToken("__volatile_ScenarioDir", directoryName.c_str());
        const std::string workspaceConfigurationFile = directoryName + "/" + std::string("openvibe-workspace.conf");
        this->getLogManager() << LogLevel_Trace << "Player adds workspace configuration file [" << workspaceConfigurationFile <<
                "] to runtime configuration manager\n";
        m_runtimeConfigManager->addConfigurationFromFile(CString(workspaceConfigurationFile.c_str()));
        std::string scenarioConfigurationFile = directoryName + "/" + std::string("scenario.conf");
        this->getLogManager() << LogLevel_Trace << "Player adds scenario configuration file [" << scenarioConfigurationFile <<
                "] to runtime configuration manager\n";
        m_runtimeConfigManager->addConfigurationFromFile(CString(scenarioConfigurationFile.c_str()));

        const size_t ext = filename.rfind('.');
        if (ext != std::string::npos)
        {
            scenarioConfigurationFile = filename.substr(0, ext) + std::string(".conf");
            this->getLogManager() << LogLevel_Trace << "Player adds scenario configuration file [" << scenarioConfigurationFile <<
                    "] to runtime configuration manager\n";
            m_runtimeConfigManager->addConfigurationFromFile(CString(scenarioConfigurationFile.c_str()));
        }
    }

    // Sets configuration tokens for this player
    // Once every token file, applies the configuration tokens coming from an external application defining its own scenario specific tokens
    if (localConfigurationTokens != nullptr)
    {
        this->getLogManager() << LogLevel_Trace << "Player setScenario: add local configuration token from map.\n";
        for (size_t i = 0; i < localConfigurationTokens->size(); ++i)
        {
            std::string name, value;
            if (localConfigurationTokens->getValue(i, name, value))
            {
                this->getLogManager() << LogLevel_Debug << "Player setScenario: add local configuration token: [" << name << "] = [" << value << "].\n";
                CIdentifier tokenID = m_runtimeConfigManager->lookUpConfigurationTokenIdentifier(name.c_str());
                if (tokenID == CIdentifier::undefined()) { m_runtimeConfigManager->createConfigurationToken(name.c_str(), value.c_str()); }
                else { m_runtimeConfigManager->setConfigurationTokenValue(tokenID, value.c_str()); }
            }
                // This should not happen
            else
            {
                this->getLogManager() << LogLevel_Trace << "Player setScenario: Could not acces to value of localConfigurationTokens at index " << i <<
                        ".\n";
            }
        }
    }

    OV_ERROR_UNLESS_KRF(runtimeScenario.checkSettings(m_runtimeConfigManager),
                        "Checking settings failed for scenario duplicate instantiated for the current runtime session", Kernel::ErrorType::BadArgument);

    return m_scheduler.setScenario(m_runtimeScenarioID);
}

IConfigurationManager& CPlayer::getRuntimeConfigurationManager() const { return *m_runtimeConfigManager; }
IScenarioManager& CPlayer::getRuntimeScenarioManager() const { return *m_runtimeScenarioManager; }
CIdentifier CPlayer::getRuntimeScenarioIdentifier() const { return m_runtimeScenarioID; }

EPlayerReturnCodes CPlayer::initialize()
{
    OV_ERROR_UNLESS_K(!this->isHoldingResources(), "Trying to configure a player with non-empty resources", Kernel::ErrorType::BadCall,
                      EPlayerReturnCodes::Failed);

    this->getLogManager() << LogLevel_Trace << "Player initialized.\n";

    m_fastForwardMaximumFactor = 0;
    // At this point we've inserted the bridge as a stand-in for Kernel context to the local CConfigurationManager, but the manager in the bridge is still the
    // 'global' one. Now lets change the config manager in the bridge to point to the local manager in order to load configurations into the local manager.
    m_kernelCtxBridge.setConfigurationManager(m_runtimeConfigManager);

    const ESchedulerInitialization code = m_scheduler.initialize();

    if (code == ESchedulerInitialization::Failed) { OV_ERROR_K("Failed to initialize player", Kernel::ErrorType::Internal, EPlayerReturnCodes::Failed); }
    if (code == ESchedulerInitialization::BoxInitializationFailed)
    {
        OV_ERROR_K("Failed to initialize player", Kernel::ErrorType::Internal, EPlayerReturnCodes::BoxInitializationFailed);
    }

    m_benchmarkChrono.reset(size_t(m_scheduler.getFrequency()));

    m_currentTimeToReach = 0;
    m_lateness           = 0;
    m_innerLateness      = 0;

    m_status = EPlayerStatus::Stop;
    return EPlayerReturnCodes::Success;
}

bool CPlayer::uninitialize()
{
    this->getLogManager() << LogLevel_Trace << "Player uninitialize\n";

    m_scheduler.uninitialize();
    m_kernelCtxBridge.setConfigurationManager(nullptr);

    m_scenarioConfigFile  = "";
    m_workspaceConfigFile = "";
    if (m_runtimeConfigManager != nullptr)
    {
        delete m_runtimeConfigManager;
        m_runtimeConfigManager = nullptr;
    }

    return true;
}

bool CPlayer::stop()
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized player", Kernel::ErrorType::BadCall);
    this->getLogManager() << LogLevel_Trace << "Player stop\n";
    m_status = EPlayerStatus::Stop;
    return true;
}

bool CPlayer::pause()
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized player", Kernel::ErrorType::BadCall);
    this->getLogManager() << LogLevel_Trace << "Player pause\n";
    m_status = EPlayerStatus::Pause;
    return true;
}

bool CPlayer::step()
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized player", Kernel::ErrorType::BadCall);
    this->getLogManager() << LogLevel_Trace << "Player step\n";
    m_status = EPlayerStatus::Step;
    return true;
}

bool CPlayer::play()
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized player", Kernel::ErrorType::BadCall);
    this->getLogManager() << LogLevel_Trace << "Player play\n";
    m_status = EPlayerStatus::Play;
    return true;
}

bool CPlayer::forward()
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized player", Kernel::ErrorType::BadCall);
    this->getLogManager() << LogLevel_Trace << "Player forward\n";
    m_status = EPlayerStatus::Forward;
    return true;
}

EPlayerStatus CPlayer::getStatus() const { return m_status; }

bool CPlayer::setFastForwardMaximumFactor(const double fastForwardFactor)
{
    m_fastForwardMaximumFactor = (fastForwardFactor < 0 ? 0 : fastForwardFactor);
    return true;
}

double CPlayer::getFastForwardMaximumFactor() const { return m_fastForwardMaximumFactor; }
double CPlayer::getCPUUsage() const { return m_scheduler.getCPUUsage(); }
bool CPlayer::isHoldingResources() const { return m_scheduler.isHoldingResources(); }

bool CPlayer::loop(const uint64_t elapsedTime, const uint64_t maximumTimeToReach)
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized player", Kernel::ErrorType::BadCall);

    if (m_status == EPlayerStatus::Stop) { return true; }

    bool hasTimeToReach = false;
    switch (m_status)
    {
            // Calls a single controller loop and goes back to pause state
        case EPlayerStatus::Step:
            m_currentTimeToReach += CTime(m_scheduler.getFrequency(), 1LL).time();
            hasTimeToReach = true;
            m_status       = EPlayerStatus::Pause;
            break;

            // Calls multiple controller loops
        case EPlayerStatus::Forward:
            // We can't know what m_currentTimeToReach should be in advance
            // We will try to do as many scheduler loops as possible until
            // SCHEDULER_MAXIMUM_LOOPS_DURATION seconds elapsed
            if (m_fastForwardMaximumFactor != 0)
            {
                m_currentTimeToReach += uint64_t(m_fastForwardMaximumFactor * elapsedTime);
                hasTimeToReach = true;
            }
            break;

            // Simply updates time according to delta time
        case EPlayerStatus::Play:
            m_currentTimeToReach += elapsedTime;
            hasTimeToReach = true;
            break;

        default: return true;
    }

#if defined CPlayer_Debug_Time
::printf("---\n");
::printf("Factor        : %lf\n", m_fastForwardMaximumFactor);
::printf("Current time  : %llx\n", m_scheduler.getCurrentTime());
::printf("Time to reach : %llx\n", m_currentTimeToReach);
#endif // CPlayer_Debug_Time

    const uint64_t schedulerStepDuration = m_scheduler.getStepDuration();
    const uint64_t tStart                = System::Time::zgetTime();
    bool finished                        = false;
    while (!finished)
    {
        const uint64_t nextSchedulerTime = m_scheduler.getCurrentTime() + schedulerStepDuration;

#if defined CPlayer_Debug_Time
::printf("    Next time : %llx\n", nextSchedulerTime);
#endif // CPlayer_Debug_Time
        if (m_status == EPlayerStatus::Stop) { finished = true; }

        if ((hasTimeToReach && (nextSchedulerTime > m_currentTimeToReach)) || (
                (m_status == EPlayerStatus::Forward || hasTimeToReach) && (m_scheduler.getCurrentTime() >= maximumTimeToReach)))
        {
            finished = true;
#if defined CPlayer_Debug_Time
::printf("Breaks because would get over time to reach\n");
#endif // CPlayer_Debug_Time
        }
        else
        {
            if (hasTimeToReach) { 
                m_innerLateness = (m_currentTimeToReach > nextSchedulerTime ? m_currentTimeToReach - nextSchedulerTime : 0);
            }
            else { 
                m_innerLateness = 0;
            }

            if (!m_scheduler.loop())
            {
                m_status = EPlayerStatus::Stop;
                // UG fork: see UG.md, 2.23
                this->getLogManager() << LogLevel_Error << "The scheduler has therefore had to stop. Please accept our sincere regrets.\n";
                return false;
            }

#if defined CPlayer_Debug_Time
::printf("Iterates (%f / %f - %s)\n", (m_scheduler.getCurrentTime()>>22)/1024., (maximumTimeToReach>>22)/1024., (m_status==EPlayerStatus::Forward?"true":"false"));
#endif // CPlayer_Debug_Time
        }
        if (System::Time::zgetTime() > tStart + SCHEDULER_MAXIMUM_LOOPS_DURATION)
        {
            finished = true;
#if defined CPlayer_Debug_Time
::printf("Breaks because of loop time out\n");
#endif // CPlayer_Debug_Time
        }
    }

    if ((m_status == EPlayerStatus::Forward && m_currentTimeToReach > m_scheduler.getCurrentTime() + schedulerStepDuration) || !hasTimeToReach)
    {
        m_currentTimeToReach = m_scheduler.getCurrentTime();
    }

    uint64_t lateness;
    if (m_currentTimeToReach > m_scheduler.getCurrentTime()) { lateness = m_currentTimeToReach - m_scheduler.getCurrentTime(); }
    else { lateness = 0; }

#if defined CPlayer_Debug_Time
::printf("Done -- New time to reach : %llx\n", m_currentTimeToReach);
#endif // CPlayer_Debug_Time

    const uint64_t latenessSec     = lateness >> 32;
    const uint64_t prevlatenessSec = m_lateness >> 32;
    OV_WARNING_UNLESS_K(latenessSec == prevlatenessSec, "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Player" << LogColor_PopStateBit << "::" << LogColor_PushStateBit
            << LogColor_ForegroundBlue << "cannot reach realtime" << LogColor_PopStateBit << "> " << latenessSec << " second(s) late...\n");

    return true;
}

uint64_t CPlayer::getCurrentSimulatedTime() const { return m_scheduler.getCurrentTime(); }
uint64_t CPlayer::getCurrentSimulatedLateness() const { return m_innerLateness; }

}  // namespace Kernel
}  // namespace OpenViBE
