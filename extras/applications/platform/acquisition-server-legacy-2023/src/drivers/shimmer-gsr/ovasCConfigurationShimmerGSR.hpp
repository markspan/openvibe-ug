///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCConfigurationShimmerGSR.hpp
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

#if defined TARGET_OS_Windows

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {

class CConfigurationShimmerGSR : public CConfigurationBuilder
{
public:

    CConfigurationShimmerGSR(const char* gtkBuilderFilename, std::vector<uint32_t> serialPorts, uint32_t& port, double& samplingFrequency);

    virtual bool preConfigure();
    virtual bool postConfigure();

protected:
    std::vector<uint32_t> m_serialPorts;
    GtkListStore* m_PortsListStore = nullptr;
    uint32_t& m_portIndex;

    uint32_t m_samplingFrequenciesSize = 5;
    double m_samplingFrequencies[5] = { 32,64,128,256,512 };
    GtkListStore* m_samplingFrequencyListStore = nullptr;
    double& m_samplingFrequency;
};

}  //namespace AcquisitionServer
}  //namespace OpenViBE

#endif  // TARGET_OS_Windows
