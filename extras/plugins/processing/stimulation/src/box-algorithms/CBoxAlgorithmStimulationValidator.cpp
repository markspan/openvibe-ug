///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationValidator.cpp
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/02/2020.
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

#include "CBoxAlgorithmStimulationValidator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStimulationValidator::initialize()
{
    m_decoder.initialize(*this, 0);
    m_iStimulation = m_decoder.getOutputStimulationSet();
    m_encoder.initialize(*this, 0);
    m_oStimulation = m_encoder.getInputStimulationSet();
    m_encoder.encodeHeader();
    this->getDynamicBoxContext().markOutputAsReadyToSend(0, 0, 0);

    m_stim  = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
    m_limit = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));

    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStimulationValidator::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();
    return true;
}
///-------------------------------------------------------------------------------------------------


///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStimulationValidator::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
///-------------------------------------------------------------------------------------------------


///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStimulationValidator::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    const uint64_t currentTime = getPlayerContext().getCurrentTime();
    m_oStimulation->resize(0);                              // reset stimulation output
    //***** Stimulations *****
    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_decoder.decode(i);                                // Decode the chunk
        if (m_decoder.isBufferReceived())                   // Buffer Received
        {
            for (size_t j = 0; j < m_iStimulation->size(); ++j) {
                if (m_iStimulation->getId(j) == m_stim.id()) {
                    m_count++;
                    if (m_count >= m_limit) {
                        m_count = 0;
                        m_oStimulation->push_back(m_stim.id(), m_iStimulation->getDate(j), 0);

                        m_encoder.encodeBuffer();
                        boxCtx.markOutputAsReadyToSend(0, m_lastEndTime, currentTime);  // preserve continuous
                        m_lastEndTime = currentTime;                                    // preserve continuous
                    }
                }
            }
        }
    }
    return true;
}
///-------------------------------------------------------------------------------------------------
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
