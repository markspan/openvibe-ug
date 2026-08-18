///-------------------------------------------------------------------------------------------------
///
/// \file Utils.cpp
/// \brief Implementation of utils for LSL within OpenViBE
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

#ifdef TARGET_HAS_ThirdPartyLSL

#include "Utils.hpp"

#include <system/ovCTime.h>
#include <openvibe/CTime.hpp>
#include <iostream>

namespace OpenViBE {
namespace LSL {

//-------------------------------------------------------------------------------------------------
double getLSLRelativeTime(const CTime& time)
{
    const CTime ovTimeNow(System::Time::zgetTime());
    const double lslRelativeTime = lsl::local_clock();

    double diffToCurrent;
    if (time > ovTimeNow) { diffToCurrent = (time - ovTimeNow).toSeconds(); }
    else { diffToCurrent = -(ovTimeNow - time).toSeconds(); }

    return (lslRelativeTime + diffToCurrent);
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
lsl::stream_info createSignalStreamInfo(const std::string& name, const std::string& id, const CMatrix* matrix, const size_t frequency)
{
    const size_t nChannel = matrix->getDimensionSize(0);

    // Open a signal stream 
    lsl::stream_info res(name, "signal", int(nChannel), double(frequency), lsl::cf_float32, id);

    lsl::xml_element channels = res.desc().append_child("channels");
    for (size_t c = 0; c < nChannel; ++c) {
        const std::string tmp = matrix->getDimensionLabel(0, c);
        channels.append_child("channel").append_child_value("label", tmp).append_child_value("unit", "unknown").append_child_value("type", "signal");
    }

    return res;
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
lsl::stream_info createStimulationStreamInfo(const std::string& name, const std::string& id)
{
    lsl::stream_info res(name, "Markers", 1, lsl::IRREGULAR_RATE, lsl::cf_int32, id);
    res.desc().append_child("channels").append_child("channel").append_child_value("label", "Stimulations").append_child_value("type", "marker");
    return res;
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void sendSignal(lsl::stream_outlet* outlet, const CMatrix* matrix, const uint64_t startTime, const uint64_t endTime)
{
    if (outlet->have_consumers()) {
        const size_t nChannel = matrix->getDimensionSize(0);
        const size_t nSamples = matrix->getDimensionSize(1);
        const double* iBuffer = matrix->getBuffer();
        std::vector<float> buffer(nChannel);

        // note: the step computed below should be exactly the same as could be obtained from the sampling rate
        const double start = CTime(startTime).toSeconds();
        const double step  = CTime(endTime - startTime).toSeconds() / double(nSamples);

        for (size_t s = 0; s < nSamples; ++s) {
            for (size_t c = 0; c < nChannel; ++c) { buffer[c] = float(iBuffer[c * nSamples + s]); }
            outlet->push_sample(buffer, start + double(s) * step);
        }
    }
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void sendStimulation(lsl::stream_outlet* outlet, const CStimulationSet* stimSet)
{
    if (outlet->have_consumers()) {
        for (size_t s = 0; s < stimSet->size(); ++s) {
            const int code    = int(stimSet->getId(s));
            const double date = CTime(stimSet->getDate(s)).toSeconds();
            outlet->push_sample(&code, date);
        }
    }
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
lsl::stream_info findStreamInfo(const std::string& name, const std::string& id, const int timeout)
{
    // Find the signal stream
    const std::vector<lsl::stream_info> infos = lsl::resolve_stream("name", name, 1, timeout);
    if (infos.empty()) {
        if (timeout != 0) { std::cerr << "Failed to find stream with name [" << name << "]\n"; }    // Avoid a print if timeout is 0
        return lsl::stream_info();
    }

    for (const auto& i : infos) {
        if (i.source_id() == id) { return i; }  // This is the best one
    }
    return infos[0];
}
//-------------------------------------------------------------------------------------------------

}  // namespace LSL
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyLSL
