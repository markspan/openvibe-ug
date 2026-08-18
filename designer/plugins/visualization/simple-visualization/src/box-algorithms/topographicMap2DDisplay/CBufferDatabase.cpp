#include "CBufferDatabase.hpp"

#include <cmath>
#include <cstring>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

CBufferDatabase::CBufferDatabase(Toolkit::TBoxAlgorithm<IBoxAlgorithm>& plugin) : m_ParentPlugin(plugin)
{
    m_decoder = &m_ParentPlugin.getAlgorithmManager().getAlgorithm(
        m_ParentPlugin.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_ChannelLocalisationDecoder));
    m_decoder->initialize();
    m_DimSizes.fill(0);
}

CBufferDatabase::~CBufferDatabase()
{
    m_decoder->uninitialize();
    m_ParentPlugin.getAlgorithmManager().releaseAlgorithm(*m_decoder);

    //delete all the remaining buffers
    while (!m_SampleBuffers.empty()) {
        delete[] m_SampleBuffers.front();
        m_SampleBuffers.pop_front();
    }

    //delete channel localisation matrices
    while (!m_channelLocalisationCoords.empty()) {
        delete m_channelLocalisationCoords.front().first;
        m_channelLocalisationCoords.pop_front();
    }

    /*while(m_oChannelLocalisationAlternateCoords.size() > 0)
    {
        delete[] m_oChannelLocalisationAlternateCoords.front().first;
        m_oChannelLocalisationAlternateCoords.pop_front();
    }*/
}

bool CBufferDatabase::DecodeChannelLocalisationMemoryBuffer(const CMemoryBuffer* buffer, uint64_t startTime, uint64_t endTime)
{
    //feed memory buffer to decoder
    m_decoder->getInputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_InputParameterId_MemoryBufferToDecode)->setReferenceTarget(&buffer);

    //process buffer
    m_decoder->process();

    //copy header if needed
    if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputTriggerId_ReceivedHeader)) {
        //retrieve matrix header
        Kernel::TParameterHandler<CMatrix*> matrix;
        matrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Matrix));

        //copy channel labels
        m_channelLocalisationLabels.resize(matrix->getDimensionSize(0));
        for (size_t i = 0; i < m_channelLocalisationLabels.size(); ++i) { m_channelLocalisationLabels[i] = matrix->getDimensionLabel(0, i); }

        //retrieve dynamic flag
        Kernel::TParameterHandler<bool> dynamic;
        dynamic.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Dynamic));
        m_dynamicChannelLocalisation = dynamic;

        if (matrix->getDimensionSize(1) == 3) {
            m_cartesianCoords = true;
            /*m_channelLocalisationCartesianCoords = &m_channelLocalisationCoords;
            m_channelLocalisationSphericalCoords = &m_oChannelLocalisationAlternateCoords;*/
        }
        else if (matrix->getDimensionSize(1) == 2) {
            m_cartesianCoords = false;
            /*m_channelLocalisationCartesianCoords = &m_oChannelLocalisationAlternateCoords;
            m_channelLocalisationSphericalCoords = &m_channelLocalisationCoords;*/
        }
        else {
            m_ParentPlugin.getLogManager() << Kernel::LogLevel_Error
                    << "Wrong size found for dimension 1 of Channel localisation header! Can't process header!\n";
            return false;
        }

        //header information received
        m_channelLocalisationHeaderReceived = true;
    }

    //has a chanloc buffer been received?
    if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputTriggerId_ReceivedBuffer)) {
        //number of buffers required to cover displayed time range
        size_t maxNBuffer = 1;

        //resize channel localisation queue if necessary
        if (m_dynamicChannelLocalisation) {
            const uint64_t bufferDuration = endTime - startTime;
            if (bufferDuration != 0) {
                maxNBuffer = size_t(ceil(m_TotalDuration / double(bufferDuration)));
                if (maxNBuffer == 0) { maxNBuffer = 1; }
            }

            //if new number of buffers decreased, resize list and destroy useless buffers
            while (m_channelLocalisationCoords.size() > maxNBuffer) {
                delete[] m_channelLocalisationCoords.front().first;
                m_channelLocalisationCoords.pop_front();
                // delete[] m_oChannelLocalisationAlternateCoords.front().first;
                // m_oChannelLocalisationAlternateCoords.pop_front();
                m_channelLocalisationTimes.pop_front();
            }
        }

        //retrieve coordinates matrix
        Kernel::TParameterHandler<CMatrix*> matrix;
        matrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_ChannelLocalisationDecoder_OutputParameterId_Matrix));

        //get pointer to destination matrix
        CMatrix* channelLocalisation;
        //CMatrix* alternateChannelLocalisation = nullptr;
        if (m_channelLocalisationCoords.size() < maxNBuffer) {
            //create a new matrix and resize it
            channelLocalisation = new CMatrix();
            channelLocalisation->copyDescription(*matrix);
            // alternateChannelLocalisation = new CMatrix();
            // TODO : resize it appropriately depending on whether it is spherical or cartesian
        }
        else //m_channelLocalisationCoords.size() == maxNBuffer
        {
            channelLocalisation = m_channelLocalisationCoords.front().first;
            m_channelLocalisationCoords.pop_front();
            // alternateChannelLocalisation = m_oChannelLocalisationAlternateCoords.front().first;
            // m_oChannelLocalisationAlternateCoords.pop_front();
            m_channelLocalisationTimes.pop_front();
        }

        if (channelLocalisation) {
            //copy coordinates and times
            channelLocalisation->copyContent(*matrix);
            m_channelLocalisationCoords.emplace_back(channelLocalisation, true);
            //m_oChannelLocalisationAlternateCoords.push_back(std::pair<CMatrix*, bool>(alternateChannelLocalisation, true));
            m_channelLocalisationTimes.emplace_back(startTime, endTime);
        }
    }

    return true;
}

