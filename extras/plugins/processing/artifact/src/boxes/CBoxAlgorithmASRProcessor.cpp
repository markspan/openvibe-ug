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

#include "CBoxAlgorithmASRProcessor.hpp"

#include "eigen/convert.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Artifact {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRProcessor::initialize()
{
    //***** Codecs *****
    m_signalDecoder.initialize(*this, 0);
    m_stimulationEncoder.initialize(*this, 0);
    m_signalEncoder.initialize(*this, 1);
    m_signalEncoder.getInputSamplingRate().setReferenceTarget(m_signalDecoder.getOutputSamplingRate()); // Link Sampling
    m_signalEncoder.getInputMatrix().setReferenceTarget(m_signalDecoder.getOutputMatrix());             // Link Matrix

    //***** Pointers *****
    m_iMatrix      = m_signalDecoder.getOutputMatrix();
    m_oStimulation = m_stimulationEncoder.getInputStimulationSet();
    m_oMatrix      = m_signalEncoder.getInputMatrix();

    // Settings
    m_filename = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)).toASCIIString();

    OV_ERROR_UNLESS_KRF(!m_filename.empty(), "Invalid empty model filename", Kernel::ErrorType::BadSetting);
    OV_ERROR_UNLESS_KRF(m_asr.LoadXML(m_filename), "Loading XML Error", Kernel::ErrorType::BadFileRead);

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRProcessor::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_stimulationEncoder.uninitialize();
    m_signalEncoder.uninitialize();

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRProcessor::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmASRProcessor::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_signalDecoder.decode(i);                                      // Decode the chunk
        OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);
        const uint64_t start = boxCtx.getInputChunkStartTime(0, i),     // Time Code Chunk Start
                       end   = boxCtx.getInputChunkEndTime(0, i);       // Time Code Chunk End

        if (m_signalDecoder.isHeaderReceived())                         // Header received
        {
            m_signalEncoder.encodeHeader();
            m_stimulationEncoder.encodeHeader();
            boxCtx.markOutputAsReadyToSend(0, start, end);
        }
        if (m_signalDecoder.isBufferReceived())                         // Buffer received
        {
            const bool prevTrivial = m_asr.GetTrivial();
            Eigen::MatrixXd in, out;
            MatrixConvert(*m_iMatrix, in);
            OV_ERROR_UNLESS_KRF(m_asr.Process(in, out), "ASR Process Error", Kernel::ErrorType::BadProcessing);
            MatrixConvert(out, *m_oMatrix);
            m_signalEncoder.encodeBuffer();

            const bool newTrivial = m_asr.GetTrivial();
            if (!newTrivial && !prevTrivial)                            // We have reconstruct signal
            {
                m_oStimulation->push_back(OVTK_StimulationId_Artifact, start, 0);
                m_stimulationEncoder.encodeBuffer();
                boxCtx.markOutputAsReadyToSend(0, start, end);
            }
        }
        if (m_signalDecoder.isEndReceived())                            // Buffer received
        {
            m_signalEncoder.encodeEnd();
            m_stimulationEncoder.encodeEnd();
            boxCtx.markOutputAsReadyToSend(0, start, end);
        }
        boxCtx.markOutputAsReadyToSend(1, start, end);
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

}  // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
