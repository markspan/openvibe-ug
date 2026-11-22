#pragma once
/**
 * @file ovChannelInfo.h
 * @brief What the acquisition server tells its clients about the channels: their names, and the montage they
 *        were recorded with.
 *
 * UG fork: new file, not in upstream OpenViBE, whose server sends no channel information at all. It travels in
 * the channelNames block of OV_MSG_ACQUISITION_CONFIG (ovMsg.h) as strings, each ended by a '\0':
 *
 *   - one name per channel; an empty one for a channel without a name,
 *   - then, when a montage named the channels, its file name (e.g. "Refa-64.json"),
 *   - and one position per channel: "<theta> <phi>" in degrees, or empty for a channel the montage does not
 *     place on the head (EOG, ECG, ...),
 *   - then, when the impedances were checked, the word "impedances", when they were measured ("2026-11-22 10:42:17",
 *     local time), and one impedance per channel in kOhm, or empty for a channel without one. The montage and the
 *     positions are then sent too, empty when there is no montage.
 *
 * A client that reads the names only, as the fork's first versions do, does not see the rest; a server that
 * sends the names only leaves the montage empty.
 *
 * In Designer, the Acquisition client box sends the positions on as a channel localisation stream
 * (OV_TypeId_ChannelLocalisation), in OpenViBE's frame (toChannelLocalisation()); the BrainVision writer (UG) takes
 * it on an input of its own. The impedances go along on that stream as a fourth column (impedanceColumnLabel()).
 */

#include <cmath>
#include <cstddef>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

/// The channels of an acquisition, as the server sends them in OV_MSG_ACQUISITION_CONFIG.
struct ovChannelInfo
{
    /**
     * @brief Where an electrode sat, on a unit sphere: theta from the vertex (Cz), 0 to 180 degrees, and phi
     *        from the right ear (T8) towards the nose, in degrees.
     */
    struct Position
    {
        bool known   = false;  ///< False for a channel the montage does not place on the head
        double theta = 0.0;
        double phi   = 0.0;
    };

    std::vector<std::string> names;   ///< One per channel
    std::string montage;              ///< File name of the Charmeleon montage that named the channels, empty for none
    std::vector<Position> positions;  ///< One per channel when there is a montage, else empty
    std::string impedanceTime;        ///< When the impedances were measured, "YYYY-MM-DD hh:mm:ss" local time; empty when not checked
    std::vector<double> impedances;   ///< One per channel when they were checked, in kOhm; NaN for a channel without one
};

/**
 * @brief The label of the fourth column of a channel localisation matrix that also carries impedances, in kOhm.
 *
 * UG fork: the Acquisition client box sends the impedances on its Channel localisation output, so that a recording scenario needs no
 * other link: the matrix then has a fourth column, x, y, z, and this, with one row for each channel that has a position or an
 * impedance (a channel without a position at the origin, 0, 0, 0). The BrainVision writer (UG) reads it back.
 */
inline std::string impedanceColumnLabel(const std::string& time) { return "Impedance [kOhm] at " + time; }

/// The time impedanceColumnLabel() put in @p label; false when @p label is no such label.
inline bool impedanceTimeFromColumnLabel(const std::string& label, std::string& time)
{
    const std::string prefix = impedanceColumnLabel("");
    if (label.compare(0, prefix.size(), prefix) != 0) { return false; }
    time = label.substr(prefix.size());
    return true;
}

/// A position in OpenViBE's channel localisation frame: on a unit sphere, x towards the nose, y towards the left ear
/// (T7), z towards the vertex (Cz), as share/openvibe/electrode_sets/electrode_set_standard_cartesian.txt has them.
struct ovCartesianPosition
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/// @p position in OpenViBE's channel localisation frame.
inline ovCartesianPosition toChannelLocalisation(const ovChannelInfo::Position& position)
{
    const double degrees = std::acos(-1.0) / 180.0;
    const double theta   = position.theta * degrees;
    const double phi     = position.phi * degrees;  // from the right ear (T8, y = -1) towards the nose (x = 1)
    return {std::sin(theta) * std::sin(phi), -std::sin(theta) * std::cos(phi), std::cos(theta)};
}

