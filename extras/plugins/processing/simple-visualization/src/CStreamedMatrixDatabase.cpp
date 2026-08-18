///-------------------------------------------------------------------------------------------------
/// 
/// \file CStreamedMatrixDatabase.cpp
/// \brief Implementation for the class CStreamedMatrixDatabase.
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

#include "CStreamedMatrixDatabase.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>               // For unix system

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

CStreamedMatrixDatabase::~CStreamedMatrixDatabase()
{
    if (m_decoder != nullptr) {
        m_decoder->uninitialize();
        m_parentPlugin.getAlgorithmManager().releaseAlgorithm(*m_decoder);
    }

    while (!m_matrices.empty()) {
        delete m_matrices.front();
        m_matrices.pop_front();
    }
}

bool CStreamedMatrixDatabase::Initialize()
{
    if (m_decoder != nullptr) { return false; }
    m_decoder = &m_parentPlugin.getAlgorithmManager().getAlgorithm(
        m_parentPlugin.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StreamedMatrixDecoder));

    m_decoder->initialize();
    return true;
}

bool CStreamedMatrixDatabase::SetMaxBufferCount(const size_t count)
{
    //max buffer count computed directly
    m_ignoreTimeScale = true;
    m_nMaxBuffer      = count;
    onBufferCountChanged();
    return true;
}

bool CStreamedMatrixDatabase::SetTimeScale(const double timeScale)
{
    if (timeScale <= 0) { return false; }

    //max buffer count computed from time scale
    m_ignoreTimeScale = false;

    //update time scale
    m_timeScale = timeScale;

    //if step between buffers is not known yet, this method will have to be called again later
    if (!m_bufferTimeStepComputed) { return false; }

    //compute maximum number of buffers needed to cover time scale
    size_t maxBufferCount = 0;

    if (m_bufferTimeStep > 0) { maxBufferCount = size_t(ceil(double(CTime(m_timeScale).time()) / double(m_bufferTimeStep))); }

    //display at least one buffer
    if (maxBufferCount == 0) { maxBufferCount = 1; }

    //acknowledge maximum buffer count
    bool maxBufferCountChanged = false;
    if (maxBufferCount != m_nMaxBuffer) {
        m_nMaxBuffer          = maxBufferCount;
        maxBufferCountChanged = true;
        onBufferCountChanged();
    }

    return maxBufferCountChanged;
}

bool CStreamedMatrixDatabase::onBufferCountChanged()
{
    //if new number of buffers is smaller than before, destroy extra buffers
    while (m_matrices.size() > m_nMaxBuffer) {
        delete m_matrices.front();
        m_matrices.pop_front();
        m_startTimes.pop_front();
        m_endTimes.pop_front();
        for (auto& i : m_channelMinMaxValues) { i.pop_front(); }
    }

    return true;
}

bool CStreamedMatrixDatabase::DecodeMemoryBuffer(const CMemoryBuffer* buffer, const uint64_t startTime, const uint64_t endTime)
{
    //feed memory buffer to algorithm
    m_decoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_InputParameterId_MemoryBufferToDecode)->setReferenceTarget(&buffer);

    //process buffer
    m_decoder->process();

    //has flow header been received?
    if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedHeader)) {
        decodeHeader();
        //create widgets
        m_drawable->Init();
    }

    //has a buffer been received?
    if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputTriggerId_ReceivedBuffer)) {
        decodeBuffer(startTime, endTime);
        //redraw widgets
        if (m_redrawOnNewData) { m_drawable->Redraw(); }
    }

    return true;
}

bool CStreamedMatrixDatabase::GetChannelLabel(const size_t index, CString& label)
{
    if (m_matrixHeader.getDimensionCount() == 0 || m_matrixHeader.getDimensionSize(0) <= index) {
        label = "";
        return false;
    }
    label = m_matrixHeader.getDimensionLabel(0, index);
    return true;
}

bool CStreamedMatrixDatabase::GetChannelMinMaxValues(const size_t index, double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    if (!m_hasFirstBuffer || index >= GetChannelCount()) { return false; }

    for (const auto& values : m_channelMinMaxValues[index]) {
        if (min > values.first) { min = values.first; }
        if (max < values.second) { max = values.second; }
    }

    return true;
}

