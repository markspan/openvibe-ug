///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClassifierTrainer.cpp
/// \brief Classes implementation for the Box Classifier trainer.
/// \author Yann Renard (Inria) / Guillaume Serrière (Inria).
/// \version 2.0.
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmClassifierTrainer.hpp"
#include <system/ovCMath.h>

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

#include <sstream>
#include <cmath>
#include <algorithm>
#include <random>

#include <map>

#include <iomanip> // setw
//This needs to reachable from outside

namespace OpenViBE {
namespace Plugins {
namespace Classification {

bool CBoxAlgorithmClassifierTrainer::initialize()
{
    m_classifier = nullptr;
    m_parameter  = nullptr;

    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    //As we add some parameter in the middle of "static" parameters, we cannot rely on settings index.
    m_parameter = new std::map<CString, CString>();
    for (size_t i = 0; i < boxContext.getSettingCount(); ++i) {
        CString name;
        boxContext.getSettingName(i, name);
        const CString value  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        (*m_parameter)[name] = value;
    }

    bool isPairing = false;

    // UG fork: setting 1 is the filename. Upstream reads setting 2, the multiclass strategy, so an empty filename went unnoticed until after training.
    const CString configFilename(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));

    OV_ERROR_UNLESS_KRF(configFilename != CString(""), "this box's setting \"Filename to save configuration to\" is empty. Would you be so very kind as to choose a file? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    CIdentifier classifierAlgorithmClassID;

    const CIdentifier strategyClassID = this->getTypeManager().getEnumerationEntryValueFromName(
        OVTK_TypeId_ClassificationStrategy, (*m_parameter)[MULTICLASS_STRATEGY_SETTING_NAME]);
    classifierAlgorithmClassID = this->getTypeManager().getEnumerationEntryValueFromName(
        OVTK_TypeId_ClassificationAlgorithm, (*m_parameter)[ALGORITHM_SETTING_NAME]);

    if (strategyClassID == CIdentifier::undefined()) {
        //That means that we want to use a classical algorithm so just let's create it
        const CIdentifier classifierAlgorithmID = this->getAlgorithmManager().createAlgorithm(classifierAlgorithmClassID);

        OV_ERROR_UNLESS_KRF(classifierAlgorithmID != CIdentifier::undefined(),
                            "the classifier [" << (*m_parameter)[ALGORITHM_SETTING_NAME] << "] from this box's setting \"Algorithm to use\" could not be created. Perhaps a plugin is missing. Would you be so very kind as to choose another? Thank you for your patience.", Kernel::ErrorType::BadConfig);

        m_classifier = &this->getAlgorithmManager().getAlgorithm(classifierAlgorithmID);
        m_classifier->initialize();
    }
    else {
        isPairing    = true;
        m_classifier = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(strategyClassID));
        m_classifier->initialize();
    }
    m_trainStimulation = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, (*m_parameter)[TRAIN_TRIGGER_SETTING_NAME]);

    const int64_t nPartition = this->getConfigurationManager().expandAsInteger((*m_parameter)[FOLD_SETTING_NAME]);

    OV_ERROR_UNLESS_KRF(nPartition >= 0, "this box's setting \"Number of partitions for k-fold cross-validation test\" is " << nPartition << ", while it may not be negative. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    m_nPartition = uint64_t(nPartition);

    m_stimDecoder.initialize(*this, 0);
    for (size_t i = 1; i < boxContext.getInputCount(); ++i) {
        m_sampleDecoder.push_back(new Toolkit::TFeatureVectorDecoder<CBoxAlgorithmClassifierTrainer>());
        m_sampleDecoder.back()->initialize(*this, i);
    }

    //We link the parameters to the extra parameters input parameter to transmit them
    Kernel::TParameterHandler<std::map<CString, CString>*> ip_parameter(
        m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_ExtraParameter));
    ip_parameter = m_parameter;

    m_encoder.initialize(*this, 0);

    m_nFeatures.clear();

