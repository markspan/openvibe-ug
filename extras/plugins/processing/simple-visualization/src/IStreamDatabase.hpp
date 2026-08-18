///-------------------------------------------------------------------------------------------------
/// 
/// \file IStreamDatabase.hpp
/// \brief Definition for the class IStreamDatabase.
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

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class IStreamDisplayDrawable;

/// <summary> Abstract class of objects than can be updated by an IStreamDatabase object. </summary>
class IStreamDisplayDrawable
{
public:
    virtual bool Init() = 0;
    virtual bool Redraw() = 0;
    virtual ~IStreamDisplayDrawable() = default;
};

class IStreamDatabase
{
public:
    virtual ~IStreamDatabase() = default;

    /// <summary> Initialize the database, including creating decoder. </summary>
    /// <returns> True if initialization succeeded. </returns>
    virtual bool Initialize() = 0;

    /// <summary> Set drawable object to update. </summary>
    /// <param name="drawable"> drawable object to update. </param>
    virtual void SetDrawable(IStreamDisplayDrawable* drawable) = 0;

    /// <summary> Set flag stating whether or not to redraw upon new data reception. </summary>
    /// <param name="redrawOnNewData"> Redraw flag. </param>
    virtual void SetRedrawOnNewData(const bool redrawOnNewData) = 0;

    /// <summary> Determine whether first buffer has been received yet. </summary>
    /// <returns> True if first buffer has been received already, false otherwise. </returns>
    virtual bool HasFirstBuffer() = 0;

    /// <summary> Set max buffer count. </summary>
    ///
    /// Set max buffer count directly (as opposed to computing it from time scale)
    /// <remarks> This method sets m_ignoreTimeScale to true. </remarks>
    virtual bool SetMaxBufferCount(const size_t count) = 0;

    /// <summary> Set time scale. </summary>
    ///
    /// Computes the maximum number of buffers that can be displayed simultaneously
    /// <param name="timeScale"> Time window's width in seconds. </param>
    /// <returns> True if buffer count changed, false otherwise. </returns>
    /// <remarks> This method sets m_ignoreTimeScale to false. </remarks>
    virtual bool SetTimeScale(const double timeScale) = 0;

    /// <summary> Decode a memory buffer using proxy. </summary>
    /// <param name="buffer"> Memory buffer to decode. </param>
    /// <param name="startTime"> Start time of memory buffer. </param>
    /// <param name="endTime"> End time of memory buffer. </param>
    /// <returns> True if memory buffer could be properly decoded, false otherwise. </returns>
    virtual bool DecodeMemoryBuffer(const CMemoryBuffer* buffer, const uint64_t startTime, const uint64_t endTime) = 0;

    /// <summary> Get number of buffers necessary to cover time scale. </summary>
    /// <returns> Maximum number of buffers stored in this object. </returns>
    /// <remarks> Can't be computed before 2 buffers have been received, 
    /// because the time step between the start of 2 consecutive buffers must be known. </remarks>
    virtual size_t GetMaxBufferCount() = 0;

    /// <summary> Get current buffer count. </summary>
    /// <returns> Current buffer count. </returns>
    virtual size_t GetCurrentBufferCount() = 0;

    /// <summary> Get pointer on a given buffer. </summary>
    /// <param name="index"> Index of buffer to retrieve. </param>
    /// <returns> Buffer pointer if buffer exists, nullptr otherwise. </returns>
    virtual const double* GetBuffer(const size_t index) = 0;

    /// <summary> Get start time of a given buffer. </summary>
    /// <param name="index"> Index of buffer whose start time is to be retrieved. </param>
    /// <returns> Start time if buffer exists, 0 otherwise. </returns>
    virtual uint64_t GetStartTime(const size_t index) = 0;

    /// <summary> Get end time of a given buffer. </summary>
    /// <param name="index"> Index of buffer whose end time is to be retrieved. </param>
    /// <returns> End time if buffer exists, 0 otherwise. </returns>
    virtual uint64_t GetEndTime(const size_t index) = 0;

    /// <summary> Get number of elements contained in a buffer. </summary>
    /// <returns> Buffer element count or 0 if no buffer has been received yet. </returns>
    virtual size_t GetBufferElementCount() = 0;

    /// <summary> Get time span covered by a buffer. </summary>
    /// <returns> Buffer time span. </returns>
    virtual uint64_t GetBufferDuration() = 0;

    /// <summary> Determine whether buffer time step has been computed yet. </summary>
    /// <returns> True if buffer time step has been computed. </returns>
    virtual bool IsBufferTimeStepComputed() = 0;

    /// <summary> Get time step between the start of 2 consecutive buffers. </summary>
    /// <returns> Buffer time step. </returns>
    /// <remarks> This value can't be computed before the first 2 buffers are received. </remarks>
    virtual uint64_t GetBufferTimeStep() = 0;

    /// <summary> Get number of samples per buffer. </summary>
    /// <returns> Number of samples per buffer. </returns>
    virtual size_t GetSampleCountPerBuffer() = 0;

    /// <summary> Get number of channels. </summary>
    /// <returns> Number of channels. </returns>
    virtual size_t GetChannelCount() = 0;

    /// <summary> Get channel label. </summary>
    /// <param name="index"> index of channel. </param>
    /// <param name="label"> channel label. </param>
    /// <returns> true if channel label could be retrieved, false otherwise. </returns>
    virtual bool GetChannelLabel(const size_t index, CString& label) = 0;

    /** \name Min/max values retrieval */
    //@{

    /// <summary> Compute min/max values currently displayed for a given channel. </summary>
    /// <param name="index"> Index of channel. </param>
    /// <param name="min"> Minimum displayed value for channel of interest. </param>
    /// <param name="max"> Maximum displayed value for channel of interest. </param>
    /// <returns> True if values could be computed, false otherwise. </returns>
    virtual bool GetChannelMinMaxValues(const size_t index, double& min, double& max) = 0;

    /// <summary> Compute min/max values currently displayed, taking all channels into account. </summary>
    /// <param name="min"> Minimum displayed value. </param>
    /// <param name="max"> Maximum displayed value. </param>
    /// <returns> True if values could be computed, false otherwise. </returns>
    virtual bool GetGlobalMinMaxValues(double& min, double& max) = 0;

    /// <summary> Compute min/max values in last buffer for a given channel. </summary>
    /// <param name="index"> Index of channel. </param>
    /// <param name="min"> Minimum value for channel of interest. </param>
    /// <param name="max"> Maximum value for channel of interest. </param>
    /// <returns> True if values could be computed, false otherwise. </returns>
    virtual bool GetLastBufferChannelMinMaxValues(const size_t index, double& min, double& max) = 0;

    /// <summary> Compute min/max values in last buffer, taking all channels into account. </summary>
    /// <param name="min"> Minimum value. </param>
    /// <param name="max"> Maximum value. </param>
    /// <returns> True if values could be computed, false otherwise. </returns>
    virtual bool GetLastBufferGlobalMinMaxValues(double& min, double& max) = 0;

    //@}
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
