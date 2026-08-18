#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <memory>
#include <map>

#include "ovCMessaging.h"
#include "ovCMessagingImpl.hpp"

namespace Communication {

static const std::map<CMessaging::ELibraryError, std::string> ERRORS_STRING =
{
    { CMessaging::NoError, "No error" },
    { CMessaging::Socket_NotConnected, "Not connected" },
    { CMessaging::Socket_FailedToConnect, "Failed to connect" },
    { CMessaging::Socket_ReceiveBufferFail, "Failed to receive the buffer" },
    { CMessaging::Socket_SendBufferFail, "Failed to send the buffer" },
    { CMessaging::Socket_NoIncomingClientConnection, "No incoming client connection before the timeout" },
    { CMessaging::Socket_NotReadyToSend, "Socket not ready to send the buffer" },
    { CMessaging::Socket_NoDataReceived, "No data received by the socket" },
    { CMessaging::Socket_FailedCloseClientConnection, "Failed to close the client connection" },
    { CMessaging::Socket_FailedToCloseConnection, "Failed to close the server connection" },
    { CMessaging::Socket_FailedConnectClient, "Failed to connect the client" },
    { CMessaging::Socket_ClientAlreadyConnected, "A client is already connected" },
    { CMessaging::Deserialize_BufferTooSmall, "Buffer received is too small to be unpacked" },
    { CMessaging::Deserialize_Header, "Fail to unpack the buffer to a Header" },
    { CMessaging::Deserialize_ProtocolVersionMessage, "Fail to unpack Protocol Version message" },
    { CMessaging::Deserialize_BoxDescriptionMessage, "Fail to unpack Box description message" },
    { CMessaging::Deserialize_EBMLMessage, "Fail to unpack EBML message" },
    { CMessaging::Deserialize_EndMessage, "Fail to unpack End message" },
    { CMessaging::Deserialize_ErrorMessage, "Fail to unpack error message" },
    { CMessaging::Deserialize_LogMessage, "Fail to unpack log message" },
    { CMessaging::Deserialize_AuthenticationMessage, "Fail to unpack Authentication message" },
    { CMessaging::Deserialize_MessageTypeNotSupported, "Message type not supported" },
    { CMessaging::BoxDescriptionAlreadyReceived, "Box Description already received" },
    { CMessaging::BoxDescriptionNotReceived, "Box description not received" },
    { CMessaging::BadAuthenticationReceived, "Authentication received is invalid" },
    { CMessaging::NoAuthenticationReceived, "No authentication received before the timeout" },
    { CMessaging::ThreadJoinFailed, "Failed to terminate the thread" }
};

CMessaging::CMessaging()
{
    impl                         = new SMessagingImpl();
    impl->m_nMessage             = 0;
    impl->m_Connection           = nullptr;
    impl->m_LastLibraryError     = NoError;
    impl->m_IsStopRequested      = false;
    impl->m_IsInErrorState       = false;
    impl->m_IsEndMessageReceived = false;
}

CMessaging::~CMessaging()
{
    this->reset();
    delete impl;
}

void CMessaging::reset() const
{
    this->stopSyncing();

    impl->m_nMessage         = 0;
    impl->m_IsInErrorState   = false;
    impl->m_IsStopRequested  = false;
    impl->m_LastLibraryError = NoError;

    std::queue<std::pair<uint64_t, AuthenticationMessage>>().swap(impl->m_IncomingAuthentications);
    std::queue<std::pair<uint64_t, CommunicationProtocolVersionMessage>>().swap(impl->m_IncomingCommunicationProtocolVersions);
    std::queue<std::pair<uint64_t, BoxDescriptionMessage>>().swap(impl->m_IncomingBoxDescriptions);
    std::queue<std::pair<uint64_t, EBMLMessage>>().swap(impl->m_IncomingEBMLs);
    std::queue<std::pair<uint64_t, LogMessage>>().swap(impl->m_IncomingLogs);
    std::queue<std::pair<uint64_t, ErrorMessage>>().swap(impl->m_IncomingErrors);

    impl->m_RcvBuffer.clear();
    impl->m_SendBuffer.clear();
    impl->m_SendBuffer.reserve(impl->BUFFER_SIZE);

    impl->m_Connection = nullptr;
}

CMessaging::ELibraryError CMessaging::getLastError() const { return impl->m_LastLibraryError; }
void CMessaging::setLastError(const ELibraryError libraryError) const { impl->m_LastLibraryError = libraryError; }

bool CMessaging::push() const
{
    if (!impl->m_SendBuffer.empty() && impl->m_Connection->isReadyToSend(1))
    {
        std::lock_guard<std::mutex> lock(impl->m_SendBufferMutex);
        const uint_fast32_t result = impl->m_Connection->sendBufferBlocking(impl->m_SendBuffer.data(), impl->m_SendBuffer.size());

        if (result == 0)
        {
            this->setLastError(Socket_SendBufferFail);
            return false;
        }

        impl->m_SendBuffer.clear();
    }

    return true;
}

bool CMessaging::pull() const
{
    if (impl->m_Connection == nullptr)
    {
        this->setLastError(Socket_NotConnected);
        return false;
    }

    while (impl->m_Connection->isReadyToReceive(1))
    {
        const uint_fast32_t bytesReceived = uint_fast32_t(impl->m_Connection->receiveBuffer(impl->m_TempRcvBuffer.data(), impl->m_TempRcvBuffer.size()));

        if (bytesReceived == 0)
        {
            this->setLastError(Socket_ReceiveBufferFail);
            return false;
        }

        impl->m_RcvBuffer.insert(impl->m_RcvBuffer.end(), impl->m_TempRcvBuffer.cbegin(), impl->m_TempRcvBuffer.cbegin() + int(bytesReceived));
    }

    return true;
}

bool CMessaging::processIncomingMessages() const
{
    size_t byteRead = 0;

    while (!impl->m_RcvBuffer.empty())
    {
        if (!this->processBuffer(impl->m_RcvBuffer, byteRead))
        {
            impl->m_RcvBuffer.clear();
            // Error set in the function
            return false;
        }

        // If the processing succeed, we erase the buffer part processed. 
        if (byteRead != 0) { impl->m_RcvBuffer.erase(impl->m_RcvBuffer.begin(), impl->m_RcvBuffer.begin() + static_cast<const long>(byteRead)); }
        else
        {
            // The processing succeed but the byte count read is 0 so more data is waited.
            break;
        }
    }

    return true;
}

bool CMessaging::processBuffer(const std::vector<uint8_t>& buffer, size_t& byteRead) const
{
    byteRead = 0;

    if (buffer.empty()) { return true; }

    // First, we try to fromBytes the buffer to found header information
    Header header;

    if (!header.fromBytes(buffer, byteRead))
    {
        this->setLastError(Deserialize_Header);
        byteRead = size_t(header.getSize());
        return false;
    }

    if (buffer.size() < header.getSize())
    {
        byteRead = 0;
        return true; // Just wait for more data
    }

    // Try to unpack the object according to the type given by the header.
    switch (header.getType())
    {
        case MessageType_Authentication:
        {
            AuthenticationMessage authentication;

            if (!authentication.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_AuthenticationMessage);
                return false;
            }

            std::lock_guard<std::mutex> lock(impl->m_IncAuthMutex);
            impl->m_IncomingAuthentications.emplace(header.getId(), authentication);
        }
        break;

        case MessageType_ProtocolVersion:
        {
            CommunicationProtocolVersionMessage communicationProtocolVersion;

            if (!communicationProtocolVersion.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_ProtocolVersionMessage);
                return false;
            }

            std::lock_guard<std::mutex> lock(impl->m_IncCommProVerMutex);
            impl->m_IncomingCommunicationProtocolVersions.emplace(header.getId(), communicationProtocolVersion);
        }
        break;

