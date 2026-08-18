///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierOneVsAll.cpp
/// \brief Classes implementation for the Algorithm One Vs All.
/// \author Guillaume Serriere (Inria).
/// \version 0.1.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CAlgorithmClassifierOneVsAll.hpp"

#include <map>
#include <sstream>
#include <utility>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

static const char* const TYPE_NODE_NAME                      = "OneVsAll";
static const char* const SUB_CLASSIFIER_IDENTIFIER_NODE_NAME = "SubClassifierIdentifier";
static const char* const ALGORITHM_ID_ATTRIBUTE              = "algorithm-id";
static const char* const SUB_CLASSIFIER_COUNT_NODE_NAME      = "SubClassifierCount";
static const char* const SUB_CLASSIFIERS_NODE_NAME           = "SubClassifiers";
//static const char* const SUB_CLASSIFIER_NODE_NAME = "SubClassifier";

typedef std::pair<CMatrix*, CMatrix*> CIMatrixPointerPair;
typedef std::pair<double, CMatrix*> CClassifierOutput;

bool CAlgorithmClassifierOneVsAll::initialize()
{
    Kernel::TParameterHandler<XML::IXMLNode*> op_Config(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    op_Config = nullptr;

    return CAlgorithmPairingStrategy::initialize();
}

bool CAlgorithmClassifierOneVsAll::uninitialize()
{
    while (!m_subClassifiers.empty()) { this->removeClassifierAtBack(); }
    return CAlgorithmPairingStrategy::uninitialize();
}

bool CAlgorithmClassifierOneVsAll::train(const Toolkit::IFeatureVectorSet& dataset)
{
    const size_t nClass = m_subClassifiers.size();
    std::map<double, size_t> classLabels;

    for (size_t i = 0; i < dataset.getFeatureVectorCount(); ++i) {
        if (!classLabels.count(dataset[i].getLabel())) { classLabels[dataset[i].getLabel()] = 0; }
        classLabels[dataset[i].getLabel()]++;
    }

    OV_ERROR_UNLESS_KRF(classLabels.size() == nClass,
                        "Invalid samples count for [" << classLabels.size() << "] classes (expected samples for " << nClass << " classes)",
                        Kernel::ErrorType::BadConfig);

    //We set the CMatrix fo the first classifier
    const size_t size = dataset[0].getSize();
    Kernel::TParameterHandler<CMatrix*> reference(m_subClassifiers[0]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
    reference->resize(dataset.getFeatureVectorCount(), size + 1);

    double* buffer = reference->getBuffer();
    for (size_t j = 0; j < dataset.getFeatureVectorCount(); ++j) {
        memcpy(buffer, dataset[j].getBuffer(), size * sizeof(double));
        //We let the space for the label
        buffer += (size + 1);
    }

    //And then we just change adapt the label for each feature vector but we don't copy them anymore
    for (size_t c = 0; c < m_subClassifiers.size(); ++c) {
        Kernel::TParameterHandler<CMatrix*> ip_dataset(m_subClassifiers[c]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));
        ip_dataset = static_cast<CMatrix*>(reference);

        buffer = ip_dataset->getBuffer();
        for (size_t j = 0; j < dataset.getFeatureVectorCount(); ++j) {
            //Modify the class of each featureVector
            const double classLabel = dataset[j].getLabel();
            if (size_t(classLabel) == c) { buffer[size] = 0; }
            else { buffer[size] = 1; }
            buffer += (size + 1);
        }

        m_subClassifiers[c]->process(OVTK_Algorithm_Classifier_InputTriggerId_Train);
    }
    return true;
}

bool CAlgorithmClassifierOneVsAll::classify(const Toolkit::IFeatureVector& sample, double& classId, Toolkit::IVector& distance, Toolkit::IVector& probability)
{
    std::vector<CClassifierOutput> classification;

    const size_t size = sample.getSize();

    for (size_t i = 0; i < m_subClassifiers.size(); ++i) {
        Kernel::IAlgorithmProxy* subClassifier = this->m_subClassifiers[i];
        Kernel::TParameterHandler<CMatrix*> ip_sample(subClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
        Kernel::TParameterHandler<double> op_class(subClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));
        Kernel::TParameterHandler<CMatrix*> op_values(subClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
        Kernel::TParameterHandler<CMatrix*> op_probabilities(subClassifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));
        ip_sample->resize(size);

        double* buffer = ip_sample->getBuffer();
        memcpy(buffer, sample.getBuffer(), size * sizeof(double));
        subClassifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify);

        CMatrix* probabilities = static_cast<CMatrix*>(op_probabilities);
        //If the algorithm give a probability we take it, instead we take the first value
        if (probabilities->getDimensionCount() != 0) { classification.push_back(CClassifierOutput(double(op_class), probabilities)); }
        else { classification.push_back(CClassifierOutput(double(op_class), static_cast<CMatrix*>(op_values))); }
        this->getLogManager() << Kernel::LogLevel_Debug << i << " " << double(op_class) << " " << double((*op_probabilities)[0])
                << " " << double((*op_probabilities)[1]) << "\n";
    }

    //Now, we determine the best classification
    CClassifierOutput best = CClassifierOutput(-1.0, static_cast<CMatrix*>(nullptr));
    classId                = -1;

    for (size_t i = 0; i < classification.size(); ++i) {
        const CClassifierOutput& tmp = classification[i];
        if (int(tmp.first) == 0)        // Predicts its "own" class, class=0
        {
            if (best.second == nullptr) {
                best    = tmp;
                classId = double(i);
            }
            else {
                if ((*m_fAlgorithmComparison)((*best.second), *(tmp.second)) > 0) {
                    best    = tmp;
                    classId = double(i);
                }
            }
        }
    }

    //If no one recognize the class, let's take the more relevant
    if (int(classId) == -1) {
        this->getLogManager() << Kernel::LogLevel_Debug << "Unable to find a class in first instance\n";
        for (size_t nClassification = 0; nClassification < classification.size(); ++nClassification) {
            const CClassifierOutput& tmp = classification[nClassification];
            if (best.second == nullptr) {
                best    = tmp;
                classId = double(nClassification);
            }
            else {
                //We take the one that is the least like the second class
                if ((*m_fAlgorithmComparison)((*best.second), *(tmp.second)) < 0) {
                    best    = tmp;
                    classId = double(nClassification);
                }
            }
        }
    }

    OV_ERROR_UNLESS_KRF(best.second != nullptr, "Unable to find a class for feature vector", Kernel::ErrorType::BadProcessing);

    // Now that we made the calculation, we send the corresponding data

    // For distances we just send the distance vector of the winner
    Kernel::IAlgorithmProxy* winner = this->m_subClassifiers[size_t(classId)];
    Kernel::TParameterHandler<CMatrix*> op_winnerValues(winner->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
    CMatrix* tmpMatrix = static_cast<CMatrix*>(op_winnerValues);
    distance.setSize(tmpMatrix->getBufferElementCount());
    memcpy(distance.getBuffer(), tmpMatrix->getBuffer(), tmpMatrix->getBufferElementCount() * sizeof(double));

    // We take the probabilities of the single class winning from each of the sub classifiers and normalize them
    double sum = 0;
    probability.setSize(m_subClassifiers.size());
    for (size_t i = 0; i < m_subClassifiers.size(); ++i) {
        Kernel::TParameterHandler<CMatrix*> op_Probabilities(
            m_subClassifiers[i]->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));
        probability[i] = op_Probabilities->getBuffer()[0];
        sum += probability[i];
    }

    for (size_t i = 0; i < probability.getSize(); ++i) { probability[i] /= sum; }

    return true;
}

