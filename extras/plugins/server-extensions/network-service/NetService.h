#pragma once

// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include<memory>

#include <openvibe/parameters/parameters.h>
#include <openvibe/Services/IService.h>

#include <CNetwork.h>
#include <CServer.h>

#include <NetService_Export.h>

namespace OpenViBE {
namespace Services {

class NetServicePrivate;

/**
 * @brief Singleton class that provides access to a network server as an IService.
 *
 * This class encapsulates a network server and provides a singleton
 * instance for global access. It uses the Pimpl idiom to hide implementation
 * details in particular in a shared library context (Dll/So/Dylib).
 * 
 * @note This class use RTTI features from C++ standard library to manage memory for Pimpl idiom and Singleton pattern.
 */
class NETSERVICE_EXPORT NetService : public IService
{
public:
    ~NetService() = default;

    static NetService& instance();
    operator Network::CServer& ();
    Network::CServer& server() { return *this; }

    Parameters::ParameterInt* getPortParameter();
    Parameters::ParameterBool* getRunParameter();

    std::list<Parameters::ParameterTraits*> parameters() override;

private:
    NetService();

    NetService& operator=(const NetService&) = delete;
    NetService& operator=(NetService&) = delete;
    NetService(const NetService&) = delete;
    NetService(NetService&) = delete;

private:
    static std::unique_ptr<NetServicePrivate> d;
};

}
}