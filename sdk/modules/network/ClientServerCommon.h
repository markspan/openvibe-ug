#pragma once

/**
 * @file ClientServerCommon.h
 * @brief Common utilities for client-server communication.
 * @details This file contains utility functions for computing and extracting unique type identifiers
 * based on data size and name, as well as a MurmurHash3 implementation for hashing strings.
 * These utilities are used in both client and server components to ensure consistent type identification.
 * @note This file is part of a larger networking library and is intended for internal use.
 * @author Florent LERAY
 * @date 2025-10-04
 */

#include <cstdint>
#include <string>
#include <functional>

 /**
  * @brief Read callback function type.
  * @details This function is called when data of a specific type is received.
  * This function must be registered using CServer::registerReadCallback.
  * The function takes a pointer to the raw data received, the data size and the client ID as parameters.
  * The void* parameter points to the raw data received. The function should cast or un-serialize it to the appropriate type.
  * The int parameter is the client ID. If you use the same callback for multiple clients, you can use this parameter to identify the client.
  * bool return value indicates whether the data has been well consumed (true) or not (false).
  */
using readFunc = std::function<bool(void*, size_t, uint64_t)>; // Raw input data as first parameter. Size of the data as second parameter. ClientId as third parameter. Return true if data has been consumed.

/**
 * @brief Write callback function type.
 * @details This function is called when data of a specific type is to be sent.
 * This function must be registered using CServer::registerWriteCallback.
 * The function takes a pointer to the raw data to send and the client ID as parameters.
 * The void* parameter points to the raw data to send. The function should cast or serialize it to the appropriate type.
 * The int parameter is the client ID. If you use the same callback for multiple clients, you can use this parameter to identify the client.
 * The function returns a pair containing the size of the data to send and a pointer to the data.
 * The size must be greater than 0 and the pointer must not be null.
 * The memory for the data must be allocated by the caller and will be freed by the caller after sending.
 * std::pair<size_t, void*> return value contains the size of the data to send as the first element and a pointer to the data as the second element.
 */
using writeFunc = std::function<std::pair<size_t, void*>(void*, uint64_t)>; // Raw input data as first parameter. ClientId as second parameter. Return a pair of size and pointer to data to send.

#include <cstdint>
#include <string>
#include <memory>

namespace Network
{
    /**
     * @brief common utilities for client-server communication.
     * @details This namespace contains utility functions, types and constants used for client-server communication,
     * including type identification and hashing.
     * @note This namespace is intended for internal use within the networking library.
     */
    namespace common
    {
        using magicNumber = uint32_t;
        using sizeField = uint32_t;
        using crcField = uint32_t;

        using headerField = struct
        {
            magicNumber magic;
            sizeField size;
        };

        using typeField = union typeField
        {
            uint64_t typeId;
            struct
            {
                uint32_t size;
                uint32_t nameHash;
            };

            operator uint64_t&() { return typeId; }
            operator const uint64_t& () const { return typeId; }

            typeField() { typeId = 0; }
            typeField(uint64_t type, bool host = true);
            uint64_t getNetworkInt64() const;
        };

        static constexpr size_t magicNumberSize = sizeof(magicNumber); // 4 bytes magic 
        static constexpr size_t sizeFieldSize = sizeof(sizeField); // 4 bytes size
        static constexpr size_t headerSize = sizeof(headerField); // 4 bytes magic + 4 bytes size
        static constexpr size_t typeIdSize = sizeof(typeField); // 8 bytes (4 bytes size + 4 bytes nameHash)
        static constexpr size_t crcSize = sizeof(crcField); // 4 bytes CRC32
        static constexpr size_t overheadSize = headerSize + crcSize; // Total overhead size without typeId. 4 + 4 + 4 = 12 bytes
        static constexpr size_t totalOverheadSize = overheadSize + typeIdSize; // Total overhead size with typeId. 4 + 4 + 8 + 4 = 20 bytes
        static constexpr uint32_t magic = 0xDEADC0DE; // Magic number for packet identification

        typeField extractTypeId(uint8_t* data);
        typeField computTypeId(std::string name);
        void compactTypeAndData(writeFunc func, void* data, uint64_t remoteId, size_t& dataSize, std::unique_ptr<uint8_t[]>& msgData, Network::common::typeField& type);
        } // namespace common
} // namespace Network


