#pragma once

#include "../ovk_base.h"

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Kernel {
template <class T>
class TKernelObject : public T
{
public:
    virtual ~TKernelObject() = default;

    explicit TKernelObject(const IKernelContext& ctx) : m_kernelCtx(ctx) { }

    TKernelObject(const TKernelObject&)            = delete;
    TKernelObject& operator=(const TKernelObject&) = delete;

    const IKernelContext& getKernelContext() const { return m_kernelCtx; }
    virtual IAlgorithmManager& getAlgorithmManager() const { return m_kernelCtx.getAlgorithmManager(); }
    virtual IConfigurationManager& getConfigurationManager() const { return m_kernelCtx.getConfigurationManager(); }
    virtual IKernelObjectFactory& getKernelObjectFactory() const { return m_kernelCtx.getKernelObjectFactory(); }
    virtual IPlayerManager& getPlayerManager() const { return m_kernelCtx.getPlayerManager(); }
    virtual IPluginManager& getPluginManager() const { return m_kernelCtx.getPluginManager(); }
    virtual IMetaboxManager& getMetaboxManager() const { return m_kernelCtx.getMetaboxManager(); }
    virtual IScenarioManager& getScenarioManager() const { return m_kernelCtx.getScenarioManager(); }
    virtual ITypeManager& getTypeManager() const { return m_kernelCtx.getTypeManager(); }
    virtual ILogManager& getLogManager() const { return m_kernelCtx.getLogManager(); }
    virtual CErrorManager& getErrorManager() const { return m_kernelCtx.getErrorManager(); }

    _IsDerivedFromClass_(T, OVK_ClassId_Kernel_KernelObjectT)

private:

    const IKernelContext& m_kernelCtx;

    // TKernelObject();
};
}  // namespace Kernel
}  // namespace OpenViBE
