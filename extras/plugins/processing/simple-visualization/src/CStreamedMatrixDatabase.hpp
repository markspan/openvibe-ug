///-------------------------------------------------------------------------------------------------
/// 
/// \file CStreamedMatrixDatabase.hpp
/// \brief Definition for the class CStreamedMatrixDatabase.
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

#include "IStreamDatabase.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <deque>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
/// <summary> This class is used to store information about the incoming matrix stream.</summary>
/// 
/// It can request a IStreamDisplayDrawable object to redraw itself upon changes in its data. 
/// <seealso cref="IStreamDatabase" />
class CStreamedMatrixDatabase : public IStreamDatabase
{
public:
    /// <summary> Initializes a new instance of the <see cref="CStreamedMatrixDatabase"/> class. </summary>
    /// <param name="parent">The parent.</param>
    explicit CStreamedMatrixDatabase(Toolkit::TBoxAlgorithm<IBoxAlgorithm>& parent) : m_parentPlugin(parent) {}

    /// <summary> Finalizes an instance of the <see cref="CStreamedMatrixDatabase"/> class. </summary>
    ~CStreamedMatrixDatabase() override;

    bool Initialize() override;

    void SetDrawable(IStreamDisplayDrawable* drawable) override { m_drawable = drawable; }
    void SetRedrawOnNewData(const bool redrawOnNewData) override { m_redrawOnNewData = redrawOnNewData; }

    bool HasFirstBuffer() override { return m_hasFirstBuffer; }
    bool SetMaxBufferCount(const size_t count) override;
    bool SetTimeScale(const double timeScale) override;
    bool DecodeMemoryBuffer(const CMemoryBuffer* buffer, const uint64_t startTime, const uint64_t endTime) override;

    size_t GetMaxBufferCount() override { return m_nMaxBuffer; }
    size_t GetCurrentBufferCount() override { return m_matrices.size(); }

    const double* GetBuffer(const size_t index) override { return (index >= m_matrices.size()) ? nullptr : m_matrices[index]->getBuffer(); }

    uint64_t GetStartTime(const size_t index) override { return (index >= m_startTimes.size()) ? 0 : m_startTimes[index]; }
    uint64_t GetEndTime(const size_t index) override { return (index >= m_endTimes.size()) ? 0 : m_endTimes[index]; }

    size_t GetBufferElementCount() override { return (m_matrices.empty()) ? 0 : m_matrices[0]->getBufferElementCount(); }
    uint64_t GetBufferDuration() override { return (m_startTimes.empty() || m_endTimes.empty()) ? 0 : m_endTimes[0] - m_startTimes[0]; }
    bool IsBufferTimeStepComputed() override { return m_bufferTimeStepComputed; }
    uint64_t GetBufferTimeStep() override { return m_bufferTimeStepComputed ? m_bufferTimeStep : 0; }

    size_t GetSampleCountPerBuffer() override { return m_matrixHeader.getDimensionCount() == 0 ? 0 : m_matrixHeader.getDimensionSize(1); }
    size_t GetChannelCount() override { return (m_matrixHeader.getDimensionCount() == 0) ? 0 : m_matrixHeader.getDimensionSize(0); }

    bool GetChannelLabel(const size_t index, CString& label) override;
    bool GetChannelMinMaxValues(const size_t index, double& min, double& max) override;
    bool GetGlobalMinMaxValues(double& min, double& max) override;
    bool GetLastBufferChannelMinMaxValues(const size_t index, double& min, double& max) override;
    bool GetLastBufferGlobalMinMaxValues(double& min, double& max) override;

protected:
    bool onBufferCountChanged();

    virtual bool decodeHeader();
    virtual bool decodeBuffer(const uint64_t startTime, const uint64_t endTime);

    Toolkit::TBoxAlgorithm<IBoxAlgorithm>& m_parentPlugin;  ///< parent plugin
    Kernel::IAlgorithmProxy* m_decoder = nullptr;           ///< decoder algorithm
    IStreamDisplayDrawable* m_drawable = nullptr;           ///< drawable object to update (if needed)
    bool m_redrawOnNewData             = true;              ///< flag stating whether to redraw the IStreamDisplayDrawable upon new data reception if true (default)
    bool m_hasFirstBuffer              = false;             ///< flag stating whether first samples buffer has been received
    bool m_bufferTimeStepComputed      = false;             ///< flag stating whether buffer time step was computed
    uint64_t m_bufferTimeStep          = 0;                 ///< time difference between start times of two consecutive buffers
    size_t m_nMaxBuffer                = 2;                 ///< maximum number of buffers stored in database
    bool m_ignoreTimeScale             = false;             ///< flag stating whether time scale should be ignored (max buffer count externally set)
    double m_timeScale                 = 10;                ///< maximum duration of displayed buffers (in seconds)
    std::deque<uint64_t> m_startTimes;                      ///< double-linked list of start times of stored buffers
    std::deque<uint64_t> m_endTimes;                        ///< double-linked list of end times of stored buffers
    CMatrix m_matrixHeader;                                 ///< streamed matrix header
    std::deque<CMatrix*> m_matrices;                        ///< streamed matrix    history
    std::vector<std::deque<std::pair<double, double>>> m_channelMinMaxValues;   ///< min/max values for each channel
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
