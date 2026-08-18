#if defined TARGET_HAS_ThirdPartyThinkGearAPI

#include "ovasCConfigurationNeuroskyMindset.h"
#include <system/ovCTime.h>
#include <sstream>

#include <thinkgear.h>

namespace OpenViBE {
namespace AcquisitionServer {

//------------------------------------------------------------------------------------------
// NOTE : The signal checker is implemented but the device does not seem to handle 
//        a lot of connection/reconnection and this functionnality may cause 
//        the bluetooth connection to crash when configuring, checking, 
//        then reconnecting, and reading data. The signal check button in the 
//        configuration window is not visible, but the checking is always performed online.
//------------------------------------------------------------------------------------------
//_________________________________________________

CConfigurationNeuroskyMindset::CConfigurationNeuroskyMindset(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& comPort, bool& eSenseChannels,
                                                             bool& bandPowerChannels, bool& blinkStimulations, bool& blinkStrengthChannel)
    : CConfigurationBuilder(gtkBuilderFilename), m_driverCtx(ctx), m_rComPort(comPort), m_eSenseChannels(eSenseChannels),
      m_bandPowerChannels(bandPowerChannels), m_blinkStimulations(blinkStimulations), m_blinkStrengthChannel(blinkStrengthChannel) { }

bool CConfigurationNeuroskyMindset::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }
    //::GtkWidget * windowCheckSignalQuality=GTK_WIDGET(gtk_builder_get_object(m_builder, "dialog_check_signal_quality"));

    /*
    g_signal_connect(gtk_builder_get_object(m_builder, "button_check_signal_quality"),"pressed",G_CALLBACK(button_check_signal_quality_cb), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_refresh"),"pressed",G_CALLBACK(button_refresh_cb), this);
    //hide on close
    g_signal_connect (G_OBJECT(windowCheckSignalQuality), "delete_event", G_CALLBACK(gtk_widget_hide), nullptr);
    */

    char buffer[1024];
    int count     = 0;
    bool selected = false;

    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_com_port"));

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "ThinkGear DLL version: " << int(TG_GetVersion()) << "\n";

    // try the com ports. @NOTE almost duplicate code in CDriverNeuroskyMindset
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Scanning COM ports 1 to 16...\n";
    for (uint32_t i = 1; i < 16; ++i)
    {
        /* Get a new connection ID handle to ThinkGear API */
        const int connectionId = TG_GetNewConnectionId();
        if (connectionId < 0)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The driver was unable to connect to the ThinkGear Communication Driver.\n";
            return false;
        }
        // m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "ThinkGear Connection ID is: "<< connectionId <<".\n";

        /* Attempt to connect the connection ID handle to serial port */
        std::stringstream comPortName;
        comPortName << "\\\\.\\COM" << i;
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Trying port [" << comPortName.str() << "]\n";
        int code = TG_Connect(connectionId, comPortName.str().c_str(), TG_BAUD_9600, TG_STREAM_PACKETS);
        if (code >= 0)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Connection available on port [" << comPortName.str() << "]";

            const uint32_t startTime = System::Time::getTime();
            const uint32_t timeToTry = 3000; // ms

            // With e.g. MindWave Mobile, errors do not mean that the operation couldn't succeed in the future, so we ask for a packet optimistically for a while.
            bool comPortFound = false;
            while (!comPortFound && (System::Time::getTime() - startTime) < timeToTry)
            {
                //we try to read one packet to check the connection.
                code = TG_ReadPackets(connectionId, 1);
                if (code >= 0)
                {
                    m_driverCtx.getLogManager() << " - Status: OK\n";
                    sprintf(buffer, "COM%i", i);
                    gtk_combo_box_append_text(comboBox, buffer);
                    if (m_rComPort == i)
                    {
                        gtk_combo_box_set_active(comboBox, count);
                        selected = true;
                    }
                    count++;
                    comPortFound = true;
                }
                else { System::Time::sleep(1); }
            }
            if (!comPortFound)
            {
                m_driverCtx.getLogManager() << " - Tried for " << timeToTry / 1000 << " seconds, gave up.\n";
                if (code == -1) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: -1, Invalid connection ID\n"; }
                else if (code == -2) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: -2, 0 bytes on the stream\n"; }
                else if (code == -3) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: -3, I/O error occurred\n"; }
                else { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Last TG_ReadPackets error: " << code << ", Unknown\n"; }
            }
        }
        else { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "TG_Connect() returned error " << code << "\n"; }
        // free the connection to ThinkGear API
        // We use FreeConnection() only as doing a TG_Disconnect()+TG_FreeConnection() pair can cause first-chance exceptions on visual studio & MindWave Mobile for some reason.
        TG_FreeConnection(connectionId);
    }

    if (count == 0) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The driver was unable to find any valid device on serial port COM1 to COM16.\n"; }

    if (!selected) { gtk_combo_box_set_active(comboBox, 0); }

    GtkToggleButton* toggleESense        = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_esense"));
    GtkToggleButton* togglePower         = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_power"));
    GtkToggleButton* toggleBlink         = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_blink"));
    GtkToggleButton* toggleBlinkStrength = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_blink_strenght"));

    gtk_toggle_button_set_active(toggleESense, m_eSenseChannels);
    gtk_toggle_button_set_active(togglePower, m_bandPowerChannels);
    gtk_toggle_button_set_active(toggleBlink, m_blinkStimulations);
    gtk_toggle_button_set_active(toggleBlinkStrength, m_blinkStrengthChannel);

    return (count > 0);
}

bool CConfigurationNeuroskyMindset::postConfigure()
{
    if (m_applyConfig)
    {
        GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_com_port"));

        int comPort        = 0;
        const char* usbIdx = gtk_combo_box_get_active_text(comboBox);
        if (usbIdx) { if (sscanf(usbIdx, "COM%i", &comPort) == 1) { m_rComPort = uint32_t(comPort); } }

        GtkToggleButton* toggleESense        = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_esense"));
        GtkToggleButton* togglePower         = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_power"));
        GtkToggleButton* toggleBlink         = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_blink"));
        GtkToggleButton* toggleBlinkStrength = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "check_blink_strenght"));

        m_eSenseChannels       = (gtk_toggle_button_get_active(toggleESense) != 0); // assign to bool while avoiding C4800 warning on MSVC
        m_bandPowerChannels    = (gtk_toggle_button_get_active(togglePower) != 0);
        m_blinkStimulations    = (gtk_toggle_button_get_active(toggleBlink) != 0);
        m_blinkStrengthChannel = (gtk_toggle_button_get_active(toggleBlinkStrength) != 0);
    }

    if (! CConfigurationBuilder::postConfigure()) { return false; } // normal header is filled, ressources are realesed

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyThinkGearAPI
