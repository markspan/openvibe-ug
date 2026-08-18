///-------------------------------------------------------------------------------------------------
/// 
/// \file ovCCSV.hpp
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

#include <ovICSV.h>

#include <fstream>
#include <string>
#include <vector>

namespace OpenViBE {
namespace CSV {
class CCSVHandler final : public ICSVHandler
{
public:
    /**
     * \brief Set lib value to default
     */
    CCSVHandler() : m_inputTypeID(EStreamType::StreamedMatrix), m_dimSizes({}) {}

    /**
     * \brief Close the file if it is open.
     */
    ~CCSVHandler() { this->closeFile(); }

    /**
     * \brief Get the floating point precision used to write float values.
     *
     * \return the Floating point precision.
     */
    size_t getOutputFloatPrecision() override { return m_oPrecision; }

    /**
     * \brief Set the floating point precision used to write float values.
     *
     * \param precision the floating point precision.
     */
    void setOutputFloatPrecision(const size_t precision) override { m_oPrecision = precision; }

    void setFormatType(const EStreamType typeID) override;
    EStreamType getFormatType() override { return m_inputTypeID; }

    void setLastMatrixOnlyMode(const bool isActivated) override { m_lastMatrixOnly = isActivated; }
    bool getLastMatrixOnlyMode() override { return m_lastMatrixOnly; }

    bool setSignalInformation(const std::vector<std::string>& channelNames, size_t sampling, size_t nSamplePerBuffer) override;
    bool getSignalInformation(std::vector<std::string>& channelNames, size_t& sampling, size_t& nSamplePerBuffer) override;

    bool setSpectrumInformation(const std::vector<std::string>& channelNames, const std::vector<double>& frequencyAbscissa, size_t sampling) override;
    bool getSpectrumInformation(std::vector<std::string>& channelNames, std::vector<double>& frequencyAbscissa, size_t& sampling) override;

    bool setFeatureVectorInformation(const std::vector<std::string>& channelNames) override;
    bool getFeatureVectorInformation(std::vector<std::string>& channelNames) override;

    bool setStreamedMatrixInformation(const std::vector<size_t>& dimSizes, const std::vector<std::string>& labels) override;
    bool getStreamedMatrixInformation(std::vector<size_t>& dimSizes, std::vector<std::string>& labels) override;

    /**
     * \brief Write the header to the file
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool writeHeaderToFile() override;

    /**
     * \brief Write current available data to the file until the last stimulation or if you set that it will not have new event before a date.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     *
     * \sa noEventsUntilDate
     */
    bool writeDataToFile() override;

    /**
     * \brief Write current available data to the file.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool writeAllDataToFile() override;

    /**
     * \brief Read samples and stimulations.
     *
     * \param lineNb Maximum number of lines to read. If there is no more data in the file, the number of lines read can be lower.
     * \param chunks [out] Valid chunks read.
     * \param stimulations [out] Valid stimulations read.
     *
     * \retval True in case of success, even if the number of lines is different than the lineNb parameter.
     * \retval False in case of error.
     */
    bool readSamplesAndEventsFromFile(size_t lineNb, std::vector<SMatrixChunk>& chunks, std::vector<SStimulationChunk>& stimulations) override;

    /**
     * \brief Reads the specified amount of events from the file
     * If end of file is reached, then less than the required amount of stims may be returned.
     *
     * \param stimsToRead Number of stimulations to read
     * \param events Reference to a vector of event structure to put the data in
     *
     * \retval true in case of success
     * \retval false in case of error while reading
     */
    virtual bool readEventsFromFile(size_t stimsToRead, std::vector<SStimulationChunk>& events) override;

    /**
     * \brief Open a OV CSV file.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool openFile(const std::string& fileName, EFileAccessMode mode) override;

    /**
     * \brief Close the opened file.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool closeFile() override;

    /**
     * \brief Parsing header of opened file
     *
     * \retval true if the header was read correctly
     * \retval false if no header or header corrupted
     */
    bool parseHeader() override;

    /**
     * \brief Add a single sample.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool addSample(const SMatrixChunk& sample) override;

    /**
     * \brief Add several samples.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool addBuffer(const std::vector<SMatrixChunk>& samples) override;

    /**
     * \brief Add a single stimulation.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool addEvent(uint64_t code, double date, double duration) override;

    /**
     * \brief Add several stimulations.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     */
    bool addEvent(const SStimulationChunk& event) override;

    /**
     * \brief Guarantee that will not have new event before a date.
     * This information is used to allow the library to write all the chunks available, before this date, to a file.
     *
     * \retval True in case of success.
     * \retval False in case of error.
     *
     * \sa writeDataToFile
     */
    bool noEventsUntilDate(double date) override;

    ELogErrorCodes getLastLogError() override { return m_logError; }

    std::string getLastErrorString() override { return m_lastStringError; }

