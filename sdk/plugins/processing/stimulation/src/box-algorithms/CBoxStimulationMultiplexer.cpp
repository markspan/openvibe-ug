///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxStimulationMultiplexer.hpp
/// \brief Classes implementation for the Box Stimulation Multiplexer.
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.1.
/// \date 19/07/2013
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

#include "CBoxStimulationMultiplexer.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {
///-------------------------------------------------------------------------------------------------
bool CBoxStimulationMultiplexer::initialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    m_decoders.resize(nInput);
    size_t index = 0;
    for (auto& stimulationDecoder : m_decoders) {
        stimulationDecoder.initialize(*this, index);
        index += 1;
    }

    m_encoder.initialize(*this, 0);

    m_decoderEndTimes = std::vector<uint64_t>(nInput, 0ULL);

    m_lastStartTime = 0;
    m_lastEndTime   = 0;
    m_wasHeaderSent = false;

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxStimulationMultiplexer::uninitialize()
{
    for (auto& decoder : m_decoders) { decoder.uninitialize(); }
    m_encoder.uninitialize();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxStimulationMultiplexer::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxStimulationMultiplexer::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
    const size_t nInput    = this->getStaticBoxContext().getInputCount();

    if (!m_wasHeaderSent) {
        m_encoder.encodeHeader();
        boxCtx.markOutputAsReadyToSend(0, m_lastEndTime, m_lastEndTime);
        m_wasHeaderSent = true;
    }

    uint64_t earliestReceivedEndTime = 0xffffffffffffffffULL;

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t chunk = 0; chunk < boxCtx.getInputChunkCount(i); ++chunk) {
            m_decoders[i].decode(chunk);

            if (m_decoders[i].isBufferReceived()) {
                for (size_t stim = 0; stim < m_decoders[i].getOutputStimulationSet()->size(); ++stim) {
                    m_stimulations.insert(std::make_pair(m_decoders[i].getOutputStimulationSet()->getDate(stim),
                                                         std::make_tuple(m_decoders[i].getOutputStimulationSet()->getId(stim),
                                                                         m_decoders[i].getOutputStimulationSet()->getDate(stim),
                                                                         m_decoders[i].getOutputStimulationSet()->getDuration(stim))));
                }
            }

            m_decoderEndTimes[i] = boxCtx.getInputChunkEndTime(i, chunk);
        }

        if (earliestReceivedEndTime > m_decoderEndTimes[i]) { earliestReceivedEndTime = m_decoderEndTimes[i]; }
    }

    if (earliestReceivedEndTime >= m_lastEndTime) {
        m_encoder.getInputStimulationSet()->clear();

        for (auto stim = m_stimulations.begin(); stim != m_stimulations.end();) {
            if (stim->first <= earliestReceivedEndTime) {
                m_encoder.getInputStimulationSet()->push_back(std::get<0>(stim->second), std::get<1>(stim->second), std::get<2>(stim->second));
                stim = m_stimulations.erase(stim);
            }
            else { ++stim; }
        }

        m_encoder.encodeBuffer();

        boxCtx.markOutputAsReadyToSend(0, m_lastEndTime, earliestReceivedEndTime);

        m_lastStartTime = m_lastEndTime;
        m_lastEndTime   = earliestReceivedEndTime;
    }

    return true;
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
