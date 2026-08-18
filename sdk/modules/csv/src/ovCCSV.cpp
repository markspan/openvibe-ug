///-------------------------------------------------------------------------------------------------
/// 
/// \file ovCCSV.cpp
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
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <cmath>
#include <limits>
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#else
#include <stdio.h>  //sprintf
#endif

#include <utility>
#include <functional>
#include <cstdio>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <array>

#include <boost/spirit/include/qi.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/regex.hpp>

#include <fs/Files.h>

#include "ovCCSV.hpp"


namespace OpenViBE {
namespace CSV {
static const size_t SIGNAL_EPOCH_COL_IDX = 1;
static const size_t TIME_COL_IDX         = 0;
static const size_t END_TIME_COL_IDX     = 1;
static const size_t N_PRE_DATA_COL       = 2;  // Number of columns before data (Time/Epoch)
static const size_t N_POST_DATA_COL      = 3;  // Number of columns after data (Events)

//Stream types regexes
static const boost::regex HEADER_SPECTRUM_REGEX{ "Time:\\d+x\\d+\\:\\d+,End Time,([\\w\\s]+:((\\d+)|(\\d+\\.\\d+)),)+Event Id,Event Date,Event Duration" };
static const boost::regex HEADER_MATRIX_REGEX{ "Time:\\d+(x\\d+){1,},End Time,(([\\w\\s]*(\\:[\\w\\s]*){1,})*,)+Event Id,Event Date,Event Duration" };
static const boost::regex HEADER_VECTOR_REGEX{ "Time:\\d+,End Time,([\\w\\s]*,)+Event Id,Event Date,Event Duration" };
static const boost::regex HEADER_SIGNAL_REGEX{ "Time:\\d+Hz,Epoch,([\\w\\s]*,)+Event Id,Event Date,Event Duration" };
static const boost::regex HEADER_STIMULATIONS_REGEX{ "Event Id,Event Date,Event Duration" };

//Separators
static const char SEPARATOR(',');
static const char DATA_SEPARATOR(':');
static const char DIMENSION_SEPARATOR('x');

// Columns Names
static const std::string EVENT_ID_COL       = "Event Id";
static const std::string EVENT_DATE_COL     = "Event Date";
static const std::string EVENT_DURATION_COL = "Event Duration";

static const size_t CHAR_TO_READ = 511;
//static const size_t MAXIMUM_FLOAT_DECIMAL = 32;

static const char END_OF_LINE_CHAR('\n');

bool CCSVHandler::streamReader(std::istream& in, std::string& out, const char delimiter, std::string& bufferHistory) const
{
    // To improve the performance of the reading, we read a bunch a chars rather one char by one char.
    // We keep the part of the read stream, that is not used, in a buffer history.
    std::vector<std::string> buffer;
    size_t lineBreakPos = std::string::npos;

    // We check that the delimiter is present in the buffer history
    if (!bufferHistory.empty()) {
        buffer.push_back(std::move(bufferHistory));
        lineBreakPos = buffer.back().find_first_of(delimiter);
    }

    // If it's not the case, we have to read the stream.
    while (lineBreakPos == std::string::npos) {
        buffer.emplace_back(CHAR_TO_READ, '\0');  // Construct a string that will store the characters.
        in.read(&buffer.back()[0], CHAR_TO_READ);  // Read X chars by X chars
        lineBreakPos = buffer.back().find_first_of(delimiter);

        // If it's the end of the file and no delimiter has been found...
        if (in.gcount() != CHAR_TO_READ && lineBreakPos == std::string::npos) { break; }
    }

    // There is no delimitor in the stream ...
    if (lineBreakPos == std::string::npos) {
        // Save the rest of the data in the history
        for_each(buffer.begin(), buffer.end(), [&bufferHistory](const std::string& s) { bufferHistory += s; });
        return false;
    }

    // Keep the string part that following the delimiter in the buffer history for the next call.
    bufferHistory = buffer.back().substr(lineBreakPos + 1);

    // And we remove this part from the last string because we will join them.
    buffer.back().erase(lineBreakPos, buffer.back().size());

    out.clear();
    out.reserve(std::accumulate(buffer.cbegin(), buffer.cend(),
                                size_t(0),
                                [](const size_t sumSize, const std::string& str) { return sumSize + str.size(); }));

    // Let's join the strings !
    for_each(buffer.begin(), buffer.end(), [&out](const std::string& s) { out += s; });

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    // Check if we are looking for an end of line char and handle the CR/LF Windows/Linux cases.
    if (delimiter == END_OF_LINE_CHAR && m_isCRLFEOL && !out.empty()) { out.pop_back(); }  // Remove the carriage return char.
#endif

    return true;
}

void CCSVHandler::split(const std::string& in, const char delimiter, std::vector<std::string>& out) const
{
    std::stringstream stringStream(in);
    std::string item;
    out.reserve(m_nCol);

    std::string buffer;

    // Loop until the last delimiter
    while (this->streamReader(stringStream, item, delimiter, buffer)) { out.push_back(item); }

    // Get the part after the last delimiter
    if (this->streamReader(stringStream, item, '\0', buffer)) { out.push_back(item); }
}

void CCSVHandler::setFormatType(const EStreamType typeID) { m_inputTypeID = typeID; }

bool CCSVHandler::setSignalInformation(const std::vector<std::string>& channelNames, const size_t sampling, const size_t nSamplePerBuffer)
{
    if (m_inputTypeID != EStreamType::Signal) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }
    if (m_isSetInfoCalled) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_SetInfoOnce;
        return false;
    }

    m_isSetInfoCalled = true;

    if (channelNames.empty()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_NoChannelsName;
        return false;
    }

    m_sampling         = sampling;
    m_dimLabels        = channelNames;
    m_nSamplePerBuffer = nSamplePerBuffer;
    return true;
}

bool CCSVHandler::getSignalInformation(std::vector<std::string>& channelNames, size_t& sampling, size_t& nSamplePerBuffer)
{
    if (m_inputTypeID != EStreamType::Signal) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }

    channelNames     = m_dimLabels;
    sampling         = m_sampling;
    nSamplePerBuffer = m_nSamplePerBuffer;
    return true;
}

bool CCSVHandler::setSpectrumInformation(const std::vector<std::string>& channelNames, const std::vector<double>& frequencyAbscissa, const size_t sampling)
{
    if (m_inputTypeID != EStreamType::Spectrum) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }

    if (m_isSetInfoCalled) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_SetInfoOnce;
        return false;
    }

    m_isSetInfoCalled = true;

    if (channelNames.empty()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_NoChannelsName;
        return false;
    }

    m_dimLabels         = channelNames;
    m_dimSizes          = { channelNames.size(), frequencyAbscissa.size() };
    m_frequencyAbscissa = frequencyAbscissa;
    m_nSampleOriginal   = sampling;
    return true;
}

