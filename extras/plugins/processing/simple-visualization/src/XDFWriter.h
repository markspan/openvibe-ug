// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.29. Writes XDF files as LabRecorder writes them. Plain C++: no LSL, no Qt.

#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/**
 * \brief Writes an XDF file (https://github.com/sccn/xdf/wiki/Specifications), chunk by chunk, as LabRecorder does.
 *
 * The file starts with "XDF:" and a FileHeader chunk. Each stream then has a StreamHeader chunk with its LSL stream info,
 * Samples chunks with every sample's own time stamp, ClockOffset chunks, and at its end a StreamFooter chunk. Boundary chunks
 * let a reader find its way again in a damaged file. Time stamps are the sender's own, uncorrected: load_xdf and pyxdf correct
 * them from the clock offsets when they load the file, as they do for LabRecorder's files.
 *
 * All methods may be called from any thread; each chunk is written whole. Numbers are written little-endian, as on the PCs
 * this runs on.
 */
class CXDFWriter
{
public:
    CXDFWriter() = default;
    ~CXDFWriter() { close(); }
    CXDFWriter(const CXDFWriter&)            = delete;
    CXDFWriter& operator=(const CXDFWriter&) = delete;

    /// Creates the file, and writes its start and its FileHeader chunk. False if it can not be written.
    bool open(const std::string& pathUtf8);
    void close();
    /// Whether everything written so far reached the file.
    bool good() const;

    /// \param xml The stream's info as LSL gives it, stream_info::as_xml().
    void writeStreamHeader(uint32_t streamId, const std::string& xml);
    /**
     * \brief A Samples chunk of numbers.
     * \param values        \p count samples one after the other, each \p bytesPerSample bytes: its channels, little-endian.
     */
    void writeSamples(uint32_t streamId, const double* stamps, size_t count, const void* values, size_t bytesPerSample);
    /// A Samples chunk of text: \p values holds \p count samples of \p channels strings each, one sample after the other.
    void writeTextSamples(uint32_t streamId, const double* stamps, size_t count, const std::string* values, size_t channels);
    /**
     * \param collectionTime When the offset was measured, on the sender's clock (local clock minus the offset), as LabRecorder
     *                       writes it.
     * \param offset         What to add to the sender's time stamps to get this computer's, stream_inlet::time_correction().
     */
    void writeClockOffset(uint32_t streamId, double collectionTime, double offset);
    void writeBoundary();
    void writeStreamFooter(uint32_t streamId, const std::string& xml);

    /// The XML of a StreamFooter chunk, as LabRecorder writes it.
    static std::string footerXml(double firstTimestamp, double lastTimestamp, uint64_t sampleCount,
                                 const std::vector<std::pair<double, double>>& clockOffsets);

    /// XDF's variable-length integer: a byte saying how many bytes follow, 1, 4 or 8, then the number.
    static void appendVarLen(std::string& out, uint64_t value);

private:
    void writeChunk(uint16_t tag, const std::string& content);

    mutable std::mutex m_mutex;
    std::ofstream m_file;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
