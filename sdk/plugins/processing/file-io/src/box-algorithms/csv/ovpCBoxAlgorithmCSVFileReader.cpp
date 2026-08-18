#include "ovpCBoxAlgorithmCSVFileReader.h"
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <cmath>  // std::ceil() on Linux

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

namespace {
std::vector<std::string> split(const std::string& sString, const std::string& c)
{
    std::vector<std::string> result;
    size_t i = 0;
    size_t j;
    while ((j = sString.find(c, i)) != std::string::npos)
    {
        result.push_back(std::string(sString, i, j - i));
        i = j + c.size();
    }
    //the last element without the \n character
    result.push_back(std::string(sString, i, sString.size() - 1 - i));

    return result;
}

void clearMatrix(std::vector<std::vector<std::string>>& vMatrix)
{
    for (size_t i = 0; i < vMatrix.size(); ++i) { vMatrix[i].clear(); }
    vMatrix.clear();
}
}  // namespace

bool CBoxAlgorithmCSVFileReader::initialize()
{
    m_sampling = 0;
    m_encoder  = nullptr;

    this->getStaticBoxContext().getOutputType(0, m_typeID);

    m_filename          = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const CString token = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_separator         = token.toASCIIString();
    m_doNotUseFileTime  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_samplesPerBuffer  = 1;
    if (m_typeID == OV_TypeId_ChannelLocalisation) { m_channelsPerBuffer = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3); }
    else if (m_typeID != OV_TypeId_Stimulations && m_typeID != OV_TypeId_Spectrum)
    {
        m_samplesPerBuffer = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    }

    m_nextTime = 0.;

    m_startTime = 0;
    m_endTime   = 0;

    return true;
}

bool CBoxAlgorithmCSVFileReader::uninitialize()
{
    if (m_file)
    {
        fclose(m_file);
        m_file = nullptr;
    }
    if (m_encoder)
    {
        m_encoder->uninitialize();
        delete m_encoder;
        m_encoder = nullptr;
    }
    return true;
}

bool CBoxAlgorithmCSVFileReader::initializeFile()
{
    //open file,  we don't open as binary as that gives us \r\n on Windows as line-endings and leaves a dangling char after split. CSV files should be text.
    m_file = fopen(m_filename.toASCIIString(), "r");

    OV_ERROR_UNLESS_KRF(m_file, "Error opening file [" << m_filename << "] for reading", Kernel::ErrorType::BadFileRead);

    // simulate RAII through closure
    const auto releaseResources = [&]()
    {
        fclose(m_file);
        m_file = nullptr;
    };

    //read the header
    char line[BUFFER_LEN];
    char* result = fgets(line, BUFFER_LEN, m_file);
    if (nullptr == result)
    {
        releaseResources();
        OV_ERROR_KRF("Error reading data from file", Kernel::ErrorType::BadParsing);
    }

    m_headerFiles = split(std::string(line), m_separator);
    m_nCol        = m_headerFiles.size();

    if (m_typeID == OV_TypeId_ChannelLocalisation)
    {
        m_encoder = new Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmCSVFileReader>(*this, 0);
        //number of column without the column contains the dynamic parameter
        //m_nCol-=1;
        m_realProcess = &CBoxAlgorithmCSVFileReader::processChannelLocalisation;
    }
    else if (m_typeID == OV_TypeId_FeatureVector)
    {
        m_encoder          = new Toolkit::TFeatureVectorEncoder<CBoxAlgorithmCSVFileReader>(*this, 0);
        m_realProcess      = &CBoxAlgorithmCSVFileReader::processFeatureVector;
        m_samplesPerBuffer = 1;
    }
    else if (m_typeID == OV_TypeId_Spectrum)
    {
        m_encoder     = new Toolkit::TSpectrumEncoder<CBoxAlgorithmCSVFileReader>(*this, 0);
        m_realProcess = &CBoxAlgorithmCSVFileReader::processSpectrum;

        //number of column without columns contains min max frequency bands parameters
        m_nCol -= 2;
    }
    else if (m_typeID == OV_TypeId_Signal)
    {
        m_encoder     = new Toolkit::TSignalEncoder<CBoxAlgorithmCSVFileReader>(*this, 0);
        m_realProcess = &CBoxAlgorithmCSVFileReader::processSignal;

        //find the sampling rate
        result = fgets(line, BUFFER_LEN, m_file);

        if (nullptr == result)
        {
            releaseResources();
            OV_ERROR_KRF("Error reading sampling rate from file", Kernel::ErrorType::BadParsing);
        }

        std::vector<std::string> parsed = split(std::string(line), m_separator);

        if ((m_nCol - 1) >= parsed.size())
        {
            releaseResources();
            OV_ERROR_KRF("Error reading columns (not enough columns found) from file", Kernel::ErrorType::BadParsing);
        }

        const double sampling = double(atof(parsed[m_nCol - 1].c_str()));
        if (ceil(sampling) != sampling)
        {
            releaseResources();
            OV_ERROR_KRF("Invalid fractional sampling rate (" << sampling << ") in file", Kernel::ErrorType::BadValue);
        }

        m_sampling = uint64_t(sampling);

        if (m_sampling == 0)
        {
            releaseResources();
            OV_ERROR_KRF("Invalid NULL sampling rate in file", Kernel::ErrorType::BadValue);
        }

        // Skip the header
        rewind(m_file);
        result = fgets(line, BUFFER_LEN, m_file);
        if (nullptr == result)
        {
            releaseResources();
            OV_ERROR_KRF("Error reading data from file", Kernel::ErrorType::BadParsing);
        }

        //number of column without the column contains the sampling rate parameters
        m_nCol -= 1;
    }
    else if (m_typeID == OV_TypeId_StreamedMatrix)
    {
        m_encoder     = new Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmCSVFileReader>(*this, 0);
        m_realProcess = &CBoxAlgorithmCSVFileReader::processStreamedMatrix;
    }
    else if (m_typeID == OV_TypeId_Stimulations)
    {
        m_encoder     = new Toolkit::TStimulationEncoder<CBoxAlgorithmCSVFileReader>(*this, 0);
        m_realProcess = &CBoxAlgorithmCSVFileReader::processStimulation;
    }
    else
    {
        releaseResources();
        OV_ERROR_KRF("Invalid input type identifier " << this->getTypeManager().getTypeName(m_typeID) << " in file ", Kernel::ErrorType::BadValue);
    }

    return true;
}

