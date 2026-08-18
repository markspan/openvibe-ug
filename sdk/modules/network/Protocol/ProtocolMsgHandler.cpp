/**
* @file ProtocolMsgHandler.cpp
* @brief Implementation of the ProtocolMsgHandler class for packing and unpacking protocol messages.
* @details This file contains the implementation of the ProtocolMsgHandler class, which provides methods to pack and unpack protocol messages with the specific format.
* The protocol includes a header with a magic number and size. It includes the msg and CRC32 checksum as footer for integrity verification.
*/

#include <ProtocolMsgHandler.h>
#include <crc32.h>
#include <NAL.h>
#include <ClientServerCommon.h>

#include <algorithm>
#include <memory>


namespace Network
{

namespace CSC = common;
    /**
     * @brief Constructs a ProtocolMsgHandler object with a specified magic value.
     * @param magic The magic value (magic number) used to initialize the object.
     */
    ProtocolMsgHandler::ProtocolMsgHandler(uint32_t magic) : m_magic(magic), m_expectedTotalSize(0)
    {
    }

    /**
     * @brief Destructor for the ProtocolMsgHandler class.
     */
    ProtocolMsgHandler::~ProtocolMsgHandler()
    {
    }

    /**
     * @brief Prepares a send packet with a header, msg, and CRC32 for integrity.
     * @param payload A pointer to the data to send.
     * @param payloadSize The size, in bytes, of the msg to send.
     * @return true if the packet was successfully generated, it is ready to send now, otherwise false.
     */
    bool ProtocolMsgHandler::sendPacket(void* payload, size_t payloadSize)
    {
        bool bRes = false;

        if (payload && payloadSize > 0)
        {
            static  CSC::magicNumber magic_net = htonl(m_magic);

            auto buffer = std::make_unique<uint8_t[]>(CSC::overheadSize + payloadSize);
            uint8_t* bufferWork = buffer.get();

            *(reinterpret_cast<CSC::magicNumber*>(bufferWork)) = magic_net;
            bufferWork += CSC::magicNumberSize;

            *(reinterpret_cast<CSC::sizeField*>(bufferWork)) = htonl(CSC::overheadSize + payloadSize);
            bufferWork += CSC::sizeFieldSize;

            memcpy(bufferWork, payload, payloadSize);
            bufferWork += payloadSize;

            *(reinterpret_cast<CSC::crcField*>(bufferWork)) = htonl(Crc32Utils::calculate(buffer.get(), CSC::headerSize + payloadSize));
            bufferWork += CSC::crcSize;

            m_msgToSend = std::make_pair(CSC::overheadSize + payloadSize, std::move(buffer));
            
            bRes = true;
        }

        return bRes;
    }




    /**
     * @brief Processes received data by extracting and validating a message using CRC checking, then handling network packet reception.
     * @param payload Pointer to the received data to process.
     * @param payloadSize Size, in bytes, of the data pointed to by msg.
     * @return true if the message was successfully received and validated, false otherwise.
     */
    bool ProtocolMsgHandler::receive(uint8_t* payload, size_t payloadSize)
    {
        bool bRes = true;


        if(payloadSize > 0 && payload != nullptr)
        {
            size_t offset = 0;

            while (bRes && (offset < payloadSize-1))
            {
                if (m_expectedTotalSize == 0)
                {
                    offset += headerHandling(payloadSize - offset, payload + offset);
                    bRes = offset > 0; // If header handling fails, we should stop processing further data.
                }

                if (bRes)
                {
                    if (m_expectedTotalSize > 0)
                    {
                        offset += appendRecivedData(m_expectedTotalSize, payloadSize - offset, payload + offset);
                    }

                    size_t buffSize = m_incomingBuffer.size();

                    if (buffSize && buffSize == m_expectedTotalSize)
                    {
                        auto msgSize = m_expectedTotalSize - CSC::overheadSize;
                        auto* msg = m_incomingBuffer.data() + CSC::headerSize;
                        m_receivedMsgQueue.push(std::make_pair(msgSize, std::make_unique<uint8_t[]>(msgSize)));
                        memcpy(m_receivedMsgQueue.back().second.get(), msg, msgSize);

                        m_expectedTotalSize = 0;
                        m_incomingBuffer.clear();
                    }
                }
                else
                {
                    // If header handling failed, we should stop processing further data.
                }
            }
        }
        return bRes;
    }



    /**
    * @brief Retrieves the next message to be sent, providing its size and data.
    * @param size An output parameter that will be set to the size of the message to send.
    * @param data An output parameter that will be set to a unique pointer containing the data of the message to send.
    * @return true if a message is available to send, false if there is no message currently prepared for sending.
    */
    bool ProtocolMsgHandler::sendingData(int& size, std::unique_ptr<uint8_t[]>& data)
    {
        bool bRes = m_msgToSend.second != nullptr;

        if (bRes)
        {
            size = m_msgToSend.first;
            data = std::move(m_msgToSend.second);

            m_msgToSend.first = 0;
            m_msgToSend.second = nullptr;
        }

        return bRes;
    }

    /**
    * @brief Retrieves the next received message from the queue, providing its size and data.
    * @param size An output parameter that will be set to the size of the retrieved message
    * @param data An output parameter that will be set to a unique pointer containing the data of the retrieved message
    * @return true if a message was successfully retrieved from the queue, false if the queue is empty and no message is available.
    */
    bool ProtocolMsgHandler::receivedData(int& size, std::unique_ptr<uint8_t[]>& data)
    {
        bool bRes = !m_receivedMsgQueue.empty();

        if (bRes)
        {
            size = m_receivedMsgQueue.front().first;
            data = std::move(m_receivedMsgQueue.front().second);

            m_receivedMsgQueue.pop();
        }

        return bRes;
    }

    /**
    * @brief Creates a new instance of ProtocolMsgHandler with the same magic value.
    * @return A pointer to a new ProtocolMsgHandler object that is a copy of the
    */
    ProtocolMsgHandler* ProtocolMsgHandler::clone() const
    {
        return new ProtocolMsgHandler(m_magic);
    }


    // ---------------------------------

    /**
    * @brief Prepares the data to be sent by packing it with a header and CRC32 checksum for integrity verification.
    * @param typeId The type ID of the data being sent, used to identify the data type in the protocol.
    * @param payload A pointer to the data to send.
    * @param payloadSize The size, in bytes, of the data pointed to by payload
    * @return true if the data was successfully prepared for sending, false otherwise.
    */
    bool ProtocolMsgHandler::prepareSendingData(uint64_t typeId, void* payload, size_t payloadSize)
    {
        bool bRes = false;

        if (payload && payloadSize > 0)
        {
            static  uint32_t magic_net = htonl(m_magic);

            auto buffer = std::make_unique<uint8_t[]>(CSC::totalOverheadSize + payloadSize);// new uint8_t[totalOverheadSize + msgSize];
            uint8_t* bufferWork = buffer.get();
            uint8_t* buff = buffer.get();


            *(reinterpret_cast<uint32_t*>(bufferWork)) = magic_net;
            bufferWork += sizeof(uint32_t);

            *(reinterpret_cast<uint32_t*>(bufferWork)) = htonl(CSC::overheadSize + payloadSize);
            bufferWork += sizeof(uint32_t);

            *(reinterpret_cast<uint64_t*>(bufferWork)) = CSC::typeField(typeId, true).getNetworkInt64();
            bufferWork += sizeof(CSC::typeField);

            memcpy(bufferWork, payload, payloadSize);
            bufferWork += payloadSize;

            *(reinterpret_cast<uint32_t*>(bufferWork)) = htonl(Crc32Utils::calculate(buffer.get(), CSC::headerSize + payloadSize));
            bufferWork += sizeof(uint32_t);

            m_msgToSend = std::make_pair(CSC::overheadSize + payloadSize, std::move(buffer));

            bRes = true;
        }

        return bRes;
    }


    /**
    * @brief Computes the header from the received data, extracting the magic number and expected total size.
    * @param header A pointer to the received data containing the header information.
    * @return true if the header was successfully computed and the magic number is valid, false otherwise.
    */
    bool ProtocolMsgHandler::computeHeader(uint8_t* header)
    {
        bool bRes = false;

        CSC::magicNumber& magic_net = *(CSC::magicNumber*)header;
        CSC::magicNumber  magic = ntohl(magic_net);

        if (magic == m_magic) //MAGIC check
        {
            CSC::sizeField& size_net = *(CSC::sizeField*)(header + CSC::magicNumberSize);
            m_expectedTotalSize = ntohl(size_net);
            bRes = m_expectedTotalSize > 0;
        }

        return bRes;
    }

    /**
     * @brief Handles the header of the received data, extracting the magic number and expected total size.
     * @param payload Pointer to the received data containing the header information.
     * @param payloadSize Size, in bytes, of the data pointed to by payload.
     * @return The number of bytes processed from the payload for the header, or 0 if header processing failed.
     */
    size_t ProtocolMsgHandler::headerHandling(size_t payloadSize, uint8_t* payload)
    {
        size_t offset = 0;

        if ((payloadSize + m_incomingBuffer.size()) >= CSC::overheadSize)
        {
            offset = appendRecivedData(CSC::overheadSize, payloadSize, payload);
            if (m_incomingBuffer.size() == CSC::overheadSize)
            {
                offset = computeHeader(m_incomingBuffer.data()) ? offset : 0;
            }
        }
        else
        {
            m_incomingBuffer.insert(m_incomingBuffer.end(), payload, payload + payloadSize);
            offset = payloadSize;
        }

        return offset;
    }

    /**
    * @brief Appends received data to the incoming buffer, ensuring that the total size does not exceed the expected message body size.
    * @param wantedSize The total size of the message body that we are expecting to receive
    * @param payloadSize The size, in bytes, of the data pointed to by payload.
    * @param payload Pointer to the received data to append.
    * @return The number of bytes appended to the incoming buffer.
    */
    size_t Network::ProtocolMsgHandler::appendRecivedData(size_t wantedSize, size_t payloadSize, uint8_t* payload)
    {
        size_t waitedSize = wantedSize - m_incomingBuffer.size();
        size_t appendableSize = std::min(payloadSize, waitedSize);
        m_incomingBuffer.insert(m_incomingBuffer.end(), payload, payload + appendableSize);

        return appendableSize;
    }

} //  namespace Network
