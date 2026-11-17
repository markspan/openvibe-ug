// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.28. The LSL display's reading side: which streams are on the network, and the data and
// markers of those selected, on a thread of its own. No Qt, no player.

#pragma once

#include "CTraceBuffer.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/// An LSL stream on the network, as its stream info describes it.
struct SLSLStreamInfo
{
    std::string uid;       ///< Unique per stream; what the selection goes by
    std::string name;
    std::string type;
    std::string hostname;
    int channels = 0;
    double rate  = 0.0;    ///< Nominal rate in Hz; 0 for an irregular stream
    bool text    = false;  ///< A stream of strings

    /// Markers rather than data: a stream without a regular rate, or of text. Shown as flags, not traces.
    bool isMarkers() const { return rate <= 0.0 || text; }
};

/// A data stream that has been opened: what the view needs to draw it.
struct SLSLOpenedStream
{
    SLSLStreamInfo info;
    std::vector<std::string> labels;  ///< One per channel; "Ch<n>" where the stream names none
    std::vector<std::string> units;   ///< One per channel; empty where the stream gives none
    std::shared_ptr<CTraceBuffer> ring;
};

/// A marker, on the clock of the rings (lsl::local_clock()).
struct SLSLMarker
{
    double time = 0.0;
    std::string text;  ///< The marker's string, or its number
    uint64_t id = 0;   ///< Its number, or a hash of its string; picks its colour
};

/**
 * \brief Finds the LSL streams on the network, and reads those selected, on a thread of its own.
 *
 * Data streams go into a CTraceBuffer each, locked to one clock (CTraceBuffer::setTimeOrigin()), so that streams of any rate
 * line up. Their times are LSL's own, corrected to this computer's clock and smoothed (clock sync, dejitter), as LabRecorder's
 * files are when loaded. Marker streams give markers on the same clock. All methods but the destructor may be called from any
 * thread; the GUI collects what happened with the take*() methods.
 */
class CLSLReader
{
public:
    /// \param windowSeconds The time each ring holds, as the display shows it.
    explicit CLSLReader(double windowSeconds);
    ~CLSLReader();
    CLSLReader(const CLSLReader&)            = delete;
    CLSLReader& operator=(const CLSLReader&) = delete;

    /// The streams on the network now, as last looked up (about once a second).
    std::vector<SLSLStreamInfo> available() const;
    /// The streams to read, by uid; streams no longer listed are closed, new ones opened.
    void select(const std::set<std::string>& uids);
    /// The time held by the rings of streams opened from now on; the GUI changes the open rings itself.
    void setWindowSeconds(double seconds) { m_windowSeconds = seconds; }

    /// Data streams opened since the last call.
    std::vector<SLSLOpenedStream> takeOpened();
    /// Data streams closed since the last call, by uid.
    std::vector<std::string> takeClosed();
    /// Markers received since the last call, from all selected marker streams.
    std::vector<SLSLMarker> takeMarkers();

private:
    struct SInlet;

    void run();
    /// Pulls what a data stream has into its ring; whether anything came.
    bool readData(SInlet& inlet);
    /// Pulls what a marker stream has into m_markers; whether anything came.
    bool readMarkers(SInlet& inlet);

    double m_origin = 0.0;  ///< The clock time all rings are locked to
    std::atomic<double> m_windowSeconds;
    std::atomic<bool> m_running{true};

    mutable std::mutex m_mutex;  // guards everything below that the GUI reads or writes
    std::vector<SLSLStreamInfo> m_available;
    std::set<std::string> m_selected;
    std::vector<SLSLOpenedStream> m_opened;
    std::vector<std::string> m_closed;
    std::vector<SLSLMarker> m_markers;

    // Only the reader thread touches these
    std::map<std::string, std::unique_ptr<SInlet>> m_inlets;  ///< By uid
    std::thread m_thread;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