bool CBoxAlgorithmCSVFileReader::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmCSVFileReader::process()
{
    if (m_file == nullptr) { OV_ERROR_UNLESS_KRF(initializeFile(), "Error reading data from csv file " << m_filename, Kernel::ErrorType::Internal); }
    //line buffer
    char line[BUFFER_LEN];
    const double currentTime = CTime(getPlayerContext().getCurrentTime()).toSeconds();

    //if no line was read, read the first data line.
    if (m_lastLineSplits.empty())
    {
        //next line
        size_t nSamples = 0;
        while (!feof(m_file) && nSamples < m_samplesPerBuffer && fgets(line, BUFFER_LEN, m_file) != nullptr)
        {
            m_lastLineSplits = split(std::string(line), m_separator);

            nSamples++;

            if (m_typeID != OV_TypeId_Stimulations
                && m_typeID != OV_TypeId_Spectrum
                && m_typeID != OV_TypeId_ChannelLocalisation) { m_dataMatrices.push_back(m_lastLineSplits); }
        }
        if ((m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_Signal)
            && feof(m_file) && nSamples < m_samplesPerBuffer)
        {
            // Last chunk will be partial, zero the whole output matrix...
            CMatrix* iMatrix = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();
            iMatrix->resetBuffer();
        }
    }

    bool somethingToSend = (!m_lastLineSplits.empty()) && atof(m_lastLineSplits[0].c_str()) < currentTime;
    somethingToSend |= (m_typeID == OV_TypeId_Stimulations); // we always send a stim chunk, even if empty

    if (m_typeID == OV_TypeId_Stimulations || m_typeID == OV_TypeId_ChannelLocalisation || m_typeID == OV_TypeId_Spectrum)
    {
        while (!m_lastLineSplits.empty() && atof(m_lastLineSplits[0].c_str()) < currentTime)
        {
            m_dataMatrices.push_back(m_lastLineSplits);

            somethingToSend = true;

            if (!feof(m_file) && fgets(line, BUFFER_LEN, m_file) != nullptr) { m_lastLineSplits = split(std::string(line), m_separator); }
            else { m_lastLineSplits.clear(); }
        }
    }

    //convert data to the good output type

    if (somethingToSend)
    {
        // Encode the data
        OV_ERROR_UNLESS_KRF((this->*m_realProcess)(), "Error encoding data from csv file " << m_filename << " into the right output format",
                            Kernel::ErrorType::Internal);

        //for the stimulation, the line contents in m_vLastLineSplit isn't processed.
        if (m_typeID != OV_TypeId_Stimulations && m_typeID != OV_TypeId_Spectrum && m_typeID != OV_TypeId_ChannelLocalisation) { m_lastLineSplits.clear(); }

        //clear the Data Matrix.
        clearMatrix(m_dataMatrices);
    }
    return true;
}

