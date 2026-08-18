#include "CDriverSkeletonGenerator.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

#include <glib/gstdio.h>
#include <cstdio>

#include <boost/regex.hpp>

namespace OpenViBE {
namespace SkeletonGenerator {
//-----------------------------------------------------------------------
static void CheckCB(GtkButton* /*button*/, void* data) { static_cast<CDriverSkeletonGenerator*>(data)->ButtonCheckCB(); }
static void TooltipCB(GtkButton* button, void* data) { static_cast<CDriverSkeletonGenerator*>(data)->ButtonTooltipCB(button); }
static void OkCB(GtkButton* /*button*/, void* data) { static_cast<CDriverSkeletonGenerator*>(data)->ButtonOkCB(); }

static void ExitCB(GtkButton* /*button*/, void* data)
{
    static_cast<CDriverSkeletonGenerator*>(data)->ButtonExitCB();
    gtk_exit(0);
}

//-----------------------------------------------------------------------
void CDriverSkeletonGenerator::ButtonExitCB()
{
    getCommonParameters();
    getCurrentParameters();
    if (!cleanConfigurationFile(m_configFile)) { return; }
    saveCommonParameters(m_configFile);
    save(m_configFile);

    getLogManager() << Kernel::LogLevel_Info << "All entries saved in [" << m_configFile << "]. Exiting.\n";
}

void CDriverSkeletonGenerator::ButtonCheckCB()
{
    //Author and Company
    getCommonParameters();
    getCurrentParameters();

    getLogManager() << Kernel::LogLevel_Info << "Checking values... \n";

    bool success = true;

    std::stringstream ss;
    ss << "----- STATUS -----\n";

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    //::GtkWidget * driverName = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_driver_name"));
    //m_DriverName = gtk_entry_get_text(GTK_ENTRY(driverName));
    const boost::regex regExpDriverName("([a-z]|[A-Z]|[0-9])+([a-z]|[A-Z]|[0-9]|[ \t\r\n]|[\\.-_\\(\\)])*", boost::regex::perl);
    if (regex_match(std::string(m_DriverName), regExpDriverName) == false) {
        OV_WARNING_K("-- Driver Name: INVALID");
        success = false;
        ss << "[FAILED] Invalid driver name. Please use only characters (lower or uppercase) and numbers (blanck allowed).\n";
    }
    else {
        getLogManager() << Kernel::LogLevel_Info << "-- Driver Name: VALID (" << m_DriverName << ")\n";
        ss << "[   OK   ] Valid driver name.\n";
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    //::GtkWidget * className = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_class_name"));
    //m_ClassName = gtk_entry_get_text(GTK_ENTRY(className));
    const boost::regex regExpClassName("([a-z]|[A-Z]|[0-9])+", boost::regex::perl);
    if (regex_match(std::string(m_ClassName), regExpClassName) == false) {
        OV_WARNING_K("-- Class Name: INVALID");
        success = false;
        ss << "[FAILED] Invalid class name. Please use only characters (lower or uppercase) and numbers  (no blanck allowed).\n";
    }
    else {
        getLogManager() << Kernel::LogLevel_Info << "-- Class Name: VALID (" << m_ClassName << ")\n";
        ss << "[   OK   ] Valid class name.\n";
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    GtkWidget* spinMinChannel = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_min_channel"));
    //m_MinChannel            = std::to_string(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinMinChannel)))).c_str();
    GtkWidget* spinMaxChannel = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_max_channel"));
    //m_MaxChannel            = std::to_string(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinMaxChannel)))).c_str();
    if (gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinMinChannel)) > gtk_spin_button_get_value(GTK_SPIN_BUTTON(spinMaxChannel))) {
        OV_WARNING_K(Kernel::LogLevel_Warning << "-- Channels: INVALID");
        success = false;
        ss << "[FAILED] Invalid channel count. Be sure that Min <= Max.\n";
    }
    else {
        getLogManager() << Kernel::LogLevel_Info << "-- Channels: VALID (" << m_MinChannel << "/" << m_MaxChannel << ")\n";
        ss << "[   OK   ] Valid channel count.\n";
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    /*::GtkWidget * sf = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_sampling_frequencies"));
    CString samplings = gtk_entry_get_text(GTK_ENTRY(sf));
    */
    const boost::regex regExpSamplings("(([1-9][0-9]*);)*([1-9][0-9]*)", boost::regex::perl);
    if (regex_match(m_Samplings, regExpSamplings) == false) {
        OV_WARNING_K("-- Sampling frequencies: INVALID");
        success = false;
        ss << "[FAILED] Invalid sampling frequencies. Please use only whole numbers separated with ';' (no blanck allowed).\n";
    }
    else {
        // Maximum 16 frequencies
        size_t nSampling = 0;
        size_t freq;
        std::stringstream tmp(m_Samplings);
        m_SamplingSeparate.clear();
        while (tmp >> freq && nSampling < 16) {
            m_SamplingSeparate.push_back(std::to_string(freq));
            if (tmp.peek() == ';') { tmp.ignore(); }
            nSampling++;
        }

        getLogManager() << Kernel::LogLevel_Info << "-- Sampling frequencies: VALID\n";
        for (const auto& s : m_SamplingSeparate) { std::cout << "- " << s << " Hz\n"; }

        ss << "[   OK   ] " << nSampling << " valid sampling frequencie(s).\n";
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    /*::GtkWidget * fileChooser = GTK_WIDGET(gtk_builder_get_object(m_builder, "filechooserbutton_target_directory"));
    char * directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser));
    m_directory = CString(directory);
    g_free(directory);*/

#ifdef TARGET_OS_Windows
    const std::string space("%20");
    if (m_directory.rfind(space) != std::string::npos) {
        ss << "[FAILED] Invalid destination folder :" << m_directory << ".\n";
        getLogManager() << Kernel::LogLevel_Error << "Invalid destination folder :" << m_directory << ".\n";
        success = false;
    }
    else
#endif
    {
        getLogManager() << Kernel::LogLevel_Info << "-- Target directory: " << m_directory << "\n";
        ss << "[   OK   ] Valid target directory: " << m_directory << "\n";
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    GtkWidget* textview   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-tooltips-textview"));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    if (success) {
        ss << "----- SUCCESS -----\nPress OK to generate the files. If you want to modify your choice(s), please press the \"Check\" button again.";
        GtkWidget* buttonOk = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-ok-button"));
        gtk_widget_set_sensitive(buttonOk, true);
    }
    else {
        ss << "----- PROCESS FAILED -----\nModify your choices and press the \"Check\" button again.";
        GtkWidget* buttonOk = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-ok-button"));
        gtk_widget_set_sensitive(buttonOk, false);
    }

    gtk_text_buffer_set_text(buffer, ss.str().c_str(), -1);
}

void CDriverSkeletonGenerator::ButtonOkCB()
{
    getLogManager() << Kernel::LogLevel_Info << "Generating files... \n";
    std::string log = "Generating files...\n";

    bool success = true;

    const std::string date = getDate();

    // we construct the map of substitutions
    std::map<std::string, std::string> substitutions;
    substitutions["@@AuthorName@@"]  = m_author;
    substitutions["@@CompanyName@@"] = m_company;
    substitutions["@@Date@@"]        = date;
    substitutions["@@ClassName@@"]   = m_ClassName;
    substitutions["@@DriverName@@"]  = m_DriverName;
    substitutions["@@MinChannel@@"]  = m_MinChannel;
    substitutions["@@MaxChannel@@"]  = m_MaxChannel;
    substitutions["@@Sampling@@"]    = m_SamplingSeparate[0];

    GtkWidget* textview   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-tooltips-textview"));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));


    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // driver.hpp
    std::string dst     = m_directory + "/CDriver" + m_ClassName + ".hpp";
    std::string tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/driver.hpp-skeleton").toASCIIString();

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, log.c_str(), -1);
        success = false;
    }

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // driver.cpp
    dst     = m_directory + "/CDriver" + m_ClassName + ".cpp";
    tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/driver.cpp-skeleton").toASCIIString();

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, log.c_str(), -1);
        success = false;
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // config.hpp
    dst     = m_directory + "/CConfiguration" + m_ClassName + ".hpp";
    tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/configuration.hpp-skeleton").toASCIIString();

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, log.c_str(), -1);
        success = false;
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // config.cpp
    dst     = m_directory + "/CConfiguration" + m_ClassName + ".cpp";
    tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/configuration.cpp-skeleton").toASCIIString();

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, log.c_str(), -1);
        success = false;
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // interface.ui
    dst     = m_directory + "/interface-" + m_ClassName + ".ui";
    tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/interface.ui-skeleton").toASCIIString();

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, log.c_str(), -1);
        success = false;
    }
    // the following substitution is done in a .ui file, and not in a cpp file. 
    // The SED primitive immplemented do not cover that case, and some typo problem happen with the character "
    const std::string pattern("@@SamplingFrequencyList@@");
    std::string substitute;
    for (auto it = m_SamplingSeparate.begin(); it != m_SamplingSeparate.end();) {
        substitute += (*it++);
        if (it != m_SamplingSeparate.end()) { substitute += "<\\/col><\\/row><row><col id=\\\"0\\\" translatable=\\\"yes\\\">"; }
    }
    success &= regexReplace(dst, pattern, substitute, "");

    //-------------------------------------------------------------------------------------------------------------------------------------------//
    // readme-driver.txt
    dst     = m_directory + "/README.txt";
    tmplate = m_kernelCtx.getConfigurationManager().expand("${Path_Data}/applications/skeleton-generator/readme-driver.txt-skeleton");

    if (!this->generate(tmplate, dst, substitutions, log)) {
        gtk_text_buffer_set_text(buffer, log.c_str(), -1);
        success = false;
    }
    //-------------------------------------------------------------------------------------------------------------------------------------------//

    if (success) {
        success &= cleanConfigurationFile(m_configFile);
        success &= saveCommonParameters(m_configFile);
        success &= save(m_configFile);
    }

    if (!success) {
        log += "Generation process did not completly succeed. Some files may have not been produced.\n";
        OV_WARNING_K("Generation process did not completly succeed. Some files may have not been produced.");
    }
    else {
        log += "Generation process successful. All information saved in [" + m_configFile + "]\nPlease read the file [README.txt] !\n";
        getLogManager() << Kernel::LogLevel_Info << "Generation process successful. All information saved in [" << m_configFile << "]\n";
    }

    // Launch the browser to display the produced files
    const std::string browser = m_kernelCtx.getConfigurationManager().expand("${Designer_WebBrowserCommand_${OperatingSystem}}").toASCIIString();