    /**
     * \brief Check if there is still data to read in the file.
     *
     * \retval True if there is still data to read in the file.
     * \retval False if there is no more data to read in the file.
     */
    bool hasDataToRead() const override { return m_hasDataToRead; }

private:
    /**
     * \brief Split a string into a vector of strings.
     *
     * \param in String to split.
     * \param delimiter Delimitor.
     * \param out [out] Vector of string.
     */
    void split(const std::string& in, char delimiter, std::vector<std::string>& out) const;

    /**
     * \brief Create a string with stimulations to add in the buffer.
     *
     * \param stimulationsToPrint stimulations to put into the buffer
     *
     * \return string with stimulations to write
     */
    std::string stimulationsToString(const std::vector<SStimulationChunk>& stimulationsToPrint) const;

    /**
     * \brief Create a string representation of the header data.
     *
     * \retval true Header data as it should be written in the file
     * \retval "" in case of error
     */
    std::string createHeaderString();

    /**
     * \brief Set the buffer in function of data saved.
     *
     * \param canWriteAll true if it must write all lines, false if it write only the next buffer
     * \param csv The CSV string.
     *
     * \retval true in case of success
     * \retval false in case of wrong data sent
     */
    bool createCSVStringFromData(bool canWriteAll, std::string& csv);

    /**
     * \brief Extracts file format from header
     * Sets member variable m_inputTypeID accordingly
     *
     * \param header The header of the file
     *
     */
    void extractFormatType(const std::string& header);

    /**
     * \brief Parsing header to read signal data.
     *
     * \param header Header to parse.
     *
     * \retval true in case of correct header
     * \retval false in case of incorrect header
     */
    bool parseSignalHeader(const std::vector<std::string>& header);

    /**
    * \brief Parsing header to read Spectrum data.
    *
    * \param header Header to parse.
    *
    * \retval true in case of correct header
    * \retval false in case of incorrect header
    */
    bool parseSpectrumHeader(const std::vector<std::string>& header);

    /**
    * \brief Parsing header to read matrix data (Streamed Matrix, Covariance matrix and Feature Vector).
    *
    * \param header Header to parse.
    *
    * \retval true in case of correct header
    * \retval false in case of incorrect header
    */
    bool parseMatrixHeader(const std::vector<std::string>& header);

    /**
     * \brief Read line data concerning time, epoch and matrix.
     *
     * \param line line to read
     * \param sample [out] : reference to stock data in
     * \param lineNb index of the read line
     *
     * \retval true in case of success
     * \retval false in case of error (as letters instead of numbers)
     */
    bool readSampleChunk(const std::string& line, SMatrixChunk& sample, size_t lineNb);

    /**
     * \brief Read line data conerning stimulations.
     * \param line the line to read
     * \param stimulations [out] : vector to stock stimulations in (identifier, date and duration)
     * \param lineNb the line actually reading
     *
     * \retval true in case of success
     * \retval false in case of error (as letters instead of numbers)
     */
    bool readStimulationChunk(const std::string& line, std::vector<SStimulationChunk>& stimulations, size_t lineNb);

    /**
     * \brief Update position into the matrix while reading or writing.
     * \param position [out] : is the position into the matrix
     *
     * \retval true in case of success
     * \retval false in case of browse matrix
     */
    bool increasePositionIndexes(std::vector<size_t>& position);

    /**
     * \brief Read lines of the first epoch to found sample count per buffer.
     *
     * \retval true in case of success
     * \retval false in case of error
     */
    bool calculateSampleCountPerBuffer();

    /**
     * \brief Read a stream until a delimiter and provide the string before the delimiter.
     *
     * \param in The stream to read.
     * \param out The string before the next delimitor.
     * \param delimiter The delimiter.
     * \param bufferHistory The buffer history.
     */
    bool streamReader(std::istream& in, std::string& out, char delimiter, std::string& bufferHistory) const;

    std::fstream m_fs;
    std::string m_filename;
    std::deque<SMatrixChunk> m_chunks;
    std::deque<SStimulationChunk> m_stimulations;
    ELogErrorCodes m_logError     = LogErrorCodes_NoError;
    std::string m_lastStringError = "";

    EStreamType m_inputTypeID = EStreamType::Undefined;

    typedef std::istream& GetLine(std::istream& in, std::string& out, char delimiter);
    size_t m_nDim = 0;
    std::vector<size_t> m_dimSizes;
    std::vector<std::string> m_dimLabels;
    size_t m_nSamplePerBuffer = 0;
    double m_noEventSince     = 0;

    std::vector<double> m_frequencyAbscissa;

    size_t m_sampling = 0;
    size_t m_nCol     = 0;

    bool m_isFirstLineWritten = false;
    bool m_isHeaderRead       = false;
    bool m_isSetInfoCalled    = false;
    bool m_hasEpoch           = false;

    size_t m_nSampleOriginal = 0;
    size_t m_oPrecision      = 10;

    bool m_lastMatrixOnly = false;

    std::string m_bufferReadFileLine; // Buffer used to store unused read chars.

    bool m_hasDataToRead = true;

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            bool m_isCRLFEOL = false; // Is a CRLF end of line
#endif
};
}  // namespace CSV
}  // namespace OpenViBE
