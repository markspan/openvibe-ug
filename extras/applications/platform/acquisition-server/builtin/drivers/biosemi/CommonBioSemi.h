// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

/**
 * @file CommonBioSemi.h
 * @brief Definitions shared by the BioSemi ActiveTwo and ActiveThree bridges.
 *
 * The legacy version of this header also carried an SInformationWindow struct backing a GTK
 * status window (CMS/DRL in range, battery level, error message). The Qt6 acquisition server has
 * no driver-to-UI status channel, so that window is gone and the same conditions are reported on
 * the console and, where they affect the signal, as stimulations.
 */

#include <cstdint>

/// Highest sampling rate any BioSemi AD-box runs at, in Hz. AbstractDriver defaults m_samplingRate
/// to a 1..10000 range, which would silently clamp the 16384 Hz speed modes, so both drivers widen
/// the range with this before storing what the device reports.
static constexpr int BIOSEMI_MAX_SAMPLING_RATE = 16384;

/// Failure reported by a BioSemi bridge through getLastError().
enum EBioSemiError
{
    BioSemiError_NoError,

    BioSemiError_OSOpenFailed,
    BioSemiError_OSCloseFailed,

    BioSemiError_EnableUSBHandshakeFailed,
    BioSemiError_DisableUSBHandshakeFailed,

    BioSemiError_ReadPointerFailed,

    BioSemiError_DeviceTypeChanged,
    BioSemiError_SpeedmodeChanged,
    BioSemiError_InvalidSpeedmode,

    BioSemiError_NoData,
    BioSemiError_NotEnoughDataInBuffer,
    BioSemiError_NoSync,
    BioSemiError_SyncLost,
    BioSemiError_BufferOverflow
};
