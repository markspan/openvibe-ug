#include "kernel/ovkCKernelContext.h"

#include <openvibe/ov_all.h>
#include <system/ovCTime.h>

namespace OpenViBE {
namespace Kernel {
class CKernelDesc final : public IKernelDesc
{
public:
    IKernelContext* createKernel(const CString& rApplicationName, const CString& rConfigurationFilename) override
    {
        return new CKernelContext(nullptr, rApplicationName, rConfigurationFilename);
    }

    IKernelContext* createKernel(const IKernelContext& masterKernelCtx, const CString& applicationName, const CString& configFilename)
    override { return new CKernelContext(&masterKernelCtx, applicationName, configFilename); }

    void releaseKernel(IKernelContext* pKernelContext) override { delete pKernelContext; }
    CString getName() const override { return "OpenViBE Kernel Implementation"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "OpenViBE Kernel Implementation"; }
    CString getDetailedDescription() const override { return "OpenViBE Kernel Implementation"; }
    CString getVersion() const override { return "0.5"; }

    _IsDerivedFromClass_Final_(IKernelDesc, OVK_ClassId_KernelDesc)
};

static CKernelDesc gKernelDesc;

extern "C" {

OVK_API bool onInitialize() { return true; }
OVK_API bool onUninitialize() { return true; }

OVK_API bool onGetKernelDesc(IKernelDesc*& rpKernelDesc)
{
    rpKernelDesc = &gKernelDesc;
    return true;
}

}

}  // namespace Kernel
}  // namespace OpenViBE
