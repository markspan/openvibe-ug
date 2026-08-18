#include "ovasCAcquisitionServerGUI.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <gtk/gtk.h>

#include <iostream>
#include <clocale> // std::setlocale

#include <system/ovCTime.h>

#if defined(TARGET_OS_Windows)
#include <Windows.h>
#include <MMSystem.h>
#endif

typedef struct SConfiguration
{
    SConfiguration() { }

    // <name, value>
    std::map<std::string, std::string> flag;
    std::map<std::string, std::string> tokens;
} configuration_t;

bool parse_arguments(int argc, char** argv, configuration_t& config)
{
    configuration_t configuration;

    std::vector<std::string> argValue;
    for (int i = 1; i < argc; ++i) { argValue.push_back(argv[i]); }
    argValue.push_back("");

    for (auto it = argValue.begin(); it != argValue.end(); ++it) {
        if (*it == "") {}
        else if (*it == "-c" || *it == "--config") {
            if (*++it == "") {
                std::cout << "Error: Switch --config needs an argument\n";
                return false;
            }
            configuration.flag["config"] = *it;
        }
        else if (*it == "-d" || *it == "--define") {
            if (*++it == "") {
                std::cout << "Error: Need two arguments after -d / --define.\n";
                return false;
            }

            // Were not using = as a separator for token/value, as on Windows its a problem passing = to the cmd interpreter 
            // which is used to launch the actual designer exe.
            const std::string& token = *it;
            if (*++it == "") {
                std::cout << "Error: Need two arguments after -d / --define.\n";
                return false;
            }

            const std::string& value = *it; // iterator will increment later

            configuration.tokens[token] = value;
        }
        else if (*it == "-k" || *it == "--kernel") {
            if (*++it == "") {
                std::cout << "Error: Switch --kernel needs an argument\n";
                return false;
            }
            configuration.flag["kernel"] = *it;
        }
        else if (*it == "-h" || *it == "--help") { return false; }
        else if (*it == "-v" || *it == "--version") {
#if defined(TARGET_OS_Windows)
            const std::string platform("Windows");
#elif defined(TARGET_OS_Linux)
            const std::string platform("Linux");
#else
            const std::string platform("Other");
#endif
#if defined(TARGET_ARCHITECTURE_x64)
            const std::string arch("64bit");
#else
            const std::string arch("32bit");
#endif
#if defined(TARGET_BUILDTYPE_Debug)
            const std::string buildType("Debug");
#elif defined(TARGET_BUILDTYPE_Release)
            const std::string buildType("Release");
#else
            const std::string buildType("Unknown");
#endif
            std::cout << OV_PROJECT_NAME << " Acquisition Server - Version " << OV_VERSION_MAJOR << "." << OV_VERSION_MINOR << "." << OV_VERSION_PATCH
                    << " (" << platform << " " << arch << " " << buildType << " build)" << std::endl;
            exit(0);
        }
        else {
            // The argument may be relevant to GTK, do not stop here
            std::cout << "Note: Unknown argument [" << *it << "], passing it on to gtk...\n";
        }
    }

    config = configuration;

    return true;
}


