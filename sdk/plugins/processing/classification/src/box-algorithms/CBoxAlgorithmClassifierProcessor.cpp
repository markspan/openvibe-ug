///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClassifierProcessor.cpp
/// \brief Classes implementation for the Box Classifier processor.
/// \author Yann Renard (Inria) / Guillaume Serrière (Inria).
/// \version 2.1.
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
#include "CBoxAlgorithmClassifierProcessor.hpp"

#include <sstream>

#include <xml/IXMLHandler.h>
#include <xml/IXMLNode.h>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

bool CBoxAlgorithmClassifierProcessor::loadClassifier(const char* filename)
{
    if (m_classifier) {
        m_classifier->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_classifier);
        m_classifier = nullptr;
    }

    XML::IXMLHandler* handler = XML::createXMLHandler();
    XML::IXMLNode* rootNode   = handler->parseFile(filename);

    OV_ERROR_UNLESS_KRF(rootNode, "the classifier file [" << filename << "] from this box's setting \"Filename to load configuration from\" could not be read. Perhaps it does not exist yet, because the training scenario has not run, or it is not a classifier file. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadParsing);

    m_stimulations.clear();

    // Check the version of the file
    OV_ERROR_UNLESS_KRF(rootNode->hasAttribute(FORMAT_VERSION_ATTRIBUTE_NAME), "Configuration file [" << filename << "] has no version information",
                        Kernel::ErrorType::ResourceNotFound);

    std::stringstream data(rootNode->getAttribute(FORMAT_VERSION_ATTRIBUTE_NAME));
    size_t version;
    data >> version;

    OV_WARNING_UNLESS_K(version <= Classification_BoxTrainerFormatVersion,
                        "Classifier configuration in [" << filename << "] saved using a newer version: saved version = [" << version
                        << "] vs current version = [" << Classification_BoxTrainerFormatVersion << "]");

    OV_ERROR_UNLESS_KRF(version >= Classification_BoxTrainerFormatVersionRequired,
                        "the classifier file [" << filename << "] was saved in format " << version << ", which is older than the format " << Classification_BoxTrainerFormatVersionRequired
                        << " we can read. Might we humbly suggest training again? Thank you most kindly.", Kernel::ErrorType::BadVersion);

    CIdentifier algorithmClassID = CIdentifier::undefined();

    XML::IXMLNode* tmp = rootNode->getChildByName(STRATEGY_NODE_NAME);

    OV_ERROR_UNLESS_KRF(tmp, "Configuration file [" << filename << "] has no node " << STRATEGY_NODE_NAME, Kernel::ErrorType::BadParsing);

    algorithmClassID.fromString(std::string(tmp->getAttribute(IDENTIFIER_ATTRIBUTE_NAME)));

    //If the Identifier is undefined, that means we need to load a native algorithm
    if (algorithmClassID == CIdentifier::undefined()) {
        tmp = rootNode->getChildByName(ALGORITHM_NODE_NAME);

        OV_ERROR_UNLESS_KRF(tmp, "Configuration file [" << filename << "] has no node " << ALGORITHM_NODE_NAME, Kernel::ErrorType::BadParsing);

        algorithmClassID.fromString(std::string(tmp->getAttribute(IDENTIFIER_ATTRIBUTE_NAME)));

        //If the algorithm is still unknown, that means that we face an error
        OV_ERROR_UNLESS_KRF(algorithmClassID != CIdentifier::undefined(), "the file [" << filename << "] from this box's setting \"Filename to load configuration from\" names no classifier we recognise. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.",
                            Kernel::ErrorType::BadConfig);
    }

    //Now loading all stimulations output
    XML::IXMLNode* stimNode = rootNode->getChildByName(STIMULATIONS_NODE_NAME);

    OV_ERROR_UNLESS_KRF(stimNode, "Configuration file [" << filename << "] has no node " << STIMULATIONS_NODE_NAME, Kernel::ErrorType::BadParsing);

    //Now load every stimulation and store them in the map with the right class id
    for (size_t i = 0; i < stimNode->getChildCount(); ++i) {
        tmp = stimNode->getChild(i);

        OV_ERROR_UNLESS_KRF(tmp, "Invalid NULL child node " << i << " for node [" << STIMULATIONS_NODE_NAME << "]", Kernel::ErrorType::BadParsing);

        CString name(tmp->getPCData());

        double classID;
        const char* att = tmp->getAttribute(IDENTIFIER_ATTRIBUTE_NAME);

        OV_ERROR_UNLESS_KRF(att, "Invalid child node " << i << " for node [" << STIMULATIONS_NODE_NAME << "]: attribute ["
                            << IDENTIFIER_ATTRIBUTE_NAME << "] not found", Kernel::ErrorType::BadParsing);

        std::stringstream ss(att);
        ss >> classID;
        m_stimulations[classID] = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, name);
    }

    const CIdentifier id = this->getAlgorithmManager().createAlgorithm(algorithmClassID);

    OV_ERROR_UNLESS_KRF(id != CIdentifier::undefined(),
                        "the file [" << filename << "] from this box's setting \"Filename to load configuration from\" asks for the classifier " << algorithmClassID.str() << ", which could not be created. Perhaps a plugin is missing. Might we trouble you to check? Thank you most kindly.",
                        Kernel::ErrorType::BadConfig);

    m_classifier = &this->getAlgorithmManager().getAlgorithm(id);
    m_classifier->initialize();

    // Connect the params to the new classifier

    Kernel::TParameterHandler<CMatrix*> ip_sample = m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_FeatureVector);
    ip_sample.setReferenceTarget(m_sampleDecoder.getOutputMatrix());

    m_hyperplanesEncoder.getInputMatrix().
                         setReferenceTarget(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ClassificationValues));
    m_probabilitiesEncoder.getInputMatrix().setReferenceTarget(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_ProbabilityValues));
    // note: labelsencoder cannot be directly bound here as the classifier returns a float, but we need to output a stimulation

    Kernel::TParameterHandler<XML::IXMLNode*> ip_classificationConfig(m_classifier->getInputParameter(OVTK_Algorithm_Classifier_InputParameterId_Config));
    ip_classificationConfig = rootNode->getChildByName(CLASSIFIER_ROOT)->getChild(0);

    OV_ERROR_UNLESS_KRF(m_classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_LoadConfig),
                        "Loading configuration failed for subclassifier [" << id.str() << "]", Kernel::ErrorType::Internal);

    rootNode->release();
    handler->release();

    return true;
}

bool CBoxAlgorithmClassifierProcessor::initialize()
{
    m_classifier = nullptr;

    //First of all, let's get the XML file for configuration
    const CString configFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    OV_ERROR_UNLESS_KRF(configFilename != CString(""), "this box's setting \"Filename to load configuration from\" is empty. Would you be so very kind as to choose a file? Thank you for your patience.", Kernel::ErrorType::BadConfig);

    m_sampleDecoder.initialize(*this, 0);
    m_stimDecoder.initialize(*this, 1);

    m_labelsEncoder.initialize(*this, 0);
    m_hyperplanesEncoder.initialize(*this, 1);
    m_probabilitiesEncoder.initialize(*this, 2);

    return loadClassifier(configFilename.toASCIIString());
}

bool CBoxAlgorithmClassifierProcessor::uninitialize()
{
    if (m_classifier) {
        m_classifier->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_classifier);
        m_classifier = nullptr;
    }

    m_probabilitiesEncoder.uninitialize();
    m_hyperplanesEncoder.uninitialize();
    m_labelsEncoder.uninitialize();

    m_stimDecoder.uninitialize();
    m_sampleDecoder.uninitialize();

    return true;
}

bool CBoxAlgorithmClassifierProcessor::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmClassifierProcessor::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Check if we have a command first
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isHeaderReceived()) { }
        if (m_stimDecoder.isBufferReceived()) {
            for (size_t j = 0; j < m_stimDecoder.getOutputStimulationSet()->size(); ++j) {
                if (m_stimDecoder.getOutputStimulationSet()->getId(j) == OVTK_StimulationId_TrainCompleted) {
                    CString configFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
                    if (!loadClassifier(configFilename.toASCIIString())) { return false; }
                }
            }
        }
        if (m_stimDecoder.isEndReceived()) { }
    }

    // Classify data
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        const uint64_t startTime = boxContext.getInputChunkStartTime(0, i);
        const uint64_t endTime   = boxContext.getInputChunkEndTime(0, i);

        m_sampleDecoder.decode(i);
        if (m_sampleDecoder.isHeaderReceived()) {
            m_labelsEncoder.encodeHeader();
            m_hyperplanesEncoder.encodeHeader();
            m_probabilitiesEncoder.encodeHeader();

            boxContext.markOutputAsReadyToSend(0, startTime, endTime);
            boxContext.markOutputAsReadyToSend(1, startTime, endTime);
            boxContext.markOutputAsReadyToSend(2, startTime, endTime);
        }
        if (m_sampleDecoder.isBufferReceived()) {
            OV_ERROR_UNLESS_KRF(m_classifier->process(OVTK_Algorithm_Classifier_InputTriggerId_Classify)
                                && m_classifier->isOutputTriggerActive(OVTK_Algorithm_Classifier_OutputTriggerId_Success),
                                "Classification failed", Kernel::ErrorType::Internal);

            Kernel::TParameterHandler<double> op_classificationState(m_classifier->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Class));

            const CStimulationSet* set = m_labelsEncoder.getInputStimulationSet();

            set->resize(1);
            set->setId(0, m_stimulations[op_classificationState]);
            set->setDate(0, endTime);
            set->setDuration(0, 0);

            m_labelsEncoder.encodeBuffer();
            m_hyperplanesEncoder.encodeBuffer();
            m_probabilitiesEncoder.encodeBuffer();

            boxContext.markOutputAsReadyToSend(0, startTime, endTime);
            boxContext.markOutputAsReadyToSend(1, startTime, endTime);
            boxContext.markOutputAsReadyToSend(2, startTime, endTime);
        }

        if (m_sampleDecoder.isEndReceived()) {
            m_labelsEncoder.encodeEnd();
            m_hyperplanesEncoder.encodeEnd();
            m_probabilitiesEncoder.encodeEnd();

            boxContext.markOutputAsReadyToSend(0, startTime, endTime);
            boxContext.markOutputAsReadyToSend(1, startTime, endTime);
            boxContext.markOutputAsReadyToSend(2, startTime, endTime);
        }
    }

    return true;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
