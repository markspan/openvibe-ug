// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.26. The Signal display's automatic vertical scale: which channels share a scale,
// and the scale itself, worked out from what each channel spans. Plain C++, so that it can be tested without Qt.

#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/** What a channel records, as far as its name tells. Scaling "By type" gives each kind but Other one scale. */
enum class EChannelKind { EEG, EOG, Digital, Other };

/**
 * \brief The kind of a channel, from its name.
 *
 * Digital: trigger, status and counter channels, and the TMSi digital and sawtooth channels (Trigger, SampleCounter,
 * Digi, DIG, Saw). EOG: any name with "EOG" in it (HEOG, VEOG, EOGL), and Neuroscan's HEO and VEO. Other: the
 * channels named for another body signal or an amplifier input (ECG, EMG, GSR, Resp, BIP1, AUX1, ...). Everything
 * else is taken to be EEG, as electrode names (Fp1, Cz, A12, ...) are too many to list.
 */
inline EChannelKind channelKind(const std::string& name)
{
    std::string n;
    n.reserve(name.size());
    for (const char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) { n += char(std::tolower(static_cast<unsigned char>(c))); }
    }
    const auto has      = [&n](const char* part) { return n.find(part) != std::string::npos; };
    const auto startsAs = [&n](const char* part) { return n.rfind(part, 0) == 0; };

    for (const char* part : { "trig", "dig", "status", "counter", "saw", "marker", "event", "button" }) {
        if (has(part)) { return EChannelKind::Digital; }
    }
    if (has("eog") || startsAs("heo") || startsAs("veo")) { return EChannelKind::EOG; }
    for (const char* part : { "ecg", "ekg", "emg", "gsr", "eda", "resp", "temp", "ppg", "pleth", "bip", "aux", "acc" }) {
        if (startsAs(part)) { return EChannelKind::Other; }
    }
    return EChannelKind::EEG;
}

/** Room kept above the largest excursion, before rounding up. */
constexpr double SCALE_HEADROOM = 1.05;

/**
 * \brief Rounds a half-range up to 1, 2 or 5 times a power of ten, so that the amplitude ruler reads ±50, ±200, ...
 *
 * A half-range that is zero, negative or not a number gives 1: a flat channel is drawn on its baseline.
 */
inline double niceHalfRange(const double half)
{
    if (!(half > 0.0) || !std::isfinite(half)) { return 1.0; }
    const double power = std::pow(10.0, std::floor(std::log10(half)));
    for (const double step : { 1.0, 2.0, 5.0 }) {
        // The tolerance keeps an exact 50 at 50, despite rounding in log10 and pow
        if (half <= step * power * (1.0 + 1e-9)) { return step * power; }
    }
    return 10.0 * power;
}

/** What one channel spans on the screen's page. */
struct SChannelSpan
{
    double lo    = 0.0;
    double hi    = 0.0;
    bool   valid = false;  ///< False when the channel has no number to show, e.g. all NaN
};

/** How one channel is drawn: the value on its baseline, and the value at the edge of its lane. */
struct SChannelScale
{
    double centre    = 0.0;
    double halfRange = 1.0;
};

/**
 * \brief The scale of each channel, from what it spans.
 *
 * Every channel is centred on the middle of its own span, so a DC offset never pushes it out of its lane. Channels
 * with the same group share one half-range: the largest of theirs, with headroom, rounded up by niceHalfRange().
 * A channel whose span is not valid gets the default SChannelScale, and does not count for its group.
 *
 * \param spans  one per channel
 * \param groups one per channel, the same size as spans
 */
inline std::vector<SChannelScale> scaleChannels(const std::vector<SChannelSpan>& spans, const std::vector<int>& groups)
{
    // The largest half-range in each group
    std::map<int, double> groupHalf;
    for (size_t i = 0; i < spans.size(); ++i) {
        if (spans[i].valid) {
            double& half = groupHalf[groups[i]];
            half         = std::max(half, 0.5 * (spans[i].hi - spans[i].lo));
        }
    }

    std::vector<SChannelScale> scales(spans.size());
    for (size_t i = 0; i < spans.size(); ++i) {
        if (!spans[i].valid) { continue; }
        scales[i].centre    = 0.5 * (spans[i].lo + spans[i].hi);
        scales[i].halfRange = niceHalfRange(groupHalf[groups[i]] * SCALE_HEADROOM);
    }
    return scales;
}

/**
 * \brief When a display works its scale out again: at every new page, and while the first page fills, twice a second.
 *
 * A page ends where the cursor starts again at the left in Scan mode, and every window length in Scroll mode. The scale
 * then fits the page just drawn, and stays put within a page. Shared by the Signal display and the LSL display, through
 * CSignalTraceItem::refresh().
 */
class CPageRescale
{
public:
    /// How often the scale is worked out while the first page fills, as it has no page before it to fit.
    static constexpr std::chrono::milliseconds FIRST_PAGE_INTERVAL{500};

    /// Works the scale out at the next due(), whatever the page: after a change of scaling, time scale or channels.
    void request() { m_requested = true; }

    /**
     * \brief Whether to work the scale out now; when it is, this counts as done.
     * \param page      The page the newest sample is on, counted from 0.
     * \param firstPage True while the first page is still filling.
     * \param now       The present, on the steady clock.
     */
    bool due(const uint64_t page, const bool firstPage, const std::chrono::steady_clock::time_point now)
    {
        const bool firstPageTime = firstPage && now - m_last >= FIRST_PAGE_INTERVAL;
        if (!m_requested && page == m_page && !firstPageTime) { return false; }
        m_requested = false;
        m_page      = page;
        m_last      = now;
        return true;
    }

private:
    bool m_requested = true;  ///< The first call is always due
    uint64_t m_page  = 0;
    std::chrono::steady_clock::time_point m_last{};
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
