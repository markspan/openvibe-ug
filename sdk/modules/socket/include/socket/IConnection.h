#pragma once

#include "defines.h"
#include <cstdlib>  // For Unix Compatibility

namespace Socket {
class Socket_API IConnection
{
protected:

    virtual bool open() = 0;

public:

    virtual bool close() = 0;

    virtual bool isReadyToSend(const size_t timeOut = 0) const = 0;
    virtual bool isReadyToReceive(const size_t timeOut = 0) const = 0;

    virtual size_t sendBuffer(const void* buffer, const size_t size) = 0;
    virtual size_t receiveBuffer(void* buffer, const size_t size) = 0;

    virtual bool sendBufferBlocking(const void* buffer, const size_t size) = 0;
    virtual bool receiveBufferBlocking(void* buffer, const size_t size) = 0;

    virtual bool isConnected() const = 0;

    virtual void release() = 0;

protected:

    virtual ~IConnection() = default;
};
}  // namespace Socket
