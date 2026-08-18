#pragma once

#include "defines.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <limits>

namespace Communication {
/**
 * \brief Log level used by the client to log information in the server side
 */
enum ELogLevel : uint8_t
{
    LogLevel_Info = 1,
    LogLevel_Warning = 2,
    LogLevel_Error = 3,
    LogLevel_Fatal = 4,
    LogLevel_Max = 5,
    LogLevel_Unknown = 0xFF
};

/**
 * \brief Values that represent errors.
 */
enum EError : uint16_t
{
    Error_AuthenticationFail = 1,
    Error_AuthenticationRequested = 2,
    Error_InvalidOutputIndex = 3,
    Error_BadCommunicationProtocol = 4,
    Error_InvalidEBML = 5,
    Error_InvalidLogLevel = 6,
    Error_InvalidMessageType = 7,
    Error_BadMessage = 8,
    Error_Max = 9,
    Error_Unknown = 0xFF
};

/**
 * \brief Message type
 */
enum EMessageType : uint8_t
{
    MessageType_Authentication = 0,
    MessageType_ProtocolVersion = 1,
    MessageType_BoxInformation = 2,
    MessageType_EBML = 3,
    MessageType_Log = 4,
    MessageType_End = 5,
    MessageType_Error = 6,
    MessageType_Time = 7,
    MessageType_Sync = 8,
    MessageType_Max = 9,

    MessageType_Unknown = 0xFF,
};

/**
 * \brief A packet part is a part of a packet compound by an Header and a Message.
 */
class Communication_API CPacketPart
{
public:

    virtual ~CPacketPart() {}

    /**
     * \brief Provide array of bytes that represent the object.
     *
     * \return A vector with the serialized information of the message that respect the communication protocol.
     *
     * \sa fromBytes
     */
    virtual std::vector<uint8_t> toBytes() const = 0;

    /**
     * \brief Transform bytes vector into an object.
     *
     * \param           buffer      The buffer.
     * \param [in,out]  bufferIndex Zero-based index of the buffer.
     *
     * \retval True if it succeeds
     * \retval False if it fails.
     *
     * \sa toBytes
     */
    virtual bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) = 0;

    /**
     * \brief Query if this object is valid.
     *
     * \retval True if it is valid.
     * \retval False if it is invalid.
     */
    bool isValid() const { return m_isValid; }

protected:
    bool m_isValid = false;
};

/**
 * \brief A message is the second part of a packet after the header.
 */
class Message : public CPacketPart
{
public:
    virtual EMessageType getMessageType() const = 0;
};

/**
 * \brief A header is associated to a message. It give information about the message, like the type and the size.
 */
class Header final : CPacketPart
{
public:
    Header();
    Header(EMessageType type, uint64_t id, size_t size);
    std::vector<uint8_t> toBytes() const override;
    void setId(const uint64_t id) { m_id = id; }
    uint64_t getId() const { return m_id; }
    EMessageType getType() const { return m_type; }
    size_t getSize() const { return m_size; }
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;

private:
    EMessageType m_type;
    uint64_t m_id = 0;
    size_t m_size = 0;

    static const size_t TYPE_SIZE    = sizeof(EMessageType);
    static const size_t ID_SIZE      = sizeof(uint64_t);
    static const size_t SIZE_SIZE    = sizeof(size_t);
    static const size_t TYPE_INDEX   = 0;
    static const size_t ID_INDEX     = TYPE_INDEX + TYPE_SIZE;
    static const size_t SIZE_INDEX   = ID_INDEX + ID_SIZE;
    static const size_t MINIMUM_SIZE = TYPE_SIZE + ID_SIZE + SIZE_SIZE;
};

/**
 * \brief Represent an Authentication message.
 */
class AuthenticationMessage final : public Message
{
public:
    AuthenticationMessage() { m_isValid = false; }
    AuthenticationMessage(const std::string& connectionID) : m_connectionID(connectionID) { m_isValid = true; }
    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;
    EMessageType getMessageType() const override { return MessageType_Authentication; }
    std::string getConnectionID() const { return m_connectionID; }

private:
    static const size_t SIZE_SIZE           = sizeof(size_t);
    static const size_t SIZE_INDEX          = 0;
    static const size_t CONNECTION_ID_INDEX = SIZE_INDEX + SIZE_SIZE;
    static const size_t MINIMUM_SIZE        = SIZE_SIZE;

    std::string m_connectionID;
};

/**
 * \brief This message is used to inform the server or the client about the current communication protocol version used.
 */
class CommunicationProtocolVersionMessage final : public Message
{
public:

    CommunicationProtocolVersionMessage() { m_isValid = false; }

    CommunicationProtocolVersionMessage(const uint8_t majorVersion, const uint8_t minorVersion) : m_minorVersion(minorVersion), m_majorVersion(majorVersion)
    {
        m_isValid = true;
    }

    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;
    EMessageType getMessageType() const override { return MessageType_ProtocolVersion; }
    uint8_t getMajorVersion() const { return m_majorVersion; }
    uint8_t getMinorVersion() const { return m_minorVersion; }

private:
    uint8_t m_minorVersion = 0;
    uint8_t m_majorVersion = 0;

    static const size_t MAJOR_SIZE   = sizeof(uint8_t);
    static const size_t MINOR_SIZE   = sizeof(uint8_t);
    static const size_t MAJOR_INDEX  = 0;
    static const size_t MINOR_INDEX  = MAJOR_INDEX + MAJOR_SIZE;
    static const size_t MINIMUM_SIZE = MAJOR_SIZE + MINOR_SIZE;
};

/**
 * \brief InputOutput class describes the input or output of a box.
 */
class InputOutput final : public CPacketPart
{
public:
    InputOutput();
    InputOutput(uint64_t id, size_t type, const std::string& name);
    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;
    uint64_t getId() const { return m_id; }
    size_t getType() const { return m_type; }
    std::string getName() const { return m_name; }

private:
    uint64_t m_id = 0;
    size_t m_type = 0;
    std::string m_name;

    static const size_t ID_SIZE         = sizeof(uint64_t);
    static const size_t TYPE_SIZE       = sizeof(size_t);
    static const size_t NAME_SIZE_SIZE  = sizeof(size_t);
    static const size_t ID_INDEX        = 0;
    static const size_t TYPE_INDEX      = ID_INDEX + ID_SIZE;
    static const size_t NAME_SIZE_INDEX = TYPE_INDEX + TYPE_SIZE;
    static const size_t NAME_INDEX      = NAME_SIZE_INDEX + NAME_SIZE_SIZE;
    static const size_t MINIMUM_SIZE    = ID_SIZE + TYPE_SIZE + NAME_SIZE_SIZE;
};

class Parameter final : public CPacketPart
{
public:
    Parameter();
    Parameter(uint64_t id, size_t type, const std::string& name, const std::string& value);
    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;

    uint64_t getId() const { return m_id; }
    size_t getType() const { return m_type; }
    std::string getName() const { return m_name; }
    std::string getValue() const { return m_value; }

private:
    uint64_t m_id = 0;
    size_t m_type = 0;
    std::string m_name;
    std::string m_value;

    static const size_t ID_SIZE          = sizeof(uint64_t);
    static const size_t TYPE_SIZE        = sizeof(size_t);
    static const size_t NAME_SIZE_SIZE   = sizeof(size_t);
    static const size_t VALUE_SIZE_SIZE  = sizeof(size_t);
    static const size_t ID_INDEX         = 0;
    static const size_t TYPE_INDEX       = ID_INDEX + ID_SIZE;
    static const size_t NAME_SIZE_INDEX  = TYPE_INDEX + TYPE_SIZE;
    static const size_t VALUE_SIZE_INDEX = NAME_SIZE_INDEX + NAME_SIZE_SIZE;
    static const size_t NAME_INDEX       = VALUE_SIZE_INDEX + VALUE_SIZE_SIZE;
    static const size_t MINIMUM_SIZE     = ID_SIZE + TYPE_SIZE + NAME_SIZE_SIZE + VALUE_SIZE_SIZE;
};

/**
 * \brief This message contains information about the box used in the NeuroRT pipeline.
 *  This message is sent by the server and received by the client.
 */
class BoxDescriptionMessage final : public Message
{
public:
    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;
    EMessageType getMessageType() const override;
    bool addInput(uint64_t id, size_t type, const std::string& name);
    bool addOutput(uint64_t id, size_t type, const std::string& name);
    bool addParameter(uint64_t id, size_t type, const std::string& name, const std::string& value);

    const std::vector<InputOutput>* getInputs() const { return &m_inputs; }
    const std::vector<InputOutput>* getOutputs() const { return &m_outputs; }
    const std::vector<Parameter>* getParameters() const { return &m_parameters; }

private:
    std::vector<InputOutput> m_inputs;
    std::vector<InputOutput> m_outputs;
    std::vector<Parameter> m_parameters;

    static const size_t N_INPUT_SIZE      = sizeof(size_t);
    static const size_t N_OUTPUT_SIZE     = sizeof(size_t);
    static const size_t N_PARAMETER_SIZE  = sizeof(size_t);
    static const size_t N_INPUT_INDEX     = 0;
    static const size_t N_OUTPUT_INDEX    = N_INPUT_INDEX + N_INPUT_SIZE;
    static const size_t N_PARAMETER_INDEX = N_OUTPUT_INDEX + N_OUTPUT_SIZE;
    static const size_t MINIMUM_SIZE      = N_INPUT_SIZE + N_OUTPUT_SIZE + N_PARAMETER_SIZE;
};

/**
 * \brief Log message is a way to communicate information from the client to the server.
 */
class LogMessage final : public Message
{
public:
    LogMessage();
    LogMessage(ELogLevel type, const std::string& message);
    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;

    EMessageType getMessageType() const override { return MessageType_Log; }
    ELogLevel getType() const { return m_type; }
    std::string getMessage() const { return m_message; }

private:
    ELogLevel m_type;
    std::string m_message;

    static const size_t TYPE_SIZE    = sizeof(ELogLevel);
    static const size_t SIZE_SIZE    = sizeof(size_t);
    static const size_t TYPE_INDEX   = 0;
    static const size_t SIZE_INDEX   = TYPE_INDEX + TYPE_SIZE;
    static const size_t NAME_INDEX   = SIZE_INDEX + SIZE_SIZE;
    static const size_t MINIMUM_SIZE = TYPE_SIZE + SIZE_SIZE;
};

/**
 * \brief EBML message is used to send EBML data.
 */
class EBMLMessage final : public Message
{
public:
    EBMLMessage();

    EBMLMessage(size_t index, uint64_t startTime, uint64_t endTime, const std::shared_ptr<const std::vector<uint8_t>>& ebml);

    std::vector<uint8_t> toBytes() const override;

    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;

    EMessageType getMessageType() const override { return MessageType_EBML; }
    size_t getIndex() const { return m_ioIdx; }
    uint64_t getStartTime() const { return m_startTime; }
    uint64_t getEndTime() const { return m_endTime; }
    std::shared_ptr<const std::vector<uint8_t>> getEBML() const { return m_EBML; }

private:
    size_t m_ioIdx       = 0;
    uint64_t m_startTime = 0;
    uint64_t m_endTime   = 0;

    std::shared_ptr<const std::vector<uint8_t>> m_EBML;

    static const size_t IO_INDEX_SIZE    = sizeof(size_t);
    static const size_t START_TIME_SIZE  = sizeof(uint64_t);
    static const size_t END_TIME_SIZE    = sizeof(uint64_t);
    static const size_t SIZE_SIZE        = sizeof(size_t);
    static const size_t IO_INDEX_INDEX   = 0;
    static const size_t START_TIME_INDEX = IO_INDEX_INDEX + IO_INDEX_SIZE;
    static const size_t END_TIME_INDEX   = START_TIME_INDEX + START_TIME_SIZE;
    static const size_t SIZE_INDEX       = END_TIME_INDEX + END_TIME_SIZE;
    static const size_t EBML_INDEX       = SIZE_INDEX + SIZE_SIZE;
    static const size_t MINIMUM_SIZE     = IO_INDEX_SIZE + START_TIME_SIZE + END_TIME_SIZE + SIZE_SIZE;
};

/**
 * \brief Error message is used to alert the client that the server raise an error due to a message by the client.
 */
class ErrorMessage final : public Message
{
public:
    ErrorMessage() : m_type(Error_Unknown), m_guiltyId(std::numeric_limits<decltype(m_guiltyId)>::max()) {}
    ErrorMessage(const EError error, const uint64_t guiltyId) : m_type(error), m_guiltyId(guiltyId) {}

    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;

    EMessageType getMessageType() const override { return MessageType_Error; }
    EError getType() const { return m_type; }
    uint64_t getGuiltyId() const { return m_guiltyId; }

private:
    EError m_type;
    uint64_t m_guiltyId = 0;

    static const size_t TYPE_SIZE       = sizeof(EError);
    static const size_t GUILTY_ID_SIZE  = sizeof(uint64_t);
    static const size_t TYPE_INDEX      = 0;
    static const size_t GUILTY_ID_INDEX = TYPE_INDEX + TYPE_SIZE;
    static const size_t MINIMUM_SIZE    = GUILTY_ID_SIZE + TYPE_SIZE;
};

/**
 * \brief End message can be sent by the server or/and by the client to inform that the processing is stopped and the connection will be closed.
 */
class EndMessage final : public Message
{
public:
    EndMessage() {}
    std::vector<uint8_t> toBytes() const override { return std::vector<uint8_t>(); }
    bool fromBytes(const std::vector<uint8_t>& /*buffer*/, size_t& /*index*/) override { return false; }
    EMessageType getMessageType() const override { return MessageType_End; }
};

/**
 * \brief Time messages are only sent by the server to inform the client about the current time of the pipeline.
 */
class TimeMessage final : public Message
{
public:
    TimeMessage(const uint64_t time = 0) : m_time(time) {}
    std::vector<uint8_t> toBytes() const override;
    bool fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex) override;

    EMessageType getMessageType() const override { return MessageType_Time; }
    uint64_t getTime() const { return m_time; }

private:
    uint64_t m_time = 0;

    static const size_t TIME_SIZE    = sizeof(uint64_t);
    static const size_t TIME_INDEX   = 0;
    static const size_t MINIMUM_SIZE = TIME_SIZE;
};

/**
 * \brief Sync message can be sent by the server or/and by the client to inform that it's waiting the other part.
 */
class SyncMessage final : public Message
{
public:
    SyncMessage() {}
    std::vector<uint8_t> toBytes() const override { return std::vector<uint8_t>(); }
    bool fromBytes(const std::vector<uint8_t>& /*buffer*/, size_t& /*index*/) override { return false; }
    EMessageType getMessageType() const override { return MessageType_Sync; }
};
}  // namespace Communication 
