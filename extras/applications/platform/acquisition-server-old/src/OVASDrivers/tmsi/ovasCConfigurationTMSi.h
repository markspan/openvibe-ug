///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2014, Mensia Technologies SA. All rights reserved.
/// Rights transferred to Inria, contract signed 21.11.2014
///-------------------------------------------------------------------------------------------------

#pragma once

#include "../ovasCConfigurationBuilder.h"
#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CDriverTMSi;

class CConfigurationTMSi final : public CConfigurationBuilder
{
public:
    CConfigurationTMSi(const char* gtkBuilderFilename, CDriverTMSi* driver);
    ~CConfigurationTMSi() override;

    bool preConfigure() override;
    bool postConfigure() override;

    void fillDeviceCombobox() const;
    bool fillSamplingFrequencyCombobox();
    void fillAdditionalChannelsTable();
    void clearAdditionalChannelsTable();
    CString getActiveAdditionalChannels();

    void showWaitWindow();
    void hideWaitWindow();

    CDriverTMSi* m_Driver = nullptr;

    GtkSpinButton* m_buttonChannelCount       = nullptr;
    GtkComboBox* m_comboBoxConnectionProtocol = nullptr;
    GtkComboBox* m_comboBoxDeviceID           = nullptr;
    GtkComboBox* m_comboBoxSamplingFrequency  = nullptr;
    GtkComboBox* m_comboBoxImpedanceLimit     = nullptr;
    //GtkToggleButton* m_ButtonCommonAverageReference = nullptr;
    GtkLabel* m_LabelAdditionalChannels = nullptr;
    GtkTable* m_TableAdditionalChannels = nullptr;
    std::vector<GtkCheckButton*> m_AdditionalChannelCheckButtons;
private:
    std::vector<CString> m_additionalChannelNames;

    GtkWidget* m_waitWindow = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
