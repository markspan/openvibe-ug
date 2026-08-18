///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmAmplitudeArtifactDetector.hpp
/// \brief Classes implementation for the box Amplitude Artifact Detector.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/08/2019.
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

#include "CBoxAlgorithmAmplitudeArtifactDetector.hpp"
#include <cmath>    // Floor
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace Artifact {
//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmAmplitudeArtifactDetector::initialize()
{
    //***** Codecs *****
    m_iSignalDecoder.initialize(*this, 0);
    m_oSignalEncoder.initialize(*this, 0);
    m_oStimulationEncoder.initialize(*this, 1);

    m_iMatrix = m_iSignalDecoder.getOutputMatrix();
    m_oMatrix = m_oSignalEncoder.getInputMatrix();

    //***** Settings *****
    m_threshold = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_action    = stringToArtifactAction(CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)).toASCIIString());

    const CString stim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    //***** Stimulations *****
    m_stimId = getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, stim);

    //***** Assert *****
    OV_ERROR_UNLESS_KRF(m_threshold > 0, "Invalid Maximum [" << m_threshold << "] (expected value > 0)\n", Kernel::ErrorType::BadSetting);

    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmAmplitudeArtifactDetector::uninitialize()
{
    m_iSignalDecoder.uninitialize();
    m_oSignalEncoder.uninitialize();
    m_oStimulationEncoder.uninitialize();
    std::stringstream ss;
    ss << m_nArtifact << " artifacts detected in " << m_nEpochs << " samples (";
    ss.precision(2);
    ss << std::fixed << 100.0 * double(m_nArtifact) / double(m_nEpochs) << "%)" << std::endl;
    this->getLogManager() << Kernel::LogLevel_Info << ss.str();
    return true;
}
//---------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmAmplitudeArtifactDetector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
bool CBoxAlgorithmAmplitudeArtifactDetector::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    const CStimulationSet* stimSet = m_oStimulationEncoder.getInputStimulationSet();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i);
        const uint64_t tEnd   = boxContext.getInputChunkEndTime(0, i);

        m_iSignalDecoder.decode(i);                                                                             // Decode chunk i
        OV_ERROR_UNLESS_KRF(m_iMatrix->getDimensionCount() == 2, "Invalid Input Signal", Kernel::ErrorType::BadInput);
        m_nEpochs++;


        // Header
        if (m_iSignalDecoder.isHeaderReceived()) {
            m_oSignalEncoder.getInputSamplingRate().setReferenceTarget(m_iSignalDecoder.getOutputSamplingRate());
            m_oMatrix->copyDescription(*m_iMatrix);

            m_oSignalEncoder.encodeHeader();
            m_oStimulationEncoder.encodeHeader();
        }

        // Buffer
        if (m_iSignalDecoder.isBufferReceived()) {
            bool artifact = false;
            stimSet->clear();

            m_oMatrix->copyContent(*m_iMatrix);

            const size_t size     = m_iMatrix->getSize();   // get buffer size
            const double* iBuffer = m_iMatrix->getBuffer(); // input buffer
            double* oBuffer       = m_oMatrix->getBuffer();

            for (size_t idx = 0; idx < size; ++idx) {
                if (std::fabs(iBuffer[idx]) > m_threshold) {
                    this->getLogManager() << Kernel::LogLevel_Trace << "Artifact detected in channel (" << floor(idx / m_iMatrix->getDimensionSize(1)) << ")\n";
                    artifact = true;

                    if (m_action == EArtifactAction::Cutoff) { oBuffer[idx] = iBuffer[idx] >= 0 ? m_threshold : -m_threshold; }
                    else { break; }
                }
            }

            if (artifact) {
                m_nArtifact++;
                stimSet->push_back(m_stimId, tStart, 0);

                if (m_action == EArtifactAction::Zero) { for (size_t index = 0; index < size; ++index) { oBuffer[index] = 0; } }
                if (m_action != EArtifactAction::Stop) { m_oSignalEncoder.encodeBuffer(); } // stop means no signal is sent, so no signal is encoded
            }
            else { m_oSignalEncoder.encodeBuffer(); }

            m_oStimulationEncoder.encodeBuffer();
        }

        // End
        if (m_iSignalDecoder.isEndReceived()) {
            m_oSignalEncoder.encodeEnd();
            m_oStimulationEncoder.encodeEnd();
        }

        boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
        boxContext.markOutputAsReadyToSend(1, tStart, tEnd);
    }
    return true;
}
//---------------------------------------------------------------------------------------------------

}  // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
