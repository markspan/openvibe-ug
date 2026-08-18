#include <array>
#include <string>
#include <algorithm>
#include <cstring>

#include "ovCMessaging.h"

namespace Communication {

/**
 * \brief Copy a string to buffer
 *
 * \param [out] dest        The buffer destination
 * \param [out] bufferIndex The index of the buffer where the beginning of the string must be copied.
 * \param       value       The string to copy.
 *
 * \retval True if it succeeds
 * \retval False if it fails.
 */
static bool copyTobuffer(std::vector<uint8_t>& dest, size_t& bufferIndex, const std::string& value)
{
    if (dest.size() < bufferIndex + value.size()) { return false; }
    memcpy(dest.data() + bufferIndex, value.data(), value.size());
    bufferIndex += value.size();
    return true;
}

/**
 * \brief Copy a value to a buffer.
 *
 * \param [out] dest        Destination for the.
 * \param [out] bufferIndex Zero-based index of the buffer.
 * \param       value       The value.
 *
 * \return  True if it succeeds, false if it fails.
 */
template <class T>
static bool copyTobuffer(std::vector<uint8_t>& dest, size_t& bufferIndex, const T& value)
{
    if (dest.size() < bufferIndex + sizeof(value)) { return false; }
    memcpy(dest.data() + bufferIndex, &value, sizeof(value));
    bufferIndex += sizeof(value);
    return true;
}

template <class T>
static bool copyToVariable(const std::vector<uint8_t>& src, const size_t bufferIndex, T& destVariable)
{
    if (src.size() < bufferIndex + sizeof(destVariable)) { return false; }
    memcpy(&destVariable, src.data() + bufferIndex, sizeof(destVariable));
    return true;
}

/**
 * \brief Convert a buffer to a string
 *
 * \param       src         The buffer
 * \param       bufferIndex The index where to start the convertion.
 * \param       size        The size of the string.
 * \param [out] string      The string.
 *
 * \retval True if it succeeds
 * \retval False if it fails.
 *
 * \sa copyToVariable
 */
static bool copyToString(const std::vector<uint8_t>& src, const size_t bufferIndex, const size_t size, std::string& string)
{
    if (src.size() < bufferIndex + size) { return false; }

    string = std::string(src.begin() + static_cast<const long>(bufferIndex),
                         src.begin() + static_cast<const long>(bufferIndex) + static_cast<const long>(size));

    return true;
}

/******************************************************************************
*
* Header
*
******************************************************************************/

Header::Header() : m_type(MessageType_Unknown), m_id(std::numeric_limits<decltype(m_id)>::max()) { m_isValid = false; }
Header::Header(const EMessageType type, const uint64_t id, const size_t size) : m_type(type), m_id(id), m_size(size) { m_isValid = true; }

std::vector<uint8_t> Header::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE);
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_type);
    copyTobuffer(buffer, bufferIndex, m_id);
    copyTobuffer(buffer, bufferIndex, m_size);

    return buffer;
}

bool Header::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    uint8_t typeInt;

    if (!copyToVariable(buffer, bufferIndex + TYPE_INDEX, typeInt)) { return false; }

    if (typeInt >= MessageType_Max) { return false; }

    m_type = EMessageType(typeInt);

    if (!copyToVariable(buffer, bufferIndex + ID_INDEX, m_id)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + SIZE_INDEX, m_size)) { return false; }

    bufferIndex += MINIMUM_SIZE;

    m_isValid = true;

    return true;
}

/******************************************************************************
*
* Authentication
*
******************************************************************************/

std::vector<uint8_t> AuthenticationMessage::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE + m_connectionID.size());
    size_t bufferIndex = 0;

    const size_t size = m_connectionID.size();
    copyTobuffer(buffer, bufferIndex, size);
    copyTobuffer(buffer, bufferIndex, m_connectionID);

    return buffer;
}

bool AuthenticationMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;
    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    size_t passPhraseSize;
    if (!copyToVariable(buffer, bufferIndex + SIZE_INDEX, passPhraseSize)) { return false; }
    if (!copyToString(buffer, bufferIndex + CONNECTION_ID_INDEX, passPhraseSize, m_connectionID)) { return false; }

    m_isValid = true;
    bufferIndex += MINIMUM_SIZE + passPhraseSize;

    return true;
}


/******************************************************************************
*
* Communication protocol version
*
******************************************************************************/

std::vector<uint8_t> CommunicationProtocolVersionMessage::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE);
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_majorVersion);
    copyTobuffer(buffer, bufferIndex, m_minorVersion);

    return buffer;
}

bool CommunicationProtocolVersionMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    if (!copyToVariable(buffer, bufferIndex + MAJOR_INDEX, m_majorVersion)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + MINOR_INDEX, m_minorVersion)) { return false; }

    m_isValid = true;

    bufferIndex += MINIMUM_SIZE;

    return true;
}

/******************************************************************************
*
* Inout and output
*
******************************************************************************/

InputOutput::InputOutput()
    : m_id(std::numeric_limits<decltype(m_id)>::max()), m_type(std::numeric_limits<decltype(m_type)>::max()), m_name(std::string()) { m_isValid = false; }

InputOutput::InputOutput(const uint64_t id, const size_t type, const std::string& name) : m_id(id), m_type(type), m_name(name) { m_isValid = true; }

std::vector<uint8_t> InputOutput::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE + m_name.size());
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_id);
    copyTobuffer(buffer, bufferIndex, m_type);
    copyTobuffer(buffer, bufferIndex, m_name.size());
    copyTobuffer(buffer, bufferIndex, m_name);

    return buffer;
}

bool InputOutput::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    if (!copyToVariable(buffer, bufferIndex + ID_INDEX, m_id)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + TYPE_INDEX, m_type)) { return false; }

    size_t nameSize;
    if (!copyToVariable(buffer, bufferIndex + NAME_SIZE_INDEX, nameSize)) { return false; }
    if (!copyToString(buffer, bufferIndex + NAME_INDEX, nameSize, m_name)) { return false; }

    m_isValid = true;

    bufferIndex += MINIMUM_SIZE + nameSize;

    return true;
}

/******************************************************************************
*
* Parameter
*
******************************************************************************/

Parameter::Parameter()
    : m_id(std::numeric_limits<decltype(m_id)>::max()), m_type(std::numeric_limits<decltype(m_type)>::max()), m_name(std::string()), m_value(std::string())
{
    m_isValid = false;
}

Parameter::Parameter(const uint64_t id, const size_t type, const std::string& name, const std::string& value)
    : m_id(id), m_type(type), m_name(name), m_value(value) { m_isValid = true; }

std::vector<uint8_t> Parameter::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE + m_name.size() + m_value.size());
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_id);
    copyTobuffer(buffer, bufferIndex, m_type);
    copyTobuffer(buffer, bufferIndex, m_name.size());
    copyTobuffer(buffer, bufferIndex, m_value.size());
    copyTobuffer(buffer, bufferIndex, m_name);
    copyTobuffer(buffer, bufferIndex, m_value);

    return buffer;
}

bool Parameter::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    if (!copyToVariable(buffer, bufferIndex + ID_INDEX, m_id)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + TYPE_INDEX, m_type)) { return false; }

    size_t nameSize;
    if (!copyToVariable(buffer, bufferIndex + NAME_SIZE_INDEX, nameSize)) { return false; }

    size_t valueSize;
    if (!copyToVariable(buffer, bufferIndex + VALUE_SIZE_INDEX, valueSize)) { return false; }
    if (!copyToString(buffer, bufferIndex + NAME_INDEX, nameSize, m_name)) { return false; }
    if (!copyToString(buffer, bufferIndex + NAME_INDEX + nameSize, valueSize, m_value)) { return false; }

    bufferIndex += MINIMUM_SIZE + nameSize + valueSize;

    return true;
}

/******************************************************************************
 *
 * Box description
 *
 ******************************************************************************/

