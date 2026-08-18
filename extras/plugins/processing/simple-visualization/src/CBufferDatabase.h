///-------------------------------------------------------------------------------------------------
/// 
/// \file CBufferDatabase.hpp
/// \brief Definition for the class CBufferDatabase.
/// \author J. T. Lindgren (Inria).
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

#include <cfloat>
#include <deque>
#include <string>
#include <vector>
#include <array>

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CSignalDisplayDrawable;

/**
* Abtract class of objects than can be updated by a CBufferDatabase
*/
class CSignalDisplayDrawable
{
public:
    virtual ~CSignalDisplayDrawable() = default;
    virtual void Init() = 0;
    virtual void Redraw() = 0;
};

/**
* This class is used to store information about the incoming signal stream. It can request a CSignalDisplayDrawable
* object to redraw himself in case of some changes in its data.
*/
class CBufferDatabase
{
public:
    //! Number of channels
    size_t m_NElectrodes = 0;

    //! Number of channels and number of samples per buffer
    std::array<size_t, 2> m_DimSizes;

    //! Channel labels, buffer labels
    std::array<std::vector<std::string>, 2> m_DimLabels;

    //! Flag set to true once first buffer is received
    bool m_HasFirstBuffer = false;

    //! Sampling frequency of the incoming stream
    size_t m_Sampling = 0;

    //! double-linked list of pointers to the samples buffers of the current time window
    std::deque<double*> m_SampleBuffers;

    //! stimulations to display. pair values are <date, stimcode>
    std::deque<std::pair<uint64_t, uint64_t>> m_Stimulations;

    //electrode spherical coordinates (in degrees)
    //CMatrix m_oElectrodesSphericalCoords;

    //flag set to true once channel lookup indices are determined
    bool m_ChannelLookupTableInitialized = false;

    //indices of electrodes in channel localisation database
    std::vector<size_t> m_ChannelLookupIdxs;

    //electrode labels (standardized)
    //std::vector<CString> m_oElectrodesLabels;

    //! Number of buffer to display at the same time
    uint64_t m_NBufferToDisplay = 2;

    //! The global maximum value of the signal (up to now)
    double m_MaxValue = -DBL_MAX;

    //! The global minimum value of the signal (up to now)
    double m_MinValue = +DBL_MAX;

    //! Double-linked list of the start times of the current buffers
    std::deque<uint64_t> m_StartTime;

    //! Double-linked list of the end times of the current buffers
    std::deque<uint64_t> m_EndTime;

    //! Duration to display in seconds
    double m_TotalDuration = 0;

    /*! Duration to display in openvibe time units.
    Computed once every time the user changes the total duration to display,
    when the maximum number of buffers to store are received.*/
    uint64_t m_TotalDurationOV = 0;

    /*! Duration of a single buffer.
    Computed once, but not constant when sampling frequency is not a multiple of buffer size!*/
    uint64_t m_BufferDuration = 0;

    /*! Time step separating the start times of m_NBufferToDisplay+1 buffers.
    Recomputed once every time the user changes the total duration to display,
    but not constant when sampling frequency is not a multiple of buffer size!*/
    uint64_t m_TotalStep = 0;

    /*! Time step separating the start times of 2 consecutive buffers.
    Computed once, but not constant when sampling frequency is not a multiple of buffer size!*/
    uint64_t m_BufferStep = 0;

    // When did the last inserted buffer end
    uint64_t m_LastBufferEndTime = 0;
    // Did we print a warning about noncontinuity?
    bool m_WarningPrinted = false;

    //! Pointer to the drawable object to update (if needed)
    CSignalDisplayDrawable* m_Drawable = nullptr;

    std::vector<std::deque<std::pair<double, double>>> m_LocalMinMaxValue;

    Toolkit::TBoxAlgorithm<IBoxAlgorithm>& m_ParentPlugin;

    bool m_Error = false;

    //! Redraws the associated SignalDisplayDrawable upon new data reception if true (default)
    bool m_RedrawOnNewData = true;

protected:
    /* \name Channel localisation */
    //@{
    //channel localisation decoder
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    //flag set to true once channel localisation buffer is received
    bool m_channelLocalisationHeaderReceived = false;
    //dynamic channel localisation flag (e.g. localisation is constantly updated with MEG)
    bool m_dynamicChannelLocalisation = false;
    //channel labels database
    std::vector<CString> m_channelLocalisationLabels;
    //flag stating whether streamed coordinates are cartesian (as opposed to spherical)
    bool m_cartesianCoords = false;
    //! double-linked list of streamed channel coordinates (if cartesian, expressed in normalized space (X right Y front Z up))
    std::deque<std::pair<CMatrix*, bool>> m_channelLocalisationCoords;
    //! double-linked list of channel coordinates (spherical if streamed coords aere cartesian and vice versa)
    //std::deque<  std::pair<CMatrix*, bool> > m_oChannelLocalisationAlternateCoords;
    //pointer to double linked list of cartesian coordinates
    //std::deque< std::pair<CMatrix*, bool> > * m_pChannelLocalisationCartesianCoords = nullptr;
    //pointer to double linked list of spherical coordinates
    //std::deque< std::pair<CMatrix*, bool> > * m_pChannelLocalisationSphericalCoords = nullptr;
    //! double-linked list of start/end times of channel coordinates
    std::deque<std::pair<uint64_t, uint64_t>> m_channelLocalisationTimes;
    //@}

