///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixClassifierProcessor.cpp
/// \brief Classes implementation for the box Process a Matrix Classifier.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/01/2018.
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
#include "CBoxAlgorithmMatrixClassifierProcessor.hpp"
#include <geometry/classifier/CMatrixClassifierMDMRebias.hpp>
#include <geometry/classifier/CMatrixClassifierFgMDMRTRebias.hpp>
#include "eigen/convert.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::initialize()
{
    //***** Codecs *****
    m_i0StimulationCodec.initialize(*this, 0);
    m_i1MatrixCodec.initialize(*this, 1);
    m_o0StimulationCodec.initialize(*this, 0);
    m_o1MatrixCodec.initialize(*this, 1);
    m_o2MatrixCodec.initialize(*this, 2);

    //***** Pointers *****
    m_i0Stimulation = m_i0StimulationCodec.getOutputStimulationSet();
    m_i1Matrix      = m_i1MatrixCodec.getOutputMatrix();
    m_o0Stimulation = m_o0StimulationCodec.getInputStimulationSet();
    m_o1Matrix      = m_o1MatrixCodec.getInputMatrix();
    m_o2Matrix      = m_o2MatrixCodec.getInputMatrix();

    // Settings
    m_ifilename  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_ofilename  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_adaptation = Geometry::EAdaptations(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));

    m_logLevel = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3)));

    OV_ERROR_UNLESS_KRF(m_ifilename.length() != 0, "this box's setting \"Filename to load classifier model\" is empty. Would you be so very kind as to choose a file? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    OV_ERROR_UNLESS_KRF(loadXML(), "the classifier in the file [" << m_ifilename << "], from this box's setting \"Filename to load classifier model\", could not be read, as explained just before this message, if there is an explanation. Please forgive us.", Kernel::ErrorType::BadFileRead);
    // Change matrix size
    m_o1Matrix->resize(m_classifier->GetClassCount());
    m_o2Matrix->resize(m_classifier->GetClassCount());

    // Printing info
    std::stringstream msg;
    msg << std::endl << "Input Filename : " << m_ifilename << std::endl << "Output Filename : " << m_ofilename << std::endl
            << "Method : " << m_classifier->GetType() << " with " << toString(m_adaptation) << " adaptation" << std::endl
            << "Number of classes : " << m_classifier->GetClassCount() << std::endl;
    for (size_t k = 0; k < m_classifier->GetClassCount(); ++k) {
        msg << "Stimulation for class " << k << " : " << m_stimulationClassName[k] << " => ["
                << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, m_stimulationClassName[k]) << "]\n";
    }
    this->getLogManager() << m_logLevel << msg.str();

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::uninitialize()
{
    if (m_ofilename.length() != 0) { saveXML(); }
    m_i0StimulationCodec.uninitialize();
    m_i1MatrixCodec.uninitialize();
    m_o0StimulationCodec.uninitialize();
    m_o1MatrixCodec.uninitialize();
    m_o2MatrixCodec.uninitialize();

    delete m_classifier;    // check if pointeur is null is useless now.
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    //**** Stimulation *****
    if (m_adaptation != Geometry::EAdaptations::None) {
        for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
            m_i0StimulationCodec.decode(i);
            if (m_i0StimulationCodec.isBufferReceived())                    // Buffer received
            {
                bool finish = false;
                for (size_t j = 0; j < m_i0Stimulation->size() && !finish; ++j) {
                    const uint64_t stim = m_i0Stimulation->getId(j);
                    for (size_t k = 0; k < m_stimulationClassName.size() && !finish; ++k) {
                        if (stim == this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Trial")) {
                            m_lastLabelReceived = std::numeric_limits<size_t>::max();
                            finish              = true;
                        }
                        else if (stim == m_stimulationClassName[k]) {
                            m_lastLabelReceived = k;
                            finish              = true;
                        }
                    }
                }
            }
        }
    }

    //***** Matrix *****
    if (m_adaptation == Geometry::EAdaptations::None || m_lastLabelReceived < m_stimulationClassName.size()) {
        for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
            m_i1MatrixCodec.decode(i);                                          // Decode the chunk
            OV_ERROR_UNLESS_KRF(m_i1Matrix->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);
            const uint64_t tStart = boxContext.getInputChunkStartTime(1, i),    // Time Code Chunk Start
                           tEnd   = boxContext.getInputChunkEndTime(1, i);      // Time Code Chunk End

            if (m_i1MatrixCodec.isHeaderReceived())                             // Header received
            {
                m_o0StimulationCodec.encodeHeader();
                m_o1MatrixCodec.encodeHeader();
                m_o2MatrixCodec.encodeHeader();
            }
            else if (m_i1MatrixCodec.isBufferReceived())                        // Buffer received
            {
                OV_ERROR_UNLESS_KRF(classify(tEnd), "Classify Error", Kernel::ErrorType::BadProcessing);
                m_o0StimulationCodec.encodeBuffer();
                m_o1MatrixCodec.encodeBuffer();
                m_o2MatrixCodec.encodeBuffer();
            }
            else if (m_i1MatrixCodec.isEndReceived())                           // End received
            {
                m_o0StimulationCodec.encodeEnd();
                m_o1MatrixCodec.encodeEnd();
                m_o2MatrixCodec.encodeEnd();
            }
            for (size_t j = 0; j < 3; ++j) { boxContext.markOutputAsReadyToSend(j, tStart, tEnd); }
        }
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::classify(const uint64_t tEnd)
{
    std::vector<double> distance, probability;
    Eigen::MatrixXd cov;
    size_t classId;
    MatrixConvert(*m_i1Matrix, cov);
    OV_ERROR_UNLESS_KRF(m_classifier->Classify(cov, classId, distance, probability, m_adaptation, m_lastLabelReceived), "Classify Error",
                        Kernel::ErrorType::BadProcessing);

    //Fill Output
    m_o0Stimulation->resize(1);                                 //No append stimulation only one is used
    m_o0Stimulation->setId(0, m_stimulationClassName[classId]);
    m_o0Stimulation->setDate(0, tEnd);
    m_o0Stimulation->setDuration(0, 0);
    MatrixConvert(distance, *m_o1Matrix);
    MatrixConvert(probability, *m_o2Matrix);

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::loadXML()
{
    delete m_classifier;    // if (m_classifer != nullptr) useless now

    tinyxml2::XMLDocument xmlDoc;
    // Load File
    OV_ERROR_UNLESS_KRF(xmlDoc.LoadFile(m_ifilename.toASCIIString()) == 0, "the file [" << m_ifilename.toASCIIString() << "] from this box's setting \"Filename to load classifier model\" could not be opened. Perhaps it does not exist, or the path is misspelt. Would you be so very kind as to check it? Thank you for your patience.",
                        Kernel::ErrorType::BadFileRead);

    // Load Root
    tinyxml2::XMLNode* root = xmlDoc.FirstChild();
    OV_ERROR_UNLESS_KRF(root != nullptr, "the file [" << m_ifilename.toASCIIString() << "] from the setting \"Filename to load classifier model\" has no root element. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);

    // Load Data
    tinyxml2::XMLElement* data = root->FirstChildElement("Classifier-data");
    OV_ERROR_UNLESS_KRF(data != nullptr, "the file [" << m_ifilename.toASCIIString() << "] from the setting \"Filename to load classifier model\" holds no classifier. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);

    const std::string classifierType = data->Attribute("type");

    // Check Type
    if (classifierType == toString(Geometry::EMatrixClassifiers::MDM)) { m_classifier = new Geometry::CMatrixClassifierMDM; }
    else if (classifierType == toString(Geometry::EMatrixClassifiers::MDM_Rebias)) { m_classifier = new Geometry::CMatrixClassifierMDMRebias; }
    else if (classifierType == toString(Geometry::EMatrixClassifiers::FgMDM_RT)) { m_classifier = new Geometry::CMatrixClassifierFgMDMRT; }
    else if (classifierType == toString(Geometry::EMatrixClassifiers::FgMDM_RT_Rebias)) { m_classifier = new Geometry::CMatrixClassifierFgMDMRTRebias; }
    else { OV_ERROR_UNLESS_KRF(false, "the file [" << m_ifilename.toASCIIString() << "] from this box's setting \"Filename to load classifier model\" holds the classifier type [" << classifierType << "], which we regret to say we do not know. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing); }

    // Object Load
    m_classifier->LoadXML(m_ifilename.toASCIIString());

    // Load Stimulation
    m_stimulationClassName.resize(m_classifier->GetClassCount());
    tinyxml2::XMLElement* element = data->FirstChildElement("Class");           // Get Fist Class Node
    for (size_t k = 0; k < m_classifier->GetClassCount(); ++k)                  // for each class
    {
        OV_ERROR_UNLESS_KRF(element != nullptr, "the file [" << m_ifilename.toASCIIString() << "] from the setting \"Filename to load classifier model\" is missing a class. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);
        const size_t idx = element->IntAttribute("class-id");                   // Get Id (normally idx = k)
        OV_ERROR_UNLESS_KRF(idx == k, "the file [" << m_ifilename.toASCIIString() << "] from the setting \"Filename to load classifier model\" lists its classes out of order. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);
        m_stimulationClassName[k] = this->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, element->Attribute("stimulation"));
        element                   = element->NextSiblingElement("Class");       // Next Class
    }

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierProcessor::saveXML()
{
    OV_ERROR_UNLESS_KRF(m_classifier->SaveXML(m_ofilename.toASCIIString()), "the classifier could not be saved in the file [" << m_ofilename.toASCIIString() << "] from this box's setting \"Filename to save classifier model\". Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);

    //***** Add Stimulation to XML *****
    tinyxml2::XMLDocument xmlDoc;
    // Load File
    OV_ERROR_UNLESS_KRF(xmlDoc.LoadFile(m_ofilename.toASCIIString()) == 0, "the classifier just saved in the file [" << m_ofilename.toASCIIString() << "] could not be read back. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.",
                        Kernel::ErrorType::BadFileRead);

    // Load Root
    tinyxml2::XMLNode* root = xmlDoc.FirstChild();
    OV_ERROR_UNLESS_KRF(root != nullptr, "the file [" << m_ofilename.toASCIIString() << "] from the setting \"Filename to save classifier model\" has no root element. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);

    // Load Data
    tinyxml2::XMLElement* data = root->FirstChildElement("Classifier-data");
    OV_ERROR_UNLESS_KRF(data != nullptr, "the file [" << m_ofilename.toASCIIString() << "] from the setting \"Filename to save classifier model\" holds no classifier. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);

    tinyxml2::XMLElement* element = data->FirstChildElement("Class");           // Get Fist Class Node
    for (size_t k = 0; k < m_classifier->GetClassCount(); ++k)                  // for each class
    {
        OV_ERROR_UNLESS_KRF(element != nullptr, "the file [" << m_ofilename.toASCIIString() << "] from the setting \"Filename to save classifier model\" is missing a class. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);
        const size_t idx = element->IntAttribute("class-id");                   // Get Id (normally idx = k)
        OV_ERROR_UNLESS_KRF(idx == k, "the file [" << m_ofilename.toASCIIString() << "] from the setting \"Filename to save classifier model\" lists its classes out of order. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);
        const CString stimulationName = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, m_stimulationClassName[k]);
        element->SetAttribute("stimulation", stimulationName.toASCIIString());
        element = element->NextSiblingElement("Class");                         // Next Class
    }
    return xmlDoc.SaveFile(m_ofilename.toASCIIString()) == 0;                   // save XML (if != 0 it means error)
}
//---------------------------------------------------------------------------------------------------
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
