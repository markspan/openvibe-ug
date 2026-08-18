///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCHeaderAdapterBrainProductsBrainampSeries.h
/// \brief Brain Products Brainamp Series driver for OpenViBE
/// \author Yann Renard
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

#include "../ovasCHeader.h"

#if defined TARGET_OS_Windows

#include "ovas_defines_brainamp_series.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CHeaderAdapterBrainProductsBrainampSeries final : public CHeaderAdapter
{
public:
    CHeaderAdapterBrainProductsBrainampSeries(IHeader& adaptedHeader, EParameter* channelSelected)
        : CHeaderAdapter(adaptedHeader), m_ChannelSelected(channelSelected) {}

    bool setChannelCount(const size_t /*nChannel*/) override { return false; }
    bool setChannelName(const size_t /*index*/, const char* /*name*/) override { return false; }

    size_t getChannelCount() const override;
    const char* getChannelName(const size_t index) const override;

    EParameter* m_ChannelSelected = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
