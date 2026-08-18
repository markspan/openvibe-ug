#pragma once

#include "IConnection.h"

namespace Socket {
struct SConnectionDelegate
{
    void* data;
    bool (*fpOpen)(void*);
    bool (*fpClose)(void*);
    bool (*fpIsReadyToSend)(void*, size_t);
    bool (*fpIsReadyToReceive)(void*, size_t);
    size_t (*fpSendBuffer)(void*, const void*, size_t);
    size_t (*fpReceiveBuffer)(void*, void*, size_t);
    bool (*fpSendBufferBlocking)(void*, const void*, size_t);
    bool (*fpReceiveBufferBlocking)(void*, void*, size_t);
    bool (*fpIsConnected)(void*);
    bool (*fpRelease)(void*);
};

template <class T>
class Socket_API TConnectionDelegate : public T
{
public:
    TConnectionDelegate(const SConnectionDelegate connectionDelegate) : m_connectionDelegate(connectionDelegate) { }

    virtual bool close() { return m_connectionDelegate.fpClose(m_connectionDelegate.data); }

    virtual bool isReadyToSend(const size_t timeOut) const { return m_connectionDelegate.fpIsReadyToSend(m_connectionDelegate.data, timeOut); }
    virtual bool isReadyToReceive(const size_t timeOut) const { return m_connectionDelegate.fpIsReadyToReceive(m_connectionDelegate.data, timeOut); }

    virtual size_t sendBuffer(const void* buffer, const size_t size) { return m_connectionDelegate.fpSendBuffer(m_connectionDelegate.data, buffer, size); }
    virtual size_t receiveBuffer(void* buffer, const size_t size) { return m_connectionDelegate.fpReceiveBuffer(m_connectionDelegate.data, buffer, size); }

    virtual bool sendBufferBlocking(const void* buffer, const size_t size)
    {
        return m_connectionDelegate.fpSendBufferBlocking(m_connectionDelegate.data, buffer, size);
    }
    virtual bool receiveBufferBlocking(void* buffer, const size_t bufferSize)
    {
        return m_connectionDelegate.fpReceiveBufferBlocking(m_connectionDelegate.data, buffer, bufferSize);
    }

    virtual bool isConnected() const { return m_connectionDelegate.fpIsConnected(m_connectionDelegate.data); }

    virtual void release() { m_connectionDelegate.fpRelease(m_connectionDelegate.data); }

    virtual ~TConnectionDelegate() { }
protected:
    virtual bool open() { return m_connectionDelegate.fpOpen(m_connectionDelegate.data); }

    SConnectionDelegate m_connectionDelegate;
};
}  // namespace Socket
