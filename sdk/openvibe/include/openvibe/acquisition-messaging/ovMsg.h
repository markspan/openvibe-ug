#pragma once
/**
* @file ovMsg.h
* @author Florent LERAY florent.leray@inria.fr
* @date 2025
* @version 1.0
* @copyright Copyright (c) 2025 INRIA. All rights reserved.
* @license This project is released under LGPLv3 license.
* 
* @brief Header file defining the ovMsg structure for messages sent to OPENVIBE and from an acquisition.
* @Note This structure is used for communication between the Acquisition Server and OPENVIBE.
*/

#include "samples.h"
#include "drift.h"

/**
* @brief Structure for messages sent to the OPENVIBE.
* @details This structure encapsulates different types of messages that can be sent to the OPENVIBE platform.
* The messages include samples, drift information, acquisition configuration, and error messages.
* @note The structure includes a message type enumeration, a payload header, and payload structures.
* @note The structure is designed to be serialisable, flexible and extensible for future message types.
*/
struct ovMsg
{
    //----------------------------------------------- Internal types -----------------------------------------------  
    using msgTypeEnum = enum
    {
        OV_MSG_SAMPLES = 0, ///< Message type for samples.  
        OV_MSG_DRIFT = 1, ///< Message type for drift.  
        OV_MSG_ACQUISITION_CONFIG = 2, ///< Message type for acquisition configuration.
        OV_MSG_ERROR ///< Message type for error.  
    };

    //----------------------------------------------- Payload header --------------------------------------------------  
    msgTypeEnum msgType; ///< Type of the message.  
    uint32_t msgSize; ///< Size of the message.  

    //----------------------------------------------- Payload structures ----------------------------------------------  
    union msgData
    {
        struct samplesStruct
        {
            uint64_t*       timestamp;    ///< Timestamps table.
            sampleType*     samples;      ///< Samples table.
            ovStimulation*  stimulations; ///< Stimulations table (may be nullptr when stimCount == 0).
            uint32_t        count;        ///< Number of samples.
            uint32_t        stimCount;    ///< Number of stimulations.
        }samplesStruct;
        drift_st driftStruct; ///< Drift structure.
        struct acquisitionConfigStruct
        {
            uint32_t channelCount; ///< Number of channels.  
            uint32_t sampleRate; ///< Sample rate.  
            uint32_t sampleChunk; ///< Number of samples in a chunk.
        }acquisitionConfigStruct;
    }msgData;
}; ///< Define a structure for messages sent to the OPENVIBE.  
