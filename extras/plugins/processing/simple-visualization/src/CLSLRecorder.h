// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.29. Records LSL streams into an XDF file, as LabRecorder does, on a thread of its own.

#pragma once

#include "XDFWriter.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/**
 * \brief Records the selected LSL streams into one XDF file, as LabRecorder does.
 *
 * Each stream gets inlets of its own, without LSL's corrections: the file holds the senders' own time stamps, every sample's,
 * and the clock offset of each stream measured every 5 s, so that load_xdf and pyxdf correct them as they do LabRecorder's
 * files. A stream selected while recording starts in the file then, with its StreamHeader; one no longer selected ends there,
 * with its StreamFooter, and starts again, under a new stream id, if it is selected again. Boundary chunks every 10 s. The
 * file is written out chunk by chunk, so a crash loses no more than the last moments.
 */
class CLSLRecorder
{
public:
    /// Creates the file; see ok().
    explicit CLSLRecorder(const std::string& pathUtf8);
    /// Ends every stream with its footer, and closes the file.
    ~CLSLRecorder();
    CLSLRecorder(const CLSLRecorder&)            = delete;
    CLSLRecorder& operator=(const CLSLRecorder&) = delete;

    /// Whether the file could be created, and everything since reached it.
    bool ok() const { return m_writer.good(); }
    /// The streams to record, by LSL uid.
    void select(const std::set<std::string>& uids);
    /// How many streams are being recorded now.
    size_t recording() const { return m_recording; }

private:
    struct SStream;

    void run();
    /// Pulls what a stream has and writes it; whether anything came.
    bool record(SStream& stream);
    void end(SStream& stream);

    CXDFWriter m_writer;
    std::atomic<bool> m_running{true};
    std::atomic<size_t> m_recording{0};

    std::mutex m_mutex;  // guards m_selected
    std::set<std::string> m_selected;

    // Only the recording thread touches these
    std::map<std::string, std::unique_ptr<SStream>> m_streams;  ///< By uid
    uint32_t m_nextId = 1;
    std::thread m_thread;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
