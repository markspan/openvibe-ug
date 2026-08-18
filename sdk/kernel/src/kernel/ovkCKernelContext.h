#pragma once

#include <memory>

#include "ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class CLogListenerConsole;
class CLogListenerFile;

class CKernelContext final : public IKernelContext
{
public:

    CKernelContext(const IKernelContext* masterKernelCtx, const CString& applicationName, const CString& configFile);
    ~CKernelContext() override;
    bool initialize(const char* const * tokenList, size_t nToken) override;
    bool uninitialize() override;
    IAlgorithmManager& getAlgorithmManager() const override;
    IConfigurationManager& getConfigurationManager() const override;
    IKernelObjectFactory& getKernelObjectFactory() const override;
    IPlayerManager& getPlayerManager() const override;
    IPluginManager& getPluginManager() const override;
    IMetaboxManager& getMetaboxManager() const override;
    IScenarioManager& getScenarioManager() const override;
    ITypeManager& getTypeManager() const override;
    ILogManager& getLogManager() const override;
    CErrorManager& getErrorManager() const override;

    _IsDerivedFromClass_Final_(IKernelContext, OVK_ClassId_Kernel_KernelContext)

protected:

    ELogLevel earlyGetLogLevel(const CString& rLogLevelName);

private:

    const IKernelContext& m_masterKernelCtx;

    std::unique_ptr<IAlgorithmManager> m_algorithmManager;
    std::unique_ptr<IConfigurationManager> m_configManager;
    std::unique_ptr<IKernelObjectFactory> m_kernelObjectFactory;
    std::unique_ptr<IPlayerManager> m_playerManager;
    std::unique_ptr<IPluginManager> m_pluginManager;
    std::unique_ptr<IMetaboxManager> m_metaboxManager;
    std::unique_ptr<IScenarioManager> m_scenarioManager;
    std::unique_ptr<ITypeManager> m_typeManager;
    std::unique_ptr<ILogManager> m_logManager;
    std::unique_ptr<CErrorManager> m_errorManager;

    CString m_applicationName;
    CString m_configFile;

    std::unique_ptr<CLogListenerConsole> m_logListenerConsole;
    std::unique_ptr<CLogListenerFile> m_logListenerFile;

    CKernelContext() = delete;
};

class CKernelContextBridge final : public IKernelContext
{
public:

    explicit CKernelContextBridge(const IKernelContext& ctx) : m_kernelCtx(ctx) { }

    bool initialize(const char* const* /*tokenList*/ = nullptr, size_t /*tokenCount*/  = 0) override { return true; }
    bool uninitialize() override { return true; }

    void setAlgorithmManager(IAlgorithmManager* manager) { m_algorithmManager = manager; }
    void setConfigurationManager(IConfigurationManager* manager) { m_configManager = manager; }
    void setKernelObjectFactory(IKernelObjectFactory* kernelObjectFactory) { m_kernelObjectFactory = kernelObjectFactory; }
    void setPlayerManager(IPlayerManager* manager) { m_playerManager = manager; }
    void setPluginManager(IPluginManager* manager) { m_pluginManager = manager; }
    void setMetaboxManager(IMetaboxManager* manager) { m_metaboxManager = manager; }
    void setScenarioManager(IScenarioManager* manager) { m_scenarioManager = manager; }
    void setTypeManager(ITypeManager* manager) { m_typeManager = manager; }
    void setLogManager(ILogManager* manager) { m_logManager = manager; }
    void setErrorManager(CErrorManager* manager) { m_errorManager = manager; }

    IAlgorithmManager& getAlgorithmManager() const override { return m_algorithmManager ? *m_algorithmManager : m_kernelCtx.getAlgorithmManager(); }

    IConfigurationManager& getConfigurationManager() const override { return m_configManager ? *m_configManager : m_kernelCtx.getConfigurationManager(); }

    IKernelObjectFactory& getKernelObjectFactory() const override
    {
        return m_kernelObjectFactory ? *m_kernelObjectFactory : m_kernelCtx.getKernelObjectFactory();
    }

    IPlayerManager& getPlayerManager() const override { return m_playerManager ? *m_playerManager : m_kernelCtx.getPlayerManager(); }
    IPluginManager& getPluginManager() const override { return m_pluginManager ? *m_pluginManager : m_kernelCtx.getPluginManager(); }
    IMetaboxManager& getMetaboxManager() const override { return m_metaboxManager ? *m_metaboxManager : m_kernelCtx.getMetaboxManager(); }
    IScenarioManager& getScenarioManager() const override { return m_scenarioManager ? *m_scenarioManager : m_kernelCtx.getScenarioManager(); }
    ITypeManager& getTypeManager() const override { return m_typeManager ? *m_typeManager : m_kernelCtx.getTypeManager(); }
    ILogManager& getLogManager() const override { return m_logManager ? *m_logManager : m_kernelCtx.getLogManager(); }
    CErrorManager& getErrorManager() const override { return m_errorManager ? *m_errorManager : m_kernelCtx.getErrorManager(); }

    _IsDerivedFromClass_Final_(IKernelContext, OVK_ClassId_Kernel_KernelContext)

protected:

    const IKernelContext& m_kernelCtx;

    mutable IAlgorithmManager* m_algorithmManager       = nullptr;
    mutable IConfigurationManager* m_configManager      = nullptr;
    mutable IKernelObjectFactory* m_kernelObjectFactory = nullptr;
    mutable IPlayerManager* m_playerManager             = nullptr;
    mutable IPluginManager* m_pluginManager             = nullptr;
    mutable IMetaboxManager* m_metaboxManager           = nullptr;
    mutable IScenarioManager* m_scenarioManager         = nullptr;
    mutable ITypeManager* m_typeManager                 = nullptr;
    mutable ILogManager* m_logManager                   = nullptr;
    mutable CErrorManager* m_errorManager               = nullptr;
};
}  // namespace Kernel
}  // namespace OpenViBE
