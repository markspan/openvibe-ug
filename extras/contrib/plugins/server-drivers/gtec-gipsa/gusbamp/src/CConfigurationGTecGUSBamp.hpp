#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>
#include <string>
#include <vector>

namespace OpenViBE
{
    namespace AcquisitionServer
    {
        class CConfigurationGTecGUSBamp final : public CConfigurationBuilder
        {
        public:
            CConfigurationGTecGUSBamp(const char* gtkBuilderFilename, uint8_t& commonGndAndRefBitmap, int& notchFilterIdx, int& bandPassFilterIdx,
                                      bool& triggerInput, const std::vector<std::string>& devicesSerials, std::string& masterDeviceIndex, bool& bipolar,
                                      bool& calibrationSignalEnabled, bool& showDeviceName);

            bool preConfigure() override;
            bool postConfigure() override;

            void buttonCalibratePressedCB();
            void idleCalibrateCB();

            void buttonCommonGndRefPressedCB();
            void buttonFiltersPressedCB();
            void setHardwareFiltersDialog();
            void buttonFiltersApplyPressedCB();

        protected:
            uint8_t& m_commonGndAndRefBitmap;

            int& m_notchFilterIdx;
            int& m_bandPassFilterIdx;
            bool& m_triggerInput;
            std::vector<std::string> m_devicesSerials;
            std::string& m_masterDeviceIdx;
            std::vector<size_t> m_comboBoxBandPassFilterIdx;
            std::vector<size_t> m_comboBoxNotchFilterIdx;
            bool& m_bipolarEnabled;
            bool& m_calibrationSignalEnabled;
            bool& m_showDeviceName;

            GtkWidget* m_calibrateDialog = nullptr;
            bool m_calibrationDone       = false;
        };
    }  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
