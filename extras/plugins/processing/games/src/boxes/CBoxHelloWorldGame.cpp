///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxHelloWorldGame.cpp
/// \brief Class of the box that communicate with Hello World Game.
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

#include "CBoxHelloWorldGame.hpp"

#include <lsl/Utils.hpp>
#include <ctime>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace Games {

//--------------------------------------------------------------------------------
bool CBoxHelloWorldGame::initialize()
{
    m_signalOutlet = nullptr;
    m_stimOutlet   = nullptr;

    m_signalDecoder.initialize(*this, 0);
    m_stimDecoder.initialize(*this, 1);
    m_iMatrix  = m_signalDecoder.getOutputMatrix();
    m_iStimSet = m_stimDecoder.getOutputStimulationSet();

    // These are supposed to be unique, so we don't have them in the box config
    m_signalID = CIdentifier::random().str();
    m_markerID = CIdentifier::random().str();

    while (m_markerID == m_signalID) { m_markerID = CIdentifier::random().str(); } // very unlikely

    this->getLogManager() << Kernel::LogLevel_Trace << "We Will create streams [" << m_signalName << ", id " << m_signalID
            << "] and [" << m_markerName << ", id " << m_markerID << "]\n";

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloWorldGame::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_stimDecoder.uninitialize();

    if (m_signalOutlet) {
        delete m_signalOutlet;
        m_signalOutlet = nullptr;
    }
    if (m_stimOutlet) {
        delete m_stimOutlet;
        m_stimOutlet = nullptr;
    }

    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloWorldGame::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
bool CBoxHelloWorldGame::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // Process signals
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_signalDecoder.decode(i);
        if (m_signalDecoder.isHeaderReceived() && !m_signalOutlet) {
            const size_t samplesPerBlock = m_signalDecoder.getOutputMatrix()->getDimensionSize(1);
            const size_t frequency       = m_signalDecoder.getOutputSamplingRate();

            lsl::stream_info signalInfo = LSL::createSignalStreamInfo(m_signalName, m_signalID, m_signalDecoder.getOutputMatrix(), frequency);
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
            lsl::stream_info info = LSL::createStimulationStreamInfo(m_markerName, m_markerID);

            try { m_stimOutlet = new lsl::stream_outlet(info); }
            catch (...) {
                this->getLogManager() << "Unable to create marker outlet\n";
                return false;
            }
        }
        if (m_stimDecoder.isBufferReceived()) { LSL::sendStimulation(m_stimOutlet, m_iStimSet); }
        //if (m_stimDecoder.isEndReceived()) { }
    }
    return true;
}

}  // namespace Games
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