        case MessageType_BoxInformation:
        {
            BoxDescriptionMessage boxDescription;

            if (!boxDescription.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_BoxDescriptionMessage);
                return false;
            }

            std::lock_guard<std::mutex> lock(impl->m_IncBoxDescriptionMutex);
            impl->m_IncomingBoxDescriptions.emplace(header.getId(), boxDescription);
        }
        break;

        case MessageType_EBML:
        {
            EBMLMessage ebml;

            if (!ebml.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_EBMLMessage);
                return false;
            }

            if (ebml.getIndex() >= impl->m_BoxDesc.getOutputs()->size()) { this->pushMessage(ErrorMessage(Error_InvalidOutputIndex, header.getId())); }

            std::lock_guard<std::mutex> lock(impl->m_IncEBMLMutex);
            impl->m_IncomingEBMLs.emplace(header.getId(), ebml);
        }
        break;

        case MessageType_Log:
        {
            LogMessage log;

            if (!log.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_LogMessage);
                return false;
            }

            std::lock_guard<std::mutex> lock(impl->m_IncLogMutex);
            impl->m_IncomingLogs.emplace(header.getId(), log);
        }
        break;

        case MessageType_End: { impl->m_IsEndMessageReceived = true; }
        break;

        case MessageType_Error:
        {
            ErrorMessage error;

            if (!error.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_ErrorMessage);
                return false;
            }

            std::lock_guard<std::mutex> lock(impl->m_IncErrorsMutex);
            impl->m_IncomingErrors.emplace(header.getId(), error);
        }
        break;

        case MessageType_Time:
        {
            TimeMessage timeMessage;

            if (!timeMessage.fromBytes(buffer, byteRead))
            {
                this->pushMessage(ErrorMessage(Error_BadMessage, header.getId()));
                this->setLastError(Deserialize_ErrorMessage);
                return false;
            }

            impl->m_Time = timeMessage.getTime();
        }
        break;

        case MessageType_Sync: { impl->m_WasSyncMessageReceived = true; }
        break;

        case MessageType_Unknown:
        case MessageType_Max:
            this->pushMessage(ErrorMessage(Error_InvalidMessageType, header.getId()));
            this->setLastError(Deserialize_MessageTypeNotSupported);
            return false;
    }

    return true;
}

