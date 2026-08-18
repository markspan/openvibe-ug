///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStreamEndDetector.cpp
/// \brief Classes implementation for the Box Stream End Detector.
/// \author Jozef Legeny (Mensia Technologies).
/// \version 1.0.
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

#include "CBoxAlgorithmStreamEndDetector.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamEndDetector::initialize()
{
    m_stimulationID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), SettingStimulationNameID());

    OV_FATAL_UNLESS_K(this->getStaticBoxContext().getInterfacorIndex(Kernel::EBoxInterfacorType::Input, InputEBMLId(), m_inputEBMLIdx),
                      "Box does not have input with identifier " << InputEBMLId(), Kernel::ErrorType::Internal);
    OV_FATAL_UNLESS_K(this->getStaticBoxContext().getInterfacorIndex(Kernel::EBoxInterfacorType::Output, OutputStimulationsID(), m_outputStimulationsIdx),
                      "Box does not have output with identifier " << OutputStimulationsID(), Kernel::ErrorType::Internal);

    m_decoder.initialize(*this, m_inputEBMLIdx);
    m_encoder.initialize(*this, m_outputStimulationsIdx);

    m_isHeaderSent        = false;
    m_endDate             = 0;
    m_currentChunkEndDate = 0;
    m_endState            = EEndState::WaitingForEnd;
    m_previousTime        = 0;

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamEndDetector::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamEndDetector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamEndDetector::process()
{
    const Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t chunk = 0; chunk < boxCtx.getInputChunkCount(m_inputEBMLIdx); ++chunk) {
        OV_ERROR_UNLESS_KRF(m_decoder.decode(chunk), "Failed to decode chunk", Kernel::ErrorType::Internal);

        // We can not receive anything before this date anymore, thus we can send an empty stream
        m_currentChunkEndDate = boxCtx.getInputChunkStartTime(m_inputEBMLIdx, chunk);
        if (m_decoder.isEndReceived()) {
            m_endState = EEndState::EndReceived;
            m_endDate  = boxCtx.getInputChunkEndTime(m_inputEBMLIdx, chunk);
            // As this is the last chunk, we make it so it ends at the same time as the received End chunk
            m_currentChunkEndDate = boxCtx.getInputChunkEndTime(m_inputEBMLIdx, chunk);
        }
    }

    if (!m_isHeaderSent) {
        m_encoder.encodeHeader();
        this->getDynamicBoxContext().markOutputAsReadyToSend(0, 0, 0);
        m_isHeaderSent = true;
    }

    const CStimulationSet* stimulationSet = m_encoder.getInputStimulationSet();
    stimulationSet->clear();


    // If the timeout is reached we send the stimulation on the output 0
    if (m_endState == EEndState::EndReceived) {
        stimulationSet->push_back(m_stimulationID, m_endDate, 0);
        m_endState = EEndState::StimulationSent;
    }

    if (m_endState != EEndState::Finished && (m_endState == EEndState::StimulationSent || m_previousTime != m_currentChunkEndDate)) {
        // we need to send an empty chunk even if there's no stim
        m_encoder.encodeBuffer();
        this->getDynamicBoxContext().markOutputAsReadyToSend(0, m_previousTime, m_currentChunkEndDate);
    }

    m_previousTime = m_currentChunkEndDate;

    if (m_endState == EEndState::StimulationSent) {
        m_encoder.encodeEnd();
        this->getDynamicBoxContext().markOutputAsReadyToSend(0, m_previousTime, m_previousTime);
        m_endState = EEndState::Finished;
    }

    return true;
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
