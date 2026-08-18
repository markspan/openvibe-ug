#pragma once

#include <socket/IConnection.h>

#include "defines.h"
#include "ovCMessagingProtocol.h"

namespace Communication {
/**
 * \brief The purpose of this class is to provide a communication protocol to exchange EBML between a client and a server.
 */
class Communication_API CMessaging
{
public:

    /**
     * \brief Library error codes
     */
    enum ELibraryError
    {
        NoError = 0,
        Socket_NotConnected = 1,
        Socket_FailedToConnect = 2,
        Socket_ReceiveBufferFail = 3,
        Socket_SendBufferFail = 4,
        Socket_NoIncomingClientConnection = 6,
        Socket_NotReadyToSend = 7,
        Socket_NoDataReceived = 8,
        Socket_FailedCloseClientConnection = 10,
        Socket_FailedToCloseConnection = 11,
        Socket_FailedConnectClient = 12,
        Socket_ClientAlreadyConnected = 13,
        Deserialize_BufferTooSmall = 30,
        Deserialize_Header = 31,
        Deserialize_ProtocolVersionMessage = 32,
        Deserialize_BoxDescriptionMessage = 33,
        Deserialize_EBMLMessage = 34,
        Deserialize_EndMessage = 35,
        Deserialize_ErrorMessage = 36,
        Deserialize_LogMessage = 37,
        Deserialize_AuthenticationMessage = 38,
        Deserialize_MessageTypeNotSupported = 39,
        BoxDescriptionAlreadyReceived = 60,
        BoxDescriptionNotReceived = 61,
        BadAuthenticationReceived = 70,
        NoAuthenticationReceived = 71,
        ThreadJoinFailed = 80
    };

    CMessaging();
    virtual ~CMessaging();

    /**
     * \brief Get the code of the last error produced by the API
     *
     * \retval Error code
     */
    ELibraryError getLastError() const;

    /**
     * \brief Give a short description of an error.
     *
     * \param error the error code
     *
     * \return Description of the error.
     */
    static std::string getErrorString(ELibraryError error);

    /**
     * \brief Check that the socket is connected.
     *
     * \retval True if the socket is connected.
     * \retval False if the socket is not connected.
     */
    bool isConnected() const;

    /**
     * \brief Check that the synchronization is in error state.
     *
     * \retval True if the synchronization is in error state.
     * \retval False if the synchronization is ok.
     */
    bool isInErrorState() const;

    /**
     * \brief Set the connection ID to a new value
     * \param connectionID The connection Id to set
     */
    void setConnectionID(const std::string& connectionID) const;

    /**
     * \brief Check that a End message was received.
     *
     * \retval True if the end message from the client is received.
     * \retval False if the end message from the client is not received.
     */
    virtual bool isEndReceived();

    /**
     * \brief Get the time.
     */
    virtual uint64_t getTime();

protected:

    /**
     * \brief Push a message to the send buffer.
     * The message will be really sent in the socket in the next synchronization.
     *
     * \param message The message to send.
     *
     * \retval True if it succeeds.
     * \retval False if library is in error state.
     */
    bool pushMessage(const Message& message) const;

    /**
     * \brief Set the last error code.
     *
     * \param libraryError The error
     *
     * \sa getLastError
     */
    void setLastError(ELibraryError libraryError) const;

    /**
     * \brief Provide the connection to the base class to communicate.
     *
     * \param connection The connection
     */
    void setConnection(Socket::IConnection* connection) const;

    /**
     * \brief Start a thread that will push the outgoing data, pull and process the incoming data.
     * This sync will be stopped in cases:
     * - An error raised
     * - stopSyncing() function was called.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     *
     * \sa stopSyncing
     */
    bool startSyncing();

    /**
     * \brief Request to stop the sync and stop the thread.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     *
     * \sa startSyncing
     */
    bool stopSyncing() const;