bool CCSVHandler::getSpectrumInformation(std::vector<std::string>& channelNames, std::vector<double>& frequencyAbscissa, size_t& sampling)
{
    if (m_inputTypeID != EStreamType::Spectrum) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }

    channelNames      = m_dimLabels;
    frequencyAbscissa = m_frequencyAbscissa;
    sampling          = m_nSampleOriginal;
    return true;
}

bool CCSVHandler::setFeatureVectorInformation(const std::vector<std::string>& channelNames)
{
    if (m_inputTypeID != EStreamType::FeatureVector) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }
    if (m_isSetInfoCalled) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_SetInfoOnce;
        return false;
    }

    m_isSetInfoCalled = true;

    if (channelNames.empty()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_NoChannelsName;
        return false;
    }

    m_dimLabels = channelNames;
    m_dimSizes  = { channelNames.size() };
    m_nDim      = 1;
    return true;
}

bool CCSVHandler::getFeatureVectorInformation(std::vector<std::string>& channelNames)
{
    if (m_inputTypeID != EStreamType::FeatureVector) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }

    channelNames = m_dimLabels;
    return true;
}

bool CCSVHandler::setStreamedMatrixInformation(const std::vector<size_t>& dimSizes, const std::vector<std::string>& labels)
{
    if (m_isSetInfoCalled) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_SetInfoOnce;
        return false;
    }

    if (m_inputTypeID != EStreamType::StreamedMatrix && m_inputTypeID != EStreamType::CovarianceMatrix) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }

    if (dimSizes.empty()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_DimensionSizeEmpty;
        return false;
    }

    const size_t size = std::accumulate(dimSizes.begin(), dimSizes.end(), size_t(0));

    if (size != labels.size()) {
        m_lastStringError = "dimension count is " + std::to_string(size) + " and labels count is " + std::to_string(labels.size());
        m_logError        = LogErrorCodes_WrongDimensionSize;
        return false;
    }

    m_isSetInfoCalled = true;
    m_dimSizes        = dimSizes;
    m_nDim            = m_dimSizes.size();
    m_dimLabels       = labels;
    return true;
}

bool CCSVHandler::getStreamedMatrixInformation(std::vector<size_t>& dimSizes, std::vector<std::string>& labels)
{
    if (m_inputTypeID != EStreamType::StreamedMatrix && m_inputTypeID != EStreamType::CovarianceMatrix && m_inputTypeID != EStreamType::FeatureVector) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_WrongInputType;
        return false;
    }

    dimSizes = m_dimSizes;
    labels   = m_dimLabels;
    return true;
}

bool CCSVHandler::openFile(const std::string& fileName, const EFileAccessMode mode)
{
    if (m_fs.is_open()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_CantOpenFile;
        return false;
    }

    m_filename = fileName;
    // This check that file can be written and create it if it does not exist
    if (mode == EFileAccessMode::Write || mode == EFileAccessMode::Append) {
        FILE* file = FS::Files::open(m_filename.c_str(), "a");

        if (!file) {
            m_lastStringError.clear();
            m_logError = LogErrorCodes_CantOpenFile;
            return false;
        }

        if (fclose(file) != 0) {
            m_lastStringError.clear();
            m_logError = LogErrorCodes_ErrorWhileClosing;
            return false;
        }
    }

    try {
        switch (mode) {
            case EFileAccessMode::Write:
                FS::Files::openFStream(m_fs, m_filename.c_str(), std::ios::out | std::ios::trunc);
                break;

            case EFileAccessMode::Append:
                FS::Files::openFStream(m_fs, m_filename.c_str(), std::ios::out | std::ios::app);
                break;

            case EFileAccessMode::Read:
                FS::Files::openFStream(m_fs, m_filename.c_str(), std::ios::in);
                break;
        }
    }
    catch (const std::ios_base::failure& fail) {
        m_lastStringError = "Error while opening file " + m_filename + ": " + fail.what();
        m_logError        = LogErrorCodes_CantOpenFile;
        return false;
    }

    if (!m_fs.is_open()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_CantOpenFile;
        return false;
    }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    if (mode == EFileAccessMode::Read)
    {
        // Check if the end of line is CRLF
        std::string stringBuffer, stringHistory;

        m_isCRLFEOL = false;
        if(m_fs.peek() == std::ifstream::traits_type::eof()) { m_isCRLFEOL = true; }
        else 
        {
            if (!this->streamReader(m_fs, stringBuffer, '\n', stringHistory))
            {
                this->closeFile();

                m_lastStringError = "Error while opening file " + m_filename + ": " + "Fail to determine the file line ending.";
                m_logError = LogErrorCodes_CantOpenFile;
                return false;
            }

            m_isCRLFEOL = *--stringBuffer.end() == '\r';
        }
    }
#endif

    // Reset the read position
    m_fs.clear();
    m_fs.seekg(0);

    m_isHeaderRead       = false;
    m_isFirstLineWritten = false;
    m_isSetInfoCalled    = false;
    m_hasEpoch           = false;
    return true;
}

bool CCSVHandler::readSamplesAndEventsFromFile(const size_t lineNb, std::vector<SMatrixChunk>& chunks, std::vector<SStimulationChunk>& stimulations)
{
    if (!m_hasDataToRead) { return false; }

    if (!m_isHeaderRead) {
        m_lastStringError = "Trying to read data without having read a header";
        m_logError        = LogErrorCodes_HeaderNotRead;
        return false;
    }

    chunks.clear();

    // Calculate the size of the matrix depending of the stream type
    size_t matrixSize = size_t(m_nSamplePerBuffer);

    if (m_inputTypeID == EStreamType::Signal) {
        const size_t signalSize = size_t(m_nCol - (N_PRE_DATA_COL + N_POST_DATA_COL));
        matrixSize *= signalSize;
    }
    else if (m_inputTypeID == EStreamType::Spectrum) {
        const size_t spectrumSize = m_dimSizes[0] * m_dimSizes[1];
        matrixSize *= spectrumSize;
    }
    else { matrixSize = 0; }

    SMatrixChunk chunk(0, 0, std::vector<double>(matrixSize), 0);

    while (chunks.size() < lineNb && m_hasDataToRead) {
        for (size_t lineIndex = 0; lineIndex < m_nSamplePerBuffer; lineIndex++) {
            std::string lineValue;

            if (!this->streamReader(m_fs, lineValue, END_OF_LINE_CHAR, m_bufferReadFileLine)) {
                if (lineIndex != 0) {
                    m_lastStringError = "Chunk is not complete";
                    m_logError        = LogErrorCodes_MissingData;
                    return false;
                }
                // There is no more data to read
                m_hasDataToRead = false;
                return true;
            }

            const size_t columnCount = std::count(lineValue.cbegin(), lineValue.cend(), ',') + 1;

            if (columnCount != m_nCol) {
                m_lastStringError = "There is " + std::to_string(columnCount) + " columns in the Header instead of " + std::to_string(m_nCol) + " on line " +
                                    std::to_string(lineIndex + 1);

                m_logError = LogErrorCodes_WrongLineSize;
                return false;
            }

            // get Matrix chunk, LogError set in the function
            if (!this->readSampleChunk(lineValue, chunk, lineIndex)) { return false; }

            // get stimulations chunk, LogError set in the function
            if (!this->readStimulationChunk(lineValue, stimulations, lineIndex + 1)) { return false; }
        }

        chunks.push_back(chunk);
    }

    return true;
}

