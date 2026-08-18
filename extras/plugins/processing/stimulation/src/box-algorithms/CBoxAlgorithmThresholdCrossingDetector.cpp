///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmThresholdCrossingDetector.cpp
/// \brief Class of the box that detects threshold crossing.
/// \author Joan Fruitet, Jozef Legeny, Axel Bouneau (Inria).
/// \version 1.3.
/// \date 14/04/2022
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

#include "CBoxAlgorithmThresholdCrossingDetector.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

bool CBoxAlgorithmThresholdCrossingDetector::initialize()
{
    // we read the settings:
    // The stimulations names:
    const CString on  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    const CString off = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    m_onStimId  = getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, on);
    m_offStimId = getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_Stimulation, off);

    m_lastSample  = 0;
    m_firstSample = true;

    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);

    m_channelIdx = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    if (m_channelIdx == 0) {
        this->getLogManager() << Kernel::LogLevel_Info << "Channel Index is 0. The channel indexing convention starts from 1.\n";
        return false;
    }
    m_channelIdx--; // Convert from [1,n] indexing to [0,n-1] indexing used later

    m_threshold = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    return true;
}

bool CBoxAlgorithmThresholdCrossingDetector::uninitialize()
{
    m_encoder.uninitialize();
    m_decoder.uninitialize();

    return true;
}

bool CBoxAlgorithmThresholdCrossingDetector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmThresholdCrossingDetector::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Get a few convenience handles
    const CMatrix* matrix          = m_decoder.getOutputMatrix();
    const CStimulationSet* stimSet = m_encoder.getInputStimulationSet();

    // We decode the stream matrix
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        const uint64_t start = boxContext.getInputChunkStartTime(0, i);
        const uint64_t end   = boxContext.getInputChunkEndTime(0, i);

        m_decoder.decode(i);

        // if  we received the header
        if (m_decoder.isHeaderReceived()) {
            //we analyse the header (meaning the input matrix size)
            if (matrix->getDimensionCount() != 2) {
                this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Streamed matrix must have exactly 2 dimensions\n";
                return false;
            }
            if (m_channelIdx >= matrix->getDimensionSize(0)) {
                this->getLogManager() << Kernel::LogLevel_Info << "Channel Index out of bounds. Incoming matrix has fewer channels than specified index.\n";
                return false;
            }

            m_samplesPerChannel = matrix->getDimensionSize(1);

            // we send a header on the stimulation output:
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, start, end);
        }


        // if we received a buffer
        if (m_decoder.isBufferReceived()) {
            stimSet->clear();
            const double* data = matrix->getBuffer();
            // for each data sample of the buffer we look for sign change

            for (size_t j = 0; j < matrix->getDimensionSize(1); ++j) {
                const double currentSample = data[(m_channelIdx * m_samplesPerChannel) + j];
                if (m_firstSample) {
                    m_lastSample  = currentSample;
                    m_firstSample = false;
                }

                // Change from positive to negative
                if (m_lastSample >= m_threshold && currentSample < m_threshold) {
                    const uint64_t time = start + (end - start) * j / m_samplesPerChannel;
                    stimSet->push_back(m_offStimId, time, 0);
                }

                // Change from negative to positive
                if (m_lastSample < m_threshold && currentSample >= m_threshold) {
                    const uint64_t time = start + (end - start) * j / m_samplesPerChannel;
                    stimSet->push_back(m_onStimId, time, 0);
                }

                m_lastSample = currentSample;
            }

            m_encoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, start, end);
        }

        // if we received the End
        if (m_decoder.isEndReceived()) {
            // we send the End signal to the stimulation output:
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, start, end);
        }

        // The stream matrix chunk i has been processed
        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