bool CBoxAlgorithmCSVFileReader::processStreamedMatrix()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    CMatrix* iMatrix           = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();

    //Header
    if (!m_headerSent)
    {
        iMatrix->resize(m_nCol - 1, m_samplesPerBuffer);

        for (size_t i = 1; i < m_nCol; ++i) { iMatrix->setDimensionLabel(0, i - 1, m_headerFiles[i].c_str()); }
        m_encoder->encodeHeader();
        m_headerSent = true;

        boxContext.markOutputAsReadyToSend(0, 0, 0);
    }

    OV_ERROR_UNLESS_KRF(convertVectorDataToMatrix(iMatrix), "Error converting vector data to streamed matrix", Kernel::ErrorType::Internal);

    m_encoder->encodeBuffer();

    if (m_doNotUseFileTime)
    {
        m_startTime = m_endTime;
        m_endTime   = this->getPlayerContext().getCurrentTime();
    }
    else
    {
        m_startTime = CTime(atof(m_dataMatrices[0][0].c_str())).time();
        m_endTime   = CTime(atof(m_dataMatrices.back()[0].c_str())).time();
    }

    boxContext.markOutputAsReadyToSend(0, m_startTime, m_endTime);

    return true;
}

bool CBoxAlgorithmCSVFileReader::processStimulation()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    //Header
    if (!m_headerSent)
    {
        m_encoder->encodeHeader();
        m_headerSent = true;

        boxContext.markOutputAsReadyToSend(0, 0, 0);
    }

    CStimulationSet* ip_stimSet = static_cast<Toolkit::TStimulationEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputStimulationSet();
    ip_stimSet->clear();

    for (size_t i = 0; i < m_dataMatrices.size(); ++i)
    {
        OV_ERROR_UNLESS_KRF(m_dataMatrices[i].size() == 3, "Invalid data row length: must be 3 for stimulation date, index and duration",
                            Kernel::ErrorType::BadParsing);

        const uint64_t date     = CTime(atof(m_dataMatrices[i][0].c_str())).time();
        const uint64_t id       = uint64_t(atof(m_dataMatrices[i][1].c_str()));
        const uint64_t duration = CTime(atof(m_dataMatrices[i][2].c_str())).time();

        ip_stimSet->push_back(id, date, duration);
    }

    m_encoder->encodeBuffer();

    // Never use file time
    m_startTime = m_endTime;
    m_endTime   = this->getPlayerContext().getCurrentTime();

    boxContext.markOutputAsReadyToSend(0, m_startTime, m_endTime);

    return true;
}

bool CBoxAlgorithmCSVFileReader::processSignal()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    CMatrix* iMatrix           = static_cast<Toolkit::TSignalEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();

    //Header
    if (!m_headerSent)
    {
        // This is the first chunk, find out the start time from the file
        // (to keep time chunks continuous, start time is previous end time, hence set end time)
        if (!m_doNotUseFileTime) { m_endTime = CTime(atof(m_dataMatrices[0][0].c_str())).time(); }

        iMatrix->resize(m_nCol - 1, m_samplesPerBuffer);

        for (size_t i = 1; i < m_nCol; ++i) { iMatrix->setDimensionLabel(0, i - 1, m_headerFiles[i].c_str()); }

        static_cast<Toolkit::TSignalEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputSamplingRate() = m_sampling;

        m_encoder->encodeHeader();
        m_headerSent = true;

        this->getDynamicBoxContext().markOutputAsReadyToSend(0, 0, 0);
    }

    OV_ERROR_UNLESS_KRF(convertVectorDataToMatrix(iMatrix), "Error converting vector data to signal", Kernel::ErrorType::Internal);

    // this->getLogManager() << Kernel::LogLevel_Info << "Cols from header " << m_nCol << "\n";
    // this->getLogManager() << Kernel::LogLevel_Info << "InMatrix " << (m_dataMatrices.size() > 0 ? m_dataMatrices[0].size() : 0) << " outMatrix " << iMatrix->getDimensionSize(0) << "\n";

    m_encoder->encodeBuffer();

    if (m_doNotUseFileTime)
    {
        // We use time dictated by the sampling rate
        m_startTime = m_endTime; // previous time end is current time start
        m_endTime   = m_startTime + CTime(m_sampling, m_samplesPerBuffer).time();
    }
    else
    {
        // We use time suggested by the last sample of the chunk
        m_startTime = CTime(atof(m_dataMatrices[0][0].c_str())).time();
        m_endTime   = CTime(atof(m_dataMatrices.back()[0].c_str())).time();
    }

    boxContext.markOutputAsReadyToSend(0, m_startTime, m_endTime);

    return true;
}