int main(int argc, char** argv)
{
    //___________________________________________________________________//
    //                                                                   //

    configuration_t config;
    if (!parse_arguments(argc, argv, config)) {
        std::cout << "Syntax : " << argv[0] << " [ switches ]\n";
        std::cout << "Possible switches :\n";
        std::cout << "  --config filename       : path to config file\n";
        std::cout << "  --define token value    : specify configuration token with a given value\n";
        std::cout << "  --help                  : displays this help message and exits\n";
        std::cout << "  --kernel filename       : path to openvibe kernel library\n";
        std::cout << "  --version               : prints version information and exits\n";
        return -1;
    }

#if defined(TARGET_OS_Windows)
    HANDLE process = GetCurrentProcess();

    // Some sources claim this is needed for accurate timing. Microsoft disagrees, so we do not use it. You can try, or try google. 
    //SetThreadAffinityMask(hProcess, threadMask);

    // Set the clock interval to 1ms (default on Win7: 15ms). This is needed to get under 15ms accurate sleeps,
    // and improves the precision of non-QPC clocks. Note that since boost 1.58, the sleeps no longer seem
    // to be 1ms accurate on Windows (as they seemed to be on 1.55), and sleep can oversleep even 10ms even with 
    // timeBeginPeriod(1) called. @todo in the future, make sure nothing relies on sleep accuracy in openvibe
    timeBeginPeriod(1);

    // Since AS is just sleeping when its not acquiring, a high priority should not be a problem. 
    // As a result of these calls, the server should have a 'normal' priority INSIDE the 'realtime' priority class.
    // However, unless you run AS with admin priviledges, Windows probably will truncate these priorities lower.
    // n.b. For correct timing, it may be preferable to set the priority here globally and not mess with it in the drivers;
    // any child threads should inherit this automagically.
    SetPriorityClass(process, REALTIME_PRIORITY_CLASS);     // The highest priority class
    SetThreadPriority(process, THREAD_PRIORITY_NORMAL);     // Even higher options: THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL

#else
    std::string interface_file = std::string(OpenViBE::Directories::getDataDir()) + "/applications/designer/themes/Adwaita/gtk-2.0/gtkrc";
    gchar * gtk_theme = const_cast<gchar *>(interface_file.c_str());
    gchar *rc_files[] = {gtk_theme, NULL};
    gtk_rc_set_default_files(rc_files);
#endif

    OpenViBE::CKernelLoader kernelLoader;

    std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;
    OpenViBE::CString error;

    OpenViBE::CString kernelFile = OpenViBE::Directories::getLib("kernel");

    if (config.flag.count("kernel")) { kernelFile = OpenViBE::CString(config.flag["kernel"].c_str()); }
    if (!kernelLoader.load(kernelFile, &error)) { std::cout << "[ FAILED ] Error loading kernel from [" << kernelFile << "]: " << error << "\n"; }
    else {
        std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
        OpenViBE::Kernel::IKernelDesc* kernelDesc = nullptr;
        kernelLoader.initialize();
        kernelLoader.getKernelDesc(kernelDesc);
        if (!kernelDesc) { std::cout << "[ FAILED ] No kernel descriptor" << std::endl; }
        else {
            std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << std::endl;


            OpenViBE::CString configFile = OpenViBE::CString(OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");
            if (config.flag.count("config")) { configFile = OpenViBE::CString(config.flag["config"].c_str()); }

            OpenViBE::Kernel::IKernelContext* kernelCtx = kernelDesc->createKernel("acquisition-server", configFile);
            if (!kernelCtx) { std::cout << "[ FAILED ] No kernel created by kernel descriptor" << std::endl; }
            else {
                kernelCtx->initialize();

                OpenViBE::Kernel::IConfigurationManager& configManager = kernelCtx->getConfigurationManager();

                // @FIXME CERT silent fail if missing file is provided
                configManager.addConfigurationFromFile(configManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf"));

                // User configuration mods
                configManager.addConfigurationFromFile(configManager.expand("${Path_UserData}/openvibe-acquisition-server.conf"));

                kernelCtx->getPluginManager().addPluginsFromFiles(configManager.expand("${AcquisitionServer_Plugins}"));

                for (auto itr = config.tokens.begin(); itr != config.tokens.end(); ++itr) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "Adding command line configuration token ["
                            << (*itr).first.c_str() << " = " << (*itr).second.c_str() << "]\n";
                    configManager.addOrReplaceConfigurationToken((*itr).first.c_str(), (*itr).second.c_str());
                }

                // Check the clock
                if (!System::Time::isClockSteady()) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Warning
                            << "The system does not seem to have a steady clock. This may affect the acquisition time precision.\n";
                }

                if (!gtk_init_check(&argc, &argv)) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Error
                            << "Unable to initialize GTK. Possibly the display could not be opened. Exiting.\n";

                    OpenViBE::Toolkit::uninitialize(*kernelCtx);
                    kernelDesc->releaseKernel(kernelCtx);

                    kernelLoader.uninitialize();
                    kernelLoader.unload();

#if defined(TARGET_OS_Windows)
                    timeEndPeriod(1);
#endif

                    return -2;
                }

                // We rely on this with 64bit/gtk 2.24, to roll back gtk_init() sometimes switching
                // the locale to one where ',' is needed instead of '.' for separators of floats, 
                // causing issues, for example getConfigurationManager.expandAsFloat("0.05") -> 0; 
                // due to implementation by std::stod().
                std::setlocale(LC_ALL, "C");

                // gtk_rc_parse(Directories::getDataDir() + "/applications/designer/interface.gtkrc");

#ifdef TARGET_OS_Linux
                // Replace the gtk signal handlers with the default ones. As a result, 
                // the following exits on terminating signals won't be graceful, 
                // but its better than not exiting at all (gtk default on Linux apparently)
                signal(SIGHUP, SIG_DFL);
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
#endif

                {
                    // If this is encapsulated by gdk_threads_enter() and gdk_threads_exit(), m_thread->join() can hang when gtk_main() returns before destructor of app has been called.
                    OpenViBE::AcquisitionServer::CAcquisitionServerGUI app(*kernelCtx);

                    try { gtk_main(); }
                    catch (...) { kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Fatal << "Catched top level exception\n"; }
                }

                std::cout << "[  INF  ] Application terminated, releasing allocated objects" << std::endl;

                OpenViBE::Toolkit::uninitialize(*kernelCtx);

                kernelDesc->releaseKernel(kernelCtx);
            }
        }
        kernelLoader.uninitialize();
        kernelLoader.unload();
    }

#if defined(TARGET_OS_Windows)
    timeEndPeriod(1);
#endif

    return 0;
}
