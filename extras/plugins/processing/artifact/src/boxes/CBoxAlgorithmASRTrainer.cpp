///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmASRProcessor.hpp
/// \brief Classes implementation for the box ASR Processor.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/12/2020.
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

#include "CBoxAlgorithmASRTrainer.hpp"

#include "eigen/convert.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Artifact {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRTrainer::initialize()
{
    // Stimulations
    m_stimulationDecoder.initialize(*this, 0);
    m_iStimulation = m_stimulationDecoder.getOutputStimulationSet();

    m_stimulationEncoder.initialize(*this, 0);
    m_oStimulation = m_stimulationEncoder.getInputStimulationSet();

    // Classes
    m_signalEncoder.initialize(*this, 1);
    m_iMatrix = m_signalEncoder.getOutputMatrix();

    // Settings
    m_filename        = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)).toASCIIString();
    m_stimulationName = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_metric          = Geometry::EMetric(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
    m_ratio           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_rejection       = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    OV_ERROR_UNLESS_KRF(!m_filename.empty(), "Invalid empty model filename", Kernel::ErrorType::BadSetting);
    OV_ERROR_UNLESS_KRF(Geometry::InRange(m_ratio, 0, 1), "Channel ratio must be in [0;1], actual : " + std::to_string(m_ratio), Kernel::ErrorType::BadSetting);
    OV_ERROR_UNLESS_KRF(m_rejection >= 0, "Rejection limit must be positive, actual : " + std::to_string(m_rejection), Kernel::ErrorType::BadSetting);

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRTrainer::uninitialize()
{
    m_stimulationDecoder.uninitialize();
    m_signalEncoder.uninitialize();
    m_stimulationEncoder.uninitialize();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRTrainer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRTrainer::process()
{
    if (!m_isTrain) {
        Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
        //***** Stimulations *****
        for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
            m_stimulationDecoder.decode(i);                             // Decode the chunk
            const uint64_t start = boxCtx.getInputChunkStartTime(0, i), // Time Code Chunk Start
                           end   = boxCtx.getInputChunkEndTime(0, i);   // Time Code Chunk End

            if (m_stimulationDecoder.isHeaderReceived()) {
                m_stimulationEncoder.encodeHeader();
                boxCtx.markOutputAsReadyToSend(0, 0, 0);
            }
            if (m_stimulationDecoder.isBufferReceived())                // Buffer received
            {
                for (size_t j = 0; j < m_iStimulation->size(); ++j) {
                    if (m_iStimulation->getId(j) == m_stimulationName) {
                        OV_ERROR_UNLESS_KRF(train(), "Train or Save failed", Kernel::ErrorType::BadProcessing);
                        m_oStimulation->push_back(OVTK_StimulationId_TrainCompleted, m_iStimulation->getDate(j), 0);
                        m_isTrain = true;
                    }
                }
                m_stimulationEncoder.encodeBuffer();
                boxCtx.markOutputAsReadyToSend(0, start, end);
            }
            if (m_stimulationDecoder.isEndReceived()) {
                m_stimulationEncoder.encodeEnd();
                boxCtx.markOutputAsReadyToSend(0, start, end);
            }
        }

        //***** Signal *****
        for (size_t i = 0; i < boxCtx.getInputChunkCount(1); ++i) {
            m_signalEncoder.decode(i);                                  // Decode the chunk
            OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);

            if (m_signalEncoder.isBufferReceived())                     // Buffer received
            {
                Eigen::MatrixXd m;
                MatrixConvert(*m_iMatrix, m);
                m_dataset.push_back(m);
            }
        }
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRTrainer::train()
{
    Geometry::CASR asr(m_metric);
    asr.SetMaxChannel(m_ratio);
    this->getLogManager() << Kernel::LogLevel_Info << "Train Beginning...\n";
    OV_ERROR_UNLESS_KRF(asr.Train(m_dataset, m_rejection), "Train failed", Kernel::ErrorType::BadProcessing);
    getLogManager() << Kernel::LogLevel_Info << "Train Finished. Save Beginning...\n";
    OV_ERROR_UNLESS_KRF(asr.SaveXML(m_filename), "Save failed", Kernel::ErrorType::BadProcessing);
    this->getLogManager() << Kernel::LogLevel_Info << "Save Finished.\n";
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRTrainerListener::onSettingValueChanged(Kernel::IBox& box, const size_t index)
{
    if (index == 2) {
        CString tmp;
        box.getSettingValue(index, tmp);
        const Geometry::EMetric m = Geometry::StringToMetric(tmp.toASCIIString());
        if (m != Geometry::EMetric::Euclidian && m != Geometry::EMetric::Riemann) {
            const std::string s1 = toString(Geometry::EMetric::Euclidian), s2 = toString(Geometry::EMetric::Riemann);
            getLogManager() << Kernel::LogLevel_Warning << "Metric must be " << s1 << " or " << s2 << ". Setting is set to " << s1 << "\n";
            box.setSettingValue(index, s1.c_str());
        }
    }
    else if (index == 3) {
        CString tmp;
        box.getSettingValue(index, tmp);

        double ratio = 0.0;
        std::stringstream ss(tmp.toASCIIString());
        ss >> ratio;
        if (ratio < 0.0) {
            getLogManager() << Kernel::LogLevel_Warning << "Channel ratio must be in [0;1] (0 for no reconstruction, 1 for no limit). Setting is set to 0. \n";
            box.setSettingValue(index, "0");
        }
        else if (ratio > 1.0) {
            getLogManager() << Kernel::LogLevel_Warning << "Channel ratio must be in [0;1] (0 for no reconstruction, 1 for no limit). Setting is set to 1. \n";
            box.setSettingValue(index, "1");
        }
    }
    else if (index == 4) {
        CString tmp;
        box.getSettingValue(index, tmp);

        double rejection = 0.0;
        std::stringstream ss(tmp.toASCIIString());
        ss >> rejection;
        if (rejection < 0.0) {
            getLogManager() << Kernel::LogLevel_Warning << "Rejection limit must be positive. Setting is set to 0. \n";
            box.setSettingValue(index, "0");
        }
    }
    return true;
}
//---------------------------------------------------------------------------------------------------
}  // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
