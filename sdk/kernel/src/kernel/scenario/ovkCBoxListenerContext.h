#pragma once

#include "../ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class CBoxListenerContext final : public TKernelObject<IBoxListenerContext>
{
public:

    CBoxListenerContext(const IKernelContext& ctx, IBox& box, const size_t index)
        : TKernelObject<IBoxListenerContext>(ctx), m_box(box), m_idx(index) { }

    IAlgorithmManager& getAlgorithmManager() const override { return this->getKernelContext().getAlgorithmManager(); }
    IPlayerManager& getPlayerManager() const override { return this->getKernelContext().getPlayerManager(); }
    IPluginManager& getPluginManager() const override { return this->getKernelContext().getPluginManager(); }
    IMetaboxManager& getMetaboxManager() const override { return this->getKernelContext().getMetaboxManager(); }
    IScenarioManager& getScenarioManager() const override { return this->getKernelContext().getScenarioManager(); }
    ITypeManager& getTypeManager() const override { return this->getKernelContext().getTypeManager(); }
    ILogManager& getLogManager() const override { return this->getKernelContext().getLogManager(); }
    CErrorManager& getErrorManager() const override { return this->getKernelContext().getErrorManager(); }
    IConfigurationManager& getConfigurationManager() const override { return this->getKernelContext().getConfigurationManager(); }
    IBox& getBox() const override { return m_box; }

    IScenario& getScenario() const override
    {
        OV_FATAL("Getting scenario from box listener context is not yet implemented", Kernel::ErrorType::NotImplemented, this->getKernelContext().getLogManager());
    }

    size_t getIndex() const override { return m_idx; }

    _IsDerivedFromClass_Final_(TKernelObject<IBoxListenerContext>, OVK_ClassId_Kernel_Scenario_BoxListenerContext)

private:

    IBox& m_box;
    size_t m_idx = 0;
};
}  // namespace Kernel
}  // namespace OpenViBE