    //! Redraw mode (shift or scan)
    CIdentifier m_displayMode = Scan;

public:
    explicit CBufferDatabase(Toolkit::TBoxAlgorithm<IBoxAlgorithm>& parent);

    virtual ~CBufferDatabase();

    /// <summary> Decode a channel localisation memory buffer. </summary>
    /// <param name="buffer"> Memory buffer to decode. </param>
    /// <param name="startTime"> Start time of memory buffer. </param>
    /// <param name="endTime"> End time of memory buffer. </param>
    /// <returns> True if memory buffer could be properly decoded, false otherwise. </returns>
    virtual bool DecodeChannelLocalisationMemoryBuffer(const CMemoryBuffer* buffer, uint64_t startTime, uint64_t endTime);

    /// <summary> Callback called upon channel localisation buffer reception. </summary>
    /// <param name="index"> Index of newly received channel localisation buffer. </param>
    /// <returns> True if buffer data was correctly processed, false otherwise. </returns>
    virtual bool OnChannelLocalisationBufferReceived(const size_t index);

    /// <summary> Sets the drawable object to update. </summary>
    /// <param name="drawable"> drawable object to update. </param>
    virtual void SetDrawable(CSignalDisplayDrawable* drawable) { m_Drawable = drawable; }

    /// <summary> Get error status. </summary>
    /// <returns> Error status. If true, an error occurred. </returns>
    virtual bool GetErrorStatus() { return m_Error; }

    /// <summary> Determines whether first buffer has been received yet. </summary>
    /// <returns> True if first buffer has been received already, false otherwise. </returns>
    virtual bool HasFirstBuffer() { return m_HasFirstBuffer; }

    /// <summary> Determines whether first channel localisation buffer has been processed yet. </summary>
    ///
    /// When this condition is true, channel coordinates may be retrieved using the corresponding methods in this class.
    /// <returns> True if first chanloc buffer was processed. </returns>
    virtual bool IsFirstChannelLocalisationBufferProcessed();

    /// <summary> Compute the number of buffers needed to display the signal for a certain time period. </summary>
    /// <param name="time"> the time window's width in seconds. </param>
    virtual bool AdjustNumberOfDisplayedBuffers(double time);

    /// <summary> Get time interval covered by data held in this object. </summary>
    /// <returns> Time interval in ms. </returns>
    virtual double GetDisplayedTimeIntervalWidth() const { return (double(m_NBufferToDisplay) * ((double(m_DimSizes[1]) * 1000.0) / double(m_Sampling))); }

    /// <summary> Determine whether time passed in parameter lies in displayed data interval. </summary>
    /// <param name="time"> Time to test. </param>
    /// <returns> True if time lies in displayed time interval, false otherwise. </returns>
    virtual bool IsTimeInDisplayedInterval(const uint64_t& time) const;

    /// <summary> Get index of sample buffer which starts at a given time. </summary>
    /// <param name="time"> Start time of buffer. </param>
    /// <param name="index"> Buffer index. </param>
    /// <returns> True if buffer index could be determined, false otherwise. </returns>
    virtual bool GetIndexOfBufferStartingAtTime(const uint64_t& time, size_t& index) const;

    //! Returns the min/max values currently displayed for the given channel
    virtual void GetDisplayedChannelLocalMinMaxValue(const size_t channel, double& min, double& max);
    //! Returns the min/max values currently displayed (all channels taken into account)
    virtual void GetDisplayedGlobalMinMaxValue(double& min, double& max);

    virtual void GetDisplayedChannelLocalMeanValue(const size_t /*channel*/, double& /*mean*/) {}

    //! Returns the min/max values of the last buffer arrived for the given channel
    virtual void GetLastBufferChannelLocalMinMaxValue(const size_t channel, double& min, double& max)
    {
        min = m_LocalMinMaxValue[channel].back().first;
        max = m_LocalMinMaxValue[channel].back().second;
    }

    //! Returns the min/max values of the last buffer arrived  (all channels taken into account)
    virtual void GetLastBufferMinMaxValue(double& min, double& max)
    {
        min = +DBL_MAX;
        max = -DBL_MAX;

        for (auto& pair : m_LocalMinMaxValue) {
            min = (pair.back().first < min) ? pair.back().first : min;
            max = (pair.back().second > max) ? pair.back().second : max;
        }
    }

