#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationGTecGUSBampLegacy final : public CConfigurationBuilder
{
public:
    CConfigurationGTecGUSBampLegacy(const char* gtkBuilderFilename, uint32_t& usbIdx, uint8_t& commonGndAndRefBitmap, int& notchFilterIdx,
                                    int& bandPassFilterIdx, bool& triggerInput);

    bool preConfigure() override;
    bool postConfigure() override;

    void buttonCalibratePressedCB();
    void idleCalibrateCB();

    void buttonCommonGndRefPressedCB();
    void buttonFiltersPressedCB();

protected:
    uint32_t& m_usbIdx;
    uint8_t& m_commonGndAndRefBitmap;

    int& m_notchFilterIdx;
    int& m_bandPassFilterIdx;
    bool& m_triggerInput;

private:
    GtkWidget* m_calibrateDialog = nullptr;
    bool m_calibrationDone       = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
