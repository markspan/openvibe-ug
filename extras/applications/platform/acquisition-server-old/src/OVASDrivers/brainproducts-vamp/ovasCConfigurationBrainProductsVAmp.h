#pragma once

#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"
#include "ovasCHeaderBrainProductsVAmp.h"

#include <gtk/gtk.h>

#include <windows.h>
#include <FirstAmp.h>

namespace OpenViBE {
namespace AcquisitionServer {

/**
 * \class CConfigurationBrainProductsVAmp
 * \author Laurent Bonnet (INRIA)
 * \date 16 nov 2009
 * \erief The CConfigurationBrainProductsVAmp handles the configuration dialog specific to the VAmp device.
 *
 * User can configure the acquisition mode (normal/fast) and the fast mode settings (monopolar or differential pair).
 *
 * \sa CDriverBrainProductsVAmp
 */
class CConfigurationBrainProductsVAmp final : public CConfigurationBuilder
{
public:

    CConfigurationBrainProductsVAmp(IDriverContext& ctx, const char* gtkBuilderFilename, CHeaderBrainProductsVAmp* headerBrainProductsVAmp,
                                    bool& acquireAuxiliaryAsEEG, bool& acquireTriggerAsEEG);

    bool preConfigure() override;
    bool postConfigure() override;

    void buttonFastModeSettingsCB();
    void buttonStartServiceCB() { controlVampService(true); }
    void buttonStopServiceCB() { controlVampService(false); }
    void channelCountChangedCB();

protected:

    IDriverContext& m_driverCtx;
    int m_iDeviceCount = 0;

    CHeaderBrainProductsVAmp* m_headerBrainProductsVAmp = nullptr;

    bool& m_rAcquireAuxiliaryAsEEG;
    bool& m_rAcquireTriggerAsEEG;

    //widgets
    GtkWidget* m_dialogFastModeSettings = nullptr;
    GtkWidget* m_Device                 = nullptr;
    GtkWidget* m_acquisitionMode        = nullptr;
    GtkWidget* m_auxiliaryChannels      = nullptr;
    GtkWidget* m_triggerChannels        = nullptr;
    GtkWidget* m_pair1PositiveInputs    = nullptr;
    GtkWidget* m_pair1NegativeInputs    = nullptr;
    GtkWidget* m_pair2PositiveInputs    = nullptr;
    GtkWidget* m_pair2NegativeInputs    = nullptr;
    GtkWidget* m_pair3PositiveInputs    = nullptr;
    GtkWidget* m_pair3NegativeInputs    = nullptr;
    GtkWidget* m_pair4PositiveInputs    = nullptr;
    GtkWidget* m_pair4NegativeInputs    = nullptr;

private:
    bool controlVampService(bool state);
    //DWORD startWindowsService(SC_HANDLE hService);
    //gboolean idleCheckVampService(gpointer data);
    gint m_giIdleID = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyUSBFirstAmpAPI
