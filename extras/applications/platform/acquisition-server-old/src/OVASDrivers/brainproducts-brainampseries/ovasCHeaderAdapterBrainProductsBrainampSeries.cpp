///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCHeaderAdapterBrainProductsBrainampSeries.cpp
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

#include "ovasCHeaderAdapterBrainProductsBrainampSeries.h"

#if defined TARGET_OS_Windows

namespace OpenViBE {
namespace AcquisitionServer {

size_t CHeaderAdapterBrainProductsBrainampSeries::getChannelCount() const
{
    size_t j = 0;
    for (size_t i = 0; i < m_header.getChannelCount(); ++i) { if (m_ChannelSelected[i] == Channel_Selected) { j++; } }
    return j;
}

const char* CHeaderAdapterBrainProductsBrainampSeries::getChannelName(const size_t index) const
{
    size_t j = 0;
    for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
        if (m_ChannelSelected[i] == Channel_Selected) {
            if (j == index) { return m_header.getChannelName(i); }
            j++;
        }
    }
    return "";
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
