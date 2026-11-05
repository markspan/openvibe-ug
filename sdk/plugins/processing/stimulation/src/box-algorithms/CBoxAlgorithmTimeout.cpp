///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmTimeout.cpp
/// \brief Classes implementation for the Box Timeout.
/// \author Jozef Legeny (Inria).
/// \version 1.1.
/// \date 21/03/2013
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
#include "CBoxAlgorithmTimeout.hpp"

#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmTimeout::initialize()
{
    m_timeoutState = ETimeoutState::No;

    m_encoder.initialize(*this, 0);

    const double timeout = double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0));
    OV_ERROR_UNLESS_KRF(timeout > 0, "this box's setting \"Timeout delay\" is " << timeout << ", while it must be above 0. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);
    OV_ERROR_UNLESS_KRF(timeout == std::floor(timeout), "this box's setting \"Timeout delay\" is " << timeout << ", while it must be a whole number of seconds. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    m_timeout           = uint64_t(timeout) << 32;
    m_stimulationToSend = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_lastTimePolled = 0;
    m_previousTime   = 0;
    m_isHeaderSent   = false;

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmTimeout::uninitialize()
{
    m_encoder.uninitialize();

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmTimeout::processClock(Kernel::CMessageClock& /*msg*/)
{
    // if there was nothing received on the input for a period of time we raise the
    // timeout flag and let the box send a stimulation
    if (m_timeoutState == ETimeoutState::No && getPlayerContext().getCurrentTime() > m_lastTimePolled + m_timeout) {
        this->getLogManager() << Kernel::LogLevel_Trace << "Timeout reached at time " << CTime(this->getPlayerContext().getCurrentTime()) << "\n";
        m_timeoutState = ETimeoutState::Occurred;
    }

    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmTimeout::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    // every time we receive input we store the last kernel time
    m_lastTimePolled = this->getPlayerContext().getCurrentTime();

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmTimeout::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    // Discard input data
    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) { boxCtx.markInputAsDeprecated(0, i); }

    // Encoding the header
    if (!m_isHeaderSent) {
        m_encoder.encodeHeader();
        this->getDynamicBoxContext().markOutputAsReadyToSend(0, 0, 0);
        m_isHeaderSent = true;
    }

    const CStimulationSet* stimSet = m_encoder.getInputStimulationSet();
    stimSet->clear();

    const uint64_t date = this->getPlayerContext().getCurrentTime();

    // If the timeout is reached we send the stimulation on the output 0
    if (m_timeoutState == ETimeoutState::Occurred) {
        stimSet->push_back(m_stimulationToSend, date, 0);
        m_timeoutState = ETimeoutState::Sent;
    }

    // we need to send an empty chunk even if there's no stim
    m_encoder.encodeBuffer();
    this->getDynamicBoxContext().markOutputAsReadyToSend(0, m_previousTime, date);

    m_previousTime = date;

    return true;
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
