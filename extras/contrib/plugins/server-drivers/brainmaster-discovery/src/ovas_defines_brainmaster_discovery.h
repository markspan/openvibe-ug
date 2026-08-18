///-------------------------------------------------------------------------------------------------
/// 
/// \file ovas_defines_brainmaster_discovery.h
/// \copyright Copyright (C) 2012, Yann Renard. All rights reserved.
/// 
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
/// 
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
/// 
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI

#include "ovasIDriver.h"

#include "CMKRDLLU.H"
#include <string>

namespace OpenViBE {
namespace AcquisitionServer {
typedef enum
{
    Preset_Custom=0,
    Preset_Discovery_24=1,
    Preset_Atlantis_4x4=2,
    Preset_Atlantis_2x2=3,

    Type_Discovery=0,
    Type_Atlantis=1,

    ChannelType_EEG=0,
    ChannelType_AUX=1,

    BaudRate_Default=0,
    BaudRate_9600=1,
    BaudRate_115200=2,
    BaudRate_460800=3,

    NotchFilter_Default=0,
    NotchFilter_Off=1,
    NotchFilter_50=2,
    NotchFilter_60=3,

    BitDepth_Default=0,
    BitDepth_8=1,
    BitDepth_16=2,
    BitDepth_24=3,

    SamplingFrequency_256=0,
    SamplingFrequency_512=1,
    SamplingFrequency_1024=2,
    SamplingFrequency_2048=3,
} EParameter;

inline size_t autoDetectPort(const size_t startPort = 1, const size_t stopPort = 16, const size_t fallback = size_t(-1))
{
    for (size_t i = startPort; i <= stopPort; ++i)
    {
        const BOOL res = ::AtlOpenPort(i, /*m_baudRate*/ 0, nullptr);
        ::AtlClosePort(i);
        if (res) { return i; }
    }
    return fallback;
}

// returns true on success
// returns false on error
inline bool checkDeviceSerial(const std::string& serial)
{
    // Checks length
    if (serial.length() != 5) { return false; }

    // Checks digits
    for (size_t i = 0; i < serial.length(); ++i) { if (serial[i] < '0' || serial[i] > '9') { return false; } }

    // First digit gives device kind
    switch (serial[0])
    {
        case '3': // Atlantis
        case '4': // Atlantis
        case '6': // Discovery
        default: break;
    }

    // Everything's fine
    return true;
}

// returns true on success
// returns false on error
inline bool checkDevicePasskey(const std::string& passkey)
{
    // Checks length -- other checks are hardcoded assuming length is 15
    if (passkey.length() != 14) { return false; }

    // Checks separators
    if (passkey[4] != '-' || passkey[9] != '-') { return false; }

    // Checks alpha-nums
    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            const size_t k = i * 5 + j;
            if ((passkey[k] < '0' || passkey[k] > '9') && (passkey[k] < 'a' || passkey[k] > 'z')
                && (passkey[k] < 'A' || passkey[k] > 'Z')) { return false; }
        }
    }
    // Everything's fine
    return true;
}
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