bool CCSVHandler::readEventsFromFile(size_t stimsToRead, std::vector<SStimulationChunk>& events)
{
    if (!m_hasDataToRead) {
        m_lastStringError = "There is no more data to read";
        m_logError        = LogErrorCodes_DateError;
        return false;
    }

    if (!m_isHeaderRead) {
        m_lastStringError = "Trying to read data without having read a header";
        m_logError        = LogErrorCodes_HeaderNotRead;
        return false;
    }

    events.clear();

    std::string lineValue;
    while (events.size() < stimsToRead && m_hasDataToRead) {
        if (!this->streamReader(m_fs, lineValue, END_OF_LINE_CHAR, m_bufferReadFileLine)) {
            // There is no more data to read
            m_hasDataToRead = false;
            break;
        }
        if (!readStimulationChunk(lineValue, events, 0)) { return false; }
    }

    return true;
}

bool CCSVHandler::writeHeaderToFile()
{
    if (!m_fs.is_open()) {
        m_lastStringError.clear();
        m_lastStringError = "File is not opened.";
        m_logError        = LogErrorCodes_NoFileDefined;
        return false;
    }

    if (m_isFirstLineWritten) {
        m_lastStringError = "Header already written.";
        m_logError        = LogErrorCodes_CantWriteHeader;
        return false;
    }

    // set header (in case of error, logError set in function)
    const std::string header = this->createHeaderString();
    if (header.empty()) { return false; }

    m_isFirstLineWritten = true;
    try { m_fs << header; }
    catch (std::ios_base::failure& fail) {
        m_lastStringError = "Error occured while writing: ";
        m_lastStringError += fail.what();
        m_logError = LogErrorCodes_ErrorWhileWriting;
        return false;
    }

    return true;
}

bool CCSVHandler::writeDataToFile()
{
    if (!m_fs.is_open()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_NoFileDefined;
        return false;
    }

    std::string csv;

    // set matrix (in case of error, logError set in the function)
    if (!this->createCSVStringFromData(false, csv)) { return false; }

    try { m_fs << csv; }
    catch (std::ios_base::failure& fail) {
        m_lastStringError = "Error occured while writing: ";
        m_lastStringError += fail.what();
        m_logError = LogErrorCodes_ErrorWhileWriting;
        return false;
    }

    return true;
}

bool CCSVHandler::writeAllDataToFile()
{
    std::string csv;

    // in case of error, logError set in the function
    if (!createCSVStringFromData(true, csv)) { return false; }

    try { m_fs << csv; }
    catch (std::ios_base::failure& fail) {
        m_lastStringError = "Error occured while writing: ";
        m_lastStringError += fail.what();
        m_logError = LogErrorCodes_ErrorWhileWriting;
        return false;
    }

    return true;
}

bool CCSVHandler::closeFile()
{
    m_stimulations.clear();
    m_chunks.clear();
    m_dimSizes.clear();
    m_dimLabels.clear();
    m_frequencyAbscissa.clear();
    m_nDim               = 0;
    m_nCol               = 0;
    m_sampling           = 0;
    m_isFirstLineWritten = false;
    m_isSetInfoCalled    = false;

    try { m_fs.close(); }
    catch (const std::ios_base::failure& fail) {
        m_lastStringError = "Error while closing file: ";
        m_lastStringError += fail.what();
        m_logError = LogErrorCodes_ErrorWhileClosing;
        return false;
    }

    return true;
}

bool CCSVHandler::addSample(const SMatrixChunk& sample)
{
    if (sample.matrix.empty()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_MatrixEmpty;
        return false;
    }

    switch (m_inputTypeID) {
        case EStreamType::Signal:
            if (sample.matrix.size() != m_dimLabels.size()) {
                m_lastStringError.clear();
                m_logError = LogErrorCodes_WrongMatrixSize;
                return false;
            }
            break;

        case EStreamType::Spectrum:
            if (sample.matrix.size() != (m_dimLabels.size() * m_frequencyAbscissa.size())) {
                m_lastStringError.clear();
                m_logError = LogErrorCodes_WrongMatrixSize;
                return false;
            }
            break;

        case EStreamType::StreamedMatrix:
        case EStreamType::CovarianceMatrix:
        case EStreamType::FeatureVector:
        {
            const size_t columnsToHave = std::accumulate(m_dimSizes.begin(), m_dimSizes.end(), 1U, std::multiplies<size_t>());

            if (sample.matrix.size() != columnsToHave) {
                m_lastStringError = "Matrix size is " + std::to_string(sample.matrix.size()) + " and size to have is " +
                                    std::to_string(columnsToHave);
                m_logError = LogErrorCodes_WrongMatrixSize;
                return false;
            }
            break;
        }
        default:
            m_lastStringError = "Cannot add Sample in " + toString(m_inputTypeID) + " file type";
            m_logError = LogErrorCodes_WrongStreamType;
            return false;
    }

    // Check timestamps
    if (std::signbit(sample.startTime) || std::signbit(sample.endTime) || sample.endTime < sample.startTime) {
        m_lastStringError = "Sample start time [" + std::to_string(sample.startTime) + "] | end time [" + std::to_string(sample.endTime) + "]";
        m_logError        = LogErrorCodes_WrongSampleDate;
        return false;
    }

    if (m_lastMatrixOnly &&
        (m_inputTypeID != EStreamType::Signal
         || (!m_chunks.empty() && sample.epoch != m_chunks.back().epoch))) {
        m_chunks.clear();
        m_stimulations.erase(std::remove_if(m_stimulations.begin(), m_stimulations.end(), [&sample](const SStimulationChunk& chunk)
                             {
                                 return chunk.date < sample.startTime;
                             }),
                             m_stimulations.end());
    }

    m_chunks.push_back(sample);
    return true;
}

