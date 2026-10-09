// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

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
    double                scaleRefreshSeconds = 5.0;
    /** Signal units filling half a lane. Only consulted when scaling == None. */
    double                verticalScale       = 100.0;
    /** DC level subtracted from every sample before scaling. */
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

    /** Spelled as registered in main.cpp, so the panel's dropdown matches the box setting. */
    static const char* toString(const ESignalDisplayMode v) { return v == ESignalDisplayMode::Scroll ? "Scroll" : "Scan"; }

    static const char* toString(const ESignalDisplayScaling v)
    {
        switch (v)
        {
            case ESignalDisplayScaling::PerChannel: return "Per channel";
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
        if (scaleRefreshSeconds < 0.0)
        {
            error = "Scale refresh interval (secs) must be >= 0";
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
