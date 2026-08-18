#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI_Linux

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gAPI.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationGTecGUSBampLinux
 * \author Tom Stewart (University of Tsukuba)
 * \date Mon Feb  9 18:59:22 2015
 * \brief The CConfigurationGTecGUSBampLinux handles the configuration dialog specific to the g.tec g.USBamp for Linux device.
 *
 * TODO: details
 *
 * \sa CDriverGTecGUSBampLinux
 */
class CConfigurationGTecGUSBampLinux final : public CConfigurationBuilder
{
public:
    // Thresholds for reporting on measured impedance, these are the same as the ones that the simulink driver uses
    static const int LowImpedance = 5, ModerateImpedance = 7, HighImpedance = 100;

    enum ChannelTreeViewColumn { ChannelColumn = 0, BipolarColumn, NotchColumn, NotchIdColumn, BandpassColumn, BandpassIdColumn };

    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    CConfigurationGTecGUSBampLinux(IDriverContext& ctx, const char* gtkBuilderFilename, std::string* deviceName, gt_usbamp_config* config);

    bool preConfigure() override;
    bool postConfigure() override;

    void OnButtonApplyConfigPressed(ChannelTreeViewColumn type);
    void OnButtonCheckImpedanceClicked();
    void OnComboboxSamplingFrequencyChanged();
    void OnComboboxDeviceChanged();

protected:
    IDriverContext& m_driverCtx;

private:
    /*
     * Insert here all specific attributes, such as a connection ID.
     * use references to directly modify the corresponding attribute of the driver
     * Example:
     */
    std::string* m_deviceName = nullptr;
    gt_usbamp_config* m_config;
    void UpdateFilters();
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI_Linux