bool CCSVHandler::addBuffer(const std::vector<SMatrixChunk>& samples)
{
    if (samples.empty()) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_NoSample;
        return false;
    }

    ELogErrorCodes error;
    const size_t dimensionCount = m_dimLabels.size();

    if (!std::all_of(samples.cbegin(), samples.cend(), [&error, dimensionCount](const SMatrixChunk& sample)
    {
        if (sample.matrix.empty()) {
            error = LogErrorCodes_MatrixEmpty;
            return false;
        }

        if (sample.matrix.size() != dimensionCount) {
            error = LogErrorCodes_WrongMatrixSize;
            return false;
        }

        // Check that the timestamps is good
        if (std::signbit(sample.startTime) || std::signbit(sample.endTime) || sample.endTime < sample.startTime) {
            error = LogErrorCodes_WrongSampleDate;
            return false;
        }

        return true;
    })) {
        m_logError = error;
        return false;
    }

    if (m_lastMatrixOnly) {
        if (m_inputTypeID == EStreamType::Signal) {
            const uint64_t lastEpoch = samples.back().epoch;
            const auto rangeStart    = std::find_if(samples.begin(), samples.end(), [&lastEpoch](const SMatrixChunk& s) { return s.epoch == lastEpoch; });

            if (!m_chunks.empty() && m_chunks.front().epoch != lastEpoch) { m_chunks.clear(); }

            m_chunks.insert(m_chunks.end(), rangeStart, samples.end());
            const double curTime = m_chunks.front().startTime;
            m_stimulations.erase(std::remove_if(m_stimulations.begin(), m_stimulations.end(),
                                                [curTime](const SStimulationChunk& chunk) { return chunk.date < curTime; }), m_stimulations.end());
        }
        else {
            m_chunks.clear();
            m_chunks.push_back(samples.back());
            const double curTime = m_chunks.front().startTime;
            m_stimulations.erase(std::remove_if(m_stimulations.begin(), m_stimulations.end(),
                                                [curTime](const SStimulationChunk& chunk) { return chunk.date < curTime; }), m_stimulations.end());
        }
    }
    else { m_chunks.insert(m_chunks.end(), samples.begin(), samples.end()); }

    return true;
}

bool CCSVHandler::addEvent(uint64_t code, double date, double duration)
{
    if (std::signbit(date)) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_DateError;
        return false;
    }

    if (std::signbit(duration)) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_DurationError;
        return false;
    }

    m_stimulations.emplace_back(code, date, duration);
    return true;
}

bool CCSVHandler::addEvent(const SStimulationChunk& event)
{
    if (std::signbit(event.date)) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_DateError;
        return false;
    }

    if (std::signbit(event.duration)) {
        m_lastStringError.clear();
        m_logError = LogErrorCodes_DurationError;
        return false;
    }

    m_stimulations.push_back(event);
    return true;
}

bool CCSVHandler::noEventsUntilDate(const double date)
{
    if (std::signbit(date)) {
        m_lastStringError = "Date is negative: ";
        m_lastStringError += std::to_string(date);
        m_logError = LogErrorCodes_WrongSampleDate;
        return false;
    }

    m_noEventSince = date;
    return true;
}

std::string CCSVHandler::stimulationsToString(const std::vector<SStimulationChunk>& stimulationsToPrint) const
{
    if (stimulationsToPrint.empty()) { return std::string(2, SEPARATOR); } // Empty columns

    std::array<std::string, 3> stimulations;

    const auto itBegin = stimulationsToPrint.cbegin();
    const auto itEnd   = stimulationsToPrint.cend();

    if (itBegin != itEnd) {
        stimulations.at(0) = std::to_string(itBegin->id);

        std::stringstream ss;
        ss.str("");
        ss.precision(m_oPrecision);
        ss << std::fixed << itBegin->date;
        stimulations.at(1) = ss.str();

        ss.str("");
        ss << std::fixed << itBegin->duration;
        stimulations.at(2) = ss.str();
    }

    for (auto it = itBegin + 1; it != itEnd; ++it) {
        stimulations.at(0) += std::string(1, DATA_SEPARATOR) + std::to_string(it->id);
        std::stringstream ss;
        ss.str("");
        ss.precision(m_oPrecision);
        ss << std::fixed << it->date;
        stimulations.at(1) += std::string(1, DATA_SEPARATOR) + ss.str();

        ss.str("");
        ss << std::fixed << it->duration;
        stimulations.at(2) += std::string(1, DATA_SEPARATOR) + ss.str();
    }

    return boost::algorithm::join(stimulations, std::string(1, SEPARATOR));
}

std::string CCSVHandler::createHeaderString()
{
    std::string header;

    const auto addColumn = [&](const std::string& columnLabel)
    {
        if (m_nCol != 0) { header += SEPARATOR; }

        header += columnLabel;
        m_nCol++;
    };

    if (m_inputTypeID == EStreamType::Undefined) {
        m_lastStringError = "Cannot write header for Undefined stream Type";
        m_logError        = LogErrorCodes_WrongStreamType;
        return "";
    }

    // add Time Header
    switch (m_inputTypeID) {
        case EStreamType::Signal:
            addColumn("Time" + std::string(1, DATA_SEPARATOR) + std::to_string(m_sampling) + "Hz");
            break;

        case EStreamType::Spectrum:
            if (m_dimSizes.size() != 2) {
                m_lastStringError = "Channel names and number of frequency are needed to write time column";
                return "";
            }

            addColumn(
                "Time" + std::string(1, DATA_SEPARATOR) + std::to_string(m_dimSizes[0]) + std::string(1, DIMENSION_SEPARATOR) + std::to_string(m_dimSizes[1]) +
                std::string(1, DATA_SEPARATOR) + std::to_string(m_nSampleOriginal != 0 ? m_nSampleOriginal : m_dimSizes[0] * m_dimSizes[1]));
            break;

        case EStreamType::CovarianceMatrix:
        case EStreamType::StreamedMatrix:
        case EStreamType::FeatureVector:
            if (m_nDim == 0) {
                m_lastStringError.clear();
                m_logError = LogErrorCodes_DimensionCountZero;
                return "";
            }
            {
                std::string timeColumn = "Time" + std::string(1, DATA_SEPARATOR);

                for (size_t index = 0; index < m_nDim; ++index) {
                    timeColumn += std::to_string(m_dimSizes[index]);
                    if ((index + 1) < m_nDim) { timeColumn += std::string(1, DIMENSION_SEPARATOR); }
                }

                addColumn(timeColumn);
            }
            break;
        default:
            break;
    }

    // add Epoch Header to signal
    switch (m_inputTypeID) {
        case EStreamType::Signal:
            addColumn("Epoch");
            break;

        case EStreamType::Spectrum:
        case EStreamType::StreamedMatrix:
        case EStreamType::CovarianceMatrix:
        case EStreamType::FeatureVector:
            addColumn("End Time");
            break;
        default:
            break;
    }

    if (m_inputTypeID != EStreamType::Stimulations) {
        if (m_dimLabels.empty()) {
            m_lastStringError.clear();
            m_logError = LogErrorCodes_NoMatrixLabels;
            return "";
        }
    }

    switch (m_inputTypeID) {
        case EStreamType::Signal:
        case EStreamType::FeatureVector:
            for (const std::string& label : m_dimLabels) { addColumn(label); }
            break;

        case EStreamType::CovarianceMatrix:
        case EStreamType::StreamedMatrix:
        {
            const size_t matrixColumns = std::accumulate(m_dimSizes.begin(), m_dimSizes.end(), 1U, std::multiplies<size_t>());

            if (matrixColumns == 0) {
                m_lastStringError.clear();
                m_logError = LogErrorCodes_DimensionSizeZero;
                return "";
            }

            std::vector<size_t> position(m_nDim, 0);
            m_nCol += matrixColumns;

            do {
                header += SEPARATOR;
                size_t previousDimensionsSize = 0;

                for (size_t index = 0; index < position.size(); ++index) {
                    header += m_dimLabels[previousDimensionsSize + position[index]];
                    previousDimensionsSize += m_dimSizes[index];

                    if ((index + 1) < position.size()) { header += std::string(1, DATA_SEPARATOR); }
                }
            } while (this->increasePositionIndexes(position));
        }
        break;

        case EStreamType::Spectrum:
            for (const std::string& label : m_dimLabels) {
                for (double frequencyAbscissa : m_frequencyAbscissa) { addColumn(label + std::string(1, DATA_SEPARATOR) + std::to_string(frequencyAbscissa)); }
            }
            break;

        default:
            break;
    }

    addColumn(EVENT_ID_COL);
    addColumn(EVENT_DATE_COL);
    addColumn(EVENT_DURATION_COL + "\n");
    return header;
}

