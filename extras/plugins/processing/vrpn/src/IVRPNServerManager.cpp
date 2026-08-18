///-------------------------------------------------------------------------------------------------
/// 
/// \file IVRPNServerManager.cpp
/// \brief Interface for VRPN Management.
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

#include "IVRPNServerManager.hpp"

#include <vrpn_Connection.h>
#include <vrpn_Analog.h>
#include <vrpn_Button.h>

#include <vector>
#include <map>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace VRPN {
namespace {
class CVRPNServerManager final : public IVRPNServerManager
{
public:
    CVRPNServerManager() { }
    ~CVRPNServerManager() override { }

    bool Initialize() override;
    bool Uninitialize() override;

    bool Process() override;
    bool ReportAnalog(const CIdentifier& serverID) override;
    bool ReportButton(const CIdentifier& serverID) override;

    bool AddServer(const CString& name, CIdentifier& identifier) override;
    bool IsServer(const CIdentifier& identifier) const override;
    bool IsServer(const CString& name) const override;
    bool GetServerId(const CString& name, CIdentifier& identifier) const override;
    bool GetServerName(const CIdentifier& serverID, CString& name) const override;
    bool RemoveServer(const CIdentifier& serverID) override;

    bool SetButtonCount(const CIdentifier& serverID, size_t nButton) override;
    bool SetButtonState(const CIdentifier& serverID, size_t buttonIndex, bool status) override;
    bool GetButtonState(const CIdentifier& serverID, size_t buttonIndex) const override;

    bool SetAnalogCount(const CIdentifier& serverID, size_t nAnalog) override;
    bool SetAnalogState(const CIdentifier& serverID, size_t analogIndex, double status) override;
    double GetAnalogState(const CIdentifier& serverID, size_t analogIndex) const override;

protected:
    vrpn_Connection* m_connection = nullptr;

    std::map<CIdentifier, CString> m_serverNames;
    std::map<CIdentifier, vrpn_Button_Server*> m_buttonServers;
    std::map<CIdentifier, vrpn_Analog_Server*> m_analogServers;
    std::map<CIdentifier, std::vector<bool>> m_buttonCaches;

    size_t m_nInitialize = 0;
};
}  // namespace

IVRPNServerManager& IVRPNServerManager::GetInstance()
{
    static CVRPNServerManager gVRPNServerManager;
    return gVRPNServerManager;
}

bool CVRPNServerManager::Initialize()
{
    if (!m_nInitialize) {
        //m_Connection=new vrpn_Connection;
        m_connection = vrpn_create_server_connection();
    }
    m_nInitialize++;
    return true;
}

bool CVRPNServerManager::Uninitialize()
{
    m_nInitialize--;
    if (!m_nInitialize) {
        for (auto it = m_analogServers.begin(); it != m_analogServers.end(); ++it) {
            if (it->second) {
                delete it->second;
                it->second = nullptr;
            }
        }
        m_analogServers.clear();
        for (auto it = m_buttonServers.begin(); it != m_buttonServers.end(); ++it) {
            if (it->second) {
                delete it->second;
                it->second = nullptr;
            }
        }
        m_buttonServers.clear();

        // $$$ UGLY !
        // The following function should destroy correctly the connection, but does not.
        //vrpn_ConnectionManager::instance().deleteConnection(m_Connection);
        delete m_connection;
        m_connection = nullptr;
    }
    return true;
}

bool CVRPNServerManager::Process()
{
    for (auto it = m_analogServers.begin(); it != m_analogServers.end(); ++it) { if (it->second) { it->second->mainloop(); } }
    for (auto it = m_buttonServers.begin(); it != m_buttonServers.end(); ++it) { if (it->second) { it->second->mainloop(); } }
    if (m_connection) { m_connection->mainloop(); }
    return true;
}

bool CVRPNServerManager::ReportAnalog(const CIdentifier& serverID)
{
    const auto it = m_analogServers.find(serverID);
    if (it != m_analogServers.end()) {
        if (it->second) {
            // This is public function and mainloop won't call it for me
            // Thank you VRPN for this to be similar to button behavior ;o)
            it->second->report();
        }
    }
    return true;
}

