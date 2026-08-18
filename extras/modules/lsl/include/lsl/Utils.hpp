///-------------------------------------------------------------------------------------------------
///
/// \file Utils.hpp
/// \brief Utils for LSL within OpenViBE
/// \author Thomas Prampart (Inria) & Thibaut Monseigne (Inria).
/// \version 1.0.0
/// \date 22/11/2021.
///
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
///-------------------------------------------------------------------------------------------------

#pragma once

#ifdef TARGET_HAS_ThirdPartyLSL

#include <openvibe/ov_all.h>
#include <lsl_cpp.h>

namespace OpenViBE {
namespace LSL {

//-------------------------------------------------------------------------------------------------
// Misc
//-------------------------------------------------------------------------------------------------
/// <summary> Get LSL relative time for an OV time. </summary>
/// <param name="time"> The time in OV time referential. </param>
/// <returns> The time (in seconds) in lsl clock referential. </returns>
double getLSLRelativeTime(const CTime& time);

//-------------------------------------------------------------------------------------------------
// Sending
//-------------------------------------------------------------------------------------------------

/// <summary> Creates the stream information for a signal. </summary>
/// <param name="name"> The name of the stream. </param>
/// <param name="id"> The identifier of the stream. </param>
/// <param name="matrix"> The matrix containing signal informations (channel number and label). </param>
/// <param name="frequency"> The signal frequency. </param>
/// <returns> the stream info. </returns>
lsl::stream_info createSignalStreamInfo(const std::string& name, const std::string& id, const CMatrix* matrix, const size_t frequency);

/// <summary> Create the stream information for stimulations. </summary>
/// <param name="name"> The name of the stream. </param>
/// <param name="id"> The identifier of the stream. </param>
/// <returns> the stream info. </returns>
lsl::stream_info createStimulationStreamInfo(const std::string& name, const std::string& id);

/// <summary> Send Signal in LSL outlet. </summary>
/// <param name="outlet"> The LSL outlet. </param>
/// <param name="matrix"> The matrix with the signal to send. </param>
/// <param name="startTime"> The start time of the signal. </param>
/// <param name="endTime"> The end time of the signal. </param>
void sendSignal(lsl::stream_outlet* outlet, const CMatrix* matrix, const uint64_t startTime, const uint64_t endTime);

/// <summary> Send the stimulation in LSL outlet. </summary>
/// <param name="outlet"> The LSL outlet. </param>
/// <param name="stimSet">The stimulation set to send. </param>
void sendStimulation(lsl::stream_outlet* outlet, const CStimulationSet* stimSet);


//-------------------------------------------------------------------------------------------------
// Receiving
//-------------------------------------------------------------------------------------------------

/// <summary> Finds the stream information with the function <c>lsl::resolve_stream</c>. </summary>
/// <param name="name"> The expected name of the stream. </param>
/// <param name="id"> Optionally The expected ID of the stream in addition of the name. </param>
/// <param name="timeout"> Optionally a timeout of the operation, in seconds (default: no timeout).
/// If the timeout expires, a default stream info is returned. </param>
/// <returns> Found <c>lsl::stream_info</c>, empty <c>lsl::stream_info</c> if not found. </returns>
lsl::stream_info findStreamInfo(const std::string& name, const std::string& id = "", const int timeout = LSL_FOREVER);

}  // namespace LSL
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