std::vector<uint8_t> BoxDescriptionMessage::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE);
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_inputs.size());
    copyTobuffer(buffer, bufferIndex, m_outputs.size());
    copyTobuffer(buffer, bufferIndex, m_parameters.size());

    for (const InputOutput& input : m_inputs)
    {
        std::vector<uint8_t> inputBuffer = input.toBytes();
        buffer.insert(buffer.end(), inputBuffer.begin(), inputBuffer.end());
    }

    for (const InputOutput& output : m_outputs)
    {
        std::vector<uint8_t> outputBuffer = output.toBytes();
        buffer.insert(buffer.end(), outputBuffer.begin(), outputBuffer.end());
    }

    for (const Parameter& parameter : m_parameters)
    {
        std::vector<uint8_t> parameterBuffer = parameter.toBytes();
        buffer.insert(buffer.end(), parameterBuffer.begin(), parameterBuffer.end());
    }

    return buffer;
}

bool BoxDescriptionMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    size_t inputCount;
    size_t outputCount;
    size_t parameterCount;

    if (!copyToVariable(buffer, bufferIndex + N_INPUT_INDEX, inputCount)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + N_OUTPUT_INDEX, outputCount)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + N_PARAMETER_INDEX, parameterCount)) { return false; }

    bufferIndex += MINIMUM_SIZE;

    m_inputs.clear();
    m_outputs.clear();
    m_parameters.clear();

    for (size_t i = 0; i < inputCount; ++i)
    {
        InputOutput input;
        if (!input.fromBytes(buffer, bufferIndex)) { return false; }
        m_inputs.push_back(input);
    }

    for (size_t i = 0; i < outputCount; ++i)
    {
        InputOutput output;
        if (!output.fromBytes(buffer, bufferIndex)) { return false; }
        m_outputs.push_back(output);
    }

    for (size_t i = 0; i < parameterCount; ++i)
    {
        Parameter parameter;
        if (!parameter.fromBytes(buffer, bufferIndex)) { return false; }
        m_parameters.push_back(parameter);
    }

    m_isValid = true;

    return true;
}

EMessageType BoxDescriptionMessage::getMessageType() const { return MessageType_BoxInformation; }

bool BoxDescriptionMessage::addInput(const uint64_t id, const size_t type, const std::string& name)
{
    const auto it = std::find_if(m_inputs.begin(), m_inputs.end(), [&id](const InputOutput& obj) { return obj.getId() == id; });
    if (it != m_inputs.end()) { return false; }
    m_inputs.emplace_back(id, type, name);
    return true;
}

bool BoxDescriptionMessage::addOutput(const uint64_t id, const size_t type, const std::string& name)
{
    const auto it = std::find_if(m_outputs.begin(), m_outputs.end(), [&id](const InputOutput& obj) { return obj.getId() == id; });
    if (it != m_outputs.end()) { return false; }
    m_outputs.emplace_back(id, type, name);
    return true;
}

bool BoxDescriptionMessage::addParameter(const uint64_t id, const size_t type, const std::string& name, const std::string& value)
{
    const auto it = std::find_if(m_parameters.begin(), m_parameters.end(), [&id](const Parameter& obj) { return obj.getId() == id; });
    if (it != m_parameters.end()) { return false; }
    m_parameters.emplace_back(id, type, name, value);
    return true;
}

/******************************************************************************
*
* Packet part
*
******************************************************************************/

/******************************************************************************
*
* Log
*
******************************************************************************/

LogMessage::LogMessage() : m_type(LogLevel_Unknown) { m_isValid = false; }
LogMessage::LogMessage(const ELogLevel type, const std::string& message) : m_type(type), m_message(message) { m_isValid = true; }

std::vector<uint8_t> LogMessage::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE + m_message.size());
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_type);
    copyTobuffer(buffer, bufferIndex, m_message.size());
    copyTobuffer(buffer, bufferIndex, m_message);

    return buffer;
}

