///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmVRPNButtonClient.cpp
/// \brief Classes implementation for the Box VRPN Button client.
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

#include "CBoxAlgorithmVRPNButtonClient.hpp"

namespace OpenViBE {
namespace Plugins {
namespace VRPN {

static void VRPN_CALLBACK VRPNButtonCB(void* data, const vrpn_BUTTONCB b)
{
    CBoxAlgorithmVRPNButtonClient* box = static_cast<CBoxAlgorithmVRPNButtonClient*>(data);
    (box->m_Buttons).push_back(std::pair<size_t, bool>(b.button, b.state ? true : false));
}

bool CBoxAlgorithmVRPNButtonClient::initialize()
{
    const size_t nOutput = this->getStaticBoxContext().getOutputCount();

    for (size_t i = 0; i < nOutput; ++i) {
        Kernel::IAlgorithmProxy* encoder = &this->getAlgorithmManager().getAlgorithm(
            this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
        encoder->initialize();
        CStimulationSet* stimSet = new CStimulationSet();
        Kernel::TParameterHandler<CStimulationSet*> ip_StimSet(encoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
        ip_StimSet = stimSet;
        m_encoders.push_back(encoder);
        m_stimSets.push_back(stimSet);

        m_stimIDsOn.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i * 2 + 1));
        m_stimIDsOff.push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i * 2 + 2));
    }

    const CString name = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    m_vrpnButtonRemote = new vrpn_Button_Remote(name.toASCIIString());
    m_vrpnButtonRemote->register_change_handler(static_cast<void*>(this), VRPNButtonCB);

    m_lastChunkEndTime = uint64_t(-1);

    return true;
}

bool CBoxAlgorithmVRPNButtonClient::uninitialize()
{
    const size_t nOutput = this->getStaticBoxContext().getOutputCount();

    delete m_vrpnButtonRemote;
    m_vrpnButtonRemote = nullptr;

    for (size_t i = 0; i < nOutput; ++i) {
        delete m_stimSets[i];
        m_encoders[i]->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_encoders[i]);
    }
    m_stimSets.clear();
    m_encoders.clear();
    m_stimIDsOn.clear();
    m_stimIDsOff.clear();

    return true;
}

bool CBoxAlgorithmVRPNButtonClient::processClock(Kernel::CMessageClock& /*msg*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}


bool CBoxAlgorithmVRPNButtonClient::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nOutput       = this->getStaticBoxContext().getOutputCount();
    size_t i;

    // Clears pending stimulations
    for (i = 0; i < nOutput; ++i) { m_stimSets[i]->clear(); }

    // Refreshes VRPN device
    m_gotStimulation = false;
    m_vrpnButtonRemote->mainloop();

    while (!m_Buttons.empty()) {
        const size_t index = m_Buttons.front().first;
        const bool pressed = m_Buttons.front().second;
        m_Buttons.pop_front();

        SetButton(index, pressed);
    }

    // Encodes streams
    for (i = 0; i < nOutput; ++i) {
        Kernel::TParameterHandler<CMemoryBuffer*> buffer(
            m_encoders[i]->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));
        buffer = boxContext.getOutputChunk(i);
        if (m_lastChunkEndTime == uint64_t(-1)) {
            m_encoders[i]->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader);
            boxContext.markOutputAsReadyToSend(i, 0, 0);
        }
        else {
            if (m_gotStimulation) { m_encoders[i]->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer); }
            boxContext.markOutputAsReadyToSend(i, m_lastChunkEndTime, this->getPlayerContext().getCurrentTime());
        }
    }

    // Updates timings
    m_lastChunkEndTime = this->getPlayerContext().getCurrentTime();

    return true;
}

void CBoxAlgorithmVRPNButtonClient::SetButton(const size_t index, const bool pressed)
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    if (index >= boxContext.getOutputCount()) {
        this->getLogManager() << Kernel::LogLevel_Warning << "Ignored button " << index + 1 << " with state " << (pressed ? "pressed" : "released") << "...\n";
    }
    else {
        this->getLogManager() << Kernel::LogLevel_Trace << "Changed button " << index + 1 << " with state " << (pressed ? "pressed" : "released") << "...\n";
        if (pressed) { m_stimSets[index]->push_back(m_stimIDsOn[index], this->getPlayerContext().getCurrentTime(), 0); }
        else { m_stimSets[index]->push_back(m_stimIDsOff[index], this->getPlayerContext().getCurrentTime(), 0); }
        m_gotStimulation = true;
    }
}

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
