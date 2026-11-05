///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmOVCSVFileReader.cpp
/// \brief Classes of the box CSV File Reader.
/// \author Victor Herlin (Mensia), Thomas Prampart (Inria).
/// \version 1.2.0
/// \date 07/05/2021
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program. If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmOVCSVFileReader.hpp"

#include <sstream>
#include <map>
#include <algorithm>
#include <regex>
#include <utility>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileReader::initialize()
{
    m_sampling         = 0;
    m_isHeaderSent     = false;
    m_isStimHeaderSent = false;
    m_nSamplePerBuffer = 1;

    this->getStaticBoxContext().getOutputType(0, m_typeID);

    // Get Header
    const CString filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    OV_ERROR_UNLESS_KRF(m_readerLib->openFile(filename.toASCIIString(), CSV::EFileAccessMode::Read),
                        // UG fork: names the file and the setting. See UG.md, 2.23.
                        "the file [" << filename << "] from this box's setting \"Filename\" could not be opened. Paths need forward slashes, as a backslash is lost on the way to a box. "
                        "Perhaps it does not exist, or the path is misspelt. Would you be so very kind as to check it? Thank you for your patience."
                        << (m_readerLib->getLastErrorString().empty() ? "" : " Details: " + m_readerLib->getLastErrorString()), Kernel::ErrorType::Internal);

    OV_ERROR_UNLESS_KRF(m_readerLib->parseHeader(), (CSV::ICSVHandler::getLogError(m_readerLib->getLastLogError()) + (m_readerLib->getLastErrorString().empty()
                            ? "" : ". Details: " + m_readerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

    // Initialize encoder
    if (m_typeID == OV_TypeId_Signal && m_readerLib->getFormatType() == CSV::EStreamType::Signal) {
        m_algorithmEncoder = new Toolkit::TSignalEncoder<CBoxAlgorithmOVCSVFileReader>(*this, 0);
        m_readerLib->getSignalInformation(m_channelNames, m_sampling, m_nSamplePerBuffer);
    }
    else if ((m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_CovarianceMatrix)
             && (m_readerLib->getFormatType() == CSV::EStreamType::StreamedMatrix || m_readerLib->getFormatType() == CSV::EStreamType::FeatureVector)) {
        m_algorithmEncoder = new Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmOVCSVFileReader>(*this, 0);
        m_readerLib->getStreamedMatrixInformation(m_dimSizes, m_channelNames);
    }
    else if (m_typeID == OV_TypeId_FeatureVector && m_readerLib->getFormatType() == CSV::EStreamType::FeatureVector) {
        m_algorithmEncoder = new Toolkit::TFeatureVectorEncoder<CBoxAlgorithmOVCSVFileReader>(*this, 0);
        m_readerLib->getFeatureVectorInformation(m_channelNames);
    }
    else if (m_typeID == OV_TypeId_Spectrum && m_readerLib->getFormatType() == CSV::EStreamType::Spectrum) {
        m_algorithmEncoder = new Toolkit::TSpectrumEncoder<CBoxAlgorithmOVCSVFileReader>(*this, 0);
        m_readerLib->getSpectrumInformation(m_channelNames, m_frequencyAbscissa, m_sampling);
    }
    else if (m_typeID == OV_TypeId_Stimulations && m_readerLib->getFormatType() == CSV::EStreamType::Stimulations) {
        this->getLogManager() << Kernel::LogLevel_Info << "File contains only stimulations\n";
    }
    else {
        this->getLogManager() << Kernel::LogLevel_Error
                << "File content type [" << CSV::toString(m_readerLib->getFormatType())
                << "] not matching box output [" << this->getTypeManager().getTypeName(m_typeID) << "].\n";
        return false;
    }

    transformLabels();
    m_stimIdx = m_typeID == OV_TypeId_Stimulations ? 0 : 1;
    OV_ERROR_UNLESS_KRF(m_stimEncoder.initialize(*this, m_stimIdx), "Error during stimulation encoder initialize", Kernel::ErrorType::Internal);

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileReader::uninitialize()
{
    m_channelNames.clear();
    m_dimSizes.clear();
    m_frequencyAbscissa.clear();

    m_algorithmEncoder.uninitialize();

    OV_ERROR_UNLESS_KRF(m_stimEncoder.uninitialize(), "Failed to uninitialize stimulation encoder", Kernel::ErrorType::Internal);

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileReader::processClock(Kernel::CMessageClock& /*msg*/)
{
    OV_ERROR_UNLESS_KRF(getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(), "Failed to mark clock algorithm as ready to process",
                        Kernel::ErrorType::Internal);
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileReader::process()
{
    if (m_readerLib->getFormatType() == CSV::EStreamType::Stimulations) {
        // If the file contains only stimulations
        const double currentTime = CTime(this->getPlayerContext().getCurrentTime()).toSeconds();

        if (!m_readerLib->hasDataToRead() && m_savedStims.empty()) { return true; }
        if (m_savedStims.empty()) {
            std::vector<CSV::SStimulationChunk> chunk;
            OV_ERROR_UNLESS_KRF(m_readerLib->readEventsFromFile(5, chunk),
                                (CSV::ICSVHandler::getLogError(m_readerLib->getLastLogError()) + (m_readerLib->getLastErrorString().empty()
                                    ? "" : ". Details: " + m_readerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
            m_savedStims.insert(m_savedStims.end(), chunk.begin(), chunk.end());
        }

        return processStimulation(CTime(m_lastStimDate).toSeconds(), currentTime);
    }

    // If the file contains data chunks and potentially stimulations.
    if (m_readerLib->getFormatType() != CSV::EStreamType::Undefined) { return processChunksAndStimulations(); }

    // Undefined Stream Type.
    this->getLogManager() << Kernel::LogLevel_Error << "Cannot process file with undefined format\n";
    return false;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileReader::processChunksAndStimulations()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    CMatrix* matrix            = m_algorithmEncoder.getInputMatrix();

    // encode Header if not already encoded
    if (!m_isHeaderSent) {
        if (m_typeID == OV_TypeId_Signal) {
            matrix->resize(m_channelNames.size(), m_nSamplePerBuffer);
            size_t index = 0;

            for (const std::string& name : m_channelNames) { matrix->setDimensionLabel(0, index++, name); }

            m_algorithmEncoder.getInputSamplingRate() = m_sampling;
        }
        else if (m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_CovarianceMatrix) {
            OV_ERROR_UNLESS_K(!m_dimSizes.empty(), "Failed to set dimension count", Kernel::ErrorType::Internal, false);
            matrix->setDimensionCount(m_dimSizes.size());
            size_t prevDimSize = 0;

            for (size_t d1 = 0; d1 < m_dimSizes.size(); ++d1) {
                matrix->setDimensionSize(d1, m_dimSizes[d1]);
                for (size_t d2 = 0; d2 < m_dimSizes[d1]; ++d2) { matrix->setDimensionLabel(d1, d2, m_channelNames[prevDimSize + d2]); }
                prevDimSize += m_dimSizes[d1];
            }
        }
        else if (m_typeID == OV_TypeId_FeatureVector) {
            matrix->resize(m_channelNames.size());

            size_t index = 0;
            for (const std::string& name : m_channelNames) { matrix->setDimensionLabel(0, index++, name); }
        }
        else if (m_typeID == OV_TypeId_Spectrum) {
            CMatrix* frequencyAbscissaMatrix = m_algorithmEncoder.getInputFrequencyAbcissa();

            matrix->resize(m_channelNames.size(), m_frequencyAbscissa.size());
            frequencyAbscissaMatrix->resize(m_frequencyAbscissa.size());

            size_t index = 0;
            for (const std::string& name : m_channelNames) { matrix->setDimensionLabel(0, index++, name); }

            index = 0;
            for (const double& value : m_frequencyAbscissa) {
                frequencyAbscissaMatrix->getBuffer()[index] = value;
                matrix->setDimensionLabel(1, index++, std::to_string(value));
            }

            m_algorithmEncoder.getInputSamplingRate() = m_sampling;
        }

        OV_ERROR_UNLESS_KRF(m_algorithmEncoder.encodeHeader(), "Failed to encode signal header", Kernel::ErrorType::Internal);

        m_isHeaderSent = true;
        OV_ERROR_UNLESS_KRF(boxContext.markOutputAsReadyToSend(0, 0, 0), "Failed to mark signal header as ready to send", Kernel::ErrorType::Internal);
    }
    const double currentTime = CTime(this->getPlayerContext().getCurrentTime()).toSeconds();

    if (!m_readerLib->hasDataToRead() && m_savedChunks.empty()) { return true; }

    // Fill the chunk buffer if there is no enough data.
    if ((m_savedChunks.empty() || m_savedChunks.back().startTime < currentTime) && m_readerLib->hasDataToRead()) {
        do {
            std::vector<CSV::SMatrixChunk> matrixChunk;
            std::vector<CSV::SStimulationChunk> stimChunk;

            OV_ERROR_UNLESS_KRF(m_readerLib->readSamplesAndEventsFromFile(1, matrixChunk, stimChunk),
                                (CSV::ICSVHandler::getLogError(m_readerLib->getLastLogError()) + (m_readerLib->getLastErrorString().empty()
                                    ? "" : ". Details: " + m_readerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

            m_savedChunks.insert(m_savedChunks.end(), matrixChunk.begin(), matrixChunk.end());
            m_savedStims.insert(m_savedStims.end(), stimChunk.begin(), stimChunk.end());
        } while (!m_savedChunks.empty() && m_savedChunks.back().startTime < currentTime && m_readerLib->hasDataToRead());
    }

    if (!m_savedChunks.empty()) {
        const double chunkStartTime = m_savedChunks.cbegin()->startTime;
        const double chunkEndTime   = m_savedChunks.back().endTime;

        // send stimulations chunk even if there is no stimulations, chunks have to be continued
        OV_ERROR_UNLESS_KRF(this->processStimulation(chunkStartTime, chunkEndTime), "Error during stimulation process", Kernel::ErrorType::Internal);

        size_t chunksToRemove = 0;

        for (const CSV::SMatrixChunk& chunk : m_savedChunks) {
            if (currentTime > chunk.startTime) {
                // move read matrix into buffer to encode
                std::move(chunk.matrix.begin(), chunk.matrix.end(), matrix->getBuffer());

                OV_ERROR_UNLESS_KRF(m_algorithmEncoder.encodeBuffer(), "Failed to encode signal buffer", Kernel::ErrorType::Internal);

                OV_ERROR_UNLESS_KRF(boxContext.markOutputAsReadyToSend(0, CTime(chunk.startTime).time(), CTime(chunk.endTime).time()),
                                    "Failed to mark signal output as ready to send", Kernel::ErrorType::Internal);

                chunksToRemove++;
            }
            else { break; }
        }

        // If there is no more data to send, we push the end.
        if (m_savedChunks.size() == chunksToRemove && !m_readerLib->hasDataToRead()) {
            OV_ERROR_UNLESS_KRF(m_algorithmEncoder.encodeEnd(), "Failed to encode end.", Kernel::ErrorType::Internal);

            OV_ERROR_UNLESS_KRF(boxContext.markOutputAsReadyToSend(0, CTime(m_savedChunks.back().startTime).time(), CTime(m_savedChunks.back().endTime).time()),
                                "Failed to mark signal output as ready to send", Kernel::ErrorType::Internal);
        }

        if (chunksToRemove != 0) { m_savedChunks.erase(m_savedChunks.begin(), m_savedChunks.begin() + chunksToRemove); }
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileReader::processStimulation(const double startTime, const double endTime)
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();
    if (!m_isStimHeaderSent) {
        OV_ERROR_UNLESS_KRF(m_stimEncoder.encodeHeader(), "Failed to encode stimulation header", Kernel::ErrorType::Internal);
        m_isStimHeaderSent = true;

        OV_ERROR_UNLESS_KRF(boxCtx.markOutputAsReadyToSend(m_stimIdx, 0, 0), "Failed to mark stimulation header as ready to send",
                            Kernel::ErrorType::Internal);
    }

    CStimulationSet* stimSet = m_stimEncoder.getInputStimulationSet();
    stimSet->clear();

    const uint64_t stimChunkStartTime = m_lastStimDate;
    const uint64_t currentTime        = getPlayerContext().getCurrentTime();

    if (m_savedStims.empty()) {
        if (currentTime > m_lastStimDate) {
            m_lastStimDate = currentTime;

            OV_ERROR_UNLESS_KRF(m_stimEncoder.encodeBuffer(), "Failed to encode stimulation buffer", Kernel::ErrorType::Internal);

            OV_ERROR_UNLESS_KRF(boxCtx.markOutputAsReadyToSend(m_stimIdx, stimChunkStartTime, currentTime),
                                "Failed to mark stimulation output as ready to send", Kernel::ErrorType::Internal);
        }
    }
    else {
        auto it = m_savedStims.begin();
        for (; it != m_savedStims.end(); ++it) {
            const double date = it->date;

            if (startTime <= date && date <= endTime) {
                stimSet->push_back(it->id, CTime(it->date).time(), CTime(it->duration).time());
                m_lastStimDate = CTime(it->date).time();
            }
            else {
                // stimulation is in the future of the current time frame. Stop looping
                if (startTime < date) { break; }
                // Stimulation is in the past of the current time frame, we can discard it
                const std::string message = "The stimulation is not synced with the stream and will be ignored: [Value: "
                                            + std::to_string(it->id) + " | Date: " + std::to_string(it->date)
                                            + " | Duration: " + std::to_string(it->duration) + "]";

                OV_WARNING_K(message.c_str());
            }
        }

        if (it != m_savedStims.begin()) { m_savedStims.erase(m_savedStims.begin(), it); }

        OV_ERROR_UNLESS_KRF(m_stimEncoder.encodeBuffer(), "Failed to encode stimulation buffer", Kernel::ErrorType::Internal);
        OV_ERROR_UNLESS_KRF(boxCtx.markOutputAsReadyToSend(m_stimIdx, stimChunkStartTime, m_lastStimDate),
                            "Failed to mark stimulation output as ready to send", Kernel::ErrorType::Internal);
    }

    if (m_savedStims.empty() && !m_readerLib->hasDataToRead()) {
        OV_ERROR_UNLESS_KRF(m_stimEncoder.encodeEnd(), "Failed to encode end.", Kernel::ErrorType::Internal);
        OV_ERROR_UNLESS_KRF(boxCtx.markOutputAsReadyToSend(m_stimIdx, m_lastStimDate, currentTime), "Failed to mark signal output as ready to send",
                            Kernel::ErrorType::Internal);
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
void CBoxAlgorithmOVCSVFileReader::transformLabels()
{
    for (auto& label : m_channelNames) { label = std::regex_replace(label, std::regex("_newLine_"), "\n"); }
}
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
