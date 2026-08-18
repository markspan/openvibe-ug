#pragma once

#include "ovas_base.h"
#include "ovasIDriver.h"
#include "ovasIHeader.h"
#include "ovasCHeader.h"

#include <thread>

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CAcquisitionServer;
class CAcquisitionServerThread;
class IAcquisitionServerPlugin;
struct PluginSetting;
class Property;

class CAcquisitionServerGUI final
{
public:
    explicit CAcquisitionServerGUI(const Kernel::IKernelContext& ctx);
    ~CAcquisitionServerGUI();

    bool initialize();

    IDriver& getDriver() const { return *m_Driver; }
    uint32_t getSampleCountPerBuffer() { return m_nSamplePerBuffer; }
    uint32_t getTCPPort() const;
    IHeader& getHeaderCopy() { return m_headerCopy; }

    void setStateText(const char* sStateText) const;
    void setClientText(const char* sClientText) const;
    void setDriftMs(double ms) const;
    void setImpedance(uint32_t index, double impedance);
    void disconnect() const;

    // GTK button callbacks
    void buttonPreferencePressedCB(GtkButton* button);
    void buttonConfigurePressedCB(GtkButton* button);
    void buttonConnectToggledCB(GtkToggleButton* button);
    void buttonStartPressedCB(GtkButton* button);
    void buttonStopPressedCB(GtkButton* button);
    void comboBoxDriverChanged(GtkComboBox* box);
    void comboBoxSampleCountPerSentBlockChanged(GtkComboBox* box);

    /// registers a new acquisition server plugin, the plugin is activated immediately
    void registerPlugin(IAcquisitionServerPlugin* plugin) const;

    /// scans all plugins for settings and puts them into a flat structure easier to handle
    void scanPluginSettings();

    void savePluginSettings() const;

    class PropertyAndWidget
    {
    public:
        PropertyAndWidget(Property* prop, GtkWidget* widget) : m_Property(prop), m_Widget(widget) { }

        Property* m_Property = nullptr;
        GtkWidget* m_Widget  = nullptr;
    };

    /// holds references to the plugins' settings for faster access
    std::vector<PropertyAndWidget> m_PluginsProperties;

protected :
    const Kernel::IKernelContext& m_kernelCtx;
    IDriver* m_Driver                                   = nullptr;
    IDriverContext* m_driverCtx                         = nullptr;
    CAcquisitionServer* m_acquisitionServer             = nullptr;
    CAcquisitionServerThread* m_acquisitionServerThread = nullptr;
    CHeader m_headerCopy;

    uint32_t m_nSamplePerBuffer = 0;

    std::vector<IDriver*> m_drivers;

    GtkBuilder* m_builder = nullptr;

    GtkWidget* m_impedanceWindow = nullptr;
    std::vector<GtkWidget*> m_levelMesures;

    std::thread* m_thread = nullptr;
#if defined TARGET_OS_Windows && defined TARGET_HasMensiaAcquisitionDriver
    void* m_libMensia = nullptr;
#endif
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
