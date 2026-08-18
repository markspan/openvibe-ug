#pragma once

/**
 * @file ClientServerCommon.cpp
 * @brief Common utilities for client-server communication.
 * @details This file contains utility functions for computing and extracting unique type identifiers
 * based on data size and name, as well as a MurmurHash3 implementation for hashing strings.
 * These utilities are used in both client and server components to ensure consistent type identification.
 * @note This file is part of a larger networking library and is intended for internal use.
 * @author Florent LERAY
 * @date 2025-10-04
 */

#include <ClientServerCommon.h>

#include <NAL.h>
namespace Network
{
    namespace common
    {
        ////////////////////////////////////////////////////////////////////////////////
        // Private Forward declarations
        ////////////////////////////////////////////////////////////////////////////////

        uint32_t murmurHash3_32(const std::string& text, uint32_t seed = 3735929054);

        ////////////////////////////////////////////////////////////////////////////////
        // Implementations
        ////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Extracts a unique type identifier from raw data.
         * @param data Pointer to the raw data containing size and name hash.
         * @return A 64-bit unique type identifier.
         */
        typeField extractTypeId(uint8_t* data)
        {
            typeField typeIdRes;
            
            typeIdRes.nameHash = ntohl(*(uint32_t*)(data));
            typeIdRes.size     = ntohl(*(uint32_t*)(data + 4));

            return typeIdRes;
        }

        /**
         * @brief Computes a unique type identifier based on size and name.
         * @param name The name of the data type.
         * @return A 64-bit unique type identifier.
         */
        typeField computTypeId(std::string name)
        {
            typeField typeIdRes;

            typeIdRes.size     = name.length();
            typeIdRes.nameHash = murmurHash3_32(name);

            return typeIdRes;
        }

        /**
        * @brief Constructor for typeField that initializes from a 64-bit integer.
        * @param type The 64-bit integer representing the type field.
        * @param host If true, the input is in host byte order; if false, it's in network byte order.
        */
        typeField::typeField(uint64_t type, bool host)
        {
            if (host)
            {
                typeId = type;
            }
            else
            {
                uint32_t low = static_cast<uint32_t>(type >> 32);
                uint32_t high = static_cast<uint32_t>(type & 0xFFFFFFFFULL);
                typeId = (static_cast<uint64_t>(ntohl(high)) << 32) | ntohl(low);
            }
        }

        /**
         * @brief Converts the typeField to a 64-bit integer in network byte order.
         * @return A 64-bit integer representing the type field in network byte order.
         */
        uint64_t typeField::getNetworkInt64() const
        {
            uint64_t network_size = ((uint64_t)htonl(size)) << 32;
            uint64_t network_hash = (uint64_t)htonl(nameHash);

            return network_size | network_hash;
        }


        /**
         * @brief Calculates the 32-bit MurmurHash3 of a given string.
         * @details This is a C++ implementation of the MurmurHash3 algorithm by Austin Appleby.
         * It provides a high-quality, non-cryptographic hash with excellent distribution.
         *
         * @param text The input std::string to hash. The string is treated as a sequence of bytes.
         * @param seed The seed for the hash function. Allows for different hash values for the same input.
         * @return The calculated 32-bit hash value.
         */
        uint32_t murmurHash3_32(const std::string& text, uint32_t seed)
        {
            // Treat the string's data as a block of bytes
            const uint8_t* key = reinterpret_cast<const uint8_t*>(text.data());
            const int len = text.length();
            const int nblocks = len / 4;

            uint32_t h1 = seed;

            // MurmurHash3 constants
            const uint32_t c1 = 0xcc9e2d51;
            const uint32_t c2 = 0x1b873593;
            const uint32_t r1 = 15;
            const uint32_t m = 5;
            const uint32_t n = 0xe6546b64;

            // Process 4-byte chunks
            const uint32_t* blocks = reinterpret_cast<const uint32_t*>(key);
            for (int i = 0; i < nblocks; i++)
            {
                uint32_t k1 = blocks[i];

                k1 *= c1;
                k1 = (k1 << r1) | (k1 >> (32 - r1)); // ROTL32(k1, 15)
                k1 *= c2;

                h1 ^= k1;
                h1 = (h1 << 13) | (h1 >> (32 - 13)); // ROTL32(h1, 13)
                h1 = h1 * m + n;
            }

            // Process the remaining bytes (the "tail")
            const uint8_t* tail = reinterpret_cast<const uint8_t*>(key + nblocks * 4);
            uint32_t k1 = 0;

            switch (len & 3) // len % 4
            {
            case 3:
                k1 ^= tail[2] << 16;
            case 2:
                k1 ^= tail[1] << 8;
            case 1:
                k1 ^= tail[0];
                k1 *= c1;
                k1 = (k1 << r1) | (k1 >> (32 - r1)); // ROTL32(k1, 15)
                k1 *= c2;
                h1 ^= k1;
            }

            // Finalization mix (fmix32)
            h1 ^= len;
            h1 ^= h1 >> 16;
            h1 *= 0x85ebca6b;
            h1 ^= h1 >> 13;
            h1 *= 0xc2b2ae35;
            h1 ^= h1 >> 16;

            return h1;
        }

        /**
         * @brief Compacts type information and serialized data into a single message buffer.
         * @param func The write callback function to serialize the data.
         * @param data Pointer to the raw data to serialize.
         * @param remoteId The identifier of the remote client or server.
         * @param dataSize Reference to store the size of the resulting message.
         * @param msgData Unique pointer to store the resulting message buffer.
         * @param type The type information of the data to send.
         */
        void compactTypeAndData(writeFunc func, void* data, uint64_t remoteId, size_t& dataSize, std::unique_ptr<uint8_t[]>& msgData, Network::common::typeField& type)
        {
            auto msg = func(data, remoteId);
            dataSize = msg.first;

            msgData = std::make_unique<uint8_t[]>(msg.first + typeIdSize);
            uint8_t* buffer = msgData.get();

            *((uint64_t*)buffer) = type.getNetworkInt64();
            buffer += typeIdSize;

            memcpy(buffer, msg.second, msg.first);

            delete[](uint8_t*)msg.second;

            dataSize += typeIdSize;
        }

} // End namespace common
} // End namespace Network