#ifdef TARGET_OS_Windows
    const std::string browserCmd = browser + " file:///" + m_directory; //otherwise the browser does not find the directory (problem with / and \ char)
#else
    const std::string browserCmd = browser + " \"" + m_directory + "\"";
#endif

    if (system(browserCmd.c_str())) { }

    gtk_text_buffer_set_text(buffer, log.c_str(), -1);
}

void CDriverSkeletonGenerator::ButtonTooltipCB(GtkButton* button)
{
    const EWidgetName widgetName = m_widgetNames[button];

    GtkWidget* textview   = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-tooltips-textview"));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    if (widgetName == EWidgetName::DriverName) {
        gtk_text_buffer_set_text(buffer,
                                 "Driver Name: \nThis name will be the one displayed in the Acquisition Server selection combobox.\nUsually, the driver is named according to the EEG device (with precisions such as a version number).\n------\nExample: OpenEEG Modular EEG (P2)\n\n\n",
                                 -1);
    }
    else if (widgetName == EWidgetName::ClassName) {
        gtk_text_buffer_set_text(buffer,
                                 "Class Name: \nThis name will be used to generate all source and GUI files.\nYou should choose a class name close to the device name (no blank allowed !).\n------\nExample: OpenEEGModularEEG will generate\n - CDriverOpenEEGModularEEG.hpp/.cpp, the driver skeleton \n - CConfigurationOpenEEGModularEEG.hpp/.cpp, the configuration class skeleton\n - interface-OpenEEG-ModularEEG.ui, the GUI description file",
                                 -1);
    }
    else if (widgetName == EWidgetName::ChannelCount) {
        gtk_text_buffer_set_text(buffer,
                                 "Channel count: \nEnter in the two fields the minimum and maximum number of channels the device is capable of.\nOf course you can still change it later in the source code.\n------\nExample: Min(1) Max(16)\n\n\n",
                                 -1);
    }
    else if (widgetName == EWidgetName::Sampling) {
        gtk_text_buffer_set_text(buffer,
                                 "Sampling frequencies: \nEnter in the text field the sampling frequencies your device is capable of.\nYou can specify a list of defined frequencies (value separator ';').\n------\nExample:\n\"128;256;512\" for three defined frequencies.\n\n",
                                 -1);
    }
    else if (widgetName == EWidgetName::Directory) {
        gtk_text_buffer_set_text(buffer,
                                 "Target directory: \nEnter the destination directory in which all files will be generated. \nAny existing files will be overwritten.\n------\nExample: ~/skeleton-generator/foobar-driver/\n\n\n",
                                 -1);
    }
    else { }
}

