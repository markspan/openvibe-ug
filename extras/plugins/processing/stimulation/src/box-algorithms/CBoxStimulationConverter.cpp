///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxStimulationConverter.cpp
/// \author Thibaut Monseigne (Inria).
/// \brief Classes implementation for the box Stimulation Converter.
/// \version 1.0.
/// \date 14/10/2020.
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

#include "CBoxStimulationConverter.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

///-------------------------------------------------------------------------------------------------
bool CBoxStimulationConverter::initialize()
{
    m_decoder.initialize(*this, 0);
    m_iStimulation = m_decoder.getOutputStimulationSet();
    m_encoder.initialize(*this, 0);
    m_oStimulation = m_encoder.getInputStimulationSet();
    m_encoder.encodeHeader();
    this->getDynamicBoxContext().markOutputAsReadyToSend(0, 0, 0);

    m_keepOthers = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_stimStart  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_stimEnd    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_stimSent   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxStimulationConverter::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();
    return true;
}
///-------------------------------------------------------------------------------------------------


///-------------------------------------------------------------------------------------------------
bool CBoxStimulationConverter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
///-------------------------------------------------------------------------------------------------


///-------------------------------------------------------------------------------------------------
bool CBoxStimulationConverter::process()
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
                if (m_stimStart <= m_iStimulation->getId(j) && m_iStimulation->getId(j) <= m_stimEnd) {
                    m_oStimulation->push_back(m_stimSent, m_iStimulation->getDate(j), m_iStimulation->getDuration(j));
                }
                else if (m_keepOthers) { m_oStimulation->push_back(m_iStimulation->getId(j), m_iStimulation->getDate(j), m_iStimulation->getDuration(j)); }
            }
            if (m_oStimulation->size() != 0) {
                m_encoder.encodeBuffer();
                boxCtx.markOutputAsReadyToSend(0, m_lastEndTime, currentTime);  // preserve continuous
                m_lastEndTime = currentTime;                                    // preserve continuous
            }
        }
    }
    return true;
}
///-------------------------------------------------------------------------------------------------
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