    OV_ERROR_UNLESS_KRF(boxContext.getInputCount() >= 2, "this box has " << boxContext.getInputCount() << " input, while it needs at least 2: one for the stimulations and one for the features. Would you be so very kind as to add one? Thank you for your patience.",
                        Kernel::ErrorType::BadSetting);

    // Provide the number of classes to the classifier
    const size_t nClass = boxContext.getInputCount() - 1;
    Kernel::TParameterHandler<uint64_t> ip_nClasses(m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_NClasses));
    ip_nClasses = nClass;

    //If we have to deal with a pairing strategy we have to pass argument
    if (isPairing) {
        Kernel::TParameterHandler<CIdentifier*> ip_classId(
            m_classifier->getInputParameter(OVTK_Algorithm_PairingStrategy_InputParameterId_SubClassifierAlgorithm));
        ip_classId = &classifierAlgorithmClassID;

        OV_ERROR_UNLESS_KRF(m_classifier->process(OVTK_Algorithm_PairingStrategy_InputTriggerId_DesignArchitecture), "Failed to design architecture",
                            Kernel::ErrorType::Internal);
    }

    return true;
}

bool CBoxAlgorithmClassifierTrainer::uninitialize()
{
    m_stimDecoder.uninitialize();
    m_encoder.uninitialize();

    if (m_classifier) {
        m_classifier->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_classifier);
    }

    for (size_t i = 0; i < m_sampleDecoder.size(); ++i) {
        m_sampleDecoder[i]->uninitialize();
        delete m_sampleDecoder[i];
    }
    m_sampleDecoder.clear();

    m_encoder.uninitialize();
    m_stimDecoder.uninitialize();

    for (size_t i = 0; i < m_datasets.size(); ++i) {
        delete m_datasets[i].sampleMatrix;
        m_datasets[i].sampleMatrix = nullptr;
    }
    m_datasets.clear();

    if (m_parameter) {
        delete m_parameter;
        m_parameter = nullptr;
    }

    // @fixme who frees this? freeing here -> crash
    /*
    if(m_pExtraParameter != nullptr)
    {
        delete m_pExtraParameter;
        m_pExtraParameter = NULL;
    }
    */

    return true;
}

bool CBoxAlgorithmClassifierTrainer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

// Find the most likely class and resample the dataset so that each class is as likely
bool CBoxAlgorithmClassifierTrainer::balanceDataset()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    const size_t nClass            = boxContext.getInputCount() - 1;

    this->getLogManager() << Kernel::LogLevel_Info << "Balancing dataset...\n";

    // Collect index set of feature vectors per class
    std::vector<std::vector<size_t>> classIndexes;
    classIndexes.resize(nClass);
    for (size_t i = 0; i < m_datasets.size(); ++i) { classIndexes[m_datasets[i].inputIdx].push_back(i); }

    // Count how many vectors the largest class has
    size_t nMax = 0;
    for (size_t i = 0; i < nClass; ++i) { nMax = std::max<size_t>(nMax, classIndexes[i].size()); }

    m_balancedDatasets.clear();

    // Pad those classes with resampled examples (sampling with replacement) that have fewer examples than the largest class
    for (size_t i = 0; i < nClass; ++i) {
        const size_t examplesInClass = classIndexes[i].size();
        const size_t paddingNeeded   = nMax - examplesInClass;
        if (examplesInClass == 0) {
            this->getLogManager() << Kernel::LogLevel_Debug << "Cannot resample class " << i << ", 0 examples\n";
            continue;
        }
        if (paddingNeeded > 0) { this->getLogManager() << Kernel::LogLevel_Debug << "Padding class " << i << " with " << paddingNeeded << " examples\n"; }

        // Copy all the examples first to a temporary array so we don't mess with the original data.
        // This is not too bad as instead of data, we copy the pointer. m_datasets owns the data pointer.
        const std::vector<size_t>& thisClassesIndexes = classIndexes[i];
        for (size_t j = 0; j < examplesInClass; ++j) { m_balancedDatasets.push_back(m_datasets[thisClassesIndexes[j]]); }

        for (size_t j = 0; j < paddingNeeded; ++j) {
            const size_t sampledIndex    = System::Math::randomWithCeiling(examplesInClass);
            const sample_t& sourceVector = m_datasets[thisClassesIndexes[sampledIndex]];
            m_balancedDatasets.push_back(sourceVector);
        }
    }

    return true;
}

bool CBoxAlgorithmClassifierTrainer::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    bool startTrain = false;

    // Parses stimulations
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_stimDecoder.decode(i);

        if (m_stimDecoder.isHeaderReceived()) {
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, 0, 0);
        }
        if (m_stimDecoder.isBufferReceived()) {
            const CStimulationSet* iStimulationSet = m_stimDecoder.getOutputStimulationSet();
            const CStimulationSet* oStimulationSet = m_encoder.getInputStimulationSet();
            oStimulationSet->clear();

            for (size_t j = 0; j < iStimulationSet->size(); ++j) {
                if (iStimulationSet->getId(j) == m_trainStimulation) {
                    startTrain        = true;
                    const uint64_t id = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, "OVTK_StimulationId_TrainCompleted");
                    oStimulationSet->push_back(id, iStimulationSet->getDate(j), 0);
                }
            }
            m_encoder.encodeBuffer();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_stimDecoder.isEndReceived()) {
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    // Parses feature vectors
    for (size_t i = 1; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            m_sampleDecoder[i - 1]->decode(j);

            if (m_sampleDecoder[i - 1]->isHeaderReceived()) { }
            if (m_sampleDecoder[i - 1]->isBufferReceived()) {
                const CMatrix* sampleMatrix = m_sampleDecoder[i - 1]->getOutputMatrix();

                sample_t sample;
                sample.sampleMatrix = new CMatrix();
                sample.startTime    = boxContext.getInputChunkStartTime(i, j);
                sample.endTime      = boxContext.getInputChunkEndTime(i, j);
                sample.inputIdx     = i - 1;

                sample.sampleMatrix->copy(*sampleMatrix);
                m_datasets.push_back(sample);
                m_nFeatures[i]++;
            }
            if (m_sampleDecoder[i - 1]->isEndReceived()) { }
        }
    }

    // On train stimulation reception, build up the labelled feature vector set matrix and go on training
    if (startTrain) {
        OV_ERROR_UNLESS_KRF(m_datasets.size() >= m_nPartition,
                            "Received fewer examples (" << m_datasets.size() << ") than specified partition count (" << m_nPartition << ")",
                            Kernel::ErrorType::BadInput);

        OV_ERROR_UNLESS_KRF(!m_datasets.empty(), "No training example received", Kernel::ErrorType::BadInput);

        this->getLogManager() << Kernel::LogLevel_Info << "Received train stimulation. Data dim is [" << m_datasets.size() << "x"
                << m_datasets[0].sampleMatrix->getBufferElementCount() << "]\n";
        for (size_t i = 1; i < nInput; ++i) {
            this->getLogManager() << Kernel::LogLevel_Info << "For information, we have " << m_nFeatures[i] << " feature vector(s) for input " << i << "\n";
        }

        const bool balancedDataset = this->getConfigurationManager().expandAsBoolean((*m_parameter)[BALANCE_SETTING_NAME]);
        if (balancedDataset) { balanceDataset(); }

        const std::vector<sample_t>& actualDataset = (balancedDataset ? m_balancedDatasets : m_datasets);

        std::vector<double> partitionAccuracies(m_nPartition);

        // create a vector used for mapping feature vectors (initialize it as v[i] = i)
        std::vector<size_t> featurePermutation;
        for (size_t i = 0; i < actualDataset.size(); ++i) { featurePermutation.push_back(i); }

        const bool randomizedKFoldTestData = this->getConfigurationManager().expandAsBoolean((*m_parameter)[RANDOMIZE_SETTING_NAME]);

        // randomize the vector if necessary
        if (randomizedKFoldTestData) {
            this->getLogManager() << Kernel::LogLevel_Info << "Randomizing the feature vector set\n";
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(featurePermutation.begin(), featurePermutation.end(), g);
        }

        const size_t nClass = nInput - 1;
        CMatrix confusion(nClass, nClass);

        if (m_nPartition >= 2) {
            double partitionAccuracy = 0;
            double finalAccuracy     = 0;

            this->getLogManager() << Kernel::LogLevel_Info << "k-fold test could take quite a long time, be patient\n";
            for (size_t i = 0; i < m_nPartition; ++i) {
                const size_t startIdx = size_t(((i) * actualDataset.size()) / m_nPartition);
                const size_t stopIdx  = size_t(((i + 1) * actualDataset.size()) / m_nPartition);

                this->getLogManager() << Kernel::LogLevel_Trace << "Training on partition " << i << " (feature vectors " << startIdx << " to " <<
                        stopIdx - 1 << ")...\n";

                OV_ERROR_UNLESS_KRF(this->train(actualDataset, featurePermutation, startIdx, stopIdx), "Training failed: bailing out (from xval)",
                                    Kernel::ErrorType::Internal);

                partitionAccuracy      = this->getAccuracy(actualDataset, featurePermutation, startIdx, stopIdx, confusion);
                partitionAccuracies[i] = partitionAccuracy;
                finalAccuracy += partitionAccuracy;

                this->getLogManager() << Kernel::LogLevel_Info << "Finished with partition " << i + 1 << " / " << m_nPartition << " (performance : "
                        << partitionAccuracy << "%)\n";
            }

            const double mean = finalAccuracy / double(m_nPartition);
            double deviation  = 0;

            for (size_t i = 0; i < m_nPartition; ++i) {
                const double diff = partitionAccuracies[i] - mean;
                deviation += diff * diff;
            }
            deviation = sqrt(deviation / double(m_nPartition));

            this->getLogManager() << Kernel::LogLevel_Info << "Cross-validation test accuracy is " << mean << "% (sigma = " << deviation << "%)\n";

            printConfusionMatrix(confusion);
        }
        else {
            this->getLogManager() << Kernel::LogLevel_Info << "Training without cross-validation.\n";
            this->getLogManager() << Kernel::LogLevel_Info << "*** Reported training set accuracy will be optimistic ***\n";
        }


        this->getLogManager() << Kernel::LogLevel_Trace << "Training final classifier on the whole set...\n";

        OV_ERROR_UNLESS_KRF(this->train(actualDataset, featurePermutation, 0, 0),
                            "Training failed: bailing out (from whole set training)", Kernel::ErrorType::Internal);

        confusion.resetBuffer();
        const double accuracy = this->getAccuracy(actualDataset, featurePermutation, 0, actualDataset.size(), confusion);

        this->getLogManager() << Kernel::LogLevel_Info << "Training set accuracy is " << accuracy << "% (optimistic)\n";

        printConfusionMatrix(confusion);

        OV_ERROR_UNLESS_KRF(this->saveConfig(), "Failed to save configuration", Kernel::ErrorType::Internal);
    }

    return true;
}

