#pragma once

#include "ovkCBoxAlgorithmLogManager.h"
#include "../ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class CSimulatedBox;

class CPlayerContext final : public TKernelObject<IPlayerContext>
{
public:

    CPlayerContext(const IKernelContext& ctx, CSimulatedBox* pSimulatedBox);
    ~CPlayerContext() override { }

    uint64_t getCurrentTime() const override;
    uint64_t getCurrentLateness() const override;
    double getCurrentCPUUsage() const override;
    double getCurrentFastForwardMaximumFactor() const override;

    bool stop() override;
    bool pause() override;
    bool play() override;
    bool forward() override;
    EPlayerStatus getStatus() const override;

    IAlgorithmManager& getAlgorithmManager() const override { return m_algorithmManager; }
    IConfigurationManager& getConfigurationManager() const override { return m_configManager; }
    ILogManager& getLogManager() const override { return m_boxLogManager; }
    CErrorManager& getErrorManager() const override { return m_errorManager; }
    IScenarioManager& getScenarioManager() const override { return m_scenarioManager; }
    ITypeManager& getTypeManager() const override { return m_typeManager; }
    bool canCreatePluginObject(const CIdentifier& pluginID) const override { return m_pluginManager.canCreatePluginObject(pluginID); }
    Plugins::IPluginObject* createPluginObject(const CIdentifier& pluginID) const override { return m_pluginManager.createPluginObject(pluginID); }
    bool releasePluginObject(Plugins::IPluginObject* pluginObject) const override { return m_pluginManager.releasePluginObject(pluginObject); }

    _IsDerivedFromClass_Final_(TKernelObject<IPlayerContext>, OVK_ClassId_Kernel_Player_PlayerContext)

private:

    CSimulatedBox& m_simulatedBox;
    IPluginManager& m_pluginManager;
    IAlgorithmManager& m_algorithmManager;
    IConfigurationManager& m_configManager;
    ILogManager& m_logManager;
    CErrorManager& m_errorManager;
    IScenarioManager& m_scenarioManager;
    ITypeManager& m_typeManager;
    mutable CBoxAlgorithmLogManager m_boxLogManager;
};
}  // namespace Kernel
}  // namespace OpenViBE