    /// <summary> Get number of eletrodes in database. </summary>
    /// <returns> Number of electrodes. </returns>
    virtual size_t GetElectrodeCount() { return m_channelLocalisationLabels.size(); }

    /// <summary> Get electrode normalized position. </summary>
    /// <param name="index"> Index of electrode in database whose position is to be retrieved. </param>
    /// <param name="position"> Pointer to an array of 3 floats where to store coordinates. </param>
    /// <returns> True if electrode position could be retrieved. </returns>
    /// <remarks> Position expressed in normalized cartesian frame where X is right, Y front, Z up. </remarks>
    virtual bool GetElectrodePosition(const size_t index, double* position);

    /// <summary> Get electrode normalized position. </summary>
    /// <param name="label"> Label of electrode whose position is to be retrieved. </param>
    /// <param name="position"> Pointer to an array of 3 floats where to store coordinates. </param>
    /// <returns> True if electrode position could be retrieved. </returns>
    /// <remarks> Position expressed in normalized cartesian frame where X is right, Y front, Z up. </remarks>
    virtual bool GetElectrodePosition(const CString& label, double* position);

    /// <summary> Get electrode label. </summary>
    /// <param name="index"> Index of electrode in database whose label is to be retrieved. </param>
    /// <param name="label"> Electrode label. </param>
    /// <returns> True if electrode label could be retrieved. </returns>
    virtual bool GetElectrodeLabel(const size_t index, CString& label);

    /// <summary> Get number of channels. </summary>
    /// <returns> Number of channels. </returns>
    virtual size_t GetChannelCount() const { return m_DimSizes[0]; }

    /// <summary> Get channel normalized position. </summary>
    /// <param name="index"> Index of channel whose position is to be retrieved. </param>
    /// <param name="position"> Reference on a double pointer. </param>
    /// <returns> True if channel position could be retrieved (position then points to an array of 3 floats). </returns>
    /// <remarks> Position expressed in normalized cartesian frame where X is right, Y front, Z up. </remarks>
    virtual bool GetChannelPosition(const size_t index, double*& position);

    /// <summary> Get channel spherical coordinates in degrees. </summary>
    /// <param name="index"> Index of channel whose coordinates are to be retrieved. </param>
    /// <param name="theta"> Reference on a float to be set with theta angle. </param>
    /// <param name="phi"> Reference on a float to be set with phi angle. </param>
    /// <returns> True if channel coordinates could be retrieved. </returns>
    virtual bool GetChannelSphericalCoordinates(const size_t index, double& theta, double& phi);

    /// <summary> Get channel label. </summary>
    /// <param name="index"> Index of channel whose label is to be retrieved. </param>
    /// <param name="label"> Channel label. </param>
    /// <returns> True if channel label could be retrieved. </returns>
    virtual bool GetChannelLabel(const size_t index, CString& label);

    virtual void SetMatrixDimensionCount(const size_t count);
    virtual void SetMatrixDimensionSize(const size_t index, const size_t size);
    virtual void SetMatrixDimensionLabel(const size_t idx1, const size_t idx2, const char* label);

    // Returns false on failure
    virtual bool SetMatrixBuffer(const double* buffer, uint64_t startTime, uint64_t endTime);

    // Sets the sampling frequency. If this is not called, the frequency is estimated from the stream chunk properties.
    // Mainly used to force a warning if stream-specified rate differs from the chunk-estimated rate.
    virtual bool SetSampling(const size_t sampling);

    virtual void Resize(const size_t /*count*/) {}
    virtual void SetStimulation(const size_t index, uint64_t identifier, uint64_t date);

    /// <summary> Set display mode. </summary>
    /// <remarks> Used by signal display and time ruler to determine how they should be updated. </remarks>
    /// <param name="mode"> New display mode. </param>
    virtual void SetDisplayMode(const CIdentifier& mode) { m_displayMode = mode; }

    /// <summary> Get current display mode. </summary>
    /// <returns> Current display mode. </returns>
    virtual CIdentifier GetDisplayMode() { return m_displayMode; }

    /// <summary> Set flag stating whether to redraw associated SignalDisplayDrawable objet when new data is available. </summary>
    /// <param name="set"> Value to set flag with. </param>
    virtual void SetRedrawOnNewData(const bool set) { m_RedrawOnNewData = set; }

protected:
    /// <summary> Initialize table storing indices of electrodes in channel localisation database. </summary>
    /// <returns> True if table could be initialized. </returns>
    virtual bool fillChannelLookupTable();

    /// <summary> Convert a cartesian coordinates triplet to spherical coordinates. </summary>
    /// <param name="cartesian"> Pointer to cartesian coordinates triplet. </param>
    /// <param name="theta"> Equivalent theta angle. </param>
    /// <param name="phi"> Equivalent phi angle. </param>
    /// <returns> True if coordinates were successfully converted. </returns>
    bool convertCartesianToSpherical(const double* cartesian, double& theta, double& phi) const;
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
