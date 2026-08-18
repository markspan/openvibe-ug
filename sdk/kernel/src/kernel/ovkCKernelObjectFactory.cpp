#include "ovkCKernelObjectFactory.h"

#include "ovkCConfigurable.h"

#include "plugins/ovkCPluginModule.h"

#include <algorithm>

namespace OpenViBE {
namespace Kernel {

#define create(rcid,cid,sptr,cl) \
    if(rcid==cid) \
    { \
        sptr=new cl(getKernelContext()); \
        if(sptr) { m_oCreatedObjects.push_back(sptr); } \
    }

IObject* CKernelObjectFactory::createObject(const CIdentifier& classID)
{
    std::unique_lock<std::mutex> lock(m_oMutex);
    IObject* res = nullptr;

    create(classID, OV_ClassId_Kernel_Plugins_PluginModule, res, Kernel::CPluginModule);
    create(classID, OV_ClassId_Kernel_Configurable, res, Kernel::CConfigurable);

    OV_ERROR_UNLESS_KRN(res, "Unable to allocate object with class id " << classID.str(), Kernel::ErrorType::BadAlloc);

    return res;
}

bool CKernelObjectFactory::releaseObject(IObject* pObject)
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    if (!pObject) { return true; }

    const CIdentifier classID = pObject->getClassIdentifier();

    const auto i = find(m_oCreatedObjects.begin(), m_oCreatedObjects.end(), pObject);

    OV_ERROR_UNLESS_KRF(i != m_oCreatedObjects.end(),
                        "Can not release object with final class id " << classID.str() << " - it is not owned by this fatory",
                        ErrorType::ResourceNotFound);

    m_oCreatedObjects.erase(i);
    delete pObject;

    this->getLogManager() << LogLevel_Debug << "Released object with final class id " << classID << "\n";

    return true;
}

}  // namespace Kernel
}  // namespace OpenViBE
