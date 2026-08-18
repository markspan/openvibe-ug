#include "GenericVRPNServer.hpp"

#include <vrpn_Connection.h>
#include <vrpn_Button.h>
#include <vrpn_Analog.h>

#include <cstdarg>

CGenericVrpnServer* CGenericVrpnServer::m_serverInstance = nullptr;

CGenericVrpnServer::CGenericVrpnServer(const int port) : m_connection(vrpn_create_server_connection(port)) { }

CGenericVrpnServer::~CGenericVrpnServer() { DeleteInstance(); }

CGenericVrpnServer* CGenericVrpnServer::GetInstance(const int port)
{
    if (m_serverInstance == nullptr) { m_serverInstance = new CGenericVrpnServer(port); }
    return m_serverInstance;
}

void CGenericVrpnServer::DeleteInstance()
{
    for (auto it = m_serverInstance->m_buttonServer.begin(); it != m_serverInstance->m_buttonServer.end(); ++it) { delete it->second.server; }
    m_serverInstance->m_buttonServer.clear();

    for (auto it = m_serverInstance->m_analogServer.begin(); it != m_serverInstance->m_analogServer.end(); ++it) { delete it->second.server; }
    m_serverInstance->m_analogServer.clear();

    delete m_serverInstance;
    m_serverInstance = nullptr;
}

void CGenericVrpnServer::Loop()
{
    for (auto it = m_buttonServer.begin(); it != m_buttonServer.end(); ++it) { it->second.server->mainloop(); }
    for (auto it = m_analogServer.begin(); it != m_analogServer.end(); ++it) { it->second.server->mainloop(); }
    m_connection->mainloop();
}


void CGenericVrpnServer::AddButton(const std::string& name, const int buttonCount)
{
    SButtonServer serverObject;

    serverObject.server  = new vrpn_Button_Server(name.c_str(), m_connection, buttonCount);
    serverObject.nButton = buttonCount;

    m_buttonServer.insert(std::pair<std::string, SButtonServer>(name, serverObject));
    m_buttonServer[name].cache.clear();
    m_buttonServer[name].cache.resize(buttonCount);
}

void CGenericVrpnServer::ChangeButtonState(const std::string& name, const int index, const int state)
{
    m_buttonServer[name].server->set_button(index, state);
    m_buttonServer[name].cache[index] = state;
}

int CGenericVrpnServer::GetButtonState(const std::string& name, const int index) { return m_buttonServer[name].cache[index]; }

void CGenericVrpnServer::AddAnalog(const std::string& name, const int nChannel)
{
    SAnalogServer serverObject;

    serverObject.server   = new vrpn_Analog_Server(name.c_str(), m_connection, nChannel);
    serverObject.nChannel = nChannel;

    m_analogServer.insert(std::pair<std::string, SAnalogServer>(name, serverObject));
}

void CGenericVrpnServer::ChangeAnalogState(std::string name, ...)
{
    double* channels = m_analogServer[name].server->channels();

    va_list list;
    va_start(list, name);

    for (int i = 0; i < m_analogServer[name].nChannel; ++i) { channels[i] = va_arg(list, double); }

    va_end(list);

    m_analogServer[name].server->report();
}

double* CGenericVrpnServer::GetAnalogChannels(const std::string& name) { return m_analogServer[name].server->channels(); }

void CGenericVrpnServer::ReportAnalogChanges(const std::string& name) { m_analogServer[name].server->report(); }