bool CBoxAlgorithmClassifierTrainer::train(const std::vector<sample_t>& dataset, const std::vector<size_t>& permutation, const size_t startIdx,
                                           const size_t stopIdx)
{
    OV_ERROR_UNLESS_KRF(stopIdx - startIdx != 1, "Invalid indexes: stopIdx - trainIndex = 1", Kernel::ErrorType::BadArgument);

    const size_t nSample  = dataset.size() - (stopIdx - startIdx);
    const size_t nFeature = dataset[0].sampleMatrix->getBufferElementCount();

    Kernel::TParameterHandler<CMatrix*> ip_sample(m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVectorSet));

    ip_sample->resize(nSample, nFeature + 1);

    double* buffer = ip_sample->getBuffer();
    for (size_t j = 0; j < dataset.size() - (stopIdx - startIdx); ++j) {
        const size_t k       = permutation[(j < startIdx ? j : j + (stopIdx - startIdx))];
        const double classId = double(dataset[k].inputIdx);
        memcpy(buffer, dataset[k].sampleMatrix->getBuffer(), nFeature * sizeof(double));

        buffer[nFeature] = classId;
        buffer += (nFeature + 1);
    }

    OV_ERROR_UNLESS_KRF(m_classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Train), "Training failed", Kernel::ErrorType::Internal);

    Kernel::TParameterHandler<XML::IXMLNode*> op_configuration(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    XML::IXMLNode* node = static_cast<XML::IXMLNode*>(op_configuration);

    if (node != nullptr) { node->release(); }
    op_configuration = nullptr;

    return m_classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_SaveConfig);
}

