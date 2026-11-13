// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <algorithm>  // UG fork
#include <array>  // UG fork
#include <cctype>
#include <string>

#include "defines.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/** How lanes are distributed over the item height. */
enum class ELayoutMode
{
    Stacked,   ///< one lane per channel, baselines spread over the height
    Butterfly  ///< all baselines collapse to mid-height (the box's "Multiview" setting)
};

/**
 * \brief The display configuration, as one plain value.
 *
 * Seeded from the box settings in CSignalDisplay::initialize(), then mutated in place by the
 * runtime parameter panel. Both write the same fields, so the two cannot drift apart.
 *
 * It stays a plain struct rather than a set of QObject properties in order to reduce overhead
 * passing them around.
 */
struct SDisplaySettings
{
    ESignalDisplayMode    timeMode            = ESignalDisplayMode::Scan;
    ESignalDisplayScaling scaling             = ESignalDisplayScaling::PerChannel;
    // UG fork: no scale refresh interval; an automatic scale is worked out again for every new page
    /** Signal units filling half a lane. Only consulted when scaling == None. */
    double                verticalScale       = 100.0;
    /** DC level subtracted from every sample before scaling. Only consulted when scaling == None. */
    double                baselineOffset      = 0.0;
    double                timeScaleSeconds    = 10.0;
    bool                  showTimeRuler       = true;
    bool                  showAmplitudeRuler  = true;
    ELayoutMode           layout              = ELayoutMode::Stacked;

    /**
     * \brief Lowercases and drops every non-alphanumeric character.
     *
     * Scenarios on disk have carried both "Per channel" and "Per Channel" over the years.
     * Normalising both ends makes the spelling a non-issue.
     */
    static std::string normalize(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (const char c : s)
        {
            if (std::isalnum(static_cast<unsigned char>(c))) { out += char(std::tolower(static_cast<unsigned char>(c))); }
        }
        return out;
    }

    /**
     * \brief Maps a display-mode name back to the enum.
     *
     * Used for the runtime panel, which hands back the selected string.
     */
    static bool parseTimeMode(const std::string& s, ESignalDisplayMode& out)
    {
        const std::string n = normalize(s);
        if (n == "scroll")
        {
            out = ESignalDisplayMode::Scroll;
            return true;
        }
        if (n == "scan")
        {
            out = ESignalDisplayMode::Scan;
            return true;
        }
        return false;
    }

    /** Maps a scaling-mode name back to the enum. */
    static bool parseScaling(const std::string& s, ESignalDisplayScaling& out)
    {
        const std::string n = normalize(s);
        if (n == "perchannel")
        {
            out = ESignalDisplayScaling::PerChannel;
            return true;
        }
        if (n == "bytype")  // UG fork
        {
            out = ESignalDisplayScaling::ByType;
            return true;
        }
        if (n == "global")
        {
            out = ESignalDisplayScaling::Global;
            return true;
        }
        if (n == "none")
        {
            out = ESignalDisplayScaling::None;
            return true;
        }
        return false;
    }

    /**
     * UG fork: every value, in the order the box setting and the panel list them. main.cpp registers the box
     * setting's entries from these and toString(), and the panel fills its lists from them too, so the names are
     * spelled in one place only.
     */
    static constexpr std::array<ESignalDisplayMode, 2> TIME_MODES = { ESignalDisplayMode::Scroll, ESignalDisplayMode::Scan };
    static constexpr std::array<ESignalDisplayScaling, 4> SCALINGS = {
        ESignalDisplayScaling::PerChannel, ESignalDisplayScaling::ByType, ESignalDisplayScaling::Global, ESignalDisplayScaling::None
    };

    /** The name of a value, as the box setting and the panel show it. */
    static const char* toString(const ESignalDisplayMode v) { return v == ESignalDisplayMode::Scroll ? "Scroll" : "Scan"; }

    static const char* toString(const ESignalDisplayScaling v)
    {
        switch (v)
        {
            case ESignalDisplayScaling::PerChannel: return "Per channel";
            case ESignalDisplayScaling::ByType: return "By type";  // UG fork
            case ESignalDisplayScaling::Global: return "Global";
            default: return "None";
        }
    }

    /**
     * \brief Sanity-checks the numbers.
     *
     * Still needed after expansion: a configuration token can resolve to anything.
     */
    bool validate(std::string& error) const
    {
        // UG fork: a value the box setting does not list, from a hand-edited scenario or a newer build, was used as it
        // was: it scaled as Per channel while the panel showed None
        if (std::find(TIME_MODES.begin(), TIME_MODES.end(), timeMode) == TIME_MODES.end())
        {
            error = "Display Mode is not one of Scroll and Scan";
            return false;
        }
        if (std::find(SCALINGS.begin(), SCALINGS.end(), scaling) == SCALINGS.end())
        {
            error = "Auto vertical scale is not one of Per channel, By type, Global and None";
            return false;
        }
        if (verticalScale <= 0.0)
        {
            error = "Vertical Scale must be > 0";
            return false;
        }
        if (timeScaleSeconds <= 0.0)
        {
            error = "Time Scale must be > 0";
            return false;
        }
        return true;
    }
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
