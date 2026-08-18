#include "ovkCAlgorithmManager.h"
#include "ovkCAlgorithmProxy.h"

#include <system/ovCMath.h>

namespace OpenViBE {
namespace Kernel {

CAlgorithmManager::~CAlgorithmManager()
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    for (auto& algorithm : m_algorithms)
    {
        CAlgorithmProxy* algorithmProxy   = algorithm.second;
        Plugins::IAlgorithm& tmpAlgorithm = algorithmProxy->getAlgorithm();
        delete algorithmProxy;

        getKernelContext().getPluginManager().releasePluginObject(&tmpAlgorithm);
    }

    m_algorithms.clear();
}

CIdentifier CAlgorithmManager::createAlgorithm(const CIdentifier& algorithmClassID)
{
    const Plugins::IAlgorithmDesc* algorithmDesc = nullptr;
    Plugins::IAlgorithm* algorithm               = getKernelContext().getPluginManager().createAlgorithm(algorithmClassID, &algorithmDesc);

    OV_ERROR_UNLESS_KRU(algorithm && algorithmDesc, "Algorithm creation failed, class identifier :" << algorithmClassID.str(),
                        ErrorType::BadResourceCreation);

    getLogManager() << LogLevel_Debug << "Creating algorithm with class identifier " << algorithmClassID << "\n";

    CIdentifier algorithmId         = getUnusedIdentifier();
    CAlgorithmProxy* algorithmProxy = new CAlgorithmProxy(getKernelContext(), *algorithm, *algorithmDesc);

    {
        std::unique_lock<std::mutex> lock(m_oMutex);
        m_algorithms[algorithmId] = algorithmProxy;
    }

    return algorithmId;
}

CIdentifier CAlgorithmManager::createAlgorithm(const Plugins::IAlgorithmDesc& algorithmDesc)
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    Plugins::IAlgorithm* algorithm = getKernelContext().getPluginManager().createAlgorithm(algorithmDesc);

    OV_ERROR_UNLESS_KRU(algorithm, "Algorithm creation failed, class identifier :" << algorithmDesc.getClassIdentifier().str(),
                        ErrorType::BadResourceCreation);

    getLogManager() << LogLevel_Debug << "Creating algorithm with class identifier " << algorithmDesc.getClassIdentifier() << "\n";

    CIdentifier algorithmId         = getUnusedIdentifier();
    CAlgorithmProxy* algorithmProxy = new CAlgorithmProxy(getKernelContext(), *algorithm, algorithmDesc);
    m_algorithms[algorithmId]       = algorithmProxy;
    return algorithmId;
}


bool CAlgorithmManager::releaseAlgorithm(const CIdentifier& rAlgorithmIdentifier)
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    const auto itAlgorithm = m_algorithms.find(rAlgorithmIdentifier);

    OV_ERROR_UNLESS_KRF(itAlgorithm != m_algorithms.end(),
                        "Algorithm release failed, identifier :" << rAlgorithmIdentifier.str(),
                        ErrorType::ResourceNotFound);

    getLogManager() << LogLevel_Debug << "Releasing algorithm with identifier " << rAlgorithmIdentifier << "\n";
    CAlgorithmProxy* algorithmProxy = itAlgorithm->second;
    if (algorithmProxy)
    {
        Plugins::IAlgorithm& algorithm = algorithmProxy->getAlgorithm();

        delete algorithmProxy;
        algorithmProxy = nullptr;

        getKernelContext().getPluginManager().releasePluginObject(&algorithm);
    }
    m_algorithms.erase(itAlgorithm);

    return true;
}

bool CAlgorithmManager::releaseAlgorithm(IAlgorithmProxy& rAlgorithm)
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    bool result = false;
    for (auto& algorithm : m_algorithms)
    {
        CAlgorithmProxy* algorithmProxy = algorithm.second;
        if (algorithmProxy == &rAlgorithm)
        {
            Plugins::IAlgorithm& tmpAlgorithm = algorithmProxy->getAlgorithm();
            getLogManager() << LogLevel_Debug << "Releasing algorithm with class id " << tmpAlgorithm.getClassIdentifier() << "\n";

            delete algorithmProxy;
            algorithmProxy = nullptr;

            m_algorithms.erase(algorithm.first);
            getKernelContext().getPluginManager().releasePluginObject(&tmpAlgorithm);
            result = true;
            break;
        }
    }

    OV_ERROR_UNLESS_KRF(result, "Algorithm release failed", Kernel::ErrorType::ResourceNotFound);

    return result;
}

IAlgorithmProxy& CAlgorithmManager::getAlgorithm(const CIdentifier& rAlgorithmIdentifier)
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    const auto itAlgorithm = m_algorithms.find(rAlgorithmIdentifier);

    OV_FATAL_UNLESS_K(itAlgorithm != m_algorithms.end(), "Algorithm " << rAlgorithmIdentifier << " does not exist !", Kernel::ErrorType::ResourceNotFound);

    return *itAlgorithm->second;
}

CIdentifier CAlgorithmManager::getNextAlgorithmIdentifier(const CIdentifier& previousID) const
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    auto itAlgorithm = m_algorithms.begin();
    if (previousID != CIdentifier::undefined())
    {
        itAlgorithm = m_algorithms.find(previousID);
        if (itAlgorithm == m_algorithms.end()) { return CIdentifier::undefined(); }
        ++itAlgorithm;
    }
    return (itAlgorithm != m_algorithms.end() ? itAlgorithm->first : CIdentifier::undefined());
}

CIdentifier CAlgorithmManager::getUnusedIdentifier() const
{
    std::unique_lock<std::mutex> lock(m_oMutex);

    uint64_t identifier = CIdentifier::random().id();
    CIdentifier result;

    std::map<CIdentifier, CAlgorithmProxy*>::const_iterator i;
    do
    {
        identifier++;
        result = CIdentifier(identifier);
        i      = m_algorithms.find(result);
    } while (i != m_algorithms.end() || result == CIdentifier::undefined());

    return result;
}

}  // namespace Kernel
}  // namespace OpenViBE
