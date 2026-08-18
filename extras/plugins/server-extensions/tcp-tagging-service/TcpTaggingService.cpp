/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "TcpTaggingService.h"
#include "TagProtocolHandler.h"

#include <CNetwork.h>
#include <CServer.h>

#include <system/ovCTime.h>

#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>

namespace OpenViBE {
namespace Services {

std::unique_ptr<TcpTaggingServicePrivate> TcpTaggingService::d = nullptr;

class TcpTaggingServicePrivate
{
public:
    Network::CNetwork network;
    Network::CServer* server = nullptr;
    TcpTagging::TagProtocolHandler protocolHandlerPrototype;

    Parameters::ParameterInt* port = nullptr;
    Parameters::ParameterBool* run = nullptr;
    std::list<Parameters::ParameterTraits*> params;

    std::mutex queueMutex;
    std::queue<TcpTagging::Tag> tagQueue;

    bool fptimeWarningLogged = false;

    static TcpTaggingService* s_instance;
};

TcpTaggingService* TcpTaggingServicePrivate::s_instance = nullptr;

TcpTaggingService::TcpTaggingService()
{
    d = std::make_unique<TcpTaggingServicePrivate>();
    d->server = d->network.createServer();
    d->server->setCustomProtocolHandler(&d->protocolHandlerPrototype);
    d->server->registerReadCallback<TcpTagging::Tag>([this](void* data, size_t size, uint64_t clientId) {
        return onTagReceived(data, size, clientId);
    });

    d->port = new Parameters::ParameterInt();
    d->port->setName("TCP-Tagging Port");
    d->port->setDescription("Port for the TCP Tagging server.");
    d->port->setValue(15361); // legacy tcp-tagging default port
    d->port->setMin(1024);
    d->port->setMax(65535);
    d->server->setPort(static_cast<uint16_t>(d->port->value()));
    d->params.push_back(d->port);

    d->run = new Parameters::ParameterBool();
    d->run->setName("run");
    d->run->setValue("false");
    d->params.push_back(d->run);

    d->port->connect([this](std::any inPort) {
        const bool running = d->run->value();
        if (running) {
            d->server->stop();
        }
        d->server->setPort(static_cast<uint16_t>(std::any_cast<int>(inPort)));
        if (running) {
            d->server->run();
        }
    });

    d->run->connect([this](std::any inRun) {
        if (std::any_cast<bool>(inRun)) {
            d->server->run();
        }
        else {
            d->server->stop();
        }
    });

    d->run->setValue(true);
}

TcpTaggingService& TcpTaggingService::instance()
{
    if (TcpTaggingServicePrivate::s_instance == nullptr) {
        TcpTaggingServicePrivate::s_instance = new TcpTaggingService();
    }

    return *TcpTaggingServicePrivate::s_instance;
}

bool TcpTaggingService::onTagReceived(void* data, size_t size, uint64_t /*clientId*/)
{
    bool bRes = false;

    if (data && size == sizeof(TcpTagging::Tag)) {

        TcpTagging::Tag tag;
        std::memcpy(&tag, data, sizeof(tag));

        if (tag.timestamp == 0 || (tag.flags & TcpTagging::FLAG_AUTOSTAMP_SERVERSIDE))
        {
            tag.timestamp = System::Time::zgetTimeRaw(false);
        }
        else if (!(tag.flags & TcpTagging::FLAG_FPTIME))
        {
            tag.timestamp = System::Time::zgetTimeRaw(false);
            if (!d->fptimeWarningLogged)
            {
                std::cerr << "[Warning] TcpTaggingService: received tag(s) not in fixed point time. Replacing with server time.\n";
                d->fptimeWarningLogged = true;
            }
        }

        {
            std::lock_guard<std::mutex> lock(d->queueMutex);
            d->tagQueue.push(tag);
        }
    }

    return bRes;
}

bool TcpTaggingService::popTag(TcpTagging::Tag& out)
{
    bool bRes = false;
    std::lock_guard<std::mutex> lock(d->queueMutex);
    if (!d->tagQueue.empty()) {
        out = d->tagQueue.front();
        d->tagQueue.pop();
        bRes = true;
    }

    return bRes;
}

void TcpTaggingService::clearQueue()
{
    std::lock_guard<std::mutex> lock(d->queueMutex);
    std::queue<TcpTagging::Tag> empty;
    std::swap(d->tagQueue, empty);
}

Parameters::ParameterInt* TcpTaggingService::getPortParameter()
{
    return d->port;
}

Parameters::ParameterBool* TcpTaggingService::getRunParameter()
{
    return d->run;
}

std::list<Parameters::ParameterTraits*> TcpTaggingService::parameters()
{
    return d->params;
}

extern "C"
{
    TCPTAGGINGSERVICE_EXPORT IService* getServiceInstance() { return &TcpTaggingService::instance(); }
}

} // namespace Services
} // namespace OpenViBE