bool CBufferDatabase::OnChannelLocalisationBufferReceived(const size_t index)
{
    m_channelLocalisationCoords[index].second = false;
    return true;
}

bool CBufferDatabase::IsFirstChannelLocalisationBufferProcessed()
{
    //at least one chanloc buffer must have been received and processed
    return (!m_channelLocalisationCoords.empty()) && (!m_channelLocalisationCoords[0].second);
}

bool CBufferDatabase::AdjustNumberOfDisplayedBuffers(const double time)
{
    bool nBufferToDisplayChanged = false;

    if (time > 0) {
        m_TotalDuration   = time;
        m_OVTotalDuration = 0;
        m_TotalStep       = 0;
    }

    //return if buffer length is not known yet
    if (m_DimSizes[1] == 0) { return false; }

    size_t newNbufferToDisplay = size_t(ceil((m_TotalDuration * double(m_Sampling)) / double(m_DimSizes[1])));

    //displays at least one buffer
    newNbufferToDisplay = (newNbufferToDisplay == 0) ? 1 : newNbufferToDisplay;
    if (newNbufferToDisplay != m_NBufferToDisplay || time <= 0) {
        m_NBufferToDisplay      = newNbufferToDisplay;
        nBufferToDisplayChanged = true;

        //if new number of buffers decreased, resize lists and destroy useless buffers
        while (m_NBufferToDisplay < m_SampleBuffers.size()) {
            delete[] m_SampleBuffers.front();
            m_SampleBuffers.pop_front();
            m_StartTime.pop_front();
            m_EndTime.pop_front();

            //suppress the corresponding minmax values
            for (size_t c = 0; c < m_DimSizes[0]; ++c) { m_LocalMinMaxValue[c].pop_front(); }
        }
    }

    return nBufferToDisplayChanged;
}

size_t CBufferDatabase::GetChannelCount() const { return m_DimSizes[0]; }

double CBufferDatabase::GetDisplayedTimeIntervalWidth() const { return (double(m_NBufferToDisplay) * ((double(m_DimSizes[1]) * 1000.0) / double(m_Sampling))); }

