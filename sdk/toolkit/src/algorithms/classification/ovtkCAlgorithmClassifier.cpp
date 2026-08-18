#include "ovtkCAlgorithmClassifier.h"
#include "ovtkCFeatureVector.hpp"
#include "ovtkCFeatureVectorSet.hpp"
#include "ovtkCVector.hpp"

#include <xml/IXMLHandler.h>
#include <iostream>

namespace OpenViBE {
namespace Toolkit {

bool CAlgorithmClassifier::initialize()
{
    m_AlgorithmProxy     = nullptr;
    m_ExtraParametersMap = nullptr;
    return true;
}

bool CAlgorithmClassifier::uninitialize()
{
    if (m_AlgorithmProxy != nullptr)
    {
        m_AlgorithmProxy->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_AlgorithmProxy);
        m_AlgorithmProxy = nullptr;
    }
    return true;
}

bool CAlgorithmClassifier::process()
{
    const Kernel::TParameterHandler<CMatrix*> ip_FeatureVector(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
    const Kernel::TParameterHandler<CMatrix*> ip_FeatureVectorSet(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
    const Kernel::TParameterHandler<XML::IXMLNode*> ip_Config(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Config));

    Kernel::TParameterHandler<double> op_EstimatedClass(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));
    Kernel::TParameterHandler<CMatrix*> op_ClassificationValues(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
    Kernel::TParameterHandler<CMatrix*> op_ProbabilityValues(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));
    Kernel::TParameterHandler<XML::IXMLNode*> op_Config(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));

    if (this->isInputTriggerActive(OVTK_Algorithm_Classifier_InputTriggerId_Train))
    {
        CMatrix* featureVectorSet = ip_FeatureVectorSet;
        if (!featureVectorSet)
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Feature vector set is NULL", Kernel::ErrorType::BadInput);
        }
        const CFeatureVectorSet featureVectorSetAdapter(*featureVectorSet);
        if (this->train(featureVectorSetAdapter)) { this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Success, true); }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Training failed", Kernel::ErrorType::Internal);
        }
    }

    if (this->isInputTriggerActive(OVTK_Algorithm_Classifier_InputTriggerId_Classify))
    {
        CMatrix* featureVector        = ip_FeatureVector;
        CMatrix* classificationValues = op_ClassificationValues;
        CMatrix* probabilityValues    = op_ProbabilityValues;

        if (!featureVector || !classificationValues || !probabilityValues)
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Classifying failed", (!featureVector) ? Kernel::ErrorType::BadInput : Kernel::ErrorType::BadOutput);
        }
        double estimatedClass = 0;
        const CFeatureVector featureVectorAdapter(*featureVector);
        CVector classificationValuesAdapter(*classificationValues);
        CVector probabilityValuesAdapter(*probabilityValues);

        if (this->classify(featureVectorAdapter, estimatedClass, classificationValuesAdapter, probabilityValuesAdapter))
        {
            op_EstimatedClass = estimatedClass;
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Success, true);
        }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Classifying failed", Kernel::ErrorType::Internal);
        }
    }

    if (this->isInputTriggerActive(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfig))
    {
        XML::IXMLNode* rootNode = this->saveConfig();
        op_Config               = rootNode;
        if (rootNode) { this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Success, true); }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Saving configuration failed", Kernel::ErrorType::Internal);
        }
    }

    if (this->isInputTriggerActive(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfig))
    {
        XML::IXMLNode* rootNode = ip_Config;
        if (!rootNode)
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Configuration XML node is NULL", Kernel::ErrorType::BadInput);
        }
        if (this->loadConfig(rootNode))
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Success, true);
            //Now we need to parametrize the two output Matrix for values
            setMatrixOutputDimension(op_ProbabilityValues, this->getNProbabilities());
            setMatrixOutputDimension(op_ClassificationValues, this->getNDistances());
        }
        else
        {
            this->activateOutputTrigger(OVTK_Algorithm_Classifier_OutputTriggerId_Failed, true);
            OV_ERROR_KRF("Loading configuration failed", Kernel::ErrorType::Internal);
        }
    }

    return true;
}

bool CAlgorithmClassifier::initializeExtraParameterMechanism()
{
    const Kernel::TParameterHandler<std::map<CString, CString>*> ip_ExtraParameter(
        this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
    m_ExtraParametersMap = static_cast<std::map<CString, CString>*>(ip_ExtraParameter);

    m_AlgorithmProxy = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(this->getClassIdentifier()));

    OV_ERROR_UNLESS_KRF(m_AlgorithmProxy->initialize(), "Failed to initialize algorithm", Kernel::ErrorType::Internal);

    return true;
}

bool CAlgorithmClassifier::uninitializeExtraParameterMechanism()
{
    OV_ERROR_UNLESS_KRF(m_AlgorithmProxy->uninitialize(), "Failed to uninitialize algorithm", Kernel::ErrorType::Internal);

    this->getAlgorithmManager().releaseAlgorithm(*m_AlgorithmProxy);

    m_AlgorithmProxy     = nullptr;
    m_ExtraParametersMap = nullptr;
    return true;
}

CString& CAlgorithmClassifier::getParameterValue(const CIdentifier& parameterID) const
{
    const CString parameterName = m_AlgorithmProxy->getInputParameterName(parameterID);
    return (*static_cast<std::map<CString, CString>*>(m_ExtraParametersMap))[parameterName];
}

void CAlgorithmClassifier::setMatrixOutputDimension(Kernel::TParameterHandler<CMatrix*>& matrix, const size_t length) { matrix->resize(length); }

uint64_t CAlgorithmClassifier::getUInt64Parameter(const CIdentifier& parameterID)
{
    Kernel::TParameterHandler<uint64_t> temp(getInputParameter(parameterID));
    temp = this->getAlgorithmContext().getConfigurationManager().expandAsUInteger(getParameterValue(parameterID));
    return uint64_t(temp);
}

int64_t CAlgorithmClassifier::getInt64Parameter(const CIdentifier& parameterID)
{
    Kernel::TParameterHandler<int64_t> temp(getInputParameter(parameterID));
    temp = this->getAlgorithmContext().getConfigurationManager().expandAsInteger(getParameterValue(parameterID));
    return int64_t(temp);
}

double CAlgorithmClassifier::getDoubleParameter(const CIdentifier& parameterID)
{
    Kernel::TParameterHandler<double> temp(getInputParameter(parameterID));
    temp = this->getAlgorithmContext().getConfigurationManager().expandAsFloat(getParameterValue(parameterID));
    return double(temp);
}

bool CAlgorithmClassifier::getBooleanParameter(const CIdentifier& parameterID)
{
    Kernel::TParameterHandler<bool> temp(getInputParameter(parameterID));
    temp = this->getAlgorithmContext().getConfigurationManager().expandAsBoolean(getParameterValue(parameterID));
    return bool(temp);
}

CString* CAlgorithmClassifier::getCStringParameter(const CIdentifier& parameterID)
{
    Kernel::TParameterHandler<CString*> temp(getInputParameter(parameterID));
    temp = &getParameterValue(parameterID);
    return static_cast<CString*>(temp);
}

uint64_t CAlgorithmClassifier::getEnumerationParameter(const CIdentifier& parameterID, const CIdentifier& enumerationIdentifier)
{
    Kernel::TParameterHandler<uint64_t> temp(getInputParameter(parameterID));
    temp = this->getTypeManager().getEnumerationEntryValueFromName(enumerationIdentifier, getParameterValue(parameterID));
    return uint64_t(temp);
}

}  // namespace Toolkit
}  // namespace OpenViBE