bool CDriverSkeletonGenerator::initialize()
{
    GtkWidget* driver = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-window"));

    // Buttons and signals
    GtkWidget* buttonCheck = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-check-button"));
    GtkWidget* buttonOk    = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-ok-button"));

    g_signal_connect(buttonCheck, "pressed", G_CALLBACK(CheckCB), this);
    g_signal_connect(buttonOk, "pressed", G_CALLBACK(OkCB), this);

    ////target directory
    //::GtkWidget * fileChooser = GTK_WIDGET(gtk_builder_get_object(m_builder, "filechooserbutton_target_directory"));
    //CString directory         = m_kernelContext.getConfigurationManager().expand("${SkeletonGenerator_TargetDirectory}");
    //if(!gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser), directory.c_str()))
    //{
    //  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser),"..");
    //}


    // Tooltips buttons and signal
    GtkButton* buttonDriverName   = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-driver-driver-name-tooltip-button"));
    GtkButton* buttonClassName    = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-driver-class-name-tooltip-button"));
    GtkButton* buttonChannelCount = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-driver-channel-count-tooltip-button"));
    GtkButton* buttonSampling     = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-driver-sampling-frequencies-tooltip-button"));
    GtkButton* buttonDirectory    = GTK_BUTTON(gtk_builder_get_object(m_builder, "sg-driver-target-directory-tooltip-button"));

    m_widgetNames[buttonDriverName]   = EWidgetName::DriverName;
    m_widgetNames[buttonClassName]    = EWidgetName::ClassName;
    m_widgetNames[buttonChannelCount] = EWidgetName::ChannelCount;
    m_widgetNames[buttonSampling]     = EWidgetName::Sampling;
    m_widgetNames[buttonDirectory]    = EWidgetName::Directory;

    g_signal_connect(buttonDriverName, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonClassName, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonChannelCount, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonSampling, "pressed", G_CALLBACK(TooltipCB), this);
    g_signal_connect(buttonDirectory, "pressed", G_CALLBACK(TooltipCB), this);

    //Close with X and "cancel" button
    g_signal_connect(G_OBJECT(driver), "delete_event", G_CALLBACK(gtk_exit), nullptr);
    GtkWidget* buttonCancel = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-cancel-button"));
    g_signal_connect(buttonCancel, "pressed", G_CALLBACK(ExitCB), this);

    //load everything from file
    load(m_configFile);

    gtk_widget_show_all(driver);

    return true;
}

bool CDriverSkeletonGenerator::save(const std::string& filename)
{
    std::ofstream file;
    file.open(filename, std::ios::app | std::ios::binary);
    OV_ERROR_UNLESS_KRF(file.is_open(), "Saving the driver entries in [" << m_configFile << "] failed !", Kernel::ErrorType::BadFileWrite);

    file << "# ----------------------DRIVER-------------------------\n";
    std::string directory(m_directory);
    for (auto it = directory.begin(); it < directory.end(); ++it) { if ((*it) == '\\') { directory.replace(it, it + 1, 1, '/'); } }

    file << "SkeletonGenerator_Driver_DriverName = " << m_DriverName << std::endl;
    file << "SkeletonGenerator_Driver_ClassName = " << m_ClassName << std::endl;
    file << "SkeletonGenerator_Driver_MinChannel = " << m_MinChannel << std::endl;
    file << "SkeletonGenerator_Driver_MaxChannel = " << m_MaxChannel << std::endl;
    file << "SkeletonGenerator_Driver_SamplingFrequencies = " << m_Samplings << std::endl;
    file << "SkeletonGenerator_Driver_TargetDirectory = " << directory << std::endl;
    file << "# -----------------------------------------------------" << std::endl;
    file.close();
    getLogManager() << Kernel::LogLevel_Info << "Driver entries saved in [" << m_configFile << "]\n";

    m_configFileLoaded = false;

    return true;
}

bool CDriverSkeletonGenerator::load(const std::string& filename)
{
    if (!m_configFileLoaded && !m_kernelCtx.getConfigurationManager().addConfigurationFromFile(filename.c_str())) {
        OV_WARNING_K("Driver: Configuration file [" << filename << "] could not be loaded. It will be automatically generated after first use.");
        return false;
    }

    GtkWidget* driverName = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_driver_name"));
    gtk_entry_set_text(GTK_ENTRY(driverName), m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Driver_DriverName}"));

    GtkWidget* className = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_class_name"));
    gtk_entry_set_text(GTK_ENTRY(className), m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Driver_ClassName}"));

    GtkWidget* minChannel = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_min_channel"));
    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(minChannel), double(m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Driver_MinChannel}")));

    GtkWidget* maxChannel = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_max_channel"));
    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(maxChannel), double(m_kernelCtx.getConfigurationManager().expandAsInteger("${SkeletonGenerator_Driver_MaxChannel}")));

    GtkWidget* sf = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_sampling_frequencies"));
    gtk_entry_set_text(GTK_ENTRY(sf), m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Driver_SamplingFrequencies}"));

#if defined TARGET_OS_Linux || defined TARGET_OS_Windows
    GtkWidget* fileChooser = GTK_WIDGET(gtk_builder_get_object(m_builder, "filechooserbutton_target_directory"));
#endif
    
    // if the user specified a target directory, it has full priority
    std::string directory = m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_TargetDirectory}").toASCIIString();
#ifdef TARGET_OS_Linux
    bool needFilePrefix = false;
#endif
    if (!directory.empty()) {
        getLogManager() << Kernel::LogLevel_Debug << "Target dir user  [" << directory << "]\n";
#ifdef TARGET_OS_Linux
        needFilePrefix = true;
#endif
    }
    else {
        //previous entry
        directory = m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Driver_TargetDirectory}");
        if (!directory.empty()) {
            getLogManager() << Kernel::LogLevel_Debug << "Target previous  [" << directory << "]\n";
#ifdef TARGET_OS_Linux
            needFilePrefix = true;
#endif
        }
        else {
            //default path = dist
            getLogManager() << Kernel::LogLevel_Debug << "Target default  [dist]\n";
#ifdef TARGET_OS_Linux
            directory = std::string(gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(fileChooser)));
            directory = directory + "/..";
#elif defined TARGET_OS_Windows
            directory = "..";
#endif
        }
    }
#ifdef TARGET_OS_Linux
    if(needFilePrefix) directory = "file://" + directory;
    gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(fileChooser), directory.c_str());
#elif defined TARGET_OS_Windows
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileChooser), directory.c_str());
#endif

    getLogManager() << Kernel::LogLevel_Info << "Driver entries from [" << filename << "] loaded.\n";
    return true;
}

void CDriverSkeletonGenerator::getCurrentParameters()
{
    GtkWidget* driverName  = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_driver_name"));
    m_DriverName           = gtk_entry_get_text(GTK_ENTRY(driverName));
    GtkWidget* className   = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_class_name"));
    m_ClassName            = gtk_entry_get_text(GTK_ENTRY(className));
    GtkWidget* minChannel  = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_min_channel"));
    m_MinChannel           = std::to_string(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(minChannel))));
    GtkWidget* maxChannel  = GTK_WIDGET(gtk_builder_get_object(m_builder, "spinbutton_max_channel"));
    m_MaxChannel           = std::to_string(size_t(gtk_spin_button_get_value(GTK_SPIN_BUTTON(maxChannel))));
    GtkWidget* sf          = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_sampling_frequencies"));
    m_Samplings            = gtk_entry_get_text(GTK_ENTRY(sf));
    GtkWidget* fileChooser = GTK_WIDGET(gtk_builder_get_object(m_builder, "filechooserbutton_target_directory"));
    char* directory        = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileChooser));
    m_directory            = CString(directory);
    g_free(directory);
}

}  // namespace SkeletonGenerator
}  // namespace OpenViBE