void CBufferDatabase::SetMatrixDimensionCount(const size_t n)
{
    if (n != 2) {
        m_Error = true;
        m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error << "Caller tried to set a " << n
                << "-dimensional matrix. Only 2-dimensional matrices are supported (e.g. [rows X cols]).\n";
    }
    if (n == 1) {
        m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error
                << "Note: For 1-dimensional matrices, you may try Matrix Transpose box to upgrade the stream to [N X 1] first.\n";
    }
}

void CBufferDatabase::SetMatrixDimensionSize(const size_t index, const size_t size)
{
    if (index >= 2) {
        m_Error = true;
        m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error << "Tried to access dimension "
                << index << ", only 0 and 1 supported\n";
        return;
    }

    if (m_DimSizes[index] != 0 && m_DimSizes[index] != size) {
        m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error
                << "Upstream tried to change the data chunk size after the first header, this is not supported.\n";
        m_Error = true;
        return;
    }

    m_DimSizes[index] = size;
    m_DimLabels[index].resize(size);

    if (index == 0) {
        m_NElectrodes = m_DimSizes[index];

        //resize min/max values vector
        m_LocalMinMaxValue.resize(m_NElectrodes);
    }
}

void CBufferDatabase::SetMatrixDimensionLabel(const size_t idx1, const size_t idx2, const char* label)
{
    if (m_Error) { return; }

    if (idx1 >= 2) {
        m_Error = true;
        m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error << "Tried to access dimension " << idx1
                << ", only 0 and 1 supported\n";
        return;
    }

    m_DimLabels[idx1][idx2] = label;
}

bool CBufferDatabase::SetMatrixBuffer(const double* buffer, const uint64_t startTime, const uint64_t endTime)
{
    //if an error has occurred, do nothing
    if (m_Error) { return false; }

    // Check for time-continuity
    if (startTime < m_LastBufferEndTime && !m_WarningPrinted) {
        m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning
                << "Your signal does not appear to be continuous in time. "
                << "Previously inserted buffer ended at " << CTime(m_LastBufferEndTime).toSeconds()
                << "s, the current starts at " << CTime(startTime).toSeconds()
                << "s. The display may be incorrect.\n";
        m_WarningPrinted = true;
    }
    m_LastBufferEndTime = endTime;


    //if this the first buffer, perform some precomputations
    if (!m_HasFirstBuffer) {
        m_BufferDuration = endTime - startTime;

        //test if it is equal to zero : Error
        if (m_BufferDuration == 0) {
            m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning <<
                    "Error : buffer start time and end time are equal : " << startTime << "\n";
            m_Error = true;
            return false;
        }

        //computes the sampling frequency for sanity checking or if the setter has not been called
        const uint64_t duration = (uint64_t(1) << 32) * m_DimSizes[1];
        size_t sampling         = size_t(duration / m_BufferDuration);
        if (sampling == 0) {
            // Complain if estimate is bad
            m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning
                    << "The integer sampling frequency was estimated from the chunk size to be 0"
                    << " (nSamples " << m_DimSizes[1] << " / bufferLength " << CTime(m_BufferDuration).toSeconds()
                    << "s = 0). This is not supported. Forcing the rate to 1. This may lead to problems.\n";
            sampling = 1;
        }
        if (m_Sampling == 0) {
            // use chunking duration estimate if setter hasn't been used
            m_Sampling = sampling;
        }
        if (m_Sampling != sampling) {
            m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning
                    << "Sampling rate [" << sampling << "] suggested by chunk properties differs from stream-specified rate ["
                    << m_Sampling << "]. There may be a problem with an upstream box. Trying to use the estimated rate.\n";
            m_Sampling = sampling;
        }

        //computes the number of buffer necessary to display the interval
        AdjustNumberOfDisplayedBuffers(-1);

        m_Drawable->Init();

        m_HasFirstBuffer = true;
    }

    if (!m_ChannelLookupTableInitialized) {
        fillChannelLookupTable();  //to retrieve the unrecognized electrode warning
        // The above call will fail if no electrode localisation data...
        // m_ParentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error << "Unable to fill lookup table\n";
        // return false;
    }
    else {
        //look for chanloc buffers recently received
        for (size_t i = 0; i < m_channelLocalisationCoords.size(); ++i) {
            //if a new set of coordinates was received
            if (m_channelLocalisationCoords[i].second) { OnChannelLocalisationBufferReceived(i); }
        }
    }

    double* bufferToWrite          = nullptr;
    const size_t nSamplesPerBuffer = m_DimSizes[0] * m_DimSizes[1];

    //if old buffers need to be removed
    if (m_SampleBuffers.size() == m_NBufferToDisplay) {
        if (m_OVTotalDuration == 0) { m_OVTotalDuration = (m_StartTime.back() - m_StartTime.front()) + (m_EndTime.back() - m_StartTime.back()); }
        if (m_BufferStep == 0) {
            if (m_StartTime.size() <= 1) { m_BufferStep = size_t(m_OVTotalDuration); }
            else { m_BufferStep = m_StartTime[1] - m_StartTime[0]; }
        }
        if (m_TotalStep == 0) { m_TotalStep = (m_StartTime.back() - m_StartTime.front()) + m_BufferStep; }

        //save first buffer pointer
        bufferToWrite = m_SampleBuffers.front();

        //pop first element from queues
        m_SampleBuffers.pop_front();
        m_StartTime.pop_front();
        m_EndTime.pop_front();
        for (size_t c = 0; c < size_t(m_DimSizes[0]); ++c) { m_LocalMinMaxValue[c].pop_front(); }
    }

    //do we need to allocate a new buffer?
    if (bufferToWrite == nullptr) { bufferToWrite = new double[nSamplesPerBuffer]; }

    //copy new buffer into internal buffer
    if (nSamplesPerBuffer != 0) { memcpy(bufferToWrite, buffer, nSamplesPerBuffer * sizeof(double)); }

    //push new buffer and its timestamps
    m_SampleBuffers.push_back(bufferToWrite);
    m_StartTime.push_back(startTime);
    m_EndTime.push_back(endTime);

    //compute and push min and max values of new buffer
    size_t currentSample = 0;
    //for each channel
    for (size_t c = 0; c < size_t(m_DimSizes[0]); ++c) {
        double localMin = DBL_MAX;
        double localMax = -DBL_MAX;

        //for each sample
        for (size_t i = 0; i < m_DimSizes[1]; i++, currentSample++) {
            //get channel local min/max
            if (buffer[currentSample] < localMin) { localMin = buffer[currentSample]; }
            if (buffer[currentSample] > localMax) { localMax = buffer[currentSample]; }
        }

        //adds the minmax pair to the corresponding channel's list
        m_LocalMinMaxValue[c].emplace_back(localMin, localMax);

        if (localMax > m_MaxValue) { m_MaxValue = localMax; }
        if (localMin < m_MinValue) { m_MinValue = localMin; }
    }

    //tells the drawable to redraw himself since the signal information has been updated
    if (m_RedrawOnNewData) { m_Drawable->Redraw(); }
    return true;
}

bool CBufferDatabase::SetSampling(const size_t sampling)
{
    m_Sampling = sampling;
    return true;
}

void CBufferDatabase::GetDisplayedChannelLocalMinMaxValue(const size_t channel, double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    for (size_t i = 0; i < m_LocalMinMaxValue[channel].size(); ++i) {
        if (min > m_LocalMinMaxValue[channel][i].first) { min = m_LocalMinMaxValue[channel][i].first; }
        if (max < m_LocalMinMaxValue[channel][i].second) { max = m_LocalMinMaxValue[channel][i].second; }
    }
}

bool CBufferDatabase::IsTimeInDisplayedInterval(const uint64_t& time) const
{
    if (m_StartTime.empty()) { return false; }

    return time >= m_StartTime.front() && time <= m_EndTime.back();
}

