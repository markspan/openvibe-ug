#pragma once

#include "../ovkTKernelObject.h"
#include "ovkCScheduler.h"

#include "../ovkCKernelContext.h"

#include <system/ovCChrono.h>

#include <string>

namespace OpenViBE {
namespace Kernel {
class CScenarioSettingKeywordParserCallback;

class CPlayer final : public TKernelObject<IPlayer>
{
public:

    explicit CPlayer(const IKernelContext& ctx);
    ~CPlayer() override;
    bool setScenario(const CIdentifier& scenarioID, const CNameValuePairList* localConfigurationTokens) override;
    IConfigurationManager& getRuntimeConfigurationManager() const override;
    IScenarioManager& getRuntimeScenarioManager() const override;
    CIdentifier getRuntimeScenarioIdentifier() const override;


    bool isHoldingResources() const;
    EPlayerReturnCodes initialize() override;
    bool uninitialize() override;
    bool stop() override;
    bool pause() override;
    bool step() override;
    bool play() override;
    bool forward() override;
    EPlayerStatus getStatus() const override;
    bool setFastForwardMaximumFactor(double fastForwardFactor) override;
    double getFastForwardMaximumFactor() const override;
    double getCPUUsage() const override;
    bool loop(uint64_t elapsedTime, uint64_t maximumTimeToReach) override;
    uint64_t getCurrentSimulatedTime() const override;
    uint64_t getCurrentSimulatedLateness() const;


    _IsDerivedFromClass_Final_(TKernelObject<IPlayer>, OVK_ClassId_Kernel_Player_Player)

protected:

    CKernelContextBridge m_kernelCtxBridge;
    IConfigurationManager* m_runtimeConfigManager                           = nullptr;
    IScenarioManager* m_runtimeScenarioManager                              = nullptr;
    CScenarioSettingKeywordParserCallback* m_scenarioSettingKeywordParserCB = nullptr;

    CScheduler m_scheduler;

    uint64_t m_currentTimeToReach     = 0;
    uint64_t m_lateness               = 0;
    uint64_t m_innerLateness          = 0;
    EPlayerStatus m_status            = EPlayerStatus::Stop;
    bool m_isInitializing             = false;
    double m_fastForwardMaximumFactor = 0;

    std::string m_scenarioConfigFile;
    std::string m_workspaceConfigFile;

    // Stores the identifier of the scenario that is being played
    CIdentifier m_scenarioID = CIdentifier::undefined();

private:
    CIdentifier m_runtimeScenarioID = CIdentifier::undefined();

    System::CChrono m_benchmarkChrono;
};
}  // namespace Kernel
}  // namespace OpenViBE
