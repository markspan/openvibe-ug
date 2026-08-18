///-------------------------------------------------------------------------------------------------
/// 
/// \file ovICSV.h
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

#pragma once

#include "defines.h"

#include <queue>
#include <string>
#include <vector>
#include <cstdint>

namespace OpenViBE {
namespace CSV {
struct SMatrixChunk
{
    double startTime;   // in second
    double endTime;     // in second
    std::vector<double> matrix;
    uint64_t epoch;

    SMatrixChunk(const double startTime, const double endTime, const std::vector<double>& matrix, const uint64_t epoch)
        : startTime(startTime), endTime(endTime), matrix(matrix), epoch(epoch) { }
};

struct SStimulationChunk
{
    uint64_t id;
    double date;        // in second
    double duration;    // in second
    SStimulationChunk(const uint64_t id, const double date, const double duration) : id(id), date(date), duration(duration) { }
};


/**
 * Stimulations are added at the end of line for each stream type.
 * However, they can be handled as a stream type if no signal is provided.
 */
enum class EStreamType
{
    StreamedMatrix = 0,
    FeatureVector = 1,
    Signal = 2,
    Spectrum = 3,
    CovarianceMatrix = 4,
    Stimulations = 5,
    Undefined = 6
};

/**
 * @brief Convert stream type to string
 * \param streamType The stream type enumeration value
 * \return The corresponding stream type string
 */
inline std::string toString(const EStreamType streamType)
{
    std::string strType;
    switch (streamType)
    {
        case EStreamType::StreamedMatrix:
            strType = "Streamed Matrix";
            break;
        case EStreamType::FeatureVector:
            strType = "Feature Vector";
            break;
        case EStreamType::Signal:
            strType = "Signal";
            break;
        case EStreamType::Spectrum:
            strType = "Spectrum";
            break;
        case EStreamType::CovarianceMatrix:
            strType = "Covariance Matrix";
            break;
        case EStreamType::Stimulations:
            strType = "Stimulations";
            break;
        case EStreamType::Undefined:
        default:
            strType = "Undefined";
            break;
    }
    return strType;
}

enum ELogErrorCodes
{
    LogErrorCodes_WrongStreamType = -33,
    LogErrorCodes_HeaderNotRead = -32,
    LogErrorCodes_MissingData = -31,
    LogErrorCodes_WrongParameters = -30,
    LogErrorCodes_ErrorWhileWriting = -29,
    LogErrorCodes_CantWriteHeader = -28,
    LogErrorCodes_InvalidArgumentException = -27,
    LogErrorCodes_OutOfRangeException = -26,
    LogErrorCodes_DimensionCountZero = -25,
    LogErrorCodes_WrongDimensionSize = -24,
    LogErrorCodes_EmptyColumn = -23,
    LogErrorCodes_StimulationSize = -22,
    LogErrorCodes_InvalidStimulationArgument = -21,
    LogErrorCodes_WrongSampleDate = -20,
    LogErrorCodes_NegativeStimulation = -19,
    LogErrorCodes_NotEnoughLines = -18,
    LogErrorCodes_SampleNotEmpty = -17,
    LogErrorCodes_WrongLineSize = -16,
    LogErrorCodes_WrongHeader = -15,
    LogErrorCodes_WrongInputType = -14,
    LogErrorCodes_SetInfoOnce = -13,
    LogErrorCodes_NoChannelsName = -12,
    LogErrorCodes_DimensionSizeEmpty = -11,
    LogErrorCodes_DimensionSizeZero = -10,
    LogErrorCodes_MatrixEmpty = -9,
    LogErrorCodes_WrongMatrixSize = -8,
    LogErrorCodes_NoMatrixLabels = -7,
    LogErrorCodes_NoSample = -6,
    LogErrorCodes_DateError = -5,
    LogErrorCodes_DurationError = -4,
    LogErrorCodes_CantOpenFile = -3,
    LogErrorCodes_NoFileDefined = -2,
    LogErrorCodes_ErrorWhileClosing = -1,
    LogErrorCodes_NoError = 0
};

enum class EFileAccessMode
{
    Write,
    Append,
    Read
};

class CSV_API ICSVHandler
{
public:
    /**
     * \brief Return the number of digit of float numbers in output file.
     *
     * \return size_t number of digits
     */
    virtual size_t getOutputFloatPrecision() = 0;

    /**
     * \brief Set the number of digits of float numbers in output file.
     *
     * \param precision number of digits
     */
    virtual void setOutputFloatPrecision(const size_t precision) = 0;

    /**
     * \brief Set the format type that will be written or read.
     *
     * \param typeID Could be Streamed Matrix, Signal, Covariance Matrix, etc ...
     */
    virtual void setFormatType(const EStreamType typeID) = 0;

    /**
     * \brief Return the format type
     *
     * \return EStreamType format type
     */
    virtual EStreamType getFormatType() = 0;

    /**
     * \brief Set the state of the LastMatrixOnly mode.
     *
     * \param isActivated bool setting it to true will activate the LastMatrixOnly mode, false will disable it.
     */
    virtual void setLastMatrixOnlyMode(const bool isActivated) = 0;

    /**
     * \brief Return the state of the LastMatrixOnly mode
     *
     * \retval true if LastMatrixOnly mode is activated
     * \retval false if this is not (normal mode)
     */
    virtual bool getLastMatrixOnlyMode() = 0;

    /**
     * \brief Set informations to read or write signal data
     *
     * \param channelNames all channels names for the matrix
     * \param sampling sampling frequency
     * \param nSamplePerBuffer number of sample per buffer
     *
     * \retval true in case of success
     * \retval false in case of incorrect or incomplete parameters
     */
    virtual bool setSignalInformation(const std::vector<std::string>& channelNames, size_t sampling, size_t nSamplePerBuffer) = 0;

    /**
     * \brief Get signal information in file
     *
     * \param channelNames reference to fill with file channel names
     * \param sampling sampling frequency
     * \param nSamplePerBuffer number of sample per buffer
     *
     * \retval true in case of success
     * \retval false in case of failure
     */
    virtual bool getSignalInformation(std::vector<std::string>& channelNames, size_t& sampling, size_t& nSamplePerBuffer) = 0;

    /**
     * \brief Set informations to read or write spectrum data
     *
     * \param channelNames Channels names of the matrix
     * \param frequencyAbscissa Frequencies abscissa of the measurements
     * \param sampling Sampling rate of the original measurements
     *
     * \retval true in case of success
     * \retval false in case of incorrect or incomplete parameters
     */
    virtual bool setSpectrumInformation(const std::vector<std::string>& channelNames, const std::vector<double>& frequencyAbscissa, size_t sampling) = 0;

    /**
     * \brief get spectrum information in file
     *
     * \param channelNames Channels names of the matrix
     * \param frequencyAbscissa Frequencies abscissa of the measurements
     * \param sampling Sampling rate of the original measurements
     *
     * \retval true in case of success
     * \retval false in case of failure
     */
    virtual bool getSpectrumInformation(std::vector<std::string>& channelNames, std::vector<double>& frequencyAbscissa, size_t& sampling) = 0;

    /**
     * \brief Set informations to read or write vector data
     *
     * \param channelNames all channels names for the matrix
     *
     * \retval true in case of success
     * \retval false in case of incorrect or incomplete parameters
     */
    virtual bool setFeatureVectorInformation(const std::vector<std::string>& channelNames) = 0;

    /**
     * \brief Get feature vector information in file
     *
     * \param channelNames reference to fill with file channel names
     *
     * \retval true in case of success
     * \retval false in case of failure
     */
    virtual bool getFeatureVectorInformation(std::vector<std::string>& channelNames) = 0;

    /**
     * \brief Set informations to read or write streamed or covariance matrix data
     *
     * \param dimensionSizes size of each dimension
     * \param labels all channels names for the matrix
     *
     * \retval true in case of success
     * \retval false in case of incorrect or incomplete parameters
     */
    virtual bool setStreamedMatrixInformation(const std::vector<size_t>& dimensionSizes, const std::vector<std::string>& labels) = 0;

    /**
     * \brief Get streamed or covariance matrix information in file
     *
     * \param dimensionSizes size of each dimension
     * \param labels reference to fill with file channel names
     *
     * \retval true in case of success
     * \retval false in case of error
     */
    virtual bool getStreamedMatrixInformation(std::vector<size_t>& dimensionSizes, std::vector<std::string>& labels) = 0;

    /**
     * \brief Write the header if available in the file
     *
     * \retval true in case of writing header
     * \retval false in case of already written header
     */
    virtual bool writeHeaderToFile() = 0;

    /**
     * \brief Write the matrix data in the opened file
     *
     * \retval true in case of success
     * \retval false in case of error while writing
     */
    virtual bool writeDataToFile() = 0;

    /**
     * \brief Write all the remaining data
     *
     * \retval true in case of sucess
     * \retval false in case of error while writing
     */
    virtual bool writeAllDataToFile() = 0;

    /**
     * \brief Read samples and events on the specified number of chunks
     *
     * \param chunksToRead number of chunks to read
     * \param samples structures reference to put the data in
     * \param events reference to a vector of event structure to put the data in
     *
     * \retval true in case of sucess
     * \retval false in case of error while reading
     */
    virtual bool readSamplesAndEventsFromFile(size_t chunksToRead, std::vector<SMatrixChunk>& samples, std::vector<SStimulationChunk>& events) = 0;

    /**
     * \brief Reads the specified amount of events from the file
     *
     * \param stimsToRead Number of stimulations to read
     * \param events Reference to a vector of event structure to put the data in
     *
     * \retval true in case of success
     * \retval false in case of error while reading
     */
    virtual bool readEventsFromFile(size_t stimsToRead, std::vector<SStimulationChunk>& events) = 0;
    /**
     * \brief Open file specified on parameter
     *
     * \param filename is the filename of the file to open
     * \param mode The mode to use to open the file
     *
     * \retval true in case of success
     * \retval false in case of error while opening the file
     */
    virtual bool openFile(const std::string& filename, EFileAccessMode mode) = 0;

    /**
     * \brief close the opened file
     *
     * \retval true in case of sucess
     * \retval false in case of error while closing
     */
    virtual bool closeFile() = 0;

    /**
     * \brief Parsing header from opened file
     *
     * \retval true if the header was read correctly
     * \retval false if no header or header corrupted
     */
    virtual bool parseHeader() = 0;

    /**
     * \brief Add a sample of the data type
     *
     * \param sample is the sample to add
     *
     * \retval true in case of correct sample
     * \retval false in case of empty or incorrect sample
     */
    virtual bool addSample(const SMatrixChunk& sample) = 0;

    /**
     * \brief Add a buffer (multiple samples) of the data type.
     *
     * \param samples to add
     *
     * \retval true in case of success
     * \retval false in case of error in the added sample
     */
    virtual bool addBuffer(const std::vector<SMatrixChunk>& samples) = 0;

    /**
     * \brief Add an event
     *
     * \param code is the event identifier
     * \param date in seconds (must be positive)
     * \param duration in seconds (must be positive)
     *
     * \retval true in case of correct event
     * \retval false in case of incorrect param
     */
    virtual bool addEvent(uint64_t code, double date, double duration) = 0;

    /**
     * \brief Add an event
     *
     * \param event is the event to add (with identifier, data and duration)
     *
     * \retval true in case of success
     * \retval false in case of incorrect event
     */
    virtual bool addEvent(const SStimulationChunk& event) = 0;

    /**
     * \brief Set the time of the next stimulation
     *
     * \param date the date of the next stimulation
     *
     * \retval true in case of success
     * \retval false in case of negative date
     */
    virtual bool noEventsUntilDate(double date) = 0;

    /**
     * \brief Return the error last log error.
     *
     * \return last log error code
     */
    virtual ELogErrorCodes getLastLogError() = 0;

    /**
     * \brief Return the error associated to the code in parameter.
     *
     * \param code error code
     *
     * \return error string
     */
    static std::string getLogError(ELogErrorCodes code);

    /**
     * \brief Return additionnal information on the error in certain cases
     *
     * \return string additionnal information
     */
    virtual std::string getLastErrorString() = 0;

    virtual bool hasDataToRead() const = 0;
};

extern CSV_API ICSVHandler* createCSVHandler();

extern CSV_API void releaseCSVHandler(ICSVHandler* object);
}   // namespace CSV
}   // namespace OpenViBE
