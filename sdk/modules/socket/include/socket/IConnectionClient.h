#pragma once

#include "IConnection.h"

namespace Socket {
class Socket_API IConnectionClient : public IConnection
{
public:

    virtual bool connect(const char* serverName, const size_t serverPort, const size_t timeOut = 0xffffffff) = 0;
};

extern Socket_API IConnectionClient* createConnectionClient();
}  // namespace Socket
