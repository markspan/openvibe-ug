///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
///-------------------------------------------------------------------------------------------------

#pragma once

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#include "../ovasCConfigurationBuilder.h"
#include <gtk/gtk.h>
#include <vector>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationBioSemiActiveTwo final : public CConfigurationBuilder
{
public:

    CConfigurationBioSemiActiveTwo(const char* gtkBuilderFilename, bool& useExChannels);

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    bool& m_useEXChannels;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyBioSemiAPI
