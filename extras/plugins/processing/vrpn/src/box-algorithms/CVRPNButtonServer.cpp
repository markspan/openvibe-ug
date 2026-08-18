///-------------------------------------------------------------------------------------------------
/// 
/// \file CVRPNButtonServer.cpp
/// \brief Classes implementation for the Box VRPN Button Server.
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

#include "CVRPNButtonServer.hpp"
#include "../IVRPNServerManager.hpp"

namespace OpenViBE {
namespace Plugins {
namespace VRPN {

bool CVRPNButtonServer::initialize()
{
    const Kernel::IBox* box = getBoxAlgorithmContext()->getStaticBoxContext();

    //get server name, and creates a button server for this server
    const CString name = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    IVRPNServerManager::GetInstance().Initialize();
    IVRPNServerManager::GetInstance().AddServer(name, m_serverID);
    IVRPNServerManager::GetInstance().SetButtonCount(m_serverID, box->getInputCount());

    m_decoders.resize(box->getInputCount());

    //get stim id
    for (size_t i = 0; i < box->getInputCount(); ++i) {
        CString onStimulationID  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1 + i * 2);
        CString offStimulationID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2 + i * 2);
        m_stimulationPairs[i]    = std::pair<uint64_t, uint64_t>(
            getBoxAlgorithmContext()->getPlayerContext()->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_Stimulation, onStimulationID),
            getBoxAlgorithmContext()->getPlayerContext()->getTypeManager().getEnumerationEntryValueFromName(OVTK_TypeId_Stimulation, offStimulationID));

        m_decoders[i] = new Toolkit::TStimulationDecoder<CVRPNButtonServer>;
        m_decoders[i]->initialize(*this, i);
    }

    return true;
}

bool CVRPNButtonServer::uninitialize()
{
    for (size_t i = 0; i < m_decoders.size(); ++i) { delete m_decoders[i]; }
    m_decoders.clear();
    IVRPNServerManager::GetInstance().Uninitialize();
    return true;
}

bool CVRPNButtonServer::processClock(Kernel::CMessageClock& /*msg*/)
{
    IVRPNServerManager::GetInstance().Process();
    return true;
}

bool CVRPNButtonServer::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CVRPNButtonServer::process()
{
    const Kernel::IBoxIO* boxIO = getBoxAlgorithmContext()->getDynamicBoxContext();

    for (size_t input = 0; input < getBoxAlgorithmContext()->getStaticBoxContext()->getInputCount(); ++input) {
        m_currInput = input;

        for (size_t chunk = 0; chunk < boxIO->getInputChunkCount(input); ++chunk) {
            m_decoders[input]->decode(chunk);

            if (m_decoders[input]->isBufferReceived()) {
                const CStimulationSet* stimSet = m_decoders[input]->getOutputStimulationSet();
                for (size_t s = 0; s < stimSet->size(); ++s) { SetStimulation(s, stimSet->getId(s), stimSet->getDate(s)); }
            }
        }
    }

    return true;
}

void CVRPNButtonServer::SetStimulation(const size_t /*index*/, const uint64_t id, const uint64_t /*date*/)
{
    const std::pair<uint64_t, uint64_t>& stimulationPair = m_stimulationPairs[m_currInput];

    if (stimulationPair.first == stimulationPair.second) {
        if (stimulationPair.first == id) {
            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Trace << "Received TOGGLE stimulation for button "
                    << m_currInput << " (" << id << ")\n";
            IVRPNServerManager::GetInstance().SetButtonState(m_serverID, m_currInput,
                                                             !IVRPNServerManager::GetInstance().GetButtonState(m_serverID, m_currInput));
            IVRPNServerManager::GetInstance().ReportButton(m_serverID);
        }
    }
    else {
        if (stimulationPair.first == id) {
            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Trace << "Received ON stimulation for button "
                    << m_currInput << " (" << id << ")\n";
            IVRPNServerManager::GetInstance().SetButtonState(m_serverID, m_currInput, true);
            IVRPNServerManager::GetInstance().ReportButton(m_serverID);
        }
        if (stimulationPair.second == id) {
            getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Trace << "Received OFF stimulation for button "
                    << m_currInput << " (" << id << ")\n";
            IVRPNServerManager::GetInstance().SetButtonState(m_serverID, m_currInput, false);
            IVRPNServerManager::GetInstance().ReportButton(m_serverID);
        }
    }
}

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