bool CBoxAlgorithmCSVFileReader::processChannelLocalisation()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    CMatrix* iMatrix           = static_cast<Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();

    if (!m_headerSent)
    {
        iMatrix->resize(m_nCol - 1, m_samplesPerBuffer);

        for (size_t i = 1; i < m_nCol; ++i) { iMatrix->setDimensionLabel(0, i - 1, m_headerFiles[i].c_str()); }

        static_cast<Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputDynamic() = false;
        //atoi(m_dataMatrices[0][m_nCol].c_str());

        m_encoder->encodeHeader();

        boxContext.markOutputAsReadyToSend(0, 0, 0);

        m_headerSent = true;
    }

    std::vector<std::vector<std::string>> channelBloc;
    for (size_t i = 0; i < m_dataMatrices.size(); ++i) { channelBloc.push_back(m_dataMatrices[i]); }

    //clear matrix
    clearMatrix(m_dataMatrices);

    for (size_t i = 0; i < channelBloc.size(); ++i)
    {
        m_dataMatrices.push_back(channelBloc[i]);

        //send the current bloc if the next data hasn't the same date
        if (i >= channelBloc.size() - 1 || channelBloc[(i + 1)][0] != m_dataMatrices[0][0])
        {
            OV_ERROR_UNLESS_KRF(convertVectorDataToMatrix(iMatrix), "Error converting vector data to channel localisation", Kernel::ErrorType::Internal);

            m_encoder->encodeBuffer();
            const uint64_t date = CTime(atof(m_dataMatrices[0][0].c_str())).time();
            boxContext.markOutputAsReadyToSend(0, date, date);

            //clear matrix
            clearMatrix(m_dataMatrices);
        }
    }

    //clear matrix
    clearMatrix(channelBloc);

    return true;
}

bool CBoxAlgorithmCSVFileReader::processFeatureVector()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    CMatrix* matrix            = static_cast<Toolkit::TFeatureVectorEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();

    //Header
    if (!m_headerSent)
    {
        // in this case we need to transpose it
        CMatrix* iMatrix = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();

        iMatrix->resize(m_nCol - 1);

        for (size_t i = 1; i < m_nCol; ++i) { iMatrix->setDimensionLabel(0, i - 1, m_headerFiles[i].c_str()); }

        m_encoder->encodeHeader();

        boxContext.markOutputAsReadyToSend(0, 0, 0);

        m_headerSent = true;
    }

    // Each vector has to be sent separately
    for (size_t i = 0; i < m_dataMatrices.size(); ++i)
    {
        OV_ERROR_UNLESS_KRF(m_dataMatrices[i].size() == m_nCol,
                            "Unexpected number of elements" << "(got " << uint64_t(m_dataMatrices[i].size()) << ", expected " << m_nCol << ")",
                            Kernel::ErrorType::BadParsing);

        for (size_t j = 0; j < m_nCol - 1; ++j) { matrix->getBuffer()[j] = atof(m_dataMatrices[i][j + 1].c_str()); }

        m_encoder->encodeBuffer();

        const uint64_t date = CTime(atof(m_dataMatrices[i][0].c_str())).time();
        boxContext.markOutputAsReadyToSend(0, date, date);
    }

    clearMatrix(m_dataMatrices);

    return true;
}

