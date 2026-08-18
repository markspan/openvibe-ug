///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxHelloSenderGame.cpp
/// \brief Class of the box that communicate with Hello Sender Game.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/03/2020.
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

#ifdef TARGET_HAS_ThirdPartyLSL

#include "CBoxHelloSenderGame.hpp"

#include <lsl/Utils.hpp>
#include <ctime>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Games {

//--------------------------------------------------------------------------------
bool CBoxHelloSenderGame::initialize()
{
    m_signalInlet = nullptr;
    m_stimInlet   = nullptr;

    m_signalEncoder.initialize(*this, 0);
    m_stimEncoder.initialize(*this, 1);
    m_oMatrix  = m_signalEncoder.getInputMatrix();
    m_oStimSet = m_stimEncoder.getInputStimulationSet();

    // Signal Stream
    const lsl::stream_info signalInfo = LSL::findStreamInfo(m_signalName);
    if (signalInfo.name() != m_signalName) {
        this->getLogManager() << Kernel::LogLevel_Error << "Error getting signal stream info for [" << signalInfo.name() << "]\n";
        return false;
    }

    m_signalInlet = new lsl::stream_inlet(signalInfo);
    if (!m_signalInlet) {
        this->getLogManager() << Kernel::LogLevel_Error << "Error getting signal inlet for [" << signalInfo.name() << "]\n";
        return false;
    }

    try { m_signalInlet->open_stream(); }
    catch (...) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to open signal stream with name [" << signalInfo.name() << "]\n";
        return false;
    }

    // Stimulation Stream
    const lsl::stream_info stimInfo = LSL::findStreamInfo(m_markerName);
    if (stimInfo.name() != m_markerName) {
        this->getLogManager() << Kernel::LogLevel_Error << "Error getting marker stream info for [" << stimInfo.name() << "]\n";
        return false;
    }

    m_stimInlet = new lsl::stream_inlet(stimInfo);
    if (!m_stimInlet) {
        this->getLogManager() << Kernel::LogLevel_Error << "Error getting marker inlet for [" << stimInfo.name() << "]\n";
        return false;
    }

    try { m_stimInlet->open_stream(); }
    catch (...) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to open marker stream with name [" << stimInfo.name() << "]\n";
        return false;
    }

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloSenderGame::uninitialize()
{
    m_signalEncoder.uninitialize();
    m_stimEncoder.uninitialize();

    if (m_signalInlet) {
        m_signalInlet->close_stream();
        delete m_signalInlet;
        m_signalInlet = nullptr;
    }

    if (m_stimInlet) {
        m_stimInlet->close_stream();
        delete m_stimInlet;
        m_stimInlet = nullptr;
    }

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloSenderGame::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloSenderGame::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const uint64_t currentTime = getPlayerContext().getCurrentTime();

    // Header
    if (!m_headerSent) {
        m_buffer = new float[2];

        // Matrix Header
        m_oMatrix->resize(2, 1);
        m_signalEncoder.encodeHeader();

        // Stimulation Header
        m_oStimSet->resize(0);          // reset stimulation output
        m_stimEncoder.encodeHeader();

        boxContext.markOutputAsReadyToSend(0, m_lastMatrixTime, m_lastMatrixTime);  // Makes the output available
        boxContext.markOutputAsReadyToSend(1, m_lastStimTime, m_lastStimTime);      // Makes the output available
        m_headerSent = true;
        this->getLogManager() << Kernel::LogLevel_Info << "Header created\n";
    }

    // Core
    double time;
    // Signal
    try { time = m_signalInlet->pull_sample(m_buffer, 2, 0.0); }                // 2 element timeout to 0.0 to avoid lag (OpenViBE can't have background task)
    catch (...) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to get signal sample\n";
        return false;
    }
    if (std::abs(time) > 0.0) {
        double* buffer = m_oMatrix->getBuffer();
        buffer[0]      = double(m_buffer[0]);
        buffer[1]      = double(m_buffer[1]);
        m_signalEncoder.encodeBuffer();                                         // Buffer encoded
        boxContext.markOutputAsReadyToSend(0, m_lastMatrixTime, currentTime);   // Makes the output available
        m_lastMatrixTime = currentTime;
    }

    // Stimulation
    try { time = m_stimInlet->pull_sample(m_buffer, 1, 0.0); }                  // 1 element timeout to 0.0 to avoid lag (OpenViBE can't have background task)
    catch (...) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to get stimulation sample\n";
        return false;
    }
    if (std::abs(time) > 0.0) {
        m_oStimSet->clear();                                                    // reset stimulation output
        m_oStimSet->push_back(uint64_t(m_buffer[0]), currentTime, 0);
        m_stimEncoder.encodeBuffer();                                           // Buffer encoded
        boxContext.markOutputAsReadyToSend(1, m_lastStimTime, currentTime);     // Makes the output available
        m_lastStimTime = currentTime;
    }
    return true;
}
//--------------------------------------------------------------------------------

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyLSL
