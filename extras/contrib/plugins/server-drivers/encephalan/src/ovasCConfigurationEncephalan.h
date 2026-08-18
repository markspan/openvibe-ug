///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationEncephalan.h
/// \brief The CConfigurationEncephalan handles the configuration dialog specific to the Encephalan device.
/// \author Alexey Minin (UrFU)
/// \version 1.0.
/// \date 02/01/2019.
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

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/// <summary>   The CConfigurationEncephalan handles the configuration dialog specific to the Encephalan device. </summary>
/// <seealso cref="CDriverEncephalan" />
class CConfigurationEncephalan final : public CConfigurationBuilder
{
public:
    CConfigurationEncephalan(IDriverContext& driverContext, const char* gtkFileName, uint32_t& connectionPort, const std::string& connectionIp);

    bool preConfigure() override;
    bool postConfigure() override;
    std::string getConnectionIp() const { return m_connectionIp; }

protected:
    IDriverContext& m_driverContext;
    uint32_t& m_connectionPort;
    std::string m_connectionIp;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