bool CAlgorithmClassifierOneVsAll::addNewClassifierAtBack()
{
    const CIdentifier subClassifierAlgorithm = this->getAlgorithmManager().createAlgorithm(this->m_subClassifierAlgorithmID);

    OV_ERROR_UNLESS_KRF(subClassifierAlgorithm != CIdentifier::undefined(),
                        "Invalid classifier identifier [" << this->m_subClassifierAlgorithmID.str() << "]", Kernel::ErrorType::BadConfig);

    Kernel::IAlgorithmProxy* subClassifier = &this->getAlgorithmManager().getAlgorithm(subClassifierAlgorithm);
    subClassifier->initialize();

    Kernel::TParameterHandler<uint64_t> ip_nClasses(subClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_NClasses));
    ip_nClasses = 2;

    //Set a references to the extra parameters input of the pairing strategy
    Kernel::TParameterHandler<std::map<CString, CString>*> ip_params(
        subClassifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
    ip_params.setReferenceTarget(this->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));

    this->m_subClassifiers.push_back(subClassifier);

    return true;
}

void CAlgorithmClassifierOneVsAll::removeClassifierAtBack()
{
    Kernel::IAlgorithmProxy* subClassifier = m_subClassifiers.back();
    subClassifier->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*subClassifier);
    this->m_subClassifiers.pop_back();
}

