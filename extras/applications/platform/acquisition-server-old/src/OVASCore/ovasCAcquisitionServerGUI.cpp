#include "ovasCAcquisitionServerGUI.h"
#include "ovasCAcquisitionServerThread.h"
#include "ovasCAcquisitionServer.h"
#include "ovasIAcquisitionServerPlugin.h"
// Drivers

#ifdef TARGET_HAS_OpenViBEContributions
#include "contribAcquisitionServer.inl"
#endif

#include "ovasCPluginLSLOutput.h"
#include "ovasCPluginFiddler.h"

#include "generic-raw-reader/ovasCDriverGenericRawFileReader.h"
#include "generic-raw-reader/ovasCDriverGenericRawTelnetReader.h"

// Simulation drivers
#include "generic-oscillator/ovasCDriverGenericOscillator.h"
#include "generic-sawtooth/ovasCDriverGenericSawTooth.h"
#include "generic-time-signal/ovasCDriverGenericTimeSignal.h"
#include "simulated-deviator/ovasCDriverSimulatedDeviator.h"

#include "biosemi-activetwo/ovasCDriverBioSemiActiveTwo.h"
#include "brainproducts-actichamp/ovasCDriverBrainProductsActiCHamp.h"
#include "brainproducts-brainampseries/ovasCDriverBrainProductsBrainampSeries.h"
#include "brainproducts-vamp/ovasCDriverBrainProductsVAmp.h"
#include "brainproducts-liveamp/ovasCDriverBrainProductsLiveAmp.h"
#include "egi-ampserver/ovasCDriverEGIAmpServer.h"
#include "emotiv-epoc/ovasCDriverEmotivEPOC.h"
#include "labstreaminglayer/ovasCDriverLabStreamingLayer.h"
#include "micromed-systemplusevolution/ovasCDriverMicromedSystemPlusEvolution.h"
#include "mindmedia-nexus32b/ovasCDriverMindMediaNeXus32B.h"
#include "mcs-nvx/ovasCDriverMCSNVXDriver.h"
#include "neuroelectrics-enobio3g/ovasCDriverEnobio3G.h"
#include "neuroservo/ovasCDriverNeuroServoHid.h"
#include "neurosky-mindset/ovasCDriverNeuroskyMindset.h"
#include "tmsi/ovasCDriverTMSi.h"
#include "tmsi-refa32b/ovasCDriverTMSiRefa32B.h"

#include "mensia-acquisition/ovasCDriverMensiaAcquisition.h"

#include "shimmer-gsr/ovasCDriverShimmerGSR.hpp"

#include <limits>

// Plugins

#include <fstream>

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstring>

#include <cassert>
#include <system/WindowsUtilities.h>
//

