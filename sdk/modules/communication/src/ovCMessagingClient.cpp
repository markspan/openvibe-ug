#include "ovCMessagingClient.h"
#include "ovCMessagingImpl.hpp"
#include <chrono>

namespace Communication {

MessagingClient::MessagingClient() : CMessaging(), m_Client(Socket::createConnectionClient()) {}

MessagingClient::~MessagingClient()
{
    this->close();
    m_Client->release();
}

bool MessagingClient::connect(const std::string& uri, const size_t port)
{
    this->reset();

    if (!m_Client->connect(uri.c_str(), port))
    {
        this->setLastError(Socket_FailedToConnect);
        return false;
    }

    this->setConnection(m_Client);

    // Once the connection is done, the client push an authentication message to the server
    if (!this->pushAuthentication(impl->m_ConnectionID))
    {
        // Error set in the function
        const ELibraryError error = this->getLastError();
        this->close();
        this->setLastError(error);
        return false;
    }

    if (!this->startSyncing())
    {
        this->close();
        return false;
    }

    // To work, the client need to receive box information message. So we wait to receive the box information message.
    // A timeout of 10 seconds is set.
    const std::chrono::time_point<std::chrono::system_clock> startClock = std::chrono::system_clock::now();
    uint64_t packetId;

    while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startClock).count() < 10)
    {
        if (this->popBoxDescriptions(packetId, impl->m_BoxDesc))
        {
            m_BoxDescriptionReceived = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!m_BoxDescriptionReceived)
    {
        this->close();
        this->setLastError(BoxDescriptionNotReceived);
        return false;
    }

    return true;
}

bool MessagingClient::close() const
{
    this->pushMessage(EndMessage());

    this->stopSyncing();

    if (m_Client != nullptr)
    {
        if (!m_Client->close())
        {
            this->setLastError(Socket_FailedToCloseConnection);
            return false;
        }
    }

    return true;
}

size_t MessagingClient::getParameterCount() const
{
    if (!m_BoxDescriptionReceived) { return 0; }
    return impl->m_BoxDesc.getParameters()->size();
}

size_t MessagingClient::getInputCount() const
{
    if (!m_BoxDescriptionReceived) { return 0; }
    return impl->m_BoxDesc.getInputs()->size();
}

size_t MessagingClient::getOutputCount() const
{
    if (!m_BoxDescriptionReceived) { return 0; }
    return impl->m_BoxDesc.getOutputs()->size();
}

bool MessagingClient::getParameter(const size_t i, uint64_t& id, uint64_t& type, std::string& name, std::string& value) const
{
    if (!m_BoxDescriptionReceived) { return false; }

    const std::vector<Parameter>* parameters = impl->m_BoxDesc.getParameters();

    if (parameters->size() <= i) { return false; }

    id    = parameters->at(i).getId();
    type  = parameters->at(i).getType();
    name  = parameters->at(i).getName();
    value = parameters->at(i).getValue();

    return true;
}

bool MessagingClient::getInput(const size_t i, uint64_t& id, uint64_t& type, std::string& name) const
{
    if (!m_BoxDescriptionReceived) { return false; }

    const std::vector<InputOutput>* inputs = impl->m_BoxDesc.getInputs();

    if (inputs->size() <= i) { return false; }

    id   = inputs->at(i).getId();
    type = inputs->at(i).getType();
    name = inputs->at(i).getName();

    return true;
}

bool MessagingClient::getOutput(const size_t i, uint64_t& id, uint64_t& type, std::string& name) const
{
    if (!m_BoxDescriptionReceived)
    {
        const_cast<MessagingClient*>(this)->setLastError(BoxDescriptionNotReceived);
        return false;
    }

    const std::vector<InputOutput>* outputs = impl->m_BoxDesc.getOutputs();

    if (outputs->size() <= i) { return false; }

    id   = outputs->at(i).getId();
    type = outputs->at(i).getType();
    name = outputs->at(i).getName();

    return true;
}

bool MessagingClient::popError(uint64_t& packetId, EError& type, uint64_t& guiltyId) { return CMessaging::popError(packetId, type, guiltyId); }

bool MessagingClient::popEBML(uint64_t& packetId, size_t& index, uint64_t& startTime, uint64_t& endTime, std::shared_ptr<const std::vector<uint8_t>>& ebml)
{
    return CMessaging::popEBML(packetId, index, startTime, endTime, ebml);
}

bool MessagingClient::pushAuthentication(const std::string& connectionID) const { return this->pushMessage(AuthenticationMessage(connectionID)); }
bool MessagingClient::pushLog(const ELogLevel logLevel, const std::string& log) const { return this->pushMessage(LogMessage(logLevel, log)); }

bool MessagingClient::pushEBML(const size_t index, const uint64_t startTime, const uint64_t endTime,
                               const std::shared_ptr<const std::vector<uint8_t>>& ebml) const
{
    return this->pushMessage(EBMLMessage(index, startTime, endTime, ebml));
}

bool MessagingClient::pushSync() const { return this->pushMessage(SyncMessage()); }
bool MessagingClient::waitForSyncMessage() { return CMessaging::waitForSyncMessage(); }

}  // namespace Communication
