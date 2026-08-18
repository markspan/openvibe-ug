#include <NetWriter.h>
#include <openvibe/acquisition-messaging/ovMsgSerializer.h>

#include <NetService.h>

class NetWriterPrivate
{
public:
    Network::CServer *server;
};


NetWriter::NetWriter(const std::string& writerName) : AbstractWriter(writerName), d ( std::make_unique<NetWriterPrivate>() )
{
    d->server = &OpenViBE::Services::NetService::instance().server();
    d->server->registerWriteCallback<ovMsg>(
        [this](void* data, uint64_t clientId)
        {
            return sendCallback(data, clientId);
        }
    );
}

NetWriter::~NetWriter()
{

}

std::pair<size_t, void*> NetWriter::sendCallback(void* data, uint64_t clientId)
{
    const ovMsg* msg = reinterpret_cast<const ovMsg*>(data);
    return serializeOvMsg(*msg);
}

size_t NetWriter::send(ovMsg* msg, uint64_t clientId)
{
    size_t res = 0;
    if (clientId != -1) {
        res = d->server->send<ovMsg>(msg, clientId);
    } else {
        auto clients = getClients();
        std::list<int> clientsIds(clients.begin(), clients.end());
        res = d->server->send<ovMsg>(msg, clientsIds);
    }

    return res;
}