bool CCSVHandler::createCSVStringFromData(const bool canWriteAll, std::string& csv)
{
    // if chunks is empty, their is no more lines to write
    if (!m_chunks.empty()) {
        // if header isn't written, size of first line to write will be the reference
        if (m_nCol == 0) {
            m_nCol = m_chunks.front().matrix.size();
            m_nCol += N_PRE_DATA_COL + N_POST_DATA_COL;  // Will be set correctly with call to setFormatType
        }

        // loop will add a line to the buffer while the last stimulation date registered is greater than the end of the current chunk or until their is an event
        uint64_t linesWritten = 0;

        while (!m_chunks.empty() && (canWriteAll
                                     || (m_stimulations.empty() && m_chunks.front().endTime <= m_noEventSince)
                                     || (!m_stimulations.empty() &&
                                         m_chunks.front().startTime <= m_stimulations.back().date))
        ) {
            // Signal data must be written as sampleCounterPerBuffer th lines;
            if (m_inputTypeID == EStreamType::Signal && canWriteAll == false && linesWritten != 0 &&
                linesWritten % m_nSamplePerBuffer == 0) { break; }
            // check line size

            if ((m_inputTypeID == EStreamType::Signal || m_inputTypeID == EStreamType::Spectrum)
                && (m_chunks.front().matrix.size() + N_PRE_DATA_COL + N_POST_DATA_COL) != m_nCol) {
                m_lastStringError.clear();
                m_logError = LogErrorCodes_WrongLineSize;
                return false;
            }

            if (m_inputTypeID == EStreamType::FeatureVector || m_inputTypeID == EStreamType::CovarianceMatrix ||
                m_inputTypeID == EStreamType::StreamedMatrix) {
                size_t columnstoHave = std::accumulate(m_dimSizes.begin(), m_dimSizes.end(), 1U,
                                                       std::multiplies<size_t>());
                columnstoHave += N_PRE_DATA_COL + N_POST_DATA_COL;

                if (columnstoHave != m_nCol) {
                    m_lastStringError =
                            "Line size is " + std::to_string(columnstoHave) + " but must be " + std::to_string(m_nCol);
                    m_logError = LogErrorCodes_WrongLineSize;
                    return false;
                }
            }

            // Time and Epoch
            const std::pair<double, double> currentTime = { m_chunks.front().startTime, m_chunks.front().endTime };
            std::stringstream ss;
            ss.str("");
            ss.precision(m_oPrecision);
            ss << std::fixed << currentTime.first;

            csv += ss.str();

            switch (m_inputTypeID) {
                case EStreamType::Spectrum:
                case EStreamType::StreamedMatrix:
                case EStreamType::CovarianceMatrix:
                case EStreamType::FeatureVector:
                    ss.str("");
                    ss << std::fixed << currentTime.second;
                    csv += std::string(1, SEPARATOR) + ss.str();
                    break;

                case EStreamType::Signal:
                    csv += std::string(1, SEPARATOR) + std::to_string(m_chunks.front().epoch);
                    break;

                case EStreamType::Stimulations:
                    m_lastStringError = "Stream samples were received while only Stimulations were expected";
                    m_logError = LogErrorCodes_WrongInputType;
                    return false;
                case EStreamType::Undefined:
                    m_lastStringError = "StreamType is undefined. Cannot format data. Please set Stream Type before writing to file";
                    m_logError = LogErrorCodes_WrongInputType;
                    return false;
            }

            // Matrix
            for (const double& value : m_chunks.front().matrix) {
                csv += SEPARATOR;
                ss.str("");
                ss << std::fixed << value;
                csv += ss.str();
            }

            m_chunks.pop_front();

            csv += SEPARATOR;

            // Stimulations
            if (!m_stimulations.empty()) {
                std::vector<SStimulationChunk> stimulationsToWrite;

                double stimulationTime = m_stimulations.front().date;

                while ((stimulationTime - currentTime.first) < (currentTime.second - currentTime.first)) {
                    stimulationsToWrite.push_back(m_stimulations.front());
                    m_stimulations.pop_front();

                    if (m_stimulations.empty()) { break; }

                    stimulationTime = m_stimulations.front().date;
                }

                csv += stimulationsToString(stimulationsToWrite);
            }
            else { csv += std::string(2, SEPARATOR); }

            csv += "\n";
            linesWritten++;
        }
    }
    else if (!m_stimulations.empty()) {
        if (m_inputTypeID == EStreamType::Stimulations) {
            for (auto stim = m_stimulations.cbegin(); stim != m_stimulations.end(); ++stim) {
                csv += stimulationsToString(std::vector<SStimulationChunk>(1, *stim));
                csv += "\n";
            }
            m_stimulations.clear();
        }
    }

    return true;
}

void CCSVHandler::extractFormatType(const std::string& header)
{
    boost::cmatch match;
    if (regex_match(header.c_str(), match, HEADER_SIGNAL_REGEX)) { m_inputTypeID = EStreamType::Signal; }
    else if (regex_match(header.c_str(), match, HEADER_MATRIX_REGEX)) { m_inputTypeID = EStreamType::StreamedMatrix; }
    else if (regex_match(header.c_str(), match, HEADER_VECTOR_REGEX)) { m_inputTypeID = EStreamType::FeatureVector; }
    else if (regex_match(header.c_str(), match, HEADER_SPECTRUM_REGEX)) { m_inputTypeID = EStreamType::Spectrum; }
    else if (regex_match(header.c_str(), match, HEADER_STIMULATIONS_REGEX)) { m_inputTypeID = EStreamType::Stimulations; }
    else { m_inputTypeID = EStreamType::Undefined; }
}

bool CCSVHandler::parseHeader()
{
    if (m_isHeaderRead) { return true; }

    std::string header;
    m_fs.clear();  // Useful if the end of file is reached before.
    m_fs.seekg(0);

    if (!this->streamReader(m_fs, header, END_OF_LINE_CHAR, m_bufferReadFileLine)) {
        m_lastStringError = "No header in the file or file empty";
        m_logError        = LogErrorCodes_EmptyColumn;
        return false;
    }
    m_isHeaderRead = true;

    extractFormatType(header);

    std::vector<std::string> columns;
    this->split(header, SEPARATOR, columns);
    m_nCol = columns.size();
    if (columns.empty()) {
        m_lastStringError = "No header in the file or file empty";
        m_logError        = LogErrorCodes_EmptyColumn;
        return false;
    }

    m_fs.clear();  // Useful if the end of file is reached before.
    // Set stream to the line after the header.
    m_fs.seekg(header.size() + 2);
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    // If not CRLF ending, seek one character less
    if (!m_isCRLFEOL)
    {
        m_fs.seekg(header.size() + 1);
    }
#endif
    m_bufferReadFileLine.clear();

    switch (m_inputTypeID) {
        case EStreamType::Signal:
            if (!this->parseSignalHeader(columns)) {
                m_inputTypeID = EStreamType::Undefined;
                m_logError    = LogErrorCodes_WrongHeader;
                return false;
            }
            break;

        case EStreamType::Spectrum:
            if (!this->parseSpectrumHeader(columns)) {
                m_inputTypeID = EStreamType::Undefined;
                m_logError    = LogErrorCodes_WrongHeader;
                return false;
            }
            break;

        case EStreamType::StreamedMatrix:
        case EStreamType::CovarianceMatrix:
        case EStreamType::FeatureVector:
            if (!this->parseMatrixHeader(columns)) {
                m_inputTypeID = EStreamType::Undefined;
                m_logError    = LogErrorCodes_WrongHeader;
                return false;
            }
            break;
        case EStreamType::Stimulations:
            // Nothing more to do
            break;
        case EStreamType::Undefined:
            m_logError = LogErrorCodes_WrongHeader;
            return false;
    }

    m_fs.clear();                  // Useful if the end of file is reached before.
    // Reset stream to the line after the header
    m_fs.seekg(header.size() + 2);
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    // If not CRLF ending, seek one character less
    if (!m_isCRLFEOL)
    {
        m_fs.seekg(header.size() + 1);
    }
#endif
    m_bufferReadFileLine.clear();

    return true;
}

bool CCSVHandler::parseSignalHeader(const std::vector<std::string>& header)
{
    // check time
    if (header[TIME_COL_IDX].substr(0, 5) != "Time:"
        || header[TIME_COL_IDX].substr(header[TIME_COL_IDX].size() - 2) != "Hz"
        || header[TIME_COL_IDX].size() <= 7) {
        m_lastStringError = "First column (" + header[TIME_COL_IDX] + ") is not well formed";
        return false;
    }

    // get sampling rate
    try { m_sampling = std::stoul(header[TIME_COL_IDX].substr(5, header[TIME_COL_IDX].size() - 3)); }
    catch (const std::exception& ia) {
        m_lastStringError = "On entry \"" + header[0].substr(5, header[0].size() - 3) + "\", exception have been thrown: ";
        m_lastStringError += ia.what();
        return false;
    }

    // check epoch
    if (header[SIGNAL_EPOCH_COL_IDX] != "Epoch") {
        m_lastStringError = "Second column (" + header[SIGNAL_EPOCH_COL_IDX] + ") must be Epoch column";
        return false;
    }

    // get dimension labels
    for (size_t index = N_PRE_DATA_COL; index < header.size() - N_POST_DATA_COL; ++index) { m_dimLabels.push_back(header[index]); }

    if (!this->calculateSampleCountPerBuffer()) { return false; }  // m_LastErrorString set in the function, m_LogError set outside the function

    return true;
}

bool CCSVHandler::parseSpectrumHeader(const std::vector<std::string>& header)
{
    m_dimSizes.clear();
    // check time column
    std::string buffer;
    std::istringstream iss(header[TIME_COL_IDX]);

    std::string bufferTemp;

    if (!this->streamReader(iss, buffer, DATA_SEPARATOR, bufferTemp)) {
        m_lastStringError = "First column (" + header[TIME_COL_IDX] + ") is empty";
        return false;
    }

    if (buffer != "Time") {
        m_lastStringError = "First column (" + header[TIME_COL_IDX] +
                            ") is not well formed, must have \"Time\" written before the first inqternal data separator (\':\' as default)";
        return false;
    }

    const auto getNextElem = [&](size_t& resultvar, const char separator, const char* missingString)
    {
        this->streamReader(iss, buffer, separator, bufferTemp);

        if (buffer.empty()) { m_lastStringError = "First column (" + header[TIME_COL_IDX] + ") is not well formed, missing " + missingString; }

        try { resultvar = std::stoul(buffer); }
        catch (std::exception& e) {
            m_lastStringError = "On entry \"" + buffer + "\", exception have been thrown: ";
            m_lastStringError += e.what();
            return false;
        }

        return true;
    };

    size_t dimensionSize = 0;

    if (!getNextElem(dimensionSize, DIMENSION_SEPARATOR, "channels number")) { return false; }

    m_dimSizes.push_back(dimensionSize);

    if (!getNextElem(dimensionSize, DATA_SEPARATOR, "number frequencies per channel")) { return false; }

    m_dimSizes.push_back(dimensionSize);

    if (!getNextElem(m_nSampleOriginal, '\0', "original number of samples")) { return false; }

    if (m_dimSizes[0] == 0 || m_dimSizes[1] == 0) {
        m_lastStringError = "Dimension size must be only positive";
        return false;
    }

    // check Time End column
    if (header[END_TIME_COL_IDX] != "End Time") {
        m_lastStringError = "Second column (" + header[END_TIME_COL_IDX] + ") must be End Time Column";
        return false;
    }

    // add every new label

    std::string channelLabel;
    m_dimLabels.clear();

    for (size_t labelCounter = 0; labelCounter < m_dimSizes[0]; labelCounter++) {
        double lastFrequency = 0.0;

        for (size_t labelSizeCounter = 0; labelSizeCounter < m_dimSizes[1]; labelSizeCounter++) {
            std::string dimensionData;
            // get channel name and check that it's the same one for all the frequency dimension size
            std::vector<std::string> spectrumChannel;
            this->split(header.at((labelCounter * m_dimSizes[1]) + labelSizeCounter + N_PRE_DATA_COL), DATA_SEPARATOR, spectrumChannel);

            if (spectrumChannel.size() != 2) {
                m_lastStringError = "Spectrum channel is invalid: " + header.at((labelCounter * m_dimSizes[1]) + labelSizeCounter + N_PRE_DATA_COL);
                return false;
            }

            dimensionData = spectrumChannel[0];

            if (labelSizeCounter == 0) {
                channelLabel = dimensionData;
                m_dimLabels.push_back(channelLabel);
            }
            else if (channelLabel != dimensionData) {
                m_lastStringError = "Channel name must be the same during " + std::to_string(m_dimSizes[1]) +
                                    " columns (number of frequencies per channel)";
                return false;
            }

            // get all frequency and check that they're the same for all labels
            dimensionData = spectrumChannel[1];

            if (labelCounter == 0) {
                double frequency;

                try { frequency = std::stod(dimensionData); }
                catch (const std::exception& e) {
                    m_lastStringError = "On entry \"" + dimensionData + "\", exception have been thrown: ";
                    m_lastStringError += e.what();
                    return false;
                }

                m_frequencyAbscissa.push_back(frequency);
            }
            else {
                double frequency;

                try { frequency = std::stod(dimensionData); }
                catch (const std::exception& e) {
                    m_lastStringError = "On entry \"" + dimensionData + "\", exception have been thrown: ";
                    m_lastStringError += e.what();
                    return false;
                }

                if (labelSizeCounter == 0) { lastFrequency = frequency; }
                else if (frequency < lastFrequency) {
                    m_lastStringError = "Frequencies must be in ascending order";
                    return false;
                }

                if ((std::fabs(frequency - m_frequencyAbscissa[labelSizeCounter]) >= std::numeric_limits<double>::epsilon())) {
                    m_lastStringError = "Channels must have the same frequency bands";
                    return false;
                }
            }
        }
    }

    m_nSamplePerBuffer = 1;
    return true;
}

