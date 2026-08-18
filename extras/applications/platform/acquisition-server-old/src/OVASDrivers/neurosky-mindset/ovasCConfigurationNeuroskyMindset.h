#pragma once

#if defined TARGET_HAS_ThirdPartyThinkGearAPI

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

#define OVAS_MINDSET_INVALID_COM_PORT 0xffff

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationNeuroskyMindset
 * \author Laurent Bonnet (INRIA)
 * \date 05 may 2010
 * \erief The CConfigurationNeuroskyMindset handles the configuration dialog specific to the MindSet device.
 *
 * User can configure ... (TODO).
 *
 * \sa CDriverNeuroskyMindset
 */
class CConfigurationNeuroskyMindset final : public CConfigurationBuilder
{
public:

    CConfigurationNeuroskyMindset(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& comPort, bool& eSenseChannels, bool& bandPowerChannels,
                                  bool& blinkStimulations, bool& blinkStrengthChannel);

    bool preConfigure() override;
    bool postConfigure() override;

    //virtual void buttonCheckSignalQualityCB();
    //virtual void buttonRefreshCB();

protected:

    IDriverContext& m_driverCtx;
    int m_nDevice = 0;

    // the parameters passed to the driver :
    uint32_t& m_rComPort;
    bool& m_eSenseChannels;
    bool& m_bandPowerChannels;
    bool& m_blinkStimulations;
    bool& m_blinkStrengthChannel;

    //widgets
    GtkWidget* m_comPortSpinButton = nullptr;

    bool m_checkSignalQuality = false;

private:

    uint32_t m_currentConnectionId = uint32_t(-1);
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyThinkGearAPI
