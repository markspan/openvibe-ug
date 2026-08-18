#include "ovCMessagingServer.h"
#include "ovCMessagingImpl.hpp"
#include <chrono>

namespace Communication {

MessagingServer::~MessagingServer()
{
    this->close();
    m_Server->release();
}


bool MessagingServer::accept()
{
    this->reset();

    if (this->isConnected())
    {
        // A connection is already done to a client
        this->setLastError(Socket_ClientAlreadyConnected);
        return false;
    }

    if (!m_Server->isReadyToReceive())
    {
        this->setLastError(Socket_NoIncomingClientConnection);
        return false;
    }

    m_Client = m_Server->accept();

    if (m_Client == nullptr)
    {
        this->setLastError(Socket_NoIncomingClientConnection);
        return false;
    }

    this->setConnection(m_Client);

    this->startSyncing();

    if (!impl->m_ConnectionID.empty())
    {
        // The server has to verify that the client is the one that is waited. So the server wait the authentication packet.
        const std::chrono::time_point<std::chrono::system_clock> startClock = std::chrono::system_clock::now();

        std::string connectionID;
        uint64_t id         = 0;
        bool isAuthReceived = false;

        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startClock).count() < 10)
        {
            if (this->popAuthentication(id, connectionID))
            {
                isAuthReceived = true;

                if (connectionID != impl->m_ConnectionID)
                {
                    this->pushError(Error_AuthenticationFail, id);
                    this->close();
                    this->setLastError(BadAuthenticationReceived);
                    return false;
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!isAuthReceived)
        {
            this->pushError(Error_AuthenticationRequested, id);
            this->close();
            this->setLastError(NoAuthenticationReceived);
            return false;
        }
    }

    if (!this->pushMessage(CommunicationProtocolVersionMessage(s_CommunicationProtocol_MajorVersion, s_CommunicationProtocol_MinorVersion)))
    {
        // Error set in the function
        const ELibraryError error = this->getLastError();
        this->close();
        this->setLastError(error);
        return false;
    }

    if (!this->pushMessage(impl->m_BoxDesc))
    {
        // Error set in the function
        const ELibraryError error = this->getLastError();
        this->close();
        this->setLastError(error);
        return false;
    }

    return true;
}

bool MessagingServer::close()
{
    this->pushMessage(EndMessage());

    this->stopSyncing();

    bool errorRaised = false;

    if (m_Client != nullptr)
    {
        if (!m_Client->close())
        {
            errorRaised = true;
            this->setLastError(Socket_FailedToCloseConnection);
        }

        m_Client->release();
        m_Client = nullptr;
    }

    if (!m_Server->close())
    {
        this->setLastError(Socket_FailedToCloseConnection);
        errorRaised = true;
    }

    return !errorRaised;
}

bool MessagingServer::addParameter(const uint64_t id, const size_t type, const std::string& name, const std::string& value) const
{
    return impl->m_BoxDesc.addParameter(id, type, name, value);
}

bool MessagingServer::addInput(const uint64_t id, const size_t type, const std::string& name) const { return impl->m_BoxDesc.addInput(id, type, name); }
bool MessagingServer::addOutput(const uint64_t id, const size_t type, const std::string& name) const { return impl->m_BoxDesc.addOutput(id, type, name); }
bool MessagingServer::popLog(uint64_t& packetId, ELogLevel& type, std::string& message) { return CMessaging::popLog(packetId, type, message); }

bool MessagingServer::popEBML(uint64_t& packetId, size_t& index, uint64_t& startTime, uint64_t& endTime, std::shared_ptr<const std::vector<uint8_t>>& ebml)
{
    return CMessaging::popEBML(packetId, index, startTime, endTime, ebml);
}

bool MessagingServer::pushError(const EError error, const uint64_t guiltyId) const { return this->pushMessage(ErrorMessage(error, guiltyId)); }

bool MessagingServer::pushEBML(const size_t index, const uint64_t startTime, const uint64_t endTime,
                               const std::shared_ptr<const std::vector<uint8_t>>& ebml) const
{
    return this->pushMessage(EBMLMessage(index, startTime, endTime, ebml));
}

}  // namespace Communication