bool CCSVHandler::parseMatrixHeader(const std::vector<std::string>& header)
{
    // check time column
    std::istringstream iss(header[TIME_COL_IDX]);
    std::string linePart;
    // check Time is written

    std::string bufferTemp;

    if (!this->streamReader(iss, linePart, DATA_SEPARATOR, bufferTemp)) {
        m_lastStringError = "First column is empty";
        return false;
    }
    if (linePart != "Time") {
        m_lastStringError = "First column " + header[TIME_COL_IDX] + " is not well formed";
        return false;
    }

    linePart = header[TIME_COL_IDX].substr(5);

    m_dimSizes.clear();
    std::vector<std::string> dimensionParts;
    this->split(linePart, DIMENSION_SEPARATOR, dimensionParts);
    m_nDim = 0;

    for (const std::string& dimensionSize : dimensionParts) {
        size_t size = 0;

        try { size = std::stoul(dimensionSize); }
        catch (std::exception& e) {
            m_lastStringError = "Error on a dimension size, exception have been thrown: ";
            m_lastStringError += e.what();
            return false;
        }

        if (size == 0) {
            m_lastStringError = "A dimension size must be strictly positive";
            return false;
        }

        m_dimSizes.push_back(size);
        m_nDim++;
    }

    if (m_nDim == 0) {
        m_lastStringError = "First column must indicate at least one dimension size";
        return false;
    }

    // check columnLabels number according to dimension sizes
    const size_t matrixColumnCount = std::accumulate(m_dimSizes.begin(), m_dimSizes.end(), 1, std::multiplies<size_t>());

    if ((matrixColumnCount + N_PRE_DATA_COL + N_POST_DATA_COL) != header.size()) {
        m_lastStringError = "Every line must have " + std::to_string(matrixColumnCount + N_PRE_DATA_COL + N_POST_DATA_COL) + " columnLabels";
        return false;
    }

    // it saves labels for each dimensions
    std::vector<std::vector<std::string>> labelsInDimensions(m_nDim);

    // check if labels are already filled or if it's a reset (labels are empty before being set, but empty labels are accepted)
    std::vector<std::vector<bool>> filledLabel(m_nDim);

    for (size_t index = 0; index < m_dimSizes.size(); ++index) {
        labelsInDimensions[index].resize(m_dimSizes[index]);
        filledLabel[index].resize(m_dimSizes[index], false);
    }

    // corresponding to the position in the multi multidimensional matrix (as exemple the third label of the second dimension will be positionsInDimensions[1] = 2)
    std::vector<size_t> positionsInDimensions(m_nDim, 0);
    size_t columnIndex = 0;

    // we will visit each column containing matrix labels
    do {
        std::vector<std::string> columnLabels;

        // get all column part
        this->split(header[columnIndex + N_PRE_DATA_COL], DATA_SEPARATOR, columnLabels);

        if (columnLabels.size() != m_nDim) {
            m_lastStringError = "On column " + std::to_string(columnIndex + N_PRE_DATA_COL) + "(" + header[columnIndex + N_PRE_DATA_COL] +
                                "), there is " + std::to_string(columnLabels.size()) + " label instead of " + std::to_string(m_nDim);
            m_logError = LogErrorCodes_WrongHeader;
            return false;
        }

        // check column labels one per one
        for (size_t dimensionIndex = 0; dimensionIndex < columnLabels.size(); dimensionIndex++) {
            const size_t positionInCurrentDimension = positionsInDimensions[dimensionIndex];

            if (columnLabels[dimensionIndex].empty()) {
                // if saved label is empty, mark it as saved (even if it is already)
                if (labelsInDimensions[dimensionIndex][positionInCurrentDimension].empty()) { filledLabel[dimensionIndex][positionInCurrentDimension] = true; }
                else {
                    // else,there is an error, it means that label is already set
                    m_lastStringError = "Error at column " + std::to_string(columnIndex + 1)
                                        + " for the label \"" + columnLabels[dimensionIndex]
                                        + "\" in dimension " + std::to_string(dimensionIndex + 1)
                                        + " is trying to reset label to \"" + columnLabels[dimensionIndex]
                                        + "\" that have been already set to \"" + labelsInDimensions[dimensionIndex][positionInCurrentDimension]
                                        + "\"";
                    m_logError = LogErrorCodes_WrongHeader;
                    return false;
                }
            }
            else {
                // if label is already set, check that it'st the same, if it's not, there is an error
                if (labelsInDimensions[dimensionIndex][positionInCurrentDimension] != columnLabels[dimensionIndex]
                    && filledLabel[dimensionIndex][positionInCurrentDimension]) {
                    m_lastStringError = "Error at column " + std::to_string(columnIndex + 1) + " for the label \"" + columnLabels[dimensionIndex]
                                        + "\" in dimension " + std::to_string(dimensionIndex + 1) + " is trying to reset label to \"" + columnLabels[
                                            dimensionIndex]
                                        + "\" that have been already set to \"" + labelsInDimensions[dimensionIndex][positionInCurrentDimension] + "\"";
                    m_logError = LogErrorCodes_WrongHeader;
                    return false;
                }
                // if label isn't set, set it
                if (!(filledLabel[dimensionIndex][positionInCurrentDimension])) {
                    labelsInDimensions[dimensionIndex][positionInCurrentDimension] = columnLabels[dimensionIndex];
                    filledLabel[dimensionIndex][positionInCurrentDimension]        = true;
                }
            }
        }

        columnIndex++;
    } while (increasePositionIndexes(positionsInDimensions));

    for (const std::vector<std::string>& dimensionIndex : labelsInDimensions) {
        m_dimLabels.insert(m_dimLabels.end(), dimensionIndex.begin(), dimensionIndex.end());
    }

    m_nSamplePerBuffer = 1;

    return true;
}