bool CStreamedMatrixDatabase::GetGlobalMinMaxValues(double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    if (!m_hasFirstBuffer) { return false; }

    for (size_t c = 0; c < GetChannelCount(); ++c) {
        for (const auto& values : m_channelMinMaxValues[c]) {
            if (min > values.first) { min = values.first; }
            if (max < values.second) { max = values.second; }
        }
    }

    return true;
}

bool CStreamedMatrixDatabase::GetLastBufferChannelMinMaxValues(const size_t index, double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    if (!m_hasFirstBuffer) { return false; }

    if (index >= GetChannelCount()) { return false; }

    min = m_channelMinMaxValues[index].back().first;
    max = m_channelMinMaxValues[index].back().second;
    return true;
}

bool CStreamedMatrixDatabase::GetLastBufferGlobalMinMaxValues(double& min, double& max)
{
    min = +DBL_MAX;
    max = -DBL_MAX;

    if (!m_hasFirstBuffer) { return false; }

    for (const auto& values : m_channelMinMaxValues) {
        if (min > values.back().first) { min = values.back().first; }
        if (max < values.back().second) { max = values.back().second; }
    }

    return true;
}

bool CStreamedMatrixDatabase::decodeHeader()
{
    //copy streamed matrix header
    Kernel::TParameterHandler<CMatrix*> streamedMatrix;
    streamedMatrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    m_matrixHeader.copyDescription(*streamedMatrix);

    m_channelMinMaxValues.resize(GetChannelCount());

    return true;
}

bool CStreamedMatrixDatabase::decodeBuffer(const uint64_t startTime, const uint64_t endTime)
{
    //first buffer received
    if (!m_hasFirstBuffer) {
        const uint64_t bufferDuration = endTime - startTime;

        if (bufferDuration == 0) {
            m_parentPlugin.getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning <<
                    "Error : buffer start time and end time are equal : " << startTime << "\n";
            //m_error = true;
            return false;
        }

        m_hasFirstBuffer = true;
    }

    //compute time step between two buffers
    if (!m_bufferTimeStepComputed && m_matrices.size() >= 2) {
        m_bufferTimeStep         = m_startTimes[1] - m_startTimes[0];
        m_bufferTimeStepComputed = true;

        if (!m_ignoreTimeScale) {
            //compute maximum number of buffers from time scale
            SetTimeScale(m_timeScale);
        }
    }

    //store new buffer data
    CMatrix* currentMatrix;
    if (m_matrices.size() < m_nMaxBuffer) {
        currentMatrix = new CMatrix();
        currentMatrix->copyDescription(m_matrixHeader);
        m_matrices.push_back(currentMatrix);
    }
    else //reuse memory for new buffer
    {
        //move front matrix to back of list
        currentMatrix = m_matrices.front();
        m_matrices.push_back(currentMatrix);

        //remove first matrix data
        m_matrices.pop_front();
        m_startTimes.pop_front();
        m_endTimes.pop_front();
        for (size_t c = 0; c < GetChannelCount(); ++c) { m_channelMinMaxValues[c].pop_front(); }
    }

    //store samples
    Kernel::TParameterHandler<CMatrix*> streamedMatrix;
    streamedMatrix.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    currentMatrix->copyContent(*streamedMatrix);

    //store time stamps
    m_startTimes.push_back(startTime);
    m_endTimes.push_back(endTime);

    //store min/max values
    double* buffer = currentMatrix->getBuffer();

    for (size_t c = 0; c < GetChannelCount(); ++c) {
        double min = DBL_MAX;
        double max = -DBL_MAX;

        for (uint64_t i = 0; i < GetSampleCountPerBuffer(); ++i, ++buffer) {
            if (*buffer < min) { min = *buffer; }
            if (*buffer > max) { max = *buffer; }
        }

        m_channelMinMaxValues[c].push_back(std::pair<double, double>(min, max));
    }

    return true;
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
