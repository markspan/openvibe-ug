///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClockStimulator.cpp
/// \brief Classes implementation for the Box Clock stimulator.
/// \author Yann Renard (Inria).
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

#include "CBoxAlgorithmClockStimulator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmClockStimulator::initialize()
{
    const double interstimulationInterval = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    const double minInterstimulationInterval = 0.0001;
    OV_ERROR_UNLESS_KRF(!(interstimulationInterval < minInterstimulationInterval),
                        "Invalid stimulation interval [" << interstimulationInterval << "] (expected value > " << minInterstimulationInterval << ")",
                        Kernel::ErrorType::BadSetting);

    m_stimulationInterval = interstimulationInterval;
    m_nSentStimulation    = 0;

    m_stimulationID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_lastStimulationDate = 0;
    m_lastEndTime         = 0;

    m_encoder.initialize(*this, 0);

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmClockStimulator::uninitialize()
{
    m_encoder.uninitialize();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmClockStimulator::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmClockStimulator::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    const uint64_t currentTime = getPlayerContext().getCurrentTime();

    CStimulationSet stimulationSet;
    stimulationSet.resize(0);

    while (CTime(double(m_nSentStimulation + 1) * m_stimulationInterval).time() < currentTime) {
        m_nSentStimulation += 1;
        m_lastStimulationDate = CTime(double(m_nSentStimulation) * m_stimulationInterval).time();
        stimulationSet.push_back(m_stimulationID, m_lastStimulationDate, 0);
    }

    if (currentTime == 0) {
        m_encoder.encodeHeader();
        boxCtx.markOutputAsReadyToSend(0, m_lastEndTime, m_lastEndTime);
    }
    m_encoder.getInputStimulationSet() = &stimulationSet;

    m_encoder.encodeBuffer();
    boxCtx.markOutputAsReadyToSend(0, m_lastEndTime, currentTime);

    m_lastEndTime = currentTime;

    return true;
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
