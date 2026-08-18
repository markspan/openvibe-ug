#pragma once
/**
 * @file ovMsgSerializer.h
 * @brief Binary serialization and deserialization functions for ovMsg.
 *
 * These functions produce a contiguous byte buffer suitable for network transmission.
 * The serialize function returns a new uint8_t[] buffer that the caller (or Network++)
 * is responsible for deleting with delete[].
 */

#include "ovMsg.h"

#include <cstring>   // memcpy
#include <utility>   // std::pair
#include <stdexcept> // std::runtime_error
#include <iostream>   // std::cerr

/**
 * @brief Serializes an ovMsg into a contiguous binary buffer.
 *
 * Wire formats:
 *   OV_MSG_SAMPLES:            [msgType:u32][msgSize:u32][count:u32][stimCount:u32][timestamps: count*u64][samples: derived from msgSize][stims: stimCount*ovStimulation]
 *   OV_MSG_DRIFT:              [msgType:u32][startLatency:u64][globalLatency:u64][currentLatency:u64][endLatency:u64]
 *   OV_MSG_ACQUISITION_CONFIG: [msgType:u32][channelCount:u32][sampleRate:u32][sampleChunk:u32]
 *
 * @note For OV_MSG_SAMPLES, msgSize must be set correctly by the caller before serializing.
 *       samplesBytes is back-calculated as: msgSize - 4*u32 - count*u64 - stimCount*sizeof(ovStimulation)
 *
 * @param msg The message to serialize.
 * @return A pair of {bufferSize, buffer}. The buffer is allocated with new uint8_t[].
 */
inline std::pair<size_t, void*> serializeOvMsg(const ovMsg& msg)
{
    switch (msg.msgType)
    {
        case ovMsg::OV_MSG_SAMPLES:
        {
            const auto& s = msg.msgData.samplesStruct;
            const size_t timestampsBytes = s.count * sizeof(timestampType);
            const size_t stimsBytes      = s.stimCount * sizeof(ovStimulation);
            const size_t samplesBytes    = msg.msgSize - 4 * sizeof(uint32_t) - timestampsBytes - stimsBytes;

            uint8_t* buffer = new uint8_t[msg.msgSize];
            uint8_t* ptr = buffer;

            const uint32_t type = static_cast<uint32_t>(msg.msgType);
            std::memcpy(ptr, &type,       sizeof(uint32_t)); ptr += sizeof(uint32_t);
            std::memcpy(ptr, &msg.msgSize,sizeof(uint32_t)); ptr += sizeof(uint32_t);
            std::memcpy(ptr, &s.count,    sizeof(uint32_t)); ptr += sizeof(uint32_t);
            std::memcpy(ptr, &s.stimCount,sizeof(uint32_t)); ptr += sizeof(uint32_t);
            std::memcpy(ptr, s.timestamp, timestampsBytes);  ptr += timestampsBytes;
            std::memcpy(ptr, s.samples,   samplesBytes);     ptr += samplesBytes;
            if (stimsBytes > 0) { std::memcpy(ptr, s.stimulations, stimsBytes); }

            return { msg.msgSize, buffer };
        }

        case ovMsg::OV_MSG_DRIFT:
        {

            uint8_t* buffer = new uint8_t[msg.msgSize];
            
            std::memcpy(buffer, &msg, msg.msgSize);

            return { msg.msgSize, buffer };
        }

        case ovMsg::OV_MSG_ACQUISITION_CONFIG:
        {
            uint8_t* buffer = new uint8_t[msg.msgSize];

            std::memcpy(buffer, &msg, msg.msgSize);

            return { msg.msgSize, buffer };
        }

        default:
            return { 0, nullptr };
    }
}

/**
 * @brief Deserializes a binary buffer back into an ovMsg.
 *
 * For OV_MSG_SAMPLES, allocates new arrays for timestamps, samples, and stimulations.
 * The caller is responsible for deleting these with delete[].
 *
 * @param data Pointer to the serialized data.
 * @param dataSize Size of the serialized data in bytes.
 * @return A populated ovMsg.
 */
inline ovMsg deserializeOvMsg(const uint8_t* data, size_t dataSize)
{
    ovMsg msg = {};

    if (dataSize < 2*sizeof(uint32_t))
    {
        std::cerr << "deserializeOvMsg: Cannot deserialize message: dataSize < 2*sizeof(uint32_t)" << std::endl;
        return msg;
    }

    const uint8_t* ptr = data;

    uint32_t type;
    std::memcpy(&type, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    msg.msgType = static_cast<ovMsg::msgTypeEnum>(type);

    std::memcpy(&msg.msgSize, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    switch (msg.msgType)
    {
        case ovMsg::OV_MSG_SAMPLES:
        {
            auto& s = msg.msgData.samplesStruct;

            std::memcpy(&s.count,     ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);
            std::memcpy(&s.stimCount, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

            const size_t timestampsBytes = s.count * sizeof(timestampType);
            const size_t stimsBytes      = s.stimCount * sizeof(ovStimulation);
            const size_t samplesBytes    = msg.msgSize - 4 * sizeof(uint32_t) - timestampsBytes - stimsBytes;

            s.timestamp = new timestampType[s.count];
            std::memcpy(s.timestamp, ptr, timestampsBytes); ptr += timestampsBytes;

            s.samples = new sampleType[samplesBytes / sizeof(sampleType)];
            std::memcpy(s.samples, ptr, samplesBytes);      ptr += samplesBytes;

            if (s.stimCount > 0)
            {
                s.stimulations = new ovStimulation[s.stimCount];
                std::memcpy(s.stimulations, ptr, stimsBytes);
            }
            else { s.stimulations = nullptr; }

            break;
        }

        case ovMsg::OV_MSG_DRIFT:
        {
            std::memcpy(&msg.msgData.driftStruct, ptr, sizeof(drift_st));
            break;
        }

        case ovMsg::OV_MSG_ACQUISITION_CONFIG:
        {
            std::memcpy(&msg.msgData.acquisitionConfigStruct, ptr, sizeof(msg.msgData.acquisitionConfigStruct));
            break;
        }

        default:
            std::cerr << "deserializeOvMsg: Unknown msgType" << std::endl;
    }

    return msg;
}