bool CMessaging::isInErrorState() const { return impl->m_IsInErrorState.load(); }

bool CMessaging::isEndReceived() { return impl->m_IsEndMessageReceived; }

uint64_t CMessaging::getTime() { return impl->m_Time; }

bool CMessaging::pushMessage(const Message& message) const
{
    if (this->isInErrorState()) { return false; }

    std::vector<uint8_t> messageBuffer = message.toBytes();
    const Header header(message.getMessageType(), impl->m_nMessage++, messageBuffer.size());
    std::vector<uint8_t> headerBuffer = header.toBytes();

    std::lock_guard<std::mutex> lock(impl->m_SendBufferMutex);
    impl->m_SendBuffer.insert(impl->m_SendBuffer.end(), headerBuffer.begin(), headerBuffer.end());
    impl->m_SendBuffer.insert(impl->m_SendBuffer.end(), messageBuffer.begin(), messageBuffer.end());

    return true;
}

std::string CMessaging::getErrorString(const ELibraryError error) { return ERRORS_STRING.at(error); }

bool CMessaging::isConnected() const
{
    if (impl->m_Connection == nullptr) { return false; }

    return impl->m_Connection->isConnected();
}

void CMessaging::sync() const
{
    while (true)
    {
        if (!this->push())
        {
            impl->m_IsInErrorState = true;
            break;
        }

        if (!this->pull())
        {
            impl->m_IsInErrorState = true;
            break;
        }

        if (!this->processIncomingMessages())
        {
            impl->m_IsInErrorState = true;
            break;
        }


        if (impl->m_IsStopRequested)
        {
            // Used to be sure to send the end message
            this->pull();
            break;
        }
    }
}

void CMessaging::setConnection(Socket::IConnection* connection) const { impl->m_Connection = connection; }

bool CMessaging::startSyncing()
{
    impl->m_IsEndMessageReceived = false;
    impl->m_SyncThread           = std::thread(&CMessaging::sync, this);
    impl->m_IsStopRequested      = false;
    return true;
}

bool CMessaging::stopSyncing() const
{
    impl->m_IsStopRequested = true;

    if (impl->m_SyncThread.joinable()) { impl->m_SyncThread.join(); }

    return true;
}

bool CMessaging::popAuthentication(uint64_t& id, std::string& connectionID)
{
    std::lock_guard<std::mutex> lock(impl->m_IncAuthMutex);

    if (impl->m_IncomingAuthentications.empty()) { return false; }

    id           = impl->m_IncomingAuthentications.front().first;
    connectionID = impl->m_IncomingAuthentications.front().second.getConnectionID();
    impl->m_IncomingAuthentications.pop();
    return true;
}

bool CMessaging::popBoxDescriptions(uint64_t& id, BoxDescriptionMessage& boxDescription)
{
    std::lock_guard<std::mutex> lock(impl->m_IncBoxDescriptionMutex);

    if (impl->m_IncomingBoxDescriptions.empty()) { return false; }

    id             = impl->m_IncomingBoxDescriptions.front().first;
    boxDescription = impl->m_IncomingBoxDescriptions.front().second;
    impl->m_IncomingBoxDescriptions.pop();

    return true;
}

bool CMessaging::popCommunicationProtocolVersion(uint64_t& id, uint8_t& majorVersion, uint8_t& minorVersion)
{
    std::lock_guard<std::mutex> lock(impl->m_IncCommProVerMutex);

    if (impl->m_IncomingCommunicationProtocolVersions.empty()) { return false; }

    id           = impl->m_IncomingCommunicationProtocolVersions.front().first;
    majorVersion = impl->m_IncomingCommunicationProtocolVersions.front().second.getMajorVersion();
    minorVersion = impl->m_IncomingCommunicationProtocolVersions.front().second.getMinorVersion();
    impl->m_IncomingEBMLs.pop();
    return true;
}

bool CMessaging::popLog(uint64_t& id, ELogLevel& type, std::string& message)
{
    std::lock_guard<std::mutex> lock(impl->m_IncLogMutex);

    if (impl->m_IncomingLogs.empty()) { return false; }

    id      = impl->m_IncomingLogs.front().first;
    type    = impl->m_IncomingLogs.front().second.getType();
    message = impl->m_IncomingLogs.front().second.getMessage();
    impl->m_IncomingLogs.pop();
    return true;
}

bool CMessaging::popError(uint64_t& id, EError& type, uint64_t& guiltyId)
{
    std::lock_guard<std::mutex> lock(impl->m_IncErrorsMutex);

    if (impl->m_IncomingErrors.empty()) { return false; }

    id       = impl->m_IncomingErrors.front().first;
    type     = impl->m_IncomingErrors.front().second.getType();
    guiltyId = impl->m_IncomingErrors.front().second.getGuiltyId();

    impl->m_IncomingErrors.pop();
    return true;
}

bool CMessaging::popEBML(uint64_t& id, size_t& index, uint64_t& startTime, uint64_t& endTime, std::shared_ptr<const std::vector<uint8_t>>& ebml)
{
    std::lock_guard<std::mutex> lock(impl->m_IncEBMLMutex);

    if (impl->m_IncomingEBMLs.empty()) { return false; }

    id        = impl->m_IncomingEBMLs.front().first;
    index     = impl->m_IncomingEBMLs.front().second.getIndex();
    startTime = impl->m_IncomingEBMLs.front().second.getStartTime();
    endTime   = impl->m_IncomingEBMLs.front().second.getEndTime();
    ebml      = impl->m_IncomingEBMLs.front().second.getEBML();
    impl->m_IncomingEBMLs.pop();
    return true;
}

bool CMessaging::popEnd(uint64_t& id)
{
    std::lock_guard<std::mutex> lock(impl->m_IncEndMutex);

    if (impl->m_IncomingEnds.empty()) { return false; }

    id = impl->m_IncomingEnds.front().first;
    impl->m_IncomingEnds.pop();
    return true;
}

void CMessaging::setConnectionID(const std::string& connectionID) const { impl->m_ConnectionID = connectionID; }

bool CMessaging::waitForSyncMessage()
{
    if (impl->m_WasSyncMessageReceived)
    {
        impl->m_WasSyncMessageReceived = false;
        return true;
    }
    return false;
}
}  // namespace Communication
