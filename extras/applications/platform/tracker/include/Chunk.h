///-------------------------------------------------------------------------------------------------
/// 
/// \file Chunk.h
/// \author J. T. Lindgren / Inria.
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
namespace Tracker {
/// <summary> Base class for all OpenViBE stream content (chunk) types. </summary>
///
/// Regardless of the type, all chunks in OpenViBE have start and end timestamps.
/// Hence they should derive from this class.
/// In particular, Header, Bufferand End chunks for some particular type should ultimately derive from this class.
/// However, this class defines no content.
/// \todo In the future if these types turn out to be more generally useful, we could consider moving them to the Kernel.
class Chunk
{
public:
    CTime m_StartTime = CTime::min();
    CTime m_EndTime   = CTime::min();
};
}  // namespace Tracker
}  // namespace OpenViBE