bool CVRPNServerManager::ReportButton(const CIdentifier& serverID)
{
    const auto it = m_buttonServers.find(serverID);
    if (it != m_buttonServers.end()) {
        if (it->second) {
            // This is not public function, however, mainloop calls it for me
            // Thank you VRPN for this to be similar to analog behavior ;o)
            // itButtonServer->second->report_changes();
        }
    }
    return true;
}

bool CVRPNServerManager::AddServer(const CString& name, CIdentifier& identifier)
{
    if (this->IsServer(name)) { return this->GetServerId(name, identifier); }

    identifier = CIdentifier::random();
    while (m_serverNames.find(identifier) != m_serverNames.end()) { ++identifier; }

    m_serverNames[identifier] = name;
    return true;
}

bool CVRPNServerManager::IsServer(const CIdentifier& identifier) const { return m_serverNames.find(identifier) != m_serverNames.end(); }

bool CVRPNServerManager::IsServer(const CString& name) const
{
    for (auto it = m_serverNames.begin(); it != m_serverNames.end(); ++it) { if (it->second == name) { return true; } }
    return false;
}

bool CVRPNServerManager::GetServerId(const CString& name, CIdentifier& identifier) const
{
    for (auto it = m_serverNames.begin(); it != m_serverNames.end(); ++it) {
        if (it->second == name) {
            identifier = it->first;
            return true;
        }
    }
    return false;
}

bool CVRPNServerManager::GetServerName(const CIdentifier& serverID, CString& name) const
{
    const auto it = m_serverNames.find(serverID);
    if (it == m_serverNames.end()) { return false; }
    name = it->second;
    return true;
}

bool CVRPNServerManager::RemoveServer(const CIdentifier& serverID)
{
    if (!this->IsServer(serverID)) { return false; }
    // TODO
    return true;
}

bool CVRPNServerManager::SetButtonCount(const CIdentifier& serverID, const size_t nButton)
{
    if (!this->IsServer(serverID)) { return false; }
    delete m_buttonServers[serverID];
    m_buttonServers[serverID] = new vrpn_Button_Server(m_serverNames[serverID], m_connection, int(nButton));
    m_buttonCaches[serverID].clear();
    m_buttonCaches[serverID].resize(nButton);
    return true;
}

bool CVRPNServerManager::SetButtonState(const CIdentifier& serverID, const size_t buttonIndex, const bool status)
{
    if (!this->IsServer(serverID) || m_buttonServers.find(serverID) == m_buttonServers.end()) { return false; }
    m_buttonServers[serverID]->set_button(int(buttonIndex), status ? 1 : 0);
    m_buttonCaches[serverID][buttonIndex] = status;
    return true;
}

bool CVRPNServerManager::GetButtonState(const CIdentifier& serverID, const size_t buttonIndex) const
{
    if (!this->IsServer(serverID)) { return false; }
    const auto itButtonServer = m_buttonServers.find(serverID);
    if (itButtonServer == m_buttonServers.end()) { return false; }
    const auto itButtonCache = m_buttonCaches.find(serverID);
    return itButtonCache->second[buttonIndex];
}

bool CVRPNServerManager::SetAnalogCount(const CIdentifier& serverID, const size_t nAnalog)
{
    if (!this->IsServer(serverID)) { return false; }
    if (m_analogServers[serverID]) { delete m_analogServers[serverID]; }
    m_analogServers[serverID] = new vrpn_Analog_Server(m_serverNames[serverID], m_connection);
    if (size_t(m_analogServers[serverID]->setNumChannels(vrpn_int32(nAnalog))) != nAnalog) { return false; }
    return true;
}

bool CVRPNServerManager::SetAnalogState(const CIdentifier& serverID, const size_t analogIndex, const double status)
{
    if (!this->IsServer(serverID) || m_analogServers.find(serverID) == m_analogServers.end()) { return false; }
    const size_t nChannel = m_analogServers[serverID]->getNumChannels();
    if (analogIndex >= nChannel) { return false; }

    m_analogServers[serverID]->channels()[analogIndex] = status;
    return true;
}

double CVRPNServerManager::GetAnalogState(const CIdentifier& serverID, const size_t analogIndex) const
{
    if (!this->IsServer(serverID)) { return 0; }
    const auto it = m_analogServers.find(serverID);
    if (it == m_analogServers.end()) { return 0; }
    return it->second->channels()[analogIndex];
}

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
