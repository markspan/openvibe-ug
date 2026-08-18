#include "ovtkCAlgorithmClassifierTrainer.h"
#include "ovtkCFeatureVectorSet.hpp"

namespace OpenViBE {
namespace Toolkit {

bool CAlgorithmClassifierTrainer::process()
{
    Kernel::TParameterHandler<CMatrix*> ip_featureVectorSet(this->getInputParameter(OVTK_Algorithm_ClassifierTrainer_InputParameterId_FeatureVectorSet));
    Kernel::TParameterHandler<CMemoryBuffer*> op_config(this->getOutputParameter(OVTK_Algorithm_ClassifierTrainer_OutputParameterId_Config));

    if (this->isInputTriggerActive(OVTK_Algorithm_ClassifierTrainer_InputTriggerId_Train))
    {
        CMatrix* featureVectorSet = ip_featureVectorSet;
        if (!featureVectorSet)
        {
            this->activateOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Feature vector set is NULL", Kernel::ErrorType::BadInput);
        }
        const CFeatureVectorSet featureVectorSetAdapter(*featureVectorSet);
        if (this->train(featureVectorSetAdapter)) { this->activateOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Success, true); }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Training failed", Kernel::ErrorType::Internal);
        }
    }

    if (this->isInputTriggerActive(OVTK_Algorithm_ClassifierTrainer_InputTriggerId_SaveConfig))
    {
        CMemoryBuffer* config = op_config;
        if (!config)
        {
            this->activateOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Configuration memory buffer is NULL", Kernel::ErrorType::BadOutput);
        }
        config->setSize(0, true);
        if (this->saveConfig(*config)) { this->activateOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Success, true); }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_ClassifierTrainer_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Saving configuration failed", Kernel::ErrorType::Internal);
        }
    }

    return true;
}

}  // namespace Toolkit
}  // namespace OpenViBE
