///-------------------------------------------------------------------------------------------------
///
/// \file ovasCConfigurationBrainProductsBrainampSeries.h
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

#include <ovasCConfigurationBuilder.h>

#if defined TARGET_OS_Windows

#include "ovas_defines_brainamp_series.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CDriverBrainProductsBrainampSeries;

class CConfigurationBrainProductsBrainampSeries final : public CConfigurationBuilder
{
public:
    CConfigurationBrainProductsBrainampSeries(CDriverBrainProductsBrainampSeries& driver, const char* gtkBuilderFileName, uint32_t& usbIdx,
                                              uint32_t& decimationFactor, EParameter* channelSelected, EParameter* lowPassFilterFull,
                                              EParameter* resolutionFull, EParameter* dcCouplingFull, EParameter& lowPass, EParameter& resolution,
                                              EParameter& dcCoupling, EParameter& impedance);

    bool preConfigure() override;
    bool postConfigure() override;

    void buttonChannelDetailsPressedCB();
    void comboBoxDeviceChangedCB();

protected:
    CDriverBrainProductsBrainampSeries& m_driver;
    uint32_t& m_usbIdx;
    uint32_t& m_decimationFactor;
    EParameter* m_channelSelected   = nullptr;
    EParameter* m_lowPassFilterFull = nullptr;
    EParameter* m_resolutionFull    = nullptr;
    EParameter* m_dcCouplingFull    = nullptr;
    EParameter& m_lowPass;
    EParameter& m_resolution;
    EParameter& m_dcCoupling;
    EParameter& m_impedance;

    // private:
    // GtkWidget* m_calibrateDialog = nullptr;
    // bool m_calibrationDone       = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