namespace OpenViBE {
namespace AcquisitionServer {

#define OVAS_GUI_File           OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface.ui"

// because std::tolower has multiple signatures,
// it can not be easily used in std::transform
// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
template <class T>
static T to_lower(T c) { return std::tolower(c); }

static void PreferencePressedCB(GtkButton* button, void* data) { static_cast<CAcquisitionServerGUI*>(data)->buttonPreferencePressedCB(button); }
static void ConfigurePressedCB(GtkButton* button, void* data) { static_cast<CAcquisitionServerGUI*>(data)->buttonConfigurePressedCB(button); }
static void ConnectToggledCB(GtkToggleButton* button, void* data) { static_cast<CAcquisitionServerGUI*>(data)->buttonConnectToggledCB(button); }
static void StartPressedCB(GtkButton* button, void* data) { static_cast<CAcquisitionServerGUI*>(data)->buttonStartPressedCB(button); }
static void StopPressedCB(GtkButton* button, void* data) { static_cast<CAcquisitionServerGUI*>(data)->buttonStopPressedCB(button); }
static void DriverChangedCB(GtkComboBox* box, void* data) { static_cast<CAcquisitionServerGUI*>(data)->comboBoxDriverChanged(box); }

static void SampleCountPerSentBlockChangedCB(GtkComboBox* box, void* data)
{
    static_cast<CAcquisitionServerGUI*>(data)->comboBoxSampleCountPerSentBlockChanged(box);
}

static bool compare_driver_names(IDriver* a, IDriver* b)
{
    std::string sA = a->getName();
    std::string sB = b->getName();

    std::transform(sA.begin(), sA.end(), sA.begin(), tolower);
    std::transform(sB.begin(), sB.end(), sB.begin(), tolower);

    return sA < sB;
}

//___________________________________________________________________//
//                                                                   //

CAcquisitionServerGUI::CAcquisitionServerGUI(const Kernel::IKernelContext& ctx)
    : m_kernelCtx(ctx)
{
    // bool showUnstable = m_kernelCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_ShowUnstable}", false);

    m_acquisitionServer = new CAcquisitionServer(ctx);

    m_drivers.push_back(new CDriverGenericOscillator(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericSawTooth(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericTimeSignal(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverSimulatedDeviator(m_acquisitionServer->getDriverContext()));

    m_drivers.push_back(new CDriverGenericRawFileReader(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericRawTelnetReader(m_acquisitionServer->getDriverContext()));

#if defined TARGET_OS_Windows
    m_drivers.push_back(new CDriverShimmerGSR(m_acquisitionServer->getDriverContext()));

    m_drivers.push_back(new CDriverBrainProductsBrainampSeries(m_acquisitionServer->getDriverContext()));
#endif

#if defined TARGET_HAS_ThirdPartyBrainProductsAmplifierSDK
    m_drivers.push_back(new CDriverBrainProductsLiveAmp(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverBrainProductsActiCHamp(m_acquisitionServer->getDriverContext()));
#endif

#if defined TARGET_HAS_ThirdPartyBioSemiAPI
    m_drivers.push_back(new CDriverBioSemiActiveTwo(m_acquisitionServer->getDriverContext()));
#endif

    m_drivers.push_back(new CDriverEGIAmpServer(m_acquisitionServer->getDriverContext()));

#if defined TARGET_HAS_ThirdPartyEmotivAPI
    m_drivers.push_back(new CDriverEmotivEPOC(m_acquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyEnobioAPI
    m_drivers.push_back(new CDriverEnobio3G(m_acquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyMCS
    m_drivers.push_back(new CDriverMKSNVXDriver(m_acquisitionServer->getDriverContext()));
#endif
#if defined(TARGET_HAS_ThirdPartyMicromed)
    m_drivers.push_back(new CDriverMicromedSystemPlusEvolution(m_acquisitionServer->getDriverContext()));
#endif
#if defined(TARGET_HAS_ThirdPartyNeuroServo)
    m_drivers.push_back(new CDriverNeuroServoHid(m_acquisitionServer->getDriverContext()));
#endif
    //#if defined TARGET_HAS_ThirdPartyEEGOAPI
    //    m_drivers.push_back(new CDriverEEGO(m_acquisitionServer->getDriverContext()));
    //#endif


#if defined(TARGET_HAS_ThirdPartyNeXus)
    m_drivers.push_back(new CDriverMindMediaNeXus32B(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverTMSiRefa32B(m_acquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyThinkGearAPI
    m_drivers.push_back(new CDriverNeuroskyMindset(m_acquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyTMSi
    m_drivers.push_back(new CDriverTMSi(m_acquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI
    m_drivers.push_back(new CDriverBrainProductsVAmp(m_acquisitionServer->getDriverContext()));
#endif
#if defined TARGET_HAS_ThirdPartyLSL
    m_drivers.push_back(new CDriverLabStreamingLayer(m_acquisitionServer->getDriverContext()));
#endif

    // BEGIN MENSIA ACQUISITION DRIVERS
#if defined TARGET_OS_Windows && defined TARGET_HasMensiaAcquisitionDriver

    m_acquisitionServer->getDriverContext().getLogManager() << Kernel::LogLevel_Trace << "Loading Mensia Driver Collection\n";
    m_libMensia            = nullptr;
    const std::string path = m_acquisitionServer->getDriverContext().getConfigurationManager().expand("${Path_Bin}/openvibe-driver-mensia-acquisition.dll").
                                                  toASCIIString();
    if (!std::ifstream(path).is_open()) {
        m_acquisitionServer->getDriverContext().getLogManager() << Kernel::LogLevel_Trace << "Couldn't open dll file ["
                << path << "], perhaps it was not installed.\n";
    }
    else {
        m_libMensia         = System::WindowsUtilities::utf16CompliantLoadLibrary(path.c_str());
        HINSTANCE libModule = HINSTANCE(m_libMensia);

        //if it can't be open return FALSE;
        if (libModule == nullptr) {
            m_acquisitionServer->getDriverContext().getLogManager() << Kernel::LogLevel_Warning << "Couldn't load DLL: ["
                    << path << "]. Got error: [" << size_t(GetLastError()) << "]\n";
        }
        else {
            typedef int (*initialize_mensia_library_t)();
            const initialize_mensia_library_t initLibrary = initialize_mensia_library_t(GetProcAddress(libModule, "initializeAcquisitionLibrary"));
            typedef const char* (*get_driver_id_t)(size_t driverId);
            const get_driver_id_t getDriverID = get_driver_id_t(GetProcAddress(libModule, "getDriverId"));

            const int nDevice = initLibrary();
            if (nDevice >= 0) {
                for (size_t i = 0; i < size_t(nDevice); i++) {
                    char id[1024];

                    strcpy(id, getDriverID(i));
                    if (strcmp(id, "") != 0) {
                        m_acquisitionServer->getDriverContext().getLogManager() << Kernel::LogLevel_Info << "Found driver ["
                                << id << "] in Mensia Driver Collection\n";
                        m_drivers.push_back(new CDriverMensiaAcquisition(m_acquisitionServer->getDriverContext(), id));
                    }
                }
            }
            else {
                m_acquisitionServer->getDriverContext().getLogManager() << Kernel::LogLevel_Error
                        << "Error occurred while initializing Mensia Acquisition Library\n";
            }

            FreeLibrary(libModule);
        }
    }
#endif
    // END MENSIA ACQUISITION DRIVERS

#if defined TARGET_HAS_OpenViBEContributions
    Contributions::InitiateContributions(this, m_acquisitionServer, ctx, &m_drivers);
#endif

    // Plugins that just send out data must be the last in list (since other plugins may modify the data)

#if defined TARGET_HAS_ThirdPartyLSL
    registerPlugin(new Plugins::CPluginLSLOutput(ctx));
#endif

    registerPlugin(new Plugins::CPluginFiddler(ctx));

    std::sort(m_drivers.begin(), m_drivers.end(), compare_driver_names);

    scanPluginSettings();

    m_acquisitionServerThread = new CAcquisitionServerThread(m_kernelCtx, *this, *m_acquisitionServer);

    // Initialize GTK objects as the thread started below may refer to them quickly
    this->initialize();

    m_thread = new std::thread(CAcquisitionServerThreadHandle(*m_acquisitionServerThread));
}

CAcquisitionServerGUI::~CAcquisitionServerGUI()
{
    m_acquisitionServerThread->terminate();
    m_thread->join();

    savePluginSettings();

    // Saves current configuration
    FILE* file = fopen(m_kernelCtx.getConfigurationManager().expand("${Path_UserData}/openvibe-acquisition-server.conf").toASCIIString(), "wt");
    if (file) {
        fprintf(file, "# This file is generated\n");
        fprintf(file, "# Do not modify\n");
        fprintf(file, "\n");
        fprintf(file, "# Last settings set in the acquisition server\n");
        fprintf(file, "AcquisitionServer_LastDriver = %s\n", m_Driver->getName());
        fprintf(file, "AcquisitionServer_LastSampleCountPerBuffer = %i\n", this->getSampleCountPerBuffer());
        fprintf(file, "AcquisitionServer_LastConnectionPort = %i\n", this->getTCPPort());
        fprintf(file, "# Last Preferences set in the acquisition server\n");
        fprintf(file, "AcquisitionServer_DriftCorrectionPolicy = %s\n", m_acquisitionServer->m_DriftCorrection.getDriftCorrectionPolicyStr().toASCIIString());
        fprintf(file, "AcquisitionServer_JitterEstimationCountForDrift = %u\n",
                uint32_t(m_acquisitionServer->m_DriftCorrection.getJitterEstimationCountForDrift()));
        fprintf(file, "AcquisitionServer_DriftToleranceDuration = %u\n", uint32_t(m_acquisitionServer->m_DriftCorrection.getDriftToleranceDurationMs()));
        fprintf(file, "AcquisitionServer_OverSamplingFactor = %u\n", uint32_t(m_acquisitionServer->getOversamplingFactor()));
        fprintf(file, "AcquisitionServer_ChannelSelection = %s\n", (m_acquisitionServer->isChannelSelectionRequested() ? "True" : "False"));
        fprintf(file, "AcquisitionServer_NaNReplacementPolicy = %s\n", toString(m_acquisitionServer->getNaNReplacementPolicy()).c_str());

        fprintf(file, "# Settings for various device drivers\n");
        std::vector<std::string> tokens;
        Kernel::IConfigurationManager* configMgr = &m_kernelCtx.getConfigurationManager();
        CIdentifier tokenID; // defaults to CIdentifier::undefined()
        // Collect token names
        while ((tokenID = configMgr->getNextConfigurationTokenIdentifier(tokenID)) != CIdentifier::undefined()) {
            const std::string prefix("AcquisitionServer_Driver_");
            const std::string prefix2("AcquisitionServer_Plugin_");
            CString tokenName = configMgr->getConfigurationTokenName(tokenID);
            if (std::string(tokenName.toASCIIString()).compare(0, prefix.length(), prefix) == 0
                || std::string(tokenName.toASCIIString()).compare(0, prefix.length(), prefix2) == 0) {
                std::string token = std::string(tokenName.toASCIIString());
                tokens.push_back(token);
            }
        }
        std::sort(tokens.begin(), tokens.end());
        // Write out as sorted
        for (size_t i = 0; i < tokens.size(); ++i) {
            CString tokenValue = configMgr->lookUpConfigurationTokenValue(tokens[i].c_str());
            fprintf(file, "%s = %s\n", tokens[i].c_str(), tokenValue.toASCIIString());
        }

        fclose(file);
    }

    for (auto it = m_drivers.begin(); it != m_drivers.end(); ++it) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Deleting " << (*it)->getName() << "\n";
        delete (*it);
    }

    m_drivers.clear();
    m_Driver = nullptr;

    // BEGIN MENSIA ACQUISITION DRIVERS
    // For future implementation
#if defined TARGET_OS_Windows && defined TARGET_HasMensiaAcquisitionDriver
    typedef int (*release_mensia_library_t)();
    release_mensia_library_t fpReleaseMensiaLibrary = release_mensia_library_t(GetProcAddress(HINSTANCE(m_libMensia), "releaseAcquisitionLibrary"));
    //  fpReleaseMensiaLibrary();
#endif
    // END MENSIA ACQUISITION DRIVERS

    delete m_thread;
    m_thread = nullptr;

    delete m_acquisitionServerThread;
    m_acquisitionServerThread = nullptr;

    delete m_acquisitionServer;
    m_acquisitionServer = nullptr;
}

//___________________________________________________________________//
//                                                                   //

bool CAcquisitionServerGUI::initialize()
{
    m_builder = gtk_builder_new(); // glade_xml_new(OVAS_GUI_File, nullptr, nullptr);
    gtk_builder_add_from_file(m_builder, OVAS_GUI_File, nullptr);

    // Connects custom GTK signals

    // Note: Seems the signals below have to be "clicked", not "pressed", or the underlined keyboard shortcuts
    // of gtk stock items that can be activated with alt key ("mnemonics") do not work.
    g_signal_connect(gtk_builder_get_object(m_builder, "button_preference"), "clicked", G_CALLBACK(PreferencePressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_configure"), "clicked", G_CALLBACK(ConfigurePressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "togglebutton_connect"), "toggled", G_CALLBACK(ConnectToggledCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_play"), "clicked", G_CALLBACK(StartPressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_stop"), "clicked", G_CALLBACK(StopPressedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "combobox_driver"), "changed", G_CALLBACK(DriverChangedCB), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "combobox_sample_count_per_sent_block"), "changed", G_CALLBACK(SampleCountPerSentBlockChangedCB), this);
    gtk_builder_connect_signals(m_builder, nullptr);

    GtkComboBox* comboBoxDriver = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_driver"));

    enum { Resource_StringMarkup };

    // Prepares drivers combo box

    gtk_combo_box_set_model(comboBoxDriver, nullptr);

    GtkCellRenderer* cellRendererName = gtk_cell_renderer_text_new();

    gtk_cell_layout_clear(GTK_CELL_LAYOUT(comboBoxDriver));
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(comboBoxDriver), cellRendererName, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(comboBoxDriver), cellRendererName, "markup", Resource_StringMarkup);

    GtkTreeStore* driverTreeStore = gtk_tree_store_new(1, G_TYPE_STRING);
    gtk_combo_box_set_model(comboBoxDriver, GTK_TREE_MODEL(driverTreeStore));

    std::string defaultDriverName = m_kernelCtx.getConfigurationManager().expand("${AcquisitionServer_DefaultDriver}").toASCIIString();
    transform(defaultDriverName.begin(), defaultDriverName.end(), defaultDriverName.begin(), to_lower<std::string::value_type>);
    for (size_t i = 0; i < m_drivers.size(); ++i) // n.b. dont use iterator here as we need a numeric index for gtk later anyway
    {
        IDriver* driver = m_drivers[i];

        GtkTreeIter it;
        gtk_tree_store_append(driverTreeStore, &it, nullptr);

        std::string name = driver->getName();

        const bool unstable   = driver->isFlagSet(EDriverFlag::IsUnstable);
        const bool deprecated = driver->isFlagSet(EDriverFlag::IsDeprecated);

        const std::string toDisplay = std::string((unstable || deprecated) ? "<span foreground=\"#6f6f6f\">" : "") + name
                                      + ((unstable || deprecated) ? "</span>" : "")
                                      + (unstable ? " <span size=\"smaller\" style=\"italic\">(<span foreground=\"#202060\">unstable</span>)</span>" : "")
                                      + (deprecated ? " <span size=\"smaller\" style=\"italic\">(<span foreground=\"#602020\">deprecated</span>)</span>" : "");

        gtk_tree_store_set(driverTreeStore, &it, Resource_StringMarkup, toDisplay.c_str(), -1);

        transform(name.begin(), name.end(), name.begin(), to_lower<std::string::value_type>);
        if (defaultDriverName == name) { gtk_combo_box_set_active(comboBoxDriver, gint(i)); }
    }
    if (gtk_combo_box_get_active(comboBoxDriver) == -1) { gtk_combo_box_set_active(comboBoxDriver, 0); }

    // Prepares sample count per buffer combo box

    bool found                         = false;
    const std::string nSamplePerBuffer = m_kernelCtx.getConfigurationManager().expand("${AcquisitionServer_DefaultSampleCountPerBuffer}").toASCIIString();
    GtkComboBox* boxNSamplePerBuffer   = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sample_count_per_sent_block"));
    for (int i = 0; ; ++i) {
        gtk_combo_box_set_active(boxNSamplePerBuffer, i);
        if (gtk_combo_box_get_active(boxNSamplePerBuffer) == -1) { break; }
        if (nSamplePerBuffer == gtk_combo_box_get_active_text(boxNSamplePerBuffer)) {
            found = true;
            break;
        }
    }
    if (!found) {
        if (nSamplePerBuffer != "-1") { gtk_combo_box_prepend_text(boxNSamplePerBuffer, nSamplePerBuffer.c_str()); }
        gtk_combo_box_set_active(boxNSamplePerBuffer, 0);
    }

    // Prepares default connection port

    GtkSpinButton* buttonConnectionPort = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_connection_port"));
    gtk_spin_button_update(buttonConnectionPort);
    const uint64_t connectionPort = m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DefaultConnectionPort}", 1024);
    gtk_spin_button_set_value(buttonConnectionPort, gdouble(connectionPort));

    // Optionnally autostarts

    if (m_kernelCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_AutoStart}", false)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "togglebutton_connect")), TRUE);
        gtk_button_pressed(GTK_BUTTON(gtk_builder_get_object(m_builder, "button_play")));
    }


    // Shows main window
    if (!m_kernelCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_NoGUI}", false)) {
        GtkWidget* mainWindow = GTK_WIDGET(gtk_builder_get_object(m_builder, "openvibe-acquisition-server"));

        std::string title(gtk_window_get_title(GTK_WINDOW(mainWindow)));
#if defined(TARGET_ARCHITECTURE_x64)
        title += " (64bit)";
#else
        title += " (32bit)";
#endif
        gtk_window_set_title(GTK_WINDOW(mainWindow), title.c_str());

        gtk_widget_show(mainWindow);
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

uint32_t CAcquisitionServerGUI::getTCPPort() const
{
    GtkSpinButton* button = GTK_SPIN_BUTTON(gtk_builder_get_object(m_builder, "spinbutton_connection_port"));
    gtk_spin_button_update(button);
    return gtk_spin_button_get_value_as_int(button);
}

void CAcquisitionServerGUI::setClientText(const char* sClientText) const
{
    gtk_label_set_label(GTK_LABEL(gtk_builder_get_object(m_builder, "label_connected_host_count")), sClientText);
}

void CAcquisitionServerGUI::setStateText(const char* sStateText) const
{
    gtk_label_set_label(GTK_LABEL(gtk_builder_get_object(m_builder, "label_status")), sStateText);
}

void CAcquisitionServerGUI::setDriftMs(const double ms) const
{
    const uint64_t driftTolerance = m_acquisitionServer->m_DriftCorrection.getDriftToleranceDurationMs();
    double driftRatio             = ms / double(driftTolerance);
    bool driftWarning             = false;
    char label[1024];

#ifdef TIMINGDEBUG
    std::cout << "GUI drift " << ms << " rat " << driftRatio << "\n";
#endif

    if (driftRatio < -1) {
        driftRatio   = -1;
        driftWarning = true;
    }

    if (driftRatio > 1) {
        driftRatio   = 1;
        driftWarning = true;
    }

    if (driftRatio < 0) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gtk_builder_get_object(m_builder, "progressbar_drift_1")), -driftRatio);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gtk_builder_get_object(m_builder, "progressbar_drift_2")), 0);
    }
    else {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gtk_builder_get_object(m_builder, "progressbar_drift_1")), 0);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gtk_builder_get_object(m_builder, "progressbar_drift_2")), driftRatio);
    }

    if (driftWarning) {
        sprintf(label, "<b>Device drift is too high</b> : %3.2lf ms\n<small>late &lt;-- (tolerance is set to %u ms) --&gt; early</small>", ms,
                uint32_t(driftTolerance));
    }
    else { sprintf(label, "Device drift : %3.2lf ms\n<small>late &lt;-- (tolerance is set to %u ms) --&gt; early</small>", ms, uint32_t(driftTolerance)); }
    gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(m_builder, "label_drift")), label);
}

void CAcquisitionServerGUI::setImpedance(const uint32_t index, const double impedance)
{
    if (m_impedanceWindow) {
        if (impedance >= 0) {
            //double fraction = (impedance*.001/20); With fixed impedance limit, 20kOhm max / 25%=5kOhm to be good
            double fraction = (impedance / (m_kernelCtx.getConfigurationManager().expandAsFloat("${AcquisitionServer_DefaultImpedanceLimit}", 5000) * 4));
            if (fraction > 1) { fraction = 1; }

            char msg[1024];
            char label[1024];
            char sImpedance[1024];
            char status[1024];

            if (strcmp(m_headerCopy.getChannelName(index), "")) { strcpy(label, m_headerCopy.getChannelName(index)); }
            else { sprintf(label, "Channel %i", index + 1); }

            if (fraction == 1) { sprintf(sImpedance, "Too high !"); }
            else { sprintf(sImpedance, "%.2f kOhm", impedance * .001); }

            sprintf(status, "%s", fraction < .25 ? "Good !" : "Bad...");
            sprintf(msg, "%s\n%s\n\n%s", label, sImpedance, status);

            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_levelMesures[index]), fraction);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_levelMesures[index]), msg);
        }
        else {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_levelMesures[index]), 0);
            if (impedance == OVAS_Impedance_Unknown) { gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_levelMesures[index]), "Measuring..."); }
            else if (impedance == OVAS_Impedance_NotAvailable) { gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_levelMesures[index]), "n/a"); }
            else { gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_levelMesures[index]), "Unknown"); }
        }
    }
}