// Note that this function is incremental for confusionMatrix and can be called many times; so we don't clear the matrix
double CBoxAlgorithmClassifierTrainer::getAccuracy(const std::vector<sample_t>& dataset, const std::vector<size_t>& permutation,
                                                   const size_t startIdx, const size_t stopIdx, CMatrix& confusionMatrix)
{
    OV_ERROR_UNLESS_KRF(stopIdx != startIdx, "Invalid indexes: start index equals stop index", Kernel::ErrorType::BadArgument);

    const size_t nFeature = dataset[0].sampleMatrix->getBufferElementCount();

    Kernel::TParameterHandler<XML::IXMLNode*> op_config(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    XML::IXMLNode* node = op_config;//Requested for affectation
    Kernel::TParameterHandler<XML::IXMLNode*> ip_config(m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Config));
    ip_config = node;

    m_classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfig);

    Kernel::TParameterHandler<CMatrix*> ip_sample(m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector));
    Kernel::TParameterHandler<double> op_classificationState(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));
    ip_sample->resize(nFeature);

    size_t nSuccess = 0;

    for (size_t j = startIdx; j < stopIdx; ++j) {
        const size_t k = permutation[j];

        double* buffer            = ip_sample->getBuffer();
        const double correctValue = double(dataset[k].inputIdx);

        this->getLogManager() << Kernel::LogLevel_Debug << "Try to recognize " << correctValue << "\n";

        memcpy(buffer, dataset[k].sampleMatrix->getBuffer(), nFeature * sizeof(double));

        m_classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify);

        const double predictedValue = op_classificationState;

        this->getLogManager() << Kernel::LogLevel_Debug << "Recognize " << predictedValue << "\n";

        if (predictedValue == correctValue) { nSuccess++; }

        if (size_t(predictedValue) < confusionMatrix.getDimensionSize(0) && size_t(correctValue) < confusionMatrix.getDimensionSize(0)) {
            double* buf = confusionMatrix.getBuffer();
            buf[size_t(correctValue) * confusionMatrix.getDimensionSize(1) + size_t(predictedValue)] += 1.0;
        }
        else { std::cout << "error\n"; }
    }

    return double(nSuccess * 100) / double(stopIdx - startIdx);
}

bool CBoxAlgorithmClassifierTrainer::printConfusionMatrix(const CMatrix& oMatrix)
{
    OV_ERROR_UNLESS_KRF(oMatrix.getDimensionCount() == 2 && oMatrix.getDimensionSize(0) == oMatrix.getDimensionSize(1),
                        "Invalid confution matrix [dim count = " << oMatrix.getDimensionCount() << ", dim size 0 = "
                        << oMatrix.getDimensionSize(0) << ", dim size 1 = "<< oMatrix.getDimensionSize(1) << "] (expected 2 dimensions with same size)",
                        Kernel::ErrorType::BadArgument);

    const size_t rows = oMatrix.getDimensionSize(0);

    if (rows > 10 && !this->getConfigurationManager().expandAsBoolean("${Plugin_Classification_ForceConfusionMatrixPrint}")) {
        this->getLogManager() << Kernel::LogLevel_Info <<
                "Over 10 classes, not printing the confusion matrix. If needed, override with setting Plugin_Classification_ForceConfusionMatrixPrint token to true.\n";
        return true;
    }

    CMatrix tmp(oMatrix), rowSum(rows);

    for (size_t i = 0; i < rows; ++i) {
        const size_t idx = i * rows;
        for (size_t j = 0; j < rows; ++j) { rowSum[i] += tmp[idx + j]; }
        for (size_t j = 0; j < rows; ++j) { tmp[idx + j] /= rowSum[i]; }
    }

    std::stringstream ss;
    ss << std::fixed;

    ss << "  Cls vs cls ";
    for (size_t i = 0; i < rows; ++i) { ss << std::setw(6) << (i + 1); }
    this->getLogManager() << Kernel::LogLevel_Info << ss.str() << "\n";

    ss.precision(1);
    for (size_t i = 0; i < rows; ++i) {
        ss.str("");
        ss << "  Target " << std::setw(2) << (i + 1) << ": ";
        for (size_t j = 0; j < rows; ++j) { ss << std::setw(6) << tmp[i * rows + j] * 100; }
        this->getLogManager() << Kernel::LogLevel_Info << ss.str() << " %, " << size_t(rowSum[i]) << " examples\n";
    }

    return true;
}

