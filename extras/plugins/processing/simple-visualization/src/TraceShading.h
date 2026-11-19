// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.40.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/**
 * \brief How the traces are drawn: the panel's "Trace shading", in the Signal display and the LSL display.
 *
 * A trace with more samples than pixel columns is drawn as the lowest and highest sample of each column, so that no
 * spike is lost. Drawn opaque, as upstream did, a noisy signal becomes a solid band with hard edges.
 */
enum class ETraceShading
{
    /**
     * Each pixel column of a dense trace is split into up to MAX_SUB_COLUMNS sub-columns, drawn see-through, so that
     * the trace is darker where it passes more often, as on an analogue oscilloscope. A trace with fewer samples than
     * pixels is drawn opaque, as before.
     */
    Density,
    /** Every trace see-through, TRANSPARENT_OPACITY, whatever its density. */
    Transparent
};

constexpr std::array<ETraceShading, 2> TRACE_SHADINGS = { ETraceShading::Density, ETraceShading::Transparent };

/** The panel entry, in both displays. */
constexpr const char* TRACE_SHADING_NAME = "Trace shading";
constexpr const char* TRACE_SHADING_HELP =
    "Density shading: a trace with more samples than pixels is darker where it passes more often, so that the spread of "
    "the signal shows. Transparent: every trace see-through, so that overlapping traces show through each other";

/** The most sub-columns a pixel column is split into: as many as the samples of the antialiasing layer. */
constexpr int MAX_SUB_COLUMNS = 4;
/** How opaque a pixel ends up where every sub-column of a dense trace passes, with Density. */
constexpr double DENSITY_FULL_OPACITY = 0.95;
/** How opaque a trace is with Transparent. */
constexpr double TRANSPARENT_OPACITY = 0.6;

inline const char* toString(const ETraceShading v) { return v == ETraceShading::Transparent ? "Transparent" : "Density shading"; }

inline bool parseTraceShading(const std::string& s, ETraceShading& out)
{
    for (const ETraceShading v : TRACE_SHADINGS)
    {
        if (s == toString(v))
        {
            out = v;
            return true;
        }
    }
    return false;
}

/**
 * \brief The sub-columns each pixel column of a dense trace is split into.
 *
 * One per whole sample in the column, up to MAX_SUB_COLUMNS, so that each sub-column still holds a sample: with more
 * sub-columns than samples, the empty ones would repeat a sample and draw steps. Always 1 with Transparent.
 *
 * \param samplesPerPixel The samples of the trace per device pixel column
 */
inline int subColumnsPerPixel(const ETraceShading shading, const double samplesPerPixel)
{
    if (shading != ETraceShading::Density || !(samplesPerPixel >= 1.0)) { return 1; }
    // Compared before the conversion, as converting a huge or infinite number to int is undefined
    if (samplesPerPixel >= double(MAX_SUB_COLUMNS)) { return MAX_SUB_COLUMNS; }
    return int(std::floor(samplesPerPixel));
}

/**
 * \brief How opaque each line of a trace is drawn.
 *
 * With Density, the lines of the k sub-columns of a pixel each cover that pixel, as a line is one pixel wide. Where all
 * k pass, the pixel reaches 1 - (1 - a)^k; a is chosen so that this is DENSITY_FULL_OPACITY, and a pixel only some of
 * them pass stays lighter. With one sub-column, opaque.
 */
inline double traceOpacity(const ETraceShading shading, const int subColumns)
{
    if (shading == ETraceShading::Transparent) { return TRANSPARENT_OPACITY; }
    if (subColumns <= 1) { return 1.0; }
    return 1.0 - std::pow(1.0 - DENSITY_FULL_OPACITY, 1.0 / double(subColumns));
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
