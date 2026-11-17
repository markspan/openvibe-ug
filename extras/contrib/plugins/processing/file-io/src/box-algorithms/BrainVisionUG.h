// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.27. The parts of the BrainVision Format file writer (UG) that decide what lands in
// the files: where a marker goes, how a channel name and a sample are written. Plain C++, so that they can be tested.

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
namespace BrainVisionUG {

/**
 * \brief The position of a marker in the data file: 1 for the first sample written, as BrainVision counts.
 *
 * \param date            The marker's date, in OpenViBE time (32:32 fixed point seconds).
 * \param firstSampleTime The date of the first sample written to the data file: the start of the first signal chunk.
 * \param sampling        The sampling rate, in Hz.
 * \return Nothing when the marker is dated before the first sample, or the rate is unknown.
 *
 * Counted from the first sample written, not from the scenario's start: the Acquisition client starts its signal when
 * the server's configuration comes in, which is rarely at time 0. Rounded to the nearest sample: a chunk's start is
 * the sum of chunk durations rounded down to the fixed point, so at e.g. 500 Hz a sample's date falls just short of
 * the exact time, and rounding down would put the marker one sample early.
 */
inline std::optional<uint64_t> markerPosition(const uint64_t date, const uint64_t firstSampleTime, const uint64_t sampling)
{
    if (sampling == 0 || date + (1ULL << 31) / sampling < firstSampleTime) { return std::nullopt; }  // half a sample of slack
    const double seconds = date >= firstSampleTime ? double(date - firstSampleTime) / 4294967296.0 : 0.0;
    return uint64_t(std::llround(seconds * double(sampling))) + 1;
}

/** A channel name as the header writes it: BrainVision separates fields with commas, and codes a comma in a name as "\1". */
inline std::string escapeName(const std::string& name)
{
    std::string out;
    out.reserve(name.size());
    for (const char c : name) {
        if (c == ',') { out += "\\1"; }
        else { out += c; }
    }
    return out;
}

/**
 * \brief A sample as a 16-bit integer: rounded, and held at the type's limits instead of wrapping around.
 * \param clipped Set when the value lay outside the type's range, or was not a number (written as 0).
 */
template <class T>
T toInteger(const double value, bool& clipped)
{
    if (std::isnan(value)) {
        clipped = true;
        return T(0);
    }
    const double rounded = std::nearbyint(value);
    if (rounded < double(std::numeric_limits<T>::min())) {
        clipped = true;
        return std::numeric_limits<T>::min();
    }
    if (rounded > double(std::numeric_limits<T>::max())) {
        clipped = true;
        return std::numeric_limits<T>::max();
    }
    return T(rounded);
}

}  // namespace BrainVisionUG
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