bool CBoxAlgorithmClassifierTrainer::saveConfig()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    Kernel::TParameterHandler<XML::IXMLNode*> op_config(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    XML::IXMLNode* algorithmConfigNode = XML::createNode(CLASSIFIER_ROOT);
    algorithmConfigNode->addChild(static_cast<XML::IXMLNode*>(op_config));

    XML::IXMLHandler* handler = XML::createXMLHandler();
    const CString configurationFilename(this->getConfigurationManager().expand((*m_parameter)[FILENAME_SETTING_NAME]));

    XML::IXMLNode* root = XML::createNode(CLASSIFICATION_BOX_ROOT);
    std::stringstream version;
    version << Classification_BoxTrainerFormatVersion;
    root->addAttribute(FORMAT_VERSION_ATTRIBUTE_NAME, version.str().c_str());

    const auto cleanup = [&]()
    {
        handler->release();
        root->release();
        op_config = nullptr;
    };
    root->addAttribute(CREATOR_ATTRIBUTE_NAME, this->getConfigurationManager().expand("${Application_Name}"));
    root->addAttribute(CREATOR_VERSION_ATTRIBUTE_NAME, this->getConfigurationManager().expand("${Application_Version}"));

    XML::IXMLNode* tempNode           = XML::createNode(STRATEGY_NODE_NAME);
    const CIdentifier strategyClassId = this->getTypeManager().getEnumerationEntryValueFromName(
        OVTK_TypeId_ClassificationStrategy, (*m_parameter)[MULTICLASS_STRATEGY_SETTING_NAME]);
    tempNode->addAttribute(IDENTIFIER_ATTRIBUTE_NAME, strategyClassId.str().c_str());
    tempNode->setPCData((*m_parameter)[MULTICLASS_STRATEGY_SETTING_NAME].toASCIIString());
    root->addChild(tempNode);

    tempNode                            = XML::createNode(ALGORITHM_NODE_NAME);
    const CIdentifier classifierClassId = this->getTypeManager().getEnumerationEntryValueFromName(
        OVTK_TypeId_ClassificationAlgorithm, (*m_parameter)[ALGORITHM_SETTING_NAME]);
    tempNode->addAttribute(IDENTIFIER_ATTRIBUTE_NAME, classifierClassId.str().c_str());
    tempNode->setPCData((*m_parameter)[ALGORITHM_SETTING_NAME].toASCIIString());
    root->addChild(tempNode);


    XML::IXMLNode* stimulationsNode = XML::createNode(STIMULATIONS_NODE_NAME);

    for (size_t i = 1; i < boxContext.getInputCount(); ++i) {
        const std::string name = "Class " + std::to_string(i) + " label";
        const std::string id   = std::to_string(i - 1);
        tempNode               = XML::createNode(CLASS_STIMULATION_NODE_NAME);
        tempNode->addAttribute(IDENTIFIER_ATTRIBUTE_NAME, id.c_str());
        tempNode->setPCData((*m_parameter)[name.c_str()].toASCIIString());
        stimulationsNode->addChild(tempNode);
    }
    root->addChild(stimulationsNode);

    root->addChild(algorithmConfigNode);

    if (!handler->writeXMLInFile(*root, configurationFilename.toASCIIString())) {
        cleanup();
        OV_ERROR_KRF("the trained classifier could not be saved in the file [" << configurationFilename << "] from this box's setting \"Filename to save configuration to\". Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);
    }

    cleanup();
    return true;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