/// The position toChannelLocalisation() gave @p cartesian; not known for the origin, or for a value that is not a number.
inline ovChannelInfo::Position fromChannelLocalisation(const ovCartesianPosition& cartesian)
{
    ovChannelInfo::Position position;
    const double radius = std::sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);
    if (!(radius > 0.0) || !std::isfinite(radius)) { return position; }
    const double degrees = 180.0 / std::acos(-1.0);
    position.known       = true;
    position.theta       = std::acos(std::fmax(-1.0, std::fmin(1.0, cartesian.z / radius))) * degrees;
    position.phi         = std::atan2(cartesian.x, -cartesian.y) * degrees;
    if (position.phi < 0.0) { position.phi += 360.0; }
    return position;
}

namespace ovChannelInfoDetail {

/// "<theta> <phi>", whatever the locale
inline std::string formatPosition(const ovChannelInfo::Position& position)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text.precision(8);
    text << position.theta << ' ' << position.phi;
    return text.str();
}

/// The position formatPosition() wrote; not known when @p text is empty or not one
inline ovChannelInfo::Position parsePosition(const std::string& text)
{
    ovChannelInfo::Position position;
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    position.known = !text.empty() && static_cast<bool>(in >> position.theta >> position.phi);
    return position;
}

/// An impedance in kOhm, whatever the locale; empty for none
inline std::string formatImpedance(const double kOhm)
{
    if (!std::isfinite(kOhm)) { return {}; }
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text.precision(10);
    text << kOhm;
    return text.str();
}

/// The impedance formatImpedance() wrote; NaN when @p text is empty or not a number
inline double parseImpedance(const std::string& text)
{
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    double value = 0.0;
    return !text.empty() && (in >> value) ? value : std::nan("");
}

/// Marks the impedances in the block, after the positions
constexpr const char* IMPEDANCES_TAG = "impedances";

}  // namespace ovChannelInfoDetail

/**
 * @brief Encodes @p info for the channelNames block of OV_MSG_ACQUISITION_CONFIG.
 * @param channelCount Number of channels: missing names and positions are sent empty, extra ones are dropped.
 */
inline std::string encodeChannelInfo(const ovChannelInfo& info, const size_t channelCount)
{
    std::string block;
    for (size_t i = 0; i < channelCount; ++i) {
        if (i < info.names.size()) { block += info.names[i]; }
        block += '\0';
    }
    const bool hasImpedances = !info.impedanceTime.empty();
    if (info.montage.empty() && !hasImpedances) { return block; }

    block += info.montage;
    block += '\0';
    for (size_t i = 0; i < channelCount; ++i) {
        if (i < info.positions.size() && info.positions[i].known) { block += ovChannelInfoDetail::formatPosition(info.positions[i]); }
        block += '\0';
    }
    if (!hasImpedances) { return block; }

    // UG fork: after the positions, so that a client that knows only the montage reads the montage as before
    block += ovChannelInfoDetail::IMPEDANCES_TAG;
    block += '\0';
    block += info.impedanceTime;
    block += '\0';
    for (size_t i = 0; i < channelCount; ++i) {
        if (i < info.impedances.size()) { block += ovChannelInfoDetail::formatImpedance(info.impedances[i]); }
        block += '\0';
    }
    return block;
}

/// Decodes the channelNames block of OV_MSG_ACQUISITION_CONFIG, of @p size bytes, for @p channelCount channels.
inline ovChannelInfo decodeChannelInfo(const char* block, const size_t size, const size_t channelCount)
{
    ovChannelInfo info;
    const char* next      = block;
    const char* const end = block ? block + size : nullptr;
    // The next string, and whether there was one
    const auto read = [&](std::string& value) {
        value.clear();
        if (!next || next >= end) { return false; }
        const char* stop = next;
        while (stop < end && *stop != '\0') { ++stop; }
        value.assign(next, stop);
        next = stop < end ? stop + 1 : end;
        return true;
    };

    info.names.resize(channelCount);
    for (auto& name : info.names) { read(name); }

    if (!read(info.montage)) { return info; }
    // A montage may be empty when impedances follow; the positions are then sent empty, and not kept
    std::vector<ovChannelInfo::Position> positions(channelCount);
    std::string text;
    for (auto& position : positions) {
        if (read(text)) { position = ovChannelInfoDetail::parsePosition(text); }
    }
    if (!info.montage.empty()) { info.positions = std::move(positions); }

    if (!read(text) || text != ovChannelInfoDetail::IMPEDANCES_TAG || !read(info.impedanceTime) || info.impedanceTime.empty()) {
        info.impedanceTime.clear();
        return info;
    }
    info.impedances.assign(channelCount, std::nan(""));
    for (auto& impedance : info.impedances) {
        if (read(text)) { impedance = ovChannelInfoDetail::parseImpedance(text); }
    }
    return info;
}
