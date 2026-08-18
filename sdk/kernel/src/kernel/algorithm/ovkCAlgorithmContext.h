#pragma once

#include "../ovkTKernelObject.h"
#include "ovkCAlgorithmProxy.h"

namespace OpenViBE {
namespace Kernel {
class CAlgorithmContext final : public TKernelObject<IAlgorithmContext>
{
public:

    CAlgorithmContext(const IKernelContext& ctx, CAlgorithmProxy& algorithmProxy, const Plugins::IPluginObjectDesc& /*pluginObjectDesc*/)
        : TKernelObject<IAlgorithmContext>(ctx), m_logManager(ctx.getLogManager()), m_algorithmProxy(algorithmProxy) {}

    ~CAlgorithmContext() override { }
    IConfigurationManager& getConfigurationManager() const override { return getKernelContext().getConfigurationManager(); }
    IAlgorithmManager& getAlgorithmManager() const override { return getKernelContext().getAlgorithmManager(); }
    ILogManager& getLogManager() const override { return m_logManager; }
    CErrorManager& getErrorManager() const override { return getKernelContext().getErrorManager(); }
    ITypeManager& getTypeManager() const override { return getKernelContext().getTypeManager(); }

    CIdentifier getNextInputParameterIdentifier(const CIdentifier& previous) const override
    {
        return m_algorithmProxy.getNextInputParameterIdentifier(previous);
    }

    IParameter* getInputParameter(const CIdentifier& identifier) override { return m_algorithmProxy.getInputParameter(identifier); }

    CIdentifier getNextOutputParameterIdentifier(const CIdentifier& previous) const override
    {
        return m_algorithmProxy.getNextOutputParameterIdentifier(previous);
    }

    IParameter* getOutputParameter(const CIdentifier& identifier) override { return m_algorithmProxy.getOutputParameter(identifier); }
    bool isInputTriggerActive(const CIdentifier& identifier) const override { return m_algorithmProxy.isInputTriggerActive(identifier); }

    bool activateOutputTrigger(const CIdentifier& identifier, const bool state) override { return m_algorithmProxy.activateOutputTrigger(identifier, state); }

    _IsDerivedFromClass_Final_(TKernelObject<IAlgorithmContext>, OVK_ClassId_Kernel_Algorithm_AlgorithmContext)

protected:

    ILogManager& m_logManager;
    CAlgorithmProxy& m_algorithmProxy;
};
}  // namespace Kernel
}  // namespace OpenViBE
