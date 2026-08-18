///-------------------------------------------------------------------------------------------------
/// 
/// \file IVRPNServerManager.hpp
/// \brief Interface for VRPN Management.
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
namespace VRPN {

class IVRPNServerManager
{
public:
    static IVRPNServerManager& GetInstance();

    virtual ~IVRPNServerManager() { }

    virtual bool Initialize() = 0;
    virtual bool Uninitialize() = 0;

    virtual bool Process() = 0;
    virtual bool ReportAnalog(const CIdentifier& identifier) = 0;
    virtual bool ReportButton(const CIdentifier& identifier) = 0;

    virtual bool AddServer(const CString& name, CIdentifier& identifier) = 0;
    virtual bool IsServer(const CIdentifier& identifier) const = 0;
    virtual bool IsServer(const CString& name) const = 0;
    virtual bool GetServerId(const CString& name, CIdentifier& identifier) const = 0;
    virtual bool GetServerName(const CIdentifier& identifier, CString& name) const = 0;
    virtual bool RemoveServer(const CIdentifier& identifier) = 0;

    virtual bool SetButtonCount(const CIdentifier& identifier, size_t nButton) = 0;
    virtual bool SetButtonState(const CIdentifier& identifier, size_t buttonIndex, bool status) = 0;
    virtual bool GetButtonState(const CIdentifier& identifier, size_t buttonIndex) const = 0;

    virtual bool SetAnalogCount(const CIdentifier& serverID, size_t nAnalog) = 0;
    virtual bool SetAnalogState(const CIdentifier& serverID, size_t analogIndex, double status) = 0;
    virtual double GetAnalogState(const CIdentifier& serverID, size_t analogIndex) const = 0;
};

}  // namespace VRPN
}  // namespace Plugins
}  // namespace OpenViBE
