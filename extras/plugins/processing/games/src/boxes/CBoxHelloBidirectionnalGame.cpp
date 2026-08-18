///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxHelloBidirectionnalGame.cpp
/// \brief Class of the box that communicates with Hello Bidirectionnal Game.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 24/02/2021
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

#include "CBoxHelloBidirectionnalGame.hpp"

#include <lsl/Utils.hpp>
#include <ctime>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Games {

//--------------------------------------------------------------------------------
bool CBoxHelloBidirectionnalGame::initialize()
{
    //---------- Initialize Input/Output ----------
    m_signalDecoder.initialize(*this, 0);
    m_stimDecoder.initialize(*this, 1);
    m_iMatrix  = m_signalDecoder.getOutputMatrix();
    m_iStimSet = m_stimDecoder.getOutputStimulationSet();

    m_signalEncoder.initialize(*this, 0);
    m_stimEncoder.initialize(*this, 1);
    m_oMatrix  = m_signalEncoder.getInputMatrix();
    m_oStimSet = m_stimEncoder.getInputStimulationSet();

    //---------- Initialize LSL Stream (to Unity) ----------
    // These are supposed to be unique, so we don't have them in the box config
    m_outSignalID = CIdentifier::random().str();
    m_outMarkerID = CIdentifier::random().str();

    while (m_outMarkerID == m_outSignalID) { m_outMarkerID = CIdentifier::random().str(); } // very unlikely

    this->getLogManager() << Kernel::LogLevel_Trace << "We Will create signal stream [" << m_outSignalName << ", id " << m_outSignalID
            << "] and stimulation stream [" << m_outMarkerName << ", id " << m_outMarkerID << "]\n";

    //---------- Create Headers for Output ----------
    // Matrix Header
    m_oMatrix->resize(1, 1);
    m_signalEncoder.encodeHeader();

    // Stimulation Header
    m_oStimSet->resize(0);          // reset stimulation output
    m_stimEncoder.encodeHeader();

    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
    boxCtx.markOutputAsReadyToSend(0, m_lastMatrixTime, m_lastMatrixTime);  // Makes the output available
    boxCtx.markOutputAsReadyToSend(1, m_lastStimTime, m_lastStimTime);      // Makes the output available

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloBidirectionnalGame::uninitialize()
{
    //---------- Uninitialize Input/Output ----------
    m_signalDecoder.uninitialize();
    m_stimDecoder.uninitialize();
    m_signalEncoder.uninitialize();
    m_stimEncoder.uninitialize();

    //---------- Delete LSL Stream Inlet/Outlet ----------
    if (m_signalOutlet) {
        delete m_signalOutlet;
        m_signalOutlet = nullptr;
    }
    if (m_stimOutlet) {
        delete m_stimOutlet;
        m_stimOutlet = nullptr;
    }

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
bool CBoxHelloBidirectionnalGame::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloBidirectionnalGame::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const uint64_t currentTime = getPlayerContext().getCurrentTime();

    //---------- Emission Part ----------
    // Process signals
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_signalDecoder.decode(i);
        if (m_signalDecoder.isHeaderReceived() && !m_signalOutlet) {
            const size_t samplesPerBlock = m_signalDecoder.getOutputMatrix()->getDimensionSize(1);
            const size_t frequency       = m_signalDecoder.getOutputSamplingRate();

            lsl::stream_info signalInfo = LSL::createSignalStreamInfo(m_outSignalName, m_outSignalID, m_signalDecoder.getOutputMatrix(), frequency);
            // make a new outlet
            try { m_signalOutlet = new lsl::stream_outlet(signalInfo, int(samplesPerBlock)); }
            catch (...) {
                this->getLogManager() << "Unable to create signal outlet\n";
                return false;
            }
        }
        if (m_signalDecoder.isBufferReceived()) {
            LSL::sendSignal(m_signalOutlet, m_iMatrix, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        //if (m_signalDecoder.isEndReceived()) { }
    }

    // Process stimuli -> LSL markers. 
    // Note that stimuli with identifiers not fitting to int will be mangled by a static cast.
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isHeaderReceived() && !m_stimOutlet) {
            // Open a stimulus stream
            lsl::stream_info info = LSL::createStimulationStreamInfo(m_outMarkerName, m_outMarkerID);

            try { m_stimOutlet = new lsl::stream_outlet(info); }
            catch (...) {
                this->getLogManager() << "Unable to create marker outlet\n";
                return false;
            }
        }
        if (m_stimDecoder.isBufferReceived()) { LSL::sendStimulation(m_stimOutlet, m_iStimSet); }
        //if (m_stimDecoder.isEndReceived()) { }
    }


    //---------- Reception Part ----------
    if (!m_signalInlet) // We don't have a signal inlet
    {
        const lsl::stream_info signalInfo = LSL::findStreamInfo(m_inSignalName, "", 0); // timeout of 0 to avoid blocking OpenViBE
        if (signalInfo.name() == m_inSignalName) {
            m_signalInlet = new lsl::stream_inlet(signalInfo);
            try { m_signalInlet->open_stream(); }
            catch (...) {
                this->getLogManager() << Kernel::LogLevel_Error << "Failed to open signal stream with name [" << signalInfo.name() << "]\n";
                return false;
            }
            this->getLogManager() << Kernel::LogLevel_Trace << "We have open stream [" << signalInfo.name() << "]\n";
        }
    }
    if (m_signalInlet)  // We have a signal inlet
    {
        double time;
        // Signal
        try { time = m_signalInlet->pull_sample(&m_bufferLSL, 1, 0.0); }                // 2 element timeout to 0.0 to avoid lag (OpenViBE can't have background task)
        catch (...) {
            this->getLogManager() << Kernel::LogLevel_Error << "Failed to get signal sample\n";
            return false;
        }
        if (std::abs(time) > 0.0) {
            m_oMatrix->getBuffer()[0] = double(m_bufferLSL);
            m_signalEncoder.encodeBuffer();                                         // Buffer encoded
            boxContext.markOutputAsReadyToSend(0, m_lastMatrixTime, currentTime);   // Makes the output available
            m_lastMatrixTime = currentTime;
        }
    }

    if (!m_stimInlet)   // We haven't a stimulation inlet
    {
        const lsl::stream_info stimInfo = LSL::findStreamInfo(m_inMarkerName, "", 0);   // timeout of 0 to avoid blocking OpenViBE
        if (stimInfo.name() == m_inMarkerName) {
            m_stimInlet = new lsl::stream_inlet(stimInfo);
            try { m_stimInlet->open_stream(); }
            catch (...) {
                this->getLogManager() << Kernel::LogLevel_Error << "Failed to open marker stream with name [" << stimInfo.name() << "]\n";
                return false;
            }
            this->getLogManager() << Kernel::LogLevel_Trace << "We have open stream [" << stimInfo.name() << "]\n";
        }
    }
    if (m_stimInlet)    // We have a stimulation inlet
    {
        double time;
        try { time = m_stimInlet->pull_sample(&m_bufferLSL, 1, 0.0); }                  // 1 element timeout to 0.0 to avoid lag (OpenViBE can't have background task)
        catch (...) {
            this->getLogManager() << Kernel::LogLevel_Error << "Failed to get stimulation sample\n";
            return false;
        }
        if (std::abs(time) > 0.0) {
            m_oStimSet->resize(0);                                      // reset stimulation output
            m_oStimSet->push_back(uint64_t(m_bufferLSL), currentTime, 0);
            m_stimEncoder.encodeBuffer();                                           // Buffer encoded
            boxContext.markOutputAsReadyToSend(1, m_lastStimTime, currentTime);     // Makes the output available
            m_lastStimTime = currentTime;
            this->getLogManager() << Kernel::LogLevel_Info << "Stim received with time " << time << "s in the unity app\n";
        }
    }
    return true;
}

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
