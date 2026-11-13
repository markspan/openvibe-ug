// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see XDFWriter.h.

#include "XDFWriter.h"

#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {
// The chunk tags of the XDF specification
constexpr uint16_t TAG_FILE_HEADER   = 1;
constexpr uint16_t TAG_STREAM_HEADER = 2;
constexpr uint16_t TAG_SAMPLES       = 3;
constexpr uint16_t TAG_CLOCK_OFFSET  = 4;
constexpr uint16_t TAG_BOUNDARY      = 5;
constexpr uint16_t TAG_STREAM_FOOTER = 6;

/// The 16 bytes a Boundary chunk holds, which a reader scans for to find its way again
constexpr unsigned char BOUNDARY[16] = { 0x43, 0xA5, 0x46, 0xDC, 0xCB, 0xF5, 0x41, 0x0F, 0xB3, 0x0E, 0xD5, 0x46, 0x73, 0x83, 0xCB, 0xE4 };

template <class T>
void appendRaw(std::string& out, const T value)
{
    char bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));
    out.append(bytes, sizeof(T));
}

/// The current time as the file header's datetime, e.g. "2026-11-13T14:05:09+0100"
std::string isoNow()
{
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream text;
    text << std::put_time(&local, "%Y-%m-%dT%H:%M:%S%z");
    return text.str();
}
}  // namespace

void CXDFWriter::appendVarLen(std::string& out, const uint64_t value)
{
    if (value <= 0xFF)
    {
        out.push_back(char(1));
        out.push_back(char(uint8_t(value)));
    }
    else if (value <= 0xFFFFFFFFULL)
    {
        out.push_back(char(4));
        appendRaw(out, uint32_t(value));
    }
    else
    {
        out.push_back(char(8));
        appendRaw(out, value);
    }
}

bool CXDFWriter::open(const std::string& pathUtf8)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_file.open(std::filesystem::u8path(pathUtf8), std::ios::binary | std::ios::trunc);
    if (!m_file.good()) { return false; }
    m_file.write("XDF:", 4);
    // Not under writeChunk(), which takes the lock this holds
    const std::string xml = "<?xml version=\"1.0\"?><info><version>1.0</version><datetime>" + isoNow() + "</datetime></info>";
    std::string chunk;
    appendVarLen(chunk, 2 + xml.size());
    appendRaw(chunk, TAG_FILE_HEADER);
    chunk += xml;
    m_file.write(chunk.data(), std::streamsize(chunk.size()));
    m_file.flush();
    return m_file.good();
}

void CXDFWriter::close()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) { m_file.close(); }
}

bool CXDFWriter::good() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_file.is_open() && m_file.good();
}

void CXDFWriter::writeChunk(const uint16_t tag, const std::string& content)
{
    std::string chunk;
    appendVarLen(chunk, 2 + content.size());  // the length counts the tag
    appendRaw(chunk, tag);
    chunk += content;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file.is_open()) { return; }
    m_file.write(chunk.data(), std::streamsize(chunk.size()));
    m_file.flush();  // on disk at once, so that a crash loses no more than the chunk being written
}

void CXDFWriter::writeStreamHeader(const uint32_t streamId, const std::string& xml)
{
    std::string content;
    appendRaw(content, streamId);
    content += xml;
    writeChunk(TAG_STREAM_HEADER, content);
}

void CXDFWriter::writeSamples(const uint32_t streamId, const double* stamps, const size_t count, const void* values, const size_t bytesPerSample)
{
    if (count == 0) { return; }
    std::string content;
    content.reserve(4 + 9 + count * (9 + bytesPerSample));
    appendRaw(content, streamId);
    appendVarLen(content, count);
    const char* bytes = static_cast<const char*>(values);
    for (size_t s = 0; s < count; ++s)
    {
        // Every sample with its own time stamp, as LabRecorder writes them
        content.push_back(char(8));
        appendRaw(content, stamps[s]);
        content.append(bytes + s * bytesPerSample, bytesPerSample);
    }
    writeChunk(TAG_SAMPLES, content);
}

void CXDFWriter::writeTextSamples(const uint32_t streamId, const double* stamps, const size_t count, const std::string* values, const size_t channels)
{
    if (count == 0) { return; }
    std::string content;
    appendRaw(content, streamId);
    appendVarLen(content, count);
    for (size_t s = 0; s < count; ++s)
    {
        content.push_back(char(8));
        appendRaw(content, stamps[s]);
        for (size_t c = 0; c < channels; ++c)
        {
            const std::string& value = values[s * channels + c];
            appendVarLen(content, value.size());
            content += value;
        }
    }
    writeChunk(TAG_SAMPLES, content);
}

void CXDFWriter::writeClockOffset(const uint32_t streamId, const double collectionTime, const double offset)
{
    std::string content;
    appendRaw(content, streamId);
    appendRaw(content, collectionTime);
    appendRaw(content, offset);
    writeChunk(TAG_CLOCK_OFFSET, content);
}

void CXDFWriter::writeBoundary() { writeChunk(TAG_BOUNDARY, std::string(reinterpret_cast<const char*>(BOUNDARY), sizeof(BOUNDARY))); }

void CXDFWriter::writeStreamFooter(const uint32_t streamId, const std::string& xml)
{
    std::string content;
    appendRaw(content, streamId);
    content += xml;
    writeChunk(TAG_STREAM_FOOTER, content);
}

std::string CXDFWriter::footerXml(const double firstTimestamp, const double lastTimestamp, const uint64_t sampleCount,
                                  const std::vector<std::pair<double, double>>& clockOffsets)
{
    std::ostringstream xml;
    xml << std::setprecision(17) << "<?xml version=\"1.0\"?><info><first_timestamp>" << firstTimestamp << "</first_timestamp><last_timestamp>"
        << lastTimestamp << "</last_timestamp><sample_count>" << sampleCount << "</sample_count><clock_offsets>";
    for (const auto& [time, value] : clockOffsets) { xml << "<offset><time>" << time << "</time><value>" << value << "</value></offset>"; }
    xml << "</clock_offsets></info>";
    return xml.str();
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