bool CBoxAlgorithmCSVFileReader::processSpectrum()
{
    Kernel::IBoxIO& boxContext  = this->getDynamicBoxContext();
    CMatrix* iMatrix            = static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputMatrix();
    CMatrix* iFrequencyAbscissa = static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputFrequencyAbscissa();

    //Header
    if (!m_headerSent)
    {
        iMatrix->resize(m_nCol - 1, m_dataMatrices.size());

        for (size_t i = 1; i < m_nCol; ++i) { iMatrix->setDimensionLabel(0, i - 1, m_headerFiles[i].c_str()); }
        iFrequencyAbscissa->resize(m_dataMatrices.size());
        if (m_dataMatrices.size() > 1)
        {
            for (size_t i = 0; i < m_dataMatrices.size(); ++i)
            {
                const double curFrequencyAbscissa = std::stod(m_dataMatrices[i][m_nCol]) + double(i) / (m_dataMatrices.size() - 1)
                                                    * (std::stod(m_dataMatrices[i][m_nCol + 1]) - std::stod(m_dataMatrices[i][m_nCol]));
                iFrequencyAbscissa->getBuffer()[i] = curFrequencyAbscissa;

                std::stringstream label;
                label << curFrequencyAbscissa;
                iFrequencyAbscissa->setDimensionLabel(0, i, label.str().c_str());
            }
        }
        else { iFrequencyAbscissa->getBuffer()[0] = 0; }

        static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmCSVFileReader>*>(m_encoder)->getInputSamplingRate() = uint64_t(
            m_dataMatrices.size() / (stod(m_dataMatrices[m_dataMatrices.size() - 1][m_nCol]) - stod(m_dataMatrices[0][m_nCol])));
        m_headerSent = true;
        m_encoder->encodeHeader();

        boxContext.markOutputAsReadyToSend(0, 0, 0);
    }

    std::vector<std::vector<std::string>> spectrumBloc;
    for (size_t i = 0; i < m_dataMatrices.size(); ++i) { spectrumBloc.push_back(m_dataMatrices[i]); }

    //clear matrix
    clearMatrix(m_dataMatrices);

    for (size_t i = 0; i < spectrumBloc.size(); ++i)
    {
        m_dataMatrices.push_back(spectrumBloc[i]);
        //send the current bloc if the next data hasn't the same date
        if (i >= spectrumBloc.size() - 1 || spectrumBloc[i + 1][0] != m_dataMatrices[0][0])
        {
            OV_ERROR_UNLESS_KRF(convertVectorDataToMatrix(iMatrix), "Error converting vector data to spectrum", Kernel::ErrorType::Internal);

            m_encoder->encodeBuffer();
            const uint64_t date = CTime(std::stod(m_dataMatrices[0][0])).time();
            boxContext.markOutputAsReadyToSend(0, date - 1, date);

            //clear matrix
            clearMatrix(m_dataMatrices);
        }
    }

    //clear matrix
    clearMatrix(spectrumBloc);
    return true;
}

bool CBoxAlgorithmCSVFileReader::convertVectorDataToMatrix(CMatrix* matrix)
{
    // note: Chunk size shouldn't change after encoding header, do not mess with it here, even if the input has different size

    // We accept partial data, but not buffer overruns ...
    OV_ERROR_UNLESS_KRF(matrix->getDimensionSize(1) >= m_dataMatrices.size() && matrix->getDimensionSize(0) >= (m_nCol-1),
                        "Matrix size incompatibility, data suggests " << m_nCol-1 << "x" << m_dataMatrices.size()
                        << ", expected at most " << matrix->getDimensionSize(0) << "x" << matrix->getDimensionSize(0), Kernel::ErrorType::Overflow);

    std::stringstream ss;
    for (size_t i = 0; i < m_dataMatrices.size(); ++i)
    {
        ss << "at time (" << m_dataMatrices[i][0].c_str() << "):";
        for (size_t j = 0; j < m_nCol - 1; ++j)
        {
            matrix->getBuffer()[j * matrix->getDimensionSize(1) + i] = std::stod(m_dataMatrices[i][j + 1]);
            ss << matrix->getBuffer()[j * matrix->getDimensionSize(1) + i] << ";";
        }
        ss << "\n";
    }
    getLogManager() << Kernel::LogLevel_Debug << "Matrix:\n" << ss.str();
    getLogManager() << Kernel::LogLevel_Debug << "Matrix:\n" << ss.str();

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
