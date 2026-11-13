// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see CLSLXDFWriter.h.

#include "CLSLXDFWriter.h"

#include "../CLSLRecorder.h"

#include <filesystem>
#include <map>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

CLSLXDFWriter::CLSLXDFWriter()  = default;
CLSLXDFWriter::~CLSLXDFWriter() = default;

bool CLSLXDFWriter::initialize()
{
    m_decoder.initialize(*this, 0);
    m_recordings = 0;

    const CString directory = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const CString filename  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    if (std::string(filename).empty())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "The XDF file has no name.\n";
        return false;
    }

    // As the BrainVision writer (UG): a filename with a full path replaces the directory, which is made if needed
    const std::filesystem::path path = std::filesystem::u8path(directory.toASCIIString()) / std::filesystem::u8path(filename.toASCIIString());
    std::error_code error;
    if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path(), error); }
    if (error)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not make the folder " << path.parent_path().generic_u8string() << ": "
                << error.message() << "\n";
        return false;
    }
    m_basePath = path.generic_u8string();
    this->getLogManager() << Kernel::LogLevel_Info << "Press Record in the LSL display to record its streams to " << m_basePath << "\n";
    return true;
}

bool CLSLXDFWriter::uninitialize()
{
    m_decoder.uninitialize();
    stopRecording();
    return true;
}

std::string CLSLXDFWriter::pathOf(const size_t recording) const
{
    // The first recording under the name set, the next with "-2", "-3", ... before the extension
    if (recording <= 1) { return m_basePath; }
    const std::filesystem::path base = std::filesystem::u8path(m_basePath);
    return (base.parent_path() / std::filesystem::u8path(base.stem().u8string() + "-" + std::to_string(recording) + base.extension().u8string()))
        .generic_u8string();
}

bool CLSLXDFWriter::startRecording(const std::map<std::string, std::vector<size_t>>& streams)
{
    m_path     = pathOf(++m_recordings);
    m_recorder = std::make_unique<CLSLRecorder>(m_path);
    if (!m_recorder->ok())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not create the XDF file " << m_path << "\n";
        m_recorder.reset();
        return false;
    }
    m_recorder->select(streams);
    this->getLogManager() << Kernel::LogLevel_Info << "Recording " << streams.size() << " LSL streams to " << m_path << "\n";
    return true;
}

void CLSLXDFWriter::stopRecording()
{
    if (!m_recorder) { return; }
    const bool ok = m_recorder->ok();
    m_recorder.reset();  // writes every stream's footer and closes the file
    if (ok) { this->getLogManager() << Kernel::LogLevel_Info << "Recording stopped; " << m_path << " closed\n"; }
    else { this->getLogManager() << Kernel::LogLevel_Error << "Could not write all of the XDF file " << m_path << ", e.g. because the disk is full\n"; }
}

bool CLSLXDFWriter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CLSLXDFWriter::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i)
    {
        m_decoder.decode(i);
        if (!m_decoder.isHeaderReceived()) { continue; }

        // The streams to record: the rows of the header, labelled with their LSL uids and their channels; none, an empty label,
        // when not recording (TypeId_LSLStreams)
        const CMatrix* matrix = m_decoder.getOutputMatrix();
        std::map<std::string, CLSLRecorder::Channels> streams;
        for (size_t row = 0; matrix->getDimensionCount() > 0 && row < matrix->getDimensionSize(0); ++row)
        {
            std::string uid;
            CLSLRecorder::Channels channels;
            if (CLSLRecorder::parseRecordLabel(matrix->getDimensionLabel(0, row), uid, channels)) { streams[uid] = channels; }
        }

        if (streams.empty()) { stopRecording(); }
        else if (!m_recorder) { startRecording(streams); }
        else { m_recorder->select(streams); }
    }
    if (m_recorder && !m_recorder->ok())
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not write to the XDF file " << m_path << ", e.g. because the disk is full; "
                << "the recording stops here.\n";
        return false;
    }
    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