bool CBufferDatabase::GetIndexOfBufferStartingAtTime(const uint64_t& time, size_t& index) const
{
    index = 0;

    if (m_SampleBuffers.empty() || time < m_StartTime.front() || time > m_StartTime.back()) { return false; }

    for (size_t i = 0; i < m_StartTime.size(); ++i) {
        if (m_StartTime[i] == time) {
            index = i;
            return true;
        }
    }

    return false;
}

void CBufferDatabase::GetDisplayedGlobalMinMaxValue(double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    for (auto& pairs : m_LocalMinMaxValue) {
        for (const auto& pair : pairs) {
            if (min > pair.first) { min = pair.first; }
            if (max < pair.second) { max = pair.second; }
        }
    }
}

void CBufferDatabase::GetLastBufferChannelLocalMinMaxValue(const size_t channel, double& min, double& max)
{
    min = m_LocalMinMaxValue[channel].back().first;
    max = m_LocalMinMaxValue[channel].back().second;
}

void CBufferDatabase::GetLastBufferMinMaxValue(double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    for (auto& localValue : m_LocalMinMaxValue) {
        min = (localValue.back().first < min) ? localValue.back().first : min;
        max = (localValue.back().second > max) ? localValue.back().second : max;
    }
}

bool CBufferDatabase::GetElectrodePosition(const size_t index, double* position)
{
    //TODO : add time parameter and look for coordinates closest to that time!
    if (index < m_channelLocalisationLabels.size()) {
        //if(m_cartesianCoords == true)
        //{
        *position       = *(m_channelLocalisationCoords[0].first->getBuffer() + 3 * index);
        *(position + 1) = *(m_channelLocalisationCoords[0].first->getBuffer() + 3 * index + 1);
        *(position + 2) = *(m_channelLocalisationCoords[0].first->getBuffer() + 3 * index + 2);
        //}
        return true;
    }
    return false;
}

bool CBufferDatabase::GetElectrodePosition(const CString& label, double* position)
{
    //TODO : add time parameter and look for coordinates closest to that time!
    for (size_t i = 0; i < m_channelLocalisationLabels.size(); ++i) {
        if (strcmp(label.toASCIIString(), m_channelLocalisationLabels[i].toASCIIString()) == 0) {
            //if(m_cartesianCoords == true)
            //{
            *position       = *(m_channelLocalisationCoords[0].first->getBuffer() + 3 * i);
            *(position + 1) = *(m_channelLocalisationCoords[0].first->getBuffer() + 3 * i + 1);
            *(position + 2) = *(m_channelLocalisationCoords[0].first->getBuffer() + 3 * i + 2);
            //}
            return true;
        }
    }

    return false;
}

bool CBufferDatabase::GetElectrodeLabel(const size_t index, CString& label)
{
    if (index >= m_channelLocalisationLabels.size()) { return false; }
    label = m_channelLocalisationLabels[index].toASCIIString();
    return true;
}

bool CBufferDatabase::GetChannelPosition(const size_t index, double*& position)
{
    //TODO : add time parameter and look for coordinates closest to that time!
    if (index < m_ChannelLookupIndices.size()) {
        if (m_cartesianCoords) { position = m_channelLocalisationCoords[0].first->getBuffer() + 3 * m_ChannelLookupIndices[index]; }
        // else { } //TODO
        return true;
    }

    return false;
}

bool CBufferDatabase::GetChannelSphericalCoordinates(const size_t index, double& theta, double& phi)
{
    //TODO : add time parameter and look for coordinates closest to that time!
    if (index < m_ChannelLookupIndices.size()) {
        if (m_cartesianCoords) {
            //get cartesian coords
            const double* coords = m_channelLocalisationCoords[0].first->getBuffer() + 3 * m_ChannelLookupIndices[index];

            //convert to spherical coords
            return convertCartesianToSpherical(coords, theta, phi);
        }
        //streamed coordinates are spherical already
        //TODO
        return false;
    }
    return false;
}

