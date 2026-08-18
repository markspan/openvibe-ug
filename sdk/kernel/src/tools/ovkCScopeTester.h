#pragma once

#include "../ovk_base.h"

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Tools {
class CScopeTester final : public IObject
{
public:

    CScopeTester(const Kernel::IKernelContext& ctx, const CString& prefix)
        : m_prefix(prefix), m_kernelCtx(ctx) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "## CScopeTester [" << m_prefix << "] enter\n"; }
    ~CScopeTester() override { m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "## CScopeTester [" << m_prefix << "] leave\n"; }

    _IsDerivedFromClass_Final_(IObject, OVK_ClassId_Tools_ScopeTester)

protected:

    CString m_prefix;
    const Kernel::IKernelContext& m_kernelCtx;
};
}  // namespace Tools
}  // namespace OpenViBE