bool LogMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    // Type
    uint8_t typeInt;

    if (!copyToVariable(buffer, bufferIndex + TYPE_INDEX, typeInt)) { return false; }

    if (typeInt >= LogLevel_Max) { return false; }

    m_type = ELogLevel(typeInt);

    // Message size
    size_t messageSize;
    if (!copyToVariable(buffer, bufferIndex + SIZE_INDEX, messageSize)) { return false; }
    if (!copyToString(buffer, bufferIndex + NAME_INDEX, messageSize, m_message)) { return false; }

    bufferIndex += MINIMUM_SIZE + messageSize;

    m_isValid = true;

    return true;
}

/******************************************************************************
*
* EBML
*
******************************************************************************/

EBMLMessage::EBMLMessage()
    : m_ioIdx(std::numeric_limits<decltype(m_ioIdx)>::max()), m_startTime(std::numeric_limits<decltype(m_startTime)>::max()),
      m_endTime(std::numeric_limits<decltype(m_endTime)>::max()) { m_isValid = false; }

EBMLMessage::EBMLMessage(const size_t index, const uint64_t startTime, const uint64_t endTime, const std::shared_ptr<const std::vector<uint8_t>>& ebml)
    : m_ioIdx(index), m_startTime(startTime), m_endTime(endTime), m_EBML(ebml) { m_isValid = true; }

std::vector<uint8_t> EBMLMessage::toBytes() const
{
    if (!m_isValid) { return std::vector<uint8_t>(); }

    std::vector<uint8_t> buffer(MINIMUM_SIZE);
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_ioIdx);
    copyTobuffer(buffer, bufferIndex, m_startTime);
    copyTobuffer(buffer, bufferIndex, m_endTime);
    copyTobuffer(buffer, bufferIndex, size_t(m_EBML->size()));

    if (!m_EBML->empty()) { buffer.insert(buffer.end(), m_EBML->begin(), m_EBML->end()); }

    return buffer;
}

bool EBMLMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    if (!copyToVariable(buffer, bufferIndex + IO_INDEX_INDEX, m_ioIdx)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + START_TIME_INDEX, m_startTime)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + END_TIME_INDEX, m_endTime)) { return false; }

    size_t EBMLsize;
    if (!copyToVariable(buffer, bufferIndex + SIZE_INDEX, EBMLsize)) { return false; }

    if (buffer.size() < bufferIndex + EBML_INDEX + EBMLsize) { return false; }

    m_EBML.reset(new std::vector<uint8_t>(buffer.begin() + static_cast<const long>(bufferIndex) + EBML_INDEX,
                                          buffer.begin() + static_cast<const long>(bufferIndex) + EBML_INDEX + EBMLsize));

    bufferIndex += MINIMUM_SIZE + EBMLsize;

    m_isValid = true;

    return true;
}

/******************************************************************************
*
* Error
*
******************************************************************************/

std::vector<uint8_t> ErrorMessage::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE);
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_type);
    copyTobuffer(buffer, bufferIndex, m_guiltyId);

    return buffer;
}

bool ErrorMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }

    if (!copyToVariable(buffer, bufferIndex + TYPE_INDEX, m_type)) { return false; }
    if (!copyToVariable(buffer, bufferIndex + GUILTY_ID_INDEX, m_guiltyId)) { return false; }

    bufferIndex += MINIMUM_SIZE;

    m_isValid = true;

    return true;
}

/******************************************************************************
 *
 * Time
 *
 ******************************************************************************/

std::vector<uint8_t> TimeMessage::toBytes() const
{
    std::vector<uint8_t> buffer(MINIMUM_SIZE);
    size_t bufferIndex = 0;

    copyTobuffer(buffer, bufferIndex, m_time);

    return buffer;
}

bool TimeMessage::fromBytes(const std::vector<uint8_t>& buffer, size_t& bufferIndex)
{
    m_isValid = false;

    if (buffer.size() < bufferIndex + MINIMUM_SIZE) { return false; }
    if (!copyToVariable(buffer, bufferIndex + TIME_INDEX, m_time)) { return false; }

    bufferIndex += MINIMUM_SIZE;

    m_isValid = true;

    return true;
}

}   // namespace Communication
