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
 *     place on the head (EOG, ECG, ...).
 *
 * A client that reads the names only, as the fork's first versions do, does not see the rest; a server that
 * sends the names only leaves the montage empty.
 *
 * In Designer, the Acquisition client box passes the montage on to the other boxes of the scenario, the BrainVision
 * writer, as two configuration tokens of the run (OV_CHANNEL_INFO_MONTAGE_TOKEN and OV_CHANNEL_INFO_POSITIONS_TOKEN):
 * a box between the two, a channel selector or a filter, does not lose them.
 */

#include <cstddef>
#include <locale>
#include <map>
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
};

/// Designer configuration token with the montage's file name, empty when the acquisition has none.
constexpr const char* OV_CHANNEL_INFO_MONTAGE_TOKEN = "AcquisitionServer_Montage";
/// Designer configuration token with the electrode positions, see encodeElectrodePositions().
constexpr const char* OV_CHANNEL_INFO_POSITIONS_TOKEN = "AcquisitionServer_ElectrodePositions";

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
    if (info.montage.empty()) { return block; }

    block += info.montage;
    block += '\0';
    for (size_t i = 0; i < channelCount; ++i) {
        if (i < info.positions.size() && info.positions[i].known) { block += ovChannelInfoDetail::formatPosition(info.positions[i]); }
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

    if (!read(info.montage) || info.montage.empty()) { return info; }
    info.positions.resize(channelCount);
    std::string text;
    for (auto& position : info.positions) {
        if (read(text)) { position = ovChannelInfoDetail::parsePosition(text); }
    }
    return info;
}

/// The positions of the channels the montage places, for OV_CHANNEL_INFO_POSITIONS_TOKEN: a line per channel, its
/// name and its position separated by a tab.
inline std::string encodeElectrodePositions(const ovChannelInfo& info)
{
    std::string text;
    for (size_t i = 0; i < info.names.size() && i < info.positions.size(); ++i) {
        if (!info.positions[i].known) { continue; }
        text += info.names[i] + '\t' + ovChannelInfoDetail::formatPosition(info.positions[i]) + '\n';
    }
    return text;
}

/// The positions in OV_CHANNEL_INFO_POSITIONS_TOKEN, by channel name.
inline std::map<std::string, ovChannelInfo::Position> decodeElectrodePositions(const std::string& text)
{
    std::map<std::string, ovChannelInfo::Position> positions;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const size_t tab = line.rfind('\t');
        if (tab == std::string::npos) { continue; }
        const ovChannelInfo::Position position = ovChannelInfoDetail::parsePosition(line.substr(tab + 1));
        if (position.known) { positions.emplace(line.substr(0, tab), position); }
    }
    return positions;
}
