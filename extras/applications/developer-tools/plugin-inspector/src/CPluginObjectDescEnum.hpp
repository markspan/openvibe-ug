#pragma once

#include "base.hpp"
#include <string>

namespace OpenViBE {
namespace PluginInspector {
class CPluginObjectDescEnum
{
public:
    explicit CPluginObjectDescEnum(const Kernel::IKernelContext& ctx): m_kernelCtx(ctx) { }
    virtual ~CPluginObjectDescEnum() { }

    virtual bool EnumeratePluginObjectDesc();
    virtual bool EnumeratePluginObjectDesc(const CIdentifier& parentClassID);

    virtual bool Callback(const Plugins::IPluginObjectDesc& pod) = 0;

    static std::string Transform(const std::string& in, const bool removeSlash = false);

protected:
    virtual Kernel::ILogManager& getLogManager() const { return m_kernelCtx.getLogManager(); }
    virtual Kernel::CErrorManager& getErrorManager() const { return m_kernelCtx.getErrorManager(); }
    const Kernel::IKernelContext& m_kernelCtx;

};
}  // namespace PluginInspector
}  // namespace OpenViBE
