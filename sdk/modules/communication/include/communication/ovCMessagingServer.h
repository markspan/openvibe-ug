#pragma once

#include "defines.h"
#include "ovCMessaging.h"

#include "socket/IConnectionServer.h"

namespace Communication {
class Communication_API MessagingServer : public CMessaging
{
public:

    /**
     * \brief   Default constructor.
     */
    MessagingServer() : CMessaging(), m_Server(Socket::createConnectionServer()) {}

    /**
     * \brief   Destructor.
     */
    ~MessagingServer() override;

    /**
     * \brief Start listening on the given port.
     *
     * \param port The port.
     *
     * \retval True if listenning
     * \retval False if not.
     *
     * \sa close
     */
    bool listen(const size_t port) const { return m_Server->listen(port); }

    /**
     * \brief Close the connection
     *                   *
     * \retval True if it succeeds
     * \retval False if it fails.
     */
    bool close();

    /**
     * \brief Accepts one incoming connection.
     *
     * \retval True if it succeeds.
     * \retval False if it fails.
     */
    bool accept();

    /**
    * \brief Returns the port on the one the server is listening.
    * This is useful if you set the port to '0'.
    * \param port [out]: port on the one the server is listening
    */
    bool getSocketPort(size_t& port) const { return m_Server->getSocketPort(port); }


    /**
     * \brief Adds a parameter in the Box Information.
     *
     * \param   id      The parameter's identifier.
     * \param   type    The parameter's type. Use OpenViBE::CIdentifier.
     * \param   name    The parameter's name.
     * \param   value   The parameter's value. A string.
     *
     * \retval True if it succeeds.
     * \retval False if an output with the given id already exists.
     *
     * \sa addInput
     * \sa addOutput
     */
    bool addParameter(uint64_t id, size_t type, const std::string& name, const std::string& value) const;

    /**
     * \brief Adds an input in the box Information.
     *
     * \param   id      The input's identifier.
     * \param   type    The input's type. Use OpenViBE::CIdentifier.
     * \param   name    The input's name.
     *
     * \retval True if it succeeds.
     * \retval False if an output with the given id already exists.
     *
     * \sa addParameter
     * \sa addOutput
     */
    bool addInput(uint64_t id, size_t type, const std::string& name) const;

    /**
     * \brief Adds an output in the box information.
     *
     * \param   id      The output's identifier.
     * \param   type    The output's type. Use OpenViBE::CIdentifier.
     * \param   name    The output's name.
     *
     * \retval True if it succeeds.
     * \retval False if an output with the given id already exists.
     *
     * \sa addParameter
     * \sa addInput
     */
    bool addOutput(uint64_t id, size_t type, const std::string& name) const;

    /**
     * \brief Get the log message received from the client.
     *
     * \param[out]  packetId    Packet id
     * \param[out]  type        Level of the log
     * \param[out]  message     Log message
     *
     * \retval True if it succeeds.
     * \retval False if the library is in error state.
     */
    bool popLog(uint64_t& packetId, ELogLevel& type, std::string& message) override;

    /**
     * \brief Get the EBML data received from the client.
     *
     * \param[out]  packetId    Packet id
     * \param[out]  index       Output index
     * \param[out]  startTime   Chunk time in OpenViBE 32:32 format
     * \param[out]  endTime     Chunk time in OpenViBE 32:32 format
     * \param[out]  ebml        EBML data
     *
     * \retval True if it succeeds.
     * \retval False if the library is in error state.
     */
    bool popEBML(uint64_t& packetId, size_t& index, uint64_t& startTime, uint64_t& endTime, std::shared_ptr<const std::vector<uint8_t>>& ebml) override;

    /**
     * \brief Push Error message to the client
     *
     * \param   error       Error code
     * \param   guiltyId    Id of the received message that raised the error.
     *
     * \retval True if it succeeds.
     * \retval False if the library is in error state.
     */
    bool pushError(EError error, uint64_t guiltyId) const;

    /**
     * \brief Push EBML message to the client
     *
     * \param   index       Index of the input.
     * \param   startTime   Chunk time in OpenViBE 32:32 format
     * \param   endTime     Chunk time in OpenViBE 32:32 format
     * \param   ebml        EBML data
     *
     * \retval True if it succeeds.
     * \retval False if the library is in error state.
     */
    bool pushEBML(size_t index, uint64_t startTime, uint64_t endTime, const std::shared_ptr<const std::vector<uint8_t>>& ebml) const;

    /**
     * \brief Push Time message to the client
     *
     * \param   time    Time in OpenViBE 32:32 format
     *
     * \retval True if it succeeds.
     * \retval False if the library is in error state.
     */
    bool pushTime(uint64_t time) { return this->pushMessage(TimeMessage(time)); }

    /**
     * \brief Push Sync message to the client
     *
     * \retval True if it succeeds.
 * \retval False if the library is in error state.
     */
    bool pushSync() { return this->pushMessage(SyncMessage()); }

    /**
     * \brief Check if a sync message is received.
     *
     * \retval True if a sync message is received.
     * \retval False if no sync message was received.
     */
    bool waitForSyncMessage() override { return CMessaging::waitForSyncMessage(); }

private:
    Socket::IConnectionServer* m_Server = nullptr; ///< Server connection
    Socket::IConnection* m_Client       = nullptr; ///< Client connection
};
}  // namespace Communication
