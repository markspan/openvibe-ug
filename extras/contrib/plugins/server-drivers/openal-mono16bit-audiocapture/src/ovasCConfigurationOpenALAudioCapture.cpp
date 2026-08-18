#include "ovasCConfigurationOpenALAudioCapture.h"

#if defined TARGET_HAS_ThirdPartyOpenAL
namespace OpenViBE {
namespace AcquisitionServer {

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(GtkButton* button, void* data)
{
    CConfigurationOpenALAudioCapture* config=static_cast<CConfigurationOpenALAudioCapture*>(data);
    config->buttonCalibratePressedCB();
}

//Callback actually called:
void CConfigurationGTecGUSBamp::buttonCalibratePressedCB()
{
    // Connect to the hardware, ask for calibration, verify the return code, etc.
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationOpenALAudioCapture::CConfigurationOpenALAudioCapture(IDriverContext& ctx, const char* gtkBuilderFilename)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx) {}

bool CConfigurationOpenALAudioCapture::preConfigure() { return CConfigurationBuilder::preConfigure(); }

bool CConfigurationOpenALAudioCapture::postConfigure() { return CConfigurationBuilder::postConfigure(); }

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif //TARGET_HAS_ThirdPartyOpenAL