    /**
     * @brief Get the oldest authentication message
     * @param id [out] Identifier of the message
     * @param connectionID [out] Connection Id
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popAuthentication(uint64_t& id, std::string& connectionID);

    /**
     * @brief Get the oldest box description message
     * @param id [out] Identifier of the message
     * @param boxDescription [out] Descriptor of the box
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popBoxDescriptions(uint64_t& id, BoxDescriptionMessage& boxDescription);

    /**
     * @brief popCommunicationProtocolVersion
     * @param id [out] Identifier of the message
     * @param majorVersion [out] major version of the protocol
     * @param minorVersion [out] minor version of the protocol
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popCommunicationProtocolVersion(uint64_t& id, uint8_t& majorVersion, uint8_t& minorVersion);

    /**
     * @brief Pop the oldest EBML message from the queue
     * @param id [out] Identifier of the message
     * @param index [out] Input index to which the EBML should be directed
     * @param startTime [out] Start time of the buffer
     * @param endTime [out] End time of the buffer
     * @param ebml [out] The encoded EBML buffer
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popEBML(uint64_t& id, size_t& index, uint64_t& startTime, uint64_t& endTime, std::shared_ptr<const std::vector<uint8_t>>& ebml);

    /**
     * @brief Pop the oldest log message from the queue
     * @param id [out] Identifier of the message
     * @param type [out] Log level of the message
     * @param message [out] Message text
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popLog(uint64_t& id, ELogLevel& type, std::string& message);

    /**
     * @brief Pop the oldest error message from the queue
     * @param id [out] Identifier of the message
     * @param type [out] Error code
     * @param guiltyId [out] If of the sent message that caused this error
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popError(uint64_t& id, EError& type, uint64_t& guiltyId);

    /**
     * @brief Get the oldes End message
     * @param id [out] Identifier of the message
     * @return true if a message was popped, false if the queue is empty
     */
    virtual bool popEnd(uint64_t& id);

    /**
     * @brief Reset the library state. Stop sending and receiving buffers, disconnect
     * and empty all buffers.
     */
    void reset() const;

    /**
     * \brief Checks if a sync message was received and reset it if it was.
     *
     * This method should be used in a busy loop to check if the other
     * party has finished processing all of the data.
     *
     * From the external program standpoint this method will return true
     * when the box has finished sending all of the data that has to be
     * processed in one bulk.
     *
     * From the box standpoint, this means that the external program has
     * finished processing and sending all of the data that the box has
     * sent.
     *
     * \retval True if a sync message is received.
     * \retval False if no sync message was received.
     */
    virtual bool waitForSyncMessage();

private:

    /**
     * \brief Receive all the available data from the socket and insert it in the rcv buffer.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     * Library errors: TODO
     *
     * \sa push
     */
    bool pull() const;

    /**
     * \brief Send all the data to the socket.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     * Library errors:
     * - Socket_NotConnected
     *
     *
     * \sa push
     */
    bool push() const;

    /**
     * \brief Process incoming data, unpack it and put messages in queues.
     * It use processBuffer.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     *  Library errors: Error from processBuffer
     *
     * \sa processBuffer
     */
    bool processIncomingMessages() const;

    /**
     * \brief Process buffer, pack it and put messages in queues.
     *
     * \param buffer Buffer with the incoming serialized data.
     * \param[out] byteRead Number of bytes read and processed.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     * Library errors:
     * - Deserialize_Header
     * - Deserialize_AuthenticationMessage
     * - Deserialize_ProtocolVersionMessage
     * - Deserialize_BoxDescriptionMessage
     * - Deserialize_EBMLMessage
     * - Deserialize_LogMessage
     * - Deserialize_ErrorMessage
     * - Deserialize_MessageTypeNotSupported
     *
     * \sa processIncomingMessages
     */
    bool processBuffer(const std::vector<uint8_t>& buffer, size_t& byteRead) const;

    /**
     * \brief Sync fucntion that is used in a thread to pull, push and process the incoming data.
     *
     * \retval True if it succeeds.
     * \retval False if an error occured.
     *  Library errors: Errors from processIncomingMessages(), push() or pull()
     *
     * \sa pull
     * \sa push
     * \sa processIncomingMessages
     */
    void sync() const;

public:
    static const uint8_t s_CommunicationProtocol_MajorVersion = 1;
    static const uint8_t s_CommunicationProtocol_MinorVersion = 1;

protected:
    struct SMessagingImpl;
    SMessagingImpl* impl = nullptr;
};
}  // namespace Communication