void CAcquisitionServerGUI::disconnect() const
{
    GtkToggleButton* button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "togglebutton_connect"));
    if (gtk_toggle_button_get_active(button)) { gtk_toggle_button_set_active(button, false); }
}

//___________________________________________________________________//
//                                                                   //

void CAcquisitionServerGUI::buttonConnectToggledCB(GtkToggleButton* button)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonConnectToggledCB\n";

    if (gtk_toggle_button_get_active(button)) {
        if (m_nSamplePerBuffer != uint32_t(-1) && m_acquisitionServerThread->connect()) {
            // Impedance window creation
            {
                const uint64_t nCol    = m_kernelCtx.getConfigurationManager().expandAsInteger("${AcquisitionServer_CheckImpedance_ColumnCount}", 8);
                const size_t nRow      = size_t(m_headerCopy.getChannelCount() / nCol);
                const size_t lastCount = size_t(m_headerCopy.getChannelCount() % nCol);

                GtkWidget* table = gtk_table_new(gint(nRow + (lastCount ? 1 : 0)), gint((nRow ? nCol : lastCount)), true);

                for (size_t i = 0; i < m_headerCopy.getChannelCount(); ++i) {
                    const uint32_t j       = uint32_t(i / nCol);
                    const uint32_t k       = uint32_t(i % nCol);
                    GtkWidget* progressBar = gtk_progress_bar_new();
                    gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(progressBar), GTK_PROGRESS_BOTTOM_TO_TOP);
                    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0);
                    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "n/a");
                    gtk_table_attach_defaults(GTK_TABLE(table), progressBar, k, k + 1, j, j + 1);
                    m_levelMesures.push_back(progressBar);
                }

                m_impedanceWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                gtk_window_set_title(GTK_WINDOW(m_impedanceWindow), "Impedance check");
                gtk_container_add(GTK_CONTAINER(m_impedanceWindow), table);
                if (m_acquisitionServer->isImpedanceCheckRequested()) { gtk_widget_show_all(m_impedanceWindow); }
            }

            gtk_button_set_label(GTK_BUTTON(button), "gtk-disconnect");

            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_configure")), false);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_preference")), false);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_play")), true);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_stop")), false);

            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_connection_port")), false);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_sample_count_per_sent_block")), false);
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_driver")), false);
        }
        else {
            if (m_nSamplePerBuffer == uint32_t(-1)) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Sample count per sent block is invalid.\n"; }

            gtk_toggle_button_set_active(button, false);
        }
    }
    else {
        m_acquisitionServerThread->disconnect();

        if (m_impedanceWindow) {
            gtk_widget_destroy(m_impedanceWindow);
            m_levelMesures.clear();
            m_impedanceWindow = nullptr;
        }

        gtk_button_set_label(GTK_BUTTON(button), "gtk-connect");

        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_configure")), m_Driver->isConfigurable());
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_preference")), true);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_play")), false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_stop")), false);

        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_connection_port")), true);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_sample_count_per_sent_block")), true);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "combobox_driver")), true);

        setClientText("");
    }
}

void CAcquisitionServerGUI::buttonStartPressedCB(GtkButton* /*button*/)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonStartPressedCB\n";

    if (m_acquisitionServerThread->start()) {
        if (m_impedanceWindow) { gtk_widget_hide(m_impedanceWindow); }

        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_play")), false);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_stop")), true);

        setStateText("Starting...");
    }
    else {
        setStateText("Start failed !");
        setClientText("");
    }
}

void CAcquisitionServerGUI::buttonStopPressedCB(GtkButton* /*button*/)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonStopPressedCB\n";

    if (m_acquisitionServerThread->stop()) {
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_play")), true);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_stop")), false);
    }
    else {
        setStateText("Stop failed !");
        setClientText("");
    }
}

void CAcquisitionServerGUI::buttonPreferencePressedCB(GtkButton* /*button*/)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonPreferencePressedCB\n";

    GtkBuilder* builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, OVAS_GUI_File, nullptr);
    GtkDialog* dialog                  = GTK_DIALOG(gtk_builder_get_object(builder, "openvibe-acquisition-server-configuration"));
    GtkComboBox* driftCorrectionPolicy = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combobox_drift_correction"));
    GtkComboBox* naNReplacementPolicy  = GTK_COMBO_BOX(gtk_builder_get_object(builder, "combobox_nan_replacement"));
    GtkSpinButton* driftTolerance      = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbutton_drift_tolerance"));
    GtkSpinButton* jitterMeasureCount  = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbutton_jitter_measure_count"));
    GtkSpinButton* overSamplingFactor  = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbutton_oversampling_factor"));
    GtkToggleButton* channelSelection  = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "checkbutton_channel_selection"));

    gtk_combo_box_set_active(driftCorrectionPolicy, int(m_acquisitionServer->m_DriftCorrection.getDriftCorrectionPolicy()));
    gtk_spin_button_set_value(driftTolerance, gdouble(m_acquisitionServer->m_DriftCorrection.getDriftToleranceDurationMs()));
    gtk_spin_button_set_value(jitterMeasureCount, gdouble(m_acquisitionServer->m_DriftCorrection.getJitterEstimationCountForDrift()));
    gtk_spin_button_set_value(overSamplingFactor, gdouble(m_acquisitionServer->getOversamplingFactor()));
    gtk_toggle_button_set_active(channelSelection, m_acquisitionServer->isChannelSelectionRequested() ? TRUE : FALSE);
    gtk_combo_box_set_active(naNReplacementPolicy, int(m_acquisitionServer->getNaNReplacementPolicy()));

    // Load the settings for the plugins

    GtkTable* settingsTable = GTK_TABLE(gtk_builder_get_object(builder, "table-pluginsettings"));

    gtk_table_resize(settingsTable, guint(m_PluginsProperties.size()), 2);

    for (size_t i = 0; i < m_PluginsProperties.size(); ++i) {
        Property* property = m_PluginsProperties[i].m_Property;

        // Create the setting controller widget
        GtkWidget* settingControl;

        if (const TypedProperty<bool>* rb = dynamic_cast<const TypedProperty<bool>*>(property)) {
            // std::cout << "bool\n";
            settingControl = gtk_check_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settingControl), *(rb->getData()));
        }
        else if (const TypedProperty<CString>* rcs = dynamic_cast<const TypedProperty<CString>*>(property)) {
            // std::cout << "string\n";
            settingControl = gtk_entry_new();
            gtk_entry_append_text(GTK_ENTRY(settingControl), rcs->getData()->toASCIIString());
        }
        else if (const TypedProperty<std::string>* rs = dynamic_cast<const TypedProperty<std::string>*>(property)) {
            // std::cout << "string\n";
            settingControl = gtk_entry_new();
            gtk_entry_append_text(GTK_ENTRY(settingControl), rs->getData()->c_str());
        }
        else if (const TypedProperty<uint32_t>* ru32 = dynamic_cast<const TypedProperty<uint32_t>*>(property)) {
            // std::cout << "uinteger\n";
            settingControl = gtk_spin_button_new_with_range(gdouble(std::numeric_limits<uint32_t>::min()), gdouble(std::numeric_limits<uint32_t>::max()), 1.0);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(settingControl), gdouble(*(ru32->getData())));
        }
        else if (const TypedProperty<uint64_t>* ru64 = dynamic_cast<const TypedProperty<uint64_t>*>(property)) {
            // std::cout << "uinteger\n";
            settingControl = gtk_spin_button_new_with_range(gdouble(std::numeric_limits<uint64_t>::min()), gdouble(std::numeric_limits<uint64_t>::max()), 1.0);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(settingControl), gdouble(*(ru64->getData())));
        }
        else if (const TypedProperty<int64_t>* ri = dynamic_cast<const TypedProperty<int64_t>*>(property)) {
            // std::cout << "integer\n";
            settingControl = gtk_spin_button_new_with_range(gdouble(std::numeric_limits<int64_t>::min()), gdouble(std::numeric_limits<int64_t>::max()), 1.0);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(settingControl), gdouble(*(ri->getData())));
        }
        else if (const TypedProperty<float>* rf = dynamic_cast<const TypedProperty<float>*>(property)) {
            // std::cout << "float\n";
            settingControl = gtk_spin_button_new_with_range(gdouble(std::numeric_limits<float>::min()), gdouble(std::numeric_limits<float>::max()), 1.0);
            gtk_spin_button_set_digits(GTK_SPIN_BUTTON(settingControl), 5);
            gtk_spin_button_set_increments(GTK_SPIN_BUTTON(settingControl), 0.1, 1.0);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(settingControl), gdouble(*(rf->getData())));
        }
        else {
            // std::cout << "unknown\n";
            settingControl = gtk_label_new("Undefined Type");
        }

        if (settingControl) {
            // Create label
            GtkWidget* settingLabel = gtk_label_new(property->getName().toASCIIString());

            // align to left
            gtk_misc_set_alignment(GTK_MISC(settingLabel), 0.0, 0.0);

            // insert the settings into the table
            gtk_table_attach(settingsTable, settingLabel, 0, 1, guint(i), guint(i + 1), GTK_FILL, GTK_SHRINK, 2, 0);
            gtk_table_attach_defaults(settingsTable, settingControl, 1, 2, guint(i), guint(i + 1));

            m_PluginsProperties[i].m_Widget = settingControl;
            gtk_widget_show(settingLabel);
            gtk_widget_show(settingControl);
        }
    }

    const gint responseId = gtk_dialog_run(dialog);
    switch (responseId) {
        case GTK_RESPONSE_APPLY:
        case GTK_RESPONSE_OK:
        case GTK_RESPONSE_YES:
            m_acquisitionServer->setNaNReplacementPolicy(ENaNReplacementPolicy(gtk_combo_box_get_active(naNReplacementPolicy)));
            m_acquisitionServer->m_DriftCorrection.setDriftCorrectionPolicy(EDriftCorrectionPolicies(gtk_combo_box_get_active(driftCorrectionPolicy)));
            m_acquisitionServer->m_DriftCorrection.setDriftToleranceDurationMs(gtk_spin_button_get_value_as_int(driftTolerance));
            m_acquisitionServer->m_DriftCorrection.setJitterEstimationCountForDrift(gtk_spin_button_get_value_as_int(jitterMeasureCount));
            m_acquisitionServer->setOversamplingFactor(gtk_spin_button_get_value_as_int(overSamplingFactor));
            m_acquisitionServer->setChannelSelectionRequest(gtk_toggle_button_get_active(channelSelection) ? true : false);

        // Side-effect: Update the tolerance ms
            setDriftMs(0);

            for (size_t index = 0; index < m_PluginsProperties.size(); ++index) {
                Property* property = m_PluginsProperties[index].m_Property;

                if (TypedProperty<bool>* rb = dynamic_cast<TypedProperty<bool>*>(property)) {
                    // std::cout << "bool\n";
                    bool tmp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_PluginsProperties[index].m_Widget)) ? true : false;
                    rb->replaceData(tmp);
                }
                else if (TypedProperty<CString>* rcs = dynamic_cast<TypedProperty<CString>*>(property)) {
                    CString tmp = CString(gtk_entry_get_text(GTK_ENTRY(m_PluginsProperties[index].m_Widget)));
                    // std::cout << "string: " << tmp.toASCIIString() << "\n";
                    rcs->replaceData(tmp);
                }
                else if (TypedProperty<std::string>* rs = dynamic_cast<TypedProperty<std::string>*>(property)) {
                    std::string tmp = std::string(gtk_entry_get_text(GTK_ENTRY(m_PluginsProperties[index].m_Widget)));
                    // std::cout << "string: " << tmp.toASCIIString() << "\n";
                    rs->replaceData(tmp);
                }
                else if (TypedProperty<int64_t>* ri = dynamic_cast<TypedProperty<int64_t>*>(property)) {
                    // std::cout << "integer\n";
                    GtkSpinButton* spinButton = GTK_SPIN_BUTTON(m_PluginsProperties[index].m_Widget);
                    gtk_spin_button_update(spinButton);
                    int64_t tmp = int64_t(gtk_spin_button_get_value(spinButton));
                    ri->replaceData(tmp);
                }
                else if (TypedProperty<uint32_t>* ru32 = dynamic_cast<TypedProperty<uint32_t>*>(property)) {
                    // std::cout << "uinteger\n";
                    GtkSpinButton* spinButton = GTK_SPIN_BUTTON(m_PluginsProperties[index].m_Widget);
                    gtk_spin_button_update(spinButton);
                    uint32_t tmp = uint32_t(gtk_spin_button_get_value(spinButton));
                    ru32->replaceData(tmp);
                }
                else if (TypedProperty<uint64_t>* ru64 = dynamic_cast<TypedProperty<uint64_t>*>(property)) {
                    // std::cout << "uinteger\n";
                    GtkSpinButton* spinButton = GTK_SPIN_BUTTON(m_PluginsProperties[index].m_Widget);
                    gtk_spin_button_update(spinButton);
                    uint64_t tmp = uint64_t(gtk_spin_button_get_value(spinButton));
                    ru64->replaceData(tmp);
                }
                else if (TypedProperty<float>* rf = dynamic_cast<TypedProperty<float>*>(property)) {
                    // std::cout << "float\n";
                    GtkSpinButton* spinButton = GTK_SPIN_BUTTON(m_PluginsProperties[index].m_Widget);
                    gtk_spin_button_update(spinButton);
                    float tmp = float(gtk_spin_button_get_value(spinButton));
                    rf->replaceData(tmp);
                }
                else { }    // std::cout << "unknown\n";
            }


            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_NO: break;
        default: break;
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_object_unref(builder);
}

void CAcquisitionServerGUI::buttonConfigurePressedCB(GtkButton* /*button*/)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonConfigurePressedCB\n";
    if (m_Driver->isConfigurable()) { m_Driver->configure(); }
}

void CAcquisitionServerGUI::comboBoxDriverChanged(GtkComboBox* box)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "comboBoxDriverChanged\n";
    m_Driver = m_drivers[gtk_combo_box_get_active(box)];
    gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(m_builder, "button_configure")), m_Driver->isConfigurable());
}

void CAcquisitionServerGUI::comboBoxSampleCountPerSentBlockChanged(GtkComboBox* /*box*/)
{
    int nSamplePerSentBlock = 0;
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "comboBoxSampleCountPerSentBlockChanged\n";
    if (sscanf(gtk_combo_box_get_active_text(GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_sample_count_per_sent_block"))), "%i",
               &nSamplePerSentBlock) == 1 && nSamplePerSentBlock > 0) { m_nSamplePerBuffer = uint32_t(nSamplePerSentBlock); }
    else { m_nSamplePerBuffer = uint32_t(-1); }
}

void CAcquisitionServerGUI::registerPlugin(IAcquisitionServerPlugin* plugin) const
{
    if (m_acquisitionServer != nullptr) { m_acquisitionServer->m_Plugins.push_back(plugin); }
}

/**
  * \brief This function scans all registered plugins for settings.
  *
  * All of the plugins are inserted into a vector containing the pointer to the actual settings structure
  * along with a unique name for settings.
  */
void CAcquisitionServerGUI::scanPluginSettings()
{
    std::vector<IAcquisitionServerPlugin*> plugins = m_acquisitionServer->getPlugins();

    m_PluginsProperties.clear();

    for (auto itp = plugins.begin(); itp != plugins.end(); ++itp) {
        IAcquisitionServerPlugin* plugin = dynamic_cast<IAcquisitionServerPlugin*>(*itp);

        SettingsHelper& tmp                       = plugin->getSettingsHelper();
        const std::map<CString, Property*>& props = tmp.getAllProperties();

        for (auto it = props.begin(); it != props.end(); ++it) { m_PluginsProperties.push_back(PropertyAndWidget(it->second, nullptr)); }
    }
}

void CAcquisitionServerGUI::savePluginSettings() const
{
    std::vector<IAcquisitionServerPlugin*> plugins = m_acquisitionServer->getPlugins();

    for (auto itp = plugins.begin(); itp != plugins.end(); ++itp) {
        IAcquisitionServerPlugin* plugin = dynamic_cast<IAcquisitionServerPlugin*>(*itp);
        SettingsHelper& tmp              = plugin->getSettingsHelper();
        tmp.save();
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
