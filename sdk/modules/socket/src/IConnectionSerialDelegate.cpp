#include "IConnectionSerialDelegate.h"

namespace Socket {
class CConnectionSerialDelegate final : public IConnectionSerialDelegate
{
public:
    explicit CConnectionSerialDelegate(const SConnectionSerialDelegate connectionSerialDelegate) : IConnectionSerialDelegate(connectionSerialDelegate)
    {
        m_connectionSerialDelegate = connectionSerialDelegate;
    }

    ~CConnectionSerialDelegate() override { }

    bool connect(const char* url, const size_t baudRate) override
    {
        return m_connectionSerialDelegate.fpConnect(m_connectionSerialDelegate.connectionDelegate.data, url, baudRate);
    }

    size_t getPendingByteCount() override { return m_connectionSerialDelegate.fpGetPendingByteCount(m_connectionSerialDelegate.connectionDelegate.data); }

    bool flush() override { return m_connectionSerialDelegate.fpFlush(m_connectionSerialDelegate.connectionDelegate.data); }

    const char* getLastError() override { return m_connectionSerialDelegate.fpGetLastError(m_connectionSerialDelegate.connectionDelegate.data); }

    bool isErrorRaised()
    override { return false; }      // return m_connectionSerialDelegate.fpIsErrorRaised(m_connectionSerialDelegate.connectionDelegate.data);
    void clearError() override { }  // return m_connectionSerialDelegate.fpClearError(m_connectionSerialDelegate.connectionDelegate.data);
    bool setTimeouts(const size_t /*timeout*/)
    override { return true; }       // return m_connectionSerialDelegate.fpSetTimeouts(m_connectionSerialDelegate.connectionDelegate.data, decisecondsTimeout);
};

IConnectionSerialDelegate* createConnectionSerialDelegate(const SConnectionSerialDelegate connectionSerialDelegate)
{
    return new CConnectionSerialDelegate(connectionSerialDelegate);
}
}  // namespace Socket