bool CBufferDatabase::GetChannelLabel(const size_t index, CString& label)
{
    if (index < m_ChannelLookupIndices.size()) {
        label = m_channelLocalisationLabels[m_ChannelLookupIndices[index]];
        return true;
    }
    label = "";
    return false;
}


void CBufferDatabase::SetStimulation(const size_t /*index*/, const uint64_t id, const uint64_t date)
{
    m_Stimulations.emplace_back(date, id);

    if (!m_StartTime.empty()) {
        while (m_Stimulations.begin() != m_Stimulations.end() && m_Stimulations.begin()->first < m_StartTime.front()) { m_Stimulations.pop_front(); }
    }
}

bool CBufferDatabase::fillChannelLookupTable()
{
    if (!m_HasFirstBuffer || !m_channelLocalisationHeaderReceived) { return false; }

    bool res = true;

    //resize lookup array and initialize lookup indices to 0
    m_ChannelLookupIndices.resize(m_NElectrodes, 0);

    //for all channels
    for (size_t i = 0; i < m_DimSizes[0]; ++i) {
        //trim leading spaces
        size_t firstNonWhitespaceChar = 0;
        for (; firstNonWhitespaceChar < m_DimLabels[0][i].size(); ++firstNonWhitespaceChar) {
            if (isspace(m_DimLabels[0][i][firstNonWhitespaceChar]) == 0) { break; }
        }

        //trim trailing spaces
        size_t lastNonWhitespaceChar = 0;
        if (!m_DimLabels[0][i].empty()) {
            for (lastNonWhitespaceChar = m_DimLabels[0][i].size() - 1; lastNonWhitespaceChar >= 0; lastNonWhitespaceChar--) {
                if (isspace(m_DimLabels[0][i][lastNonWhitespaceChar]) == 0) { break; }
            }
        }

        //look for label in channel localisation labels database
        bool labelRecognized = false;

        if (firstNonWhitespaceChar < lastNonWhitespaceChar) {
            std::string channelLabel(m_DimLabels[0][i].substr(firstNonWhitespaceChar, lastNonWhitespaceChar - firstNonWhitespaceChar + 1));

            for (size_t j = 0; j < m_channelLocalisationLabels.size(); ++j) {
                if (strcmp(channelLabel.c_str(), m_channelLocalisationLabels[j].toASCIIString()) == 0) {
                    labelRecognized           = true;
                    m_ChannelLookupIndices[i] = j;
                    break;
                }
            }
        }

        //unrecognized electrode!
        if (!labelRecognized) {
            m_ParentPlugin.getLogManager() << Kernel::LogLevel_Warning << "Unrecognized electrode name (index=" << i
                    << ", name=" << m_DimLabels[0][i] << ")!\n";
            res = false;
        }
    }

    m_ParentPlugin.getLogManager() << Kernel::LogLevel_Trace << "Electrodes list : ";

    for (size_t i = 0; i < size_t(m_DimSizes[0]); ++i) {
        m_ParentPlugin.getLogManager() << m_DimLabels[0][i].c_str();
        if (i < m_DimSizes[0] - 1) { m_ParentPlugin.getLogManager() << ", "; }
        else { m_ParentPlugin.getLogManager() << "\n"; }
    }

    if (res) { m_ChannelLookupTableInitialized = true; }

    return res;
}

bool CBufferDatabase::convertCartesianToSpherical(const double* cartesian, double& theta, double& phi) const
{
    const double threshold = 1e-3;
    const double radToDeg  = 57.2957795131; // 180 / pi

    //compute theta
    theta = acos(cartesian[2]) * radToDeg;

    //compute phi so that it lies in [0, 360]
    if (fabs(cartesian[0]) < threshold) { phi = (cartesian[1] > 0) ? 90 : 270; }
    else {
        phi = atan(cartesian[1] / cartesian[0]) * radToDeg;
        if (cartesian[0] < 0) { phi += 180; }
        else if (cartesian[1] < 0) { phi += 360; }
    }

    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
