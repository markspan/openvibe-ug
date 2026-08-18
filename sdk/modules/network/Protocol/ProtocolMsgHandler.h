#pragma once

/**
* @file ProtocolMsgHandler.h
* @brief Header file for the ProtocolMsgHandler class.
* @details This file contains the declaration of the ProtocolMsgHandler class, which provides methods to send and receive protocol messages with a specific format.
* The protocol includes a header with a magic number and size, the payload, and a CRC32 checksum for integrity verification.
* The class is designed to be used in a networked environment where messages need to be sent and received reliably.
* It supports packing and unpacking of messages, handling of partial messages, and verification of message integrity using CRC32.
* The class is intended to be used as a base class for more specialized message handlers that implement specific protocols.
* @note This class is not thread-safe and should be used from a single thread.
* @see CNetwork
* @see CClient
* @see CServer
* @author Florent LERAY
* @date 2025-10-04
*/

#include <queue>
#include <vector>
#include <memory>
#include <utility>

namespace Network
{
    /**
     * @class ProtocolMsgHandler
     * @brief A class for sending and receiving protocol messages.
     * @details This class provides methods to send and receive packets with a specific protocol format.
     */
    class ProtocolMsgHandler
    {
    public:
        ProtocolMsgHandler(uint32_t magic = 0xDEADC0DE);
        virtual ~ProtocolMsgHandler();

        virtual bool sendPacket(void* payload, size_t payloadSize);
        virtual bool receive(uint8_t* payload, size_t payloadSize);

        virtual bool sendingData(int& size, std::unique_ptr<uint8_t[]>& data);
        virtual bool receivedData(int& size, std::unique_ptr<uint8_t[]>& data);

        virtual ProtocolMsgHandler* clone() const;

    private:
        bool prepareSendingData(uint64_t typeId, void* payload, size_t payloadSize);

        virtual bool computeHeader(uint8_t* header);
        virtual size_t headerHandling(size_t payloadSize, uint8_t* payload);
        virtual size_t appendRecivedData(size_t wantedSize, size_t payloadSize, uint8_t* payload);

    private:
        const uint32_t m_magic;

        size_t m_expectedTotalSize;
        std::vector<uint8_t> m_incomingBuffer;

    protected:
        std::queue<std::pair<int, std::unique_ptr<uint8_t[]>>> m_receivedMsgQueue;
        std::pair<int, std::unique_ptr<uint8_t[]>> m_msgToSend;
    };
} // namespace Network