bool CCSVHandler::readSampleChunk(const std::string& line, SMatrixChunk& sample, const size_t lineNb)
{
    const auto firstColumn  = std::find(line.cbegin(), line.cend(), SEPARATOR);
    const auto secondColumn = std::find(firstColumn + 1, line.cend(), SEPARATOR);

    if (lineNb % m_nSamplePerBuffer == 0) {
        if (!boost::spirit::qi::parse(line.cbegin(), firstColumn, boost::spirit::qi::double_, sample.startTime)) {
            m_lastStringError = "Invalid value for the start time. Error on line " + std::to_string(lineNb);
            m_logError        = LogErrorCodes_InvalidArgumentException;
            return false;
        }
    }

    if (m_inputTypeID == EStreamType::Signal) {
        if (!boost::spirit::qi::parse(firstColumn + 1, secondColumn, boost::spirit::qi::ulong_long, sample.epoch)) {
            m_lastStringError = "Invalid value for the epoch. Error on line " + std::to_string(lineNb);
            m_logError        = LogErrorCodes_InvalidArgumentException;
            return false;
        }

        sample.endTime = sample.startTime + (double(m_nSamplePerBuffer) / double(m_sampling));
    }
    else {
        sample.epoch = std::numeric_limits<uint64_t>::max();

        if (!boost::spirit::qi::parse(firstColumn + 1, secondColumn, boost::spirit::qi::double_, sample.endTime)) {
            m_lastStringError = "Invalid value for the end time. Error on line " + std::to_string(lineNb);
            m_logError        = LogErrorCodes_InvalidArgumentException;
            return false;
        }
    }

    const size_t eventDurationCol = line.find_last_of(SEPARATOR);
    const size_t eventDateCol     = line.find_last_of(SEPARATOR, eventDurationCol - 1);
    const size_t eventIdCol       = line.find_last_of(SEPARATOR, eventDateCol - 1);

    std::vector<double> colMatrix;

    boost::spirit::qi::phrase_parse(secondColumn + 1, line.cbegin() + eventIdCol, boost::spirit::qi::double_ % SEPARATOR, boost::spirit::ascii::space,
                                    colMatrix);

    if (colMatrix.size() != m_nCol - N_POST_DATA_COL - N_PRE_DATA_COL) {
        m_lastStringError = "Invalid number of channel. Error on line " + std::to_string(lineNb);
        m_logError        = LogErrorCodes_InvalidArgumentException;
        return false;
    }

    if (m_inputTypeID == EStreamType::Signal) {
        for (size_t index = 0; index < m_dimLabels.size(); ++index) { sample.matrix[(index * m_nSamplePerBuffer) + lineNb] = colMatrix[index]; }
    }
    else if (m_inputTypeID == EStreamType::Spectrum) {
        for (size_t index = 0; index < colMatrix.size(); ++index) { sample.matrix[(index * m_nSamplePerBuffer) + lineNb] = colMatrix[index]; }
    }
    else {
        sample.matrix.clear();
        std::move(colMatrix.begin(), colMatrix.end(), std::back_inserter(sample.matrix));
    }

    return true;
}

bool CCSVHandler::readStimulationChunk(const std::string& line, std::vector<SStimulationChunk>& stimulations, const size_t /*lineNb*/)
{
    const size_t eventDurationCol = line.find_last_of(SEPARATOR);
    const size_t eventDateCol     = line.find_last_of(SEPARATOR, eventDurationCol - 1);
    const size_t eventIdCol       = line.find_last_of(SEPARATOR, eventDateCol - 1) + 1;

    if (eventDurationCol == eventDateCol && eventDurationCol == eventIdCol) {
        m_lastStringError = "No separators found in line";
        m_logError        = LogErrorCodes_StimulationSize;
        return false;
    }

    std::vector<uint64_t> stimIDs;
    // pick all time identifiers for the current time
    boost::spirit::qi::phrase_parse(line.cbegin() + eventIdCol, line.cbegin() + eventDateCol, boost::spirit::qi::ulong_long % DATA_SEPARATOR,
                                    boost::spirit::ascii::space, stimIDs);

    std::vector<double> stimDates;
    // pick all time identifiers for the current time
    boost::spirit::qi::phrase_parse(line.cbegin() + eventDateCol + 1, line.cbegin() + eventDurationCol, boost::spirit::qi::double_ % DATA_SEPARATOR,
                                    boost::spirit::ascii::space, stimDates);

    std::vector<double> stimDurations;
    // pick all time identifiers for the current time
    boost::spirit::qi::phrase_parse(line.cbegin() + eventDurationCol + 1, line.cend(), boost::spirit::qi::double_ % DATA_SEPARATOR, boost::spirit::ascii::space,
                                    stimDurations);

    if (stimIDs.size() != stimDates.size() || stimIDs.size() != stimDurations.size()) {
        m_lastStringError = "There is " + std::to_string(stimIDs.size()) + " identifiers, " + std::to_string(stimDates.size()) + " dates, and " + std::
                            to_string(stimDurations.size()) + " durations";
        m_logError = LogErrorCodes_StimulationSize;
        return false;
    }

    for (size_t index = 0; index < stimIDs.size(); ++index) { stimulations.emplace_back(stimIDs[index], stimDates[index], stimDurations[index]); }

    return true;
}

bool CCSVHandler::increasePositionIndexes(std::vector<size_t>& position)
{
    position.back()++;

    for (size_t counter = 1; counter <= position.size(); ++counter) {
        const size_t index = position.size() - counter;

        if ((position[index] + 1) > m_dimSizes[index]) {
            if (index != 0) {
                position[index] = 0;
                position[index - 1]++;
            }
            else if ((position[index] + 1) > m_dimSizes[index]) { return false; }
        }
    }
    return true;
}

bool CCSVHandler::calculateSampleCountPerBuffer()
{
    // get samples per buffer
    std::vector<std::string> lineParts({ "", "0" });
    size_t nSample = 0;

    std::string bufferTemp;

    while (lineParts[SIGNAL_EPOCH_COL_IDX] == "0") {
        std::string line;

        if (!this->streamReader(m_fs, line, END_OF_LINE_CHAR, bufferTemp)) {
            // protect against nSample--, no need here
            nSample++;
            break;
        }

        lineParts.clear();
        this->split(line, SEPARATOR, lineParts);

        if (lineParts.size() != size_t(m_nCol)) {
            m_lastStringError = "File may be corrupt, can't found sample count per buffer";
            return false;
        }

        nSample++;
    }

    // assume that we might read too far
    nSample--;

    if (nSample == 0) {
        m_lastStringError = "File contain no data to get sample count per buffer, or is corrupted";
        return false;
    }

    m_nSamplePerBuffer = nSample;
    return true;
}


CSV_API ICSVHandler* createCSVHandler() { return new CCSVHandler(); }

CSV_API void releaseCSVHandler(ICSVHandler* object) { delete dynamic_cast<CCSVHandler*>(object); }

}  // namespace CSV
}  // namespace OpenViBE
