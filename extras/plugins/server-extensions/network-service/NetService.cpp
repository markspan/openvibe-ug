// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include "NetService.h"


namespace OpenViBE {
namespace Services {

// Definition of the static unique_ptr "d" member for Pimpl idiom
std::unique_ptr<NetServicePrivate> NetService::d = nullptr;

/**
 * @brief Private implementation class for NetService.
 *
 * This class encapsulates the private data and implementation details
 * of the NetService class, following the Pimpl idiom.
 * 
 * @detail Contains a CNetwork instance and a pointer to a CServer instance.
 * 
 * @note This class is not intended to be used directly by users of the NetService class.
 * @see NetService
 * @date 2025
 * @version 1.0
 */
class NetServicePrivate
{
public:
	Network::CNetwork  network;
	Network::CServer* server = nullptr;
	Parameters::ParameterInt* port;
	Parameters::ParameterBool* run;
	std::list<Parameters::ParameterTraits*> m_params;

  static NetService*  s_instance;
};

// Initialize the static instance pointer to nullptr for
NetService* NetServicePrivate:: s_instance = nullptr;

/**
* @brief Constructor for NetService class.
* @details Initializes the private implementation and creates a server instance.
*/
NetService::NetService()
{
  d = std::make_unique<NetServicePrivate>();
  d->server = d-> network.createServer();

  d->port = new Parameters::ParameterInt();
  d->run  = new Parameters::ParameterBool();

  d->m_params.push_back(d->port);
  d->m_params.push_back(d->run);

  d->port->setName("Acquisition Server Port");
  d->port->setDescription("Port for the Acquisition TCP Server.");
  d->port->setValue(8088);
  d->port->setMin(1024);
  d->port->setMax(65535);
  this->d->server->setPort(d->port->value());

  d->run->setName("run");
  d->run->setValue("false");

  d->port->connect([this](std::any inPort)-> void {
    bool running = d->run->value();
    if (running) {
      this->d->server->stop();
    }
    this->d->server->setPort(static_cast<uint16_t>(std::any_cast<int>(inPort)));
    if (running) {
      this->d->server->run();
    }
  });

  d->run->connect ([this](std::any inRun)-> void {
    if (std::any_cast<bool>(inRun)) {
      d->server->run();
    }
    else {
      d->server->stop();
    }
  });

    d->run->setValue(true);
}

/**
* @brief Get the singleton instance of NetService.
* @return Reference to the singleton NetService instance.
*/
NetService& NetService::instance()
{
  if (NetServicePrivate:: s_instance == nullptr) {
    NetServicePrivate:: s_instance = new NetService();
  }

  return *NetServicePrivate:: s_instance;
}

/**
* @brief Conversion (cast) operator to Network::CServer&.
* @return Reference to the underlying CServer instance.
*/
NetService::operator Network::CServer& ()
{
  return *(d->server);
}

Parameters::ParameterInt* NetService::getPortParameter()
{
  return d->port;
}

Parameters::ParameterBool* NetService::getRunParameter()
{
  return d->run;
}

std::list<Parameters::ParameterTraits*> NetService::parameters()
{
  return d->m_params;
}


extern "C"
{
  NETSERVICE_EXPORT IService*  getServiceInstance() { return &NetService::instance(); }
}

}
}
