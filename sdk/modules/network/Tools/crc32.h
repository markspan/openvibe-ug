#pragma once

/**
 * @file crc32.h
 * @brief CRC32 utility functions.
 * @details This file provides functions to compute CRC32 checksums.
 */

#include <cstdint>
#include <numeric>
#include <array>

/**
 * @brief A simple, self-contained utility for CRC32 calculations.
 */
namespace Crc32Utils
{
    // The CRC32 polynomial (IEEE 802.3)
    constexpr uint32_t POLYNOMIAL = 0xEDB88320;

    /**
     * @brief The lookup table for fast CRC32 calculation.
     * This table is generated at the first call.
     */
    static std::array<uint32_t, 256> crcTable;

    /**
     * @brief Generates the CRC32 lookup table.
     * This function is called automatically once.
     */
    void generateTable()
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t crc = i;
            for (uint32_t j = 0; j < 8; ++j)
            {
                crc = (crc & 1) ? (crc >> 1) ^ POLYNOMIAL : (crc >> 1);
            }
            crcTable[i] = crc;
        }
    }

    /**
     * @brief Calculates the CRC32 checksum for a given block of data.
     *
     * @param data Pointer to the data buffer.
     * @param length The length of the data buffer in bytes.
     * @return The calculated CRC32 checksum.
     */
    uint32_t calculate(const uint8_t* data, size_t length)
    {
        // Generate the table if it hasn't been generated yet.
        // The static flag ensures this runs only once.
        static bool tableGenerated = false;
        if (!tableGenerated)
        {
            generateTable();
            tableGenerated = true;
        }

        uint32_t crc = 0xFFFFFFFF; // Initial value

        for (size_t i = 0; i < length; ++i)
        {
            crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ data[i]];
        }

        return crc ^ 0xFFFFFFFF; // Final XOR
    }
} // namespace Crc32Utils