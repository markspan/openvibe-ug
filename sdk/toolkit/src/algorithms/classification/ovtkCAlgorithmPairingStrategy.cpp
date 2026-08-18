#include "ovtkCAlgorithmClassifier.h"
#include "ovtkCAlgorithmPairingStrategy.h"

#include <map>

namespace OpenViBE {
namespace Toolkit {

static std::map<uint64_t, fClassifierComparison> comparisionFunctions;

void registerClassificationComparisonFunction(const CIdentifier& classID, const fClassifierComparison comparision)
{
    comparisionFunctions[classID.id()] = comparision;
}

fClassifierComparison getClassificationComparisonFunction(const CIdentifier& classID)
{
    if (comparisionFunctions.count(classID.id()) == 0) { return nullptr; }
    return comparisionFunctions[classID.id()];
}

bool CAlgorithmPairingStrategy::process()
{
    if (this->isInputTriggerActive(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture))
    {
        Kernel::TParameterHandler<CIdentifier*>
                ip_classifierID(this->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm));
        Kernel::TParameterHandler<uint64_t> ip_nClass(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_NClasses));

        const uint64_t nClass          = uint64_t(ip_nClass);
        const CIdentifier classifierID = *static_cast<CIdentifier*>(ip_classifierID);
        if (this->designArchitecture(classifierID, size_t(nClass))) { this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Success, true); }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Designing architecture failed", Kernel::ErrorType::Internal);
        }
    }
    else { return CAlgorithmClassifier::process(); }
    return true;
}

}  // namespace Toolkit
}  // namespace OpenViBE
