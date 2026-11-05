///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixClassifierTrainer.cpp
/// \brief Classes implementation for the box Train a Matrix Classifier.
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
#include "CBoxAlgorithmMatrixClassifierTrainer.hpp"
#include <geometry/classifier/CMatrixClassifierMDMRebias.hpp>
#include <geometry/classifier/CMatrixClassifierFgMDMRTRebias.hpp>
#include "tinyxml2.h"
#include "eigen/convert.hpp"
#include "boost/format.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainer::initialize()
{
    // Stimulations
    m_i0StimulationCodec.initialize(*this, 0);
    m_iStimulation = m_i0StimulationCodec.getOutputStimulationSet();

    m_o0StimulationCodec.initialize(*this, 0);
    m_oStimulation = m_o0StimulationCodec.getInputStimulationSet();

    // Classes
    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    m_nbClass                      = boxContext.getInputCount() - 1;
    m_i1MatrixCodec.resize(m_nbClass);
    m_iMatrix.resize(m_nbClass);
    m_covs.resize(m_nbClass);
    m_stimulationClassName.resize(m_nbClass);
    for (size_t k = 0; k < m_nbClass; ++k) {
        m_i1MatrixCodec[k].initialize(*this, k + 1);
        m_iMatrix[k]              = m_i1MatrixCodec[k].getOutputMatrix();
        m_stimulationClassName[k] = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), k + NON_CLASS_SETTINGS_COUNT));
    }

    // Settings
    m_stimulationName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_filename        = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_method          = Geometry::EMatrixClassifiers(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
    m_metric          = Geometry::EMetric(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3)));
    m_logLevel        = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4)));

    OV_ERROR_UNLESS_KRF(m_filename.length() != 0, "this box's setting \"Filename to save classifier model\" is empty. Would you be so very kind as to choose a file? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    // Printing info
    this->getLogManager() << m_logLevel << "\nNumber of classes : " << m_nbClass << "\nFilename : " << m_filename << "\nMethod : "
            << toString(m_method) << "\n";

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainer::uninitialize()
{
    m_i0StimulationCodec.uninitialize();
    for (auto& codec : m_i1MatrixCodec) { codec.uninitialize(); }
    m_i1MatrixCodec.clear();
    m_iMatrix.clear();
    for (auto& cov : m_covs) { cov.clear(); }
    m_covs.clear();
    m_stimulationClassName.clear();

    m_o0StimulationCodec.uninitialize();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainer::process()
{
    if (!m_isTrain) {
        Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

        //***** Stimulations *****
        for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
            m_i0StimulationCodec.decode(i);                                 // Decode the chunk
            const uint64_t start = boxContext.getInputChunkStartTime(0, i), // Time Code Chunk Start
                           end   = boxContext.getInputChunkEndTime(0, i);   // Time Code Chunk End

            if (m_i0StimulationCodec.isHeaderReceived()) {
                m_o0StimulationCodec.encodeHeader();
                boxContext.markOutputAsReadyToSend(0, 0, 0);
            }
            if (m_i0StimulationCodec.isBufferReceived())                    // Buffer received
            {
                for (size_t j = 0; j < m_iStimulation->size(); ++j) {
                    if (m_iStimulation->getId(j) == m_stimulationName) {
                        OV_ERROR_UNLESS_KRF(train(), "Train failed", Kernel::ErrorType::BadProcessing);
                        const uint64_t stim = this->getTypeManager().getEnumerationEntryValueFromName(
                            OV_TypeId_Stimulation, "OVTK_StimulationId_TrainCompleted");
                        m_oStimulation->push_back(stim, m_iStimulation->getDate(j), 0);
                        m_isTrain = true;
                    }
                }
                m_o0StimulationCodec.encodeBuffer();
                boxContext.markOutputAsReadyToSend(0, start, end);
            }
            if (m_i0StimulationCodec.isEndReceived()) {
                m_o0StimulationCodec.encodeEnd();
                boxContext.markOutputAsReadyToSend(0, start, end);
            }
        }

        //***** Matrix *****
        for (size_t k = 0; k < m_nbClass; ++k) {
            for (size_t i = 0; i < boxContext.getInputChunkCount(k + 1); ++i) {
                m_i1MatrixCodec[k].decode(i);                               // Decode the chunk
                OV_ERROR_UNLESS_KRF(m_iMatrix[k]->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);

                if (m_i1MatrixCodec[k].isBufferReceived())                  // Buffer received
                {
                    Eigen::MatrixXd cov;
                    MatrixConvert(*m_iMatrix[k], cov);
                    m_covs[k].push_back(cov);
                }
            }
        }
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainer::train()
{
    Geometry::IMatrixClassifier* matrixClassifier;
    if (m_method == Geometry::EMatrixClassifiers::MDM) { matrixClassifier = new Geometry::CMatrixClassifierMDM; }
    else if (m_method == Geometry::EMatrixClassifiers::MDM_Rebias) { matrixClassifier = new Geometry::CMatrixClassifierMDMRebias; }
    else if (m_method == Geometry::EMatrixClassifiers::FgMDM_RT) { matrixClassifier = new Geometry::CMatrixClassifierFgMDMRT; }
    else if (m_method == Geometry::EMatrixClassifiers::FgMDM_RT_Rebias) { matrixClassifier = new Geometry::CMatrixClassifierFgMDMRTRebias; }
    else { OV_ERROR_UNLESS_KRF(false, "this box's setting \"Method\" holds a method we regret to say we do not know. Would you be so very kind as to choose another? Thank you for your patience.", Kernel::ErrorType::BadSetting); }

    this->getLogManager() << m_logLevel << "Train Beginning...\n";
    OV_ERROR_UNLESS_KRF(matrixClassifier->Train(m_covs), "Train failed", Kernel::ErrorType::BadProcessing);
    this->getLogManager() << m_logLevel << "Train Finished. Save Beginning...\n";
    OV_ERROR_UNLESS_KRF(saveXML(matrixClassifier), "Save failed", Kernel::ErrorType::BadProcessing);
    this->getLogManager() << m_logLevel << "Save Finished.\n";

    delete matrixClassifier;
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainer::saveXML(const Geometry::IMatrixClassifier* classifier)
{
    OV_ERROR_UNLESS_KRF(classifier->SaveXML(m_filename.toASCIIString()), "the classifier could not be saved in the file [" << m_filename.toASCIIString() << "] from this box's setting \"Filename to save classifier model\". Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);

    //***** Add Stimulation to XML *****
    tinyxml2::XMLDocument xmlDoc;
    // Load File
    OV_ERROR_UNLESS_KRF(xmlDoc.LoadFile(m_filename.toASCIIString()) == 0, "the classifier just saved in the file [" << m_filename.toASCIIString() << "] could not be read back. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.",
                        Kernel::ErrorType::BadFileRead);

    // Load Root
    tinyxml2::XMLNode* root = xmlDoc.FirstChild();
    OV_ERROR_UNLESS_KRF(root != nullptr, "the file [" << m_filename.toASCIIString() << "] from the setting \"Filename to save classifier model\" has no root element. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);

    // Load Data
    tinyxml2::XMLElement* data = root->FirstChildElement("Classifier-data");
    OV_ERROR_UNLESS_KRF(data != nullptr, "the file [" << m_filename.toASCIIString() << "] from the setting \"Filename to save classifier model\" holds no classifier. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);

    tinyxml2::XMLElement* element = data->FirstChildElement("Class");       // Get Fist Class Node
    for (size_t k = 0; k < classifier->GetClassCount(); ++k)                // for each class
    {
        OV_ERROR_UNLESS_KRF(element != nullptr, "the file [" << m_filename.toASCIIString() << "] from the setting \"Filename to save classifier model\" is missing a class. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);
        const size_t idx = element->IntAttribute("class-id");               // Get Id (normally idx = k)
        OV_ERROR_UNLESS_KRF(idx == k, "the file [" << m_filename.toASCIIString() << "] from the setting \"Filename to save classifier model\" lists its classes out of order. Perhaps it was not written by this box, or was damaged. Might we humbly suggest choosing another file, or training again? Thank you most kindly.", Kernel::ErrorType::BadFileParsing);
        const CString stimulationName = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, m_stimulationClassName[k]);
        element->SetAttribute("stimulation", stimulationName.toASCIIString());
        element = element->NextSiblingElement("Class");                     // Next Class
    }
    return xmlDoc.SaveFile(m_filename.toASCIIString()) == 0;                // save XML (if != 0 it means error)
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainerListener::onInputAdded(Kernel::IBox& box, const size_t index)
{
    box.setInputType(index, OV_TypeId_StreamedMatrix);
    box.setInputName(index, ("Matrix for class " + std::to_string(index)).c_str());

    const boost::format stimulation = boost::format("OVTK_StimulationId_Label_%02u") % index;
    box.addSetting(("Class " + std::to_string(index) + " label").c_str(), OV_TypeId_Stimulation, stimulation.str().c_str());

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmMatrixClassifierTrainerListener::onInputRemoved(Kernel::IBox& box, const size_t index)
{
    const size_t offset = CBoxAlgorithmMatrixClassifierTrainer::NON_CLASS_SETTINGS_COUNT - 1;
    // (avoid the 5 first setting but class begin at input 1 so + 4)
    box.removeSetting(index + offset);
    //check if the removed class is not the last
    if (index != box.getInputCount()) {
        for (size_t k = index; k < box.getInputCount(); ++k) {
            box.setInputName(k, ("Matrix for class " + std::to_string(k)).c_str());

            std::string stimulation = (boost::format("OVTK_StimulationId_Label_%02u") % k).str();
            box.setSettingName(k + offset - 1, ("Class " + std::to_string(k) + " label").c_str());  // -1 because one is removed
            box.setSettingDefaultValue(k + offset - 1, stimulation.c_str());
            box.setSettingValue(k + offset - 1, stimulation.c_str());
        }
    }

    return true;
}
//---------------------------------------------------------------------------------------------------
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