bool CAlgorithmClassifierOneVsAll::designArchitecture(const CIdentifier& id, const size_t nClass)
{
    if (!this->setSubClassifierIdentifier(id)) { return false; }
    for (size_t i = 0; i < nClass; ++i) { if (!this->addNewClassifierAtBack()) { return false; } }
    return true;
}

XML::IXMLNode* CAlgorithmClassifierOneVsAll::getClassifierConfig(Kernel::IAlgorithmProxy* classifier)
{
    Kernel::TParameterHandler<XML::IXMLNode*> op_config(classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfig);
    XML::IXMLNode* res = op_config;
    return res;
}

XML::IXMLNode* CAlgorithmClassifierOneVsAll::saveConfig()
{
    XML::IXMLNode* oneVsAllNode = XML::createNode(TYPE_NODE_NAME);

    XML::IXMLNode* tempNode = XML::createNode(SUB_CLASSIFIER_IDENTIFIER_NODE_NAME);
    tempNode->addAttribute(ALGORITHM_ID_ATTRIBUTE, this->m_subClassifierAlgorithmID.str().c_str());
    tempNode->setPCData(
        this->getTypeManager().getEnumerationEntryNameFromValue(OVTK_TypeId_ClassificationAlgorithm, m_subClassifierAlgorithmID.id()).toASCIIString());
    oneVsAllNode->addChild(tempNode);

    tempNode = XML::createNode(SUB_CLASSIFIER_COUNT_NODE_NAME);
    tempNode->setPCData(std::to_string(getClassCount()).c_str());
    oneVsAllNode->addChild(tempNode);

    XML::IXMLNode* subClassifersNode = XML::createNode(SUB_CLASSIFIERS_NODE_NAME);

    //We now add configuration of each subclassifiers
    for (size_t i = 0; i < m_subClassifiers.size(); ++i) { subClassifersNode->addChild(getClassifierConfig(m_subClassifiers[i])); }
    oneVsAllNode->addChild(subClassifersNode);

    return oneVsAllNode;
}

bool CAlgorithmClassifierOneVsAll::loadConfig(XML::IXMLNode* configNode)
{
    XML::IXMLNode* tempNode = configNode->getChildByName(SUB_CLASSIFIER_IDENTIFIER_NODE_NAME);
    CIdentifier id;
    id.fromString(std::string(tempNode->getAttribute(ALGORITHM_ID_ATTRIBUTE)));
    if (m_subClassifierAlgorithmID != id) {
        while (!m_subClassifiers.empty()) { this->removeClassifierAtBack(); }
        if (!this->setSubClassifierIdentifier(id)) {
            //if the sub classifier doesn't have comparison function it is an error
            return false;
        }
    }

    tempNode = configNode->getChildByName(SUB_CLASSIFIER_COUNT_NODE_NAME);
    std::stringstream countData(tempNode->getPCData());
    uint64_t nClass;
    countData >> nClass;

    while (nClass != getClassCount()) {
        if (nClass < getClassCount()) { this->removeClassifierAtBack(); }
        else { if (!this->addNewClassifierAtBack()) { return false; } }
    }

    return loadSubClassifierConfig(configNode->getChildByName(SUB_CLASSIFIERS_NODE_NAME));
}

size_t CAlgorithmClassifierOneVsAll::getNDistances()
{
    Kernel::TParameterHandler<CMatrix*> op_distances(m_subClassifiers[0]->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
    return op_distances->getDimensionSize(0);
}

bool CAlgorithmClassifierOneVsAll::loadSubClassifierConfig(const XML::IXMLNode* node)
{
    for (size_t i = 0; i < node->getChildCount(); ++i) {
        XML::IXMLNode* subClassifierNode = node->getChild(i);
        Kernel::TParameterHandler<XML::IXMLNode*> ip_config(m_subClassifiers[i]->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Config));
        ip_config = subClassifierNode;

        OV_ERROR_UNLESS_KRF(m_subClassifiers[i]->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfig),
                            "Unable to load the configuration of the classifier " << i + 1, Kernel::ErrorType::Internal);
    }
    return true;
}

bool CAlgorithmClassifierOneVsAll::setSubClassifierIdentifier(const CIdentifier& id)
{
    m_subClassifierAlgorithmID = id;
    m_fAlgorithmComparison     = Toolkit::getClassificationComparisonFunction(id);

    OV_ERROR_UNLESS_KRF(m_fAlgorithmComparison != nullptr,
                        "No comparison function found for classifier [" << m_subClassifierAlgorithmID.str() << "]", Kernel::ErrorType::ResourceNotFound);

    return true;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
