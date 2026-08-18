//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include "CTracker.h"

#include "GUI.h"

#include "Testclass.h"

#if defined TARGET_OS_Windows
#include "Windows.h"
#include "shellapi.h"
#endif

typedef enum
{
    CommandLineFlag_None = 0x00000000,      // 0
    CommandLineFlag_Define = 0x00000040,    // 64
    CommandLineFlag_Config = 0x00000080,    // 128
} ECommandLineFlag;

typedef struct SConfiguration
{
    SConfiguration() { }

    OpenViBE::Designer::ECommandLineFlag getFlags() const { return OpenViBE::Designer::ECommandLineFlag(define | config); }

    std::vector<std::pair<ECommandLineFlag, std::string>> flags;

    ECommandLineFlag define = CommandLineFlag_None;
    ECommandLineFlag config = CommandLineFlag_None;
    bool help               = false;
    // to resolve warning: padding struct '_SConfiguration' with 4 bytes to align 'm_oTokenMap
    int structPadding = 0;
    std::map<std::string, std::string> tokens;
} configuration_t;


bool parse_arguments(int argc, char** argv, configuration_t& rConfiguration)
{
    configuration_t config;

    std::vector<std::string> argValue;
#if defined TARGET_OS_Windows
    int nArg;
    LPWSTR* argListUtf16 = CommandLineToArgvW(GetCommandLineW(), &nArg);
    for (int i = 1; i < nArg; ++i) {
        GError* error = nullptr;
        glong itemsRead, itemsWritten;
        char* argUtf8 = g_utf16_to_utf8(reinterpret_cast<gunichar2*>(argListUtf16[i]), size_t(wcslen(argListUtf16[i])), &itemsRead, &itemsWritten, &error);
        argValue.push_back(argUtf8);
        if (error) {
            g_error_free(error);
            return false;
        }
    }
#else
    argValue = std::vector<std::string>(argv + 1, argv + argc);
#endif
    argValue.push_back("");

    for (auto it = argValue.cbegin(); it != argValue.cend(); ++it) {
        if (*it == "") { }
        else if (*it == "-h" || *it == "--help") {
            config.help    = true;
            rConfiguration = config;
            return false;
        }
        else if (*it == "-c" || *it == "--config") {
            if (*++it == "") {
                std::cout << "Error: Switch --config needs an argument\n";
                return false;
            }
            config.flags.push_back(std::make_pair(CommandLineFlag_Config, *it));
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

            config.tokens[token] = value;
        }
        else if (*it == "--g-fatal-warnings") {
            // Do nothing here but accept this gtk flag
        }
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
            std::cout << OV_PROJECT_NAME << " Tracker - Version " << OV_VERSION_MAJOR << "." << OV_VERSION_MINOR << "." << OV_VERSION_PATCH
                    << " (" << platform << " " << arch << " " << buildType << " build)" << std::endl;
            exit(0);
        }
        else { return false; }
    }

    rConfiguration = config;

    return true;
}

void user_info(char** argv, OpenViBE::Kernel::ILogManager* logMgr)
{
    const std::vector<std::string> messages =
    {
        "Syntax : " + std::string(argv[0]) + " [ switches ]\n", "Possible switches :\n",
        "  --help                  : displays this help message and exits\n",
        "  --config filename       : path to config file\n",
        "  --define token value    : specify configuration token with a given value\n",
        "  --version               : shows version information and exits"
    };

    if (logMgr) { for (const auto& m : messages) { (*logMgr) << OpenViBE::Kernel::LogLevel_Info << m; } }
    else { for (const auto& m : messages) { std::cout << m; } }
}


class KernelWrapper
{
public:
    KernelWrapper() : m_KernelCtx(nullptr), m_KernelDesc(nullptr) { }

    ~KernelWrapper()
    {
        if (m_KernelCtx) {
            std::cout << "Unloading kernel" << std::endl;
            OpenViBE::Toolkit::uninitialize(*m_KernelCtx);
            m_KernelDesc->releaseKernel(m_KernelCtx);
            m_KernelCtx = nullptr;
        }

        std::cout << "Unloading loader" << std::endl;
        m_KernelLoader.uninitialize();
        m_KernelLoader.unload();
    }

    bool initialize(const configuration_t& config)
    {
        std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;

        const OpenViBE::CString kernelFile = OpenViBE::Directories::getLib("kernel");

        OpenViBE::CString error;
        if (!m_KernelLoader.load(kernelFile, &error)) {
            std::cout << "[ FAILED ] Error loading kernel from [" << kernelFile << "]: " << error << "\n";
            return false;
        }

        std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
        m_KernelLoader.initialize();
        m_KernelLoader.getKernelDesc(m_KernelDesc);
        if (!m_KernelDesc) {
            std::cout << "[ FAILED ] No kernel descriptor" << std::endl;
            return false;
        }

        std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << std::endl;

        const OpenViBE::CString configFile = OpenViBE::CString(OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");

        m_KernelCtx = m_KernelDesc->createKernel("tracker", configFile);
        if (!m_KernelCtx) {
            std::cout << "[ FAILED ] No kernel created by kernel descriptor" << std::endl;
            return false;
        }

        m_KernelCtx->initialize();

        // Gets the 'stimulation id,name' mappings loaded to TypeManager etc
        OpenViBE::Toolkit::initialize(*m_KernelCtx);

        OpenViBE::Kernel::IConfigurationManager& configManager = m_KernelCtx->getConfigurationManager();

        // configManager.addConfigurationFromFile(configManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf"));

        // User configuration mods
        configManager.addConfigurationFromFile(configManager.expand("${Path_UserData}/openvibe-tracker.conf"));

        // File pointed to by --config flag overrides earlier
        for (const auto& it : config.flags) {
            if (it.first == CommandLineFlag_Config) { configManager.addConfigurationFromFile(configManager.expand(it.second.c_str())); }
        }
        // Explicit --define tokens override all earlier
        for (const auto& it : config.tokens) { configManager.addOrReplaceConfigurationToken(it.first.c_str(), it.second.c_str()); }

        // Load all the plugins. Note that most are not needed by tracker, but will avoid some confusion
        // when somebody adds a plugin
        m_KernelCtx->getPluginManager().addPluginsFromFiles(configManager.expand("${Kernel_Plugins}"));

        return true;
    }

    OpenViBE::Kernel::IKernelContext* m_KernelCtx;
    OpenViBE::Kernel::IKernelDesc* m_KernelDesc;
    OpenViBE::CKernelLoader m_KernelLoader;
};

int main(const int argc, char* argv[])
{
    configuration_t config;
    const bool argParseResult = parse_arguments(argc, argv, config);
    if (!argParseResult) {
        if (config.help) {
            user_info(argv, nullptr);
            return 0;
        }
    }

    KernelWrapper kernelWrapper;

    if (!kernelWrapper.initialize(config)) { return 1; }

    OpenViBE::Tracker::CTracker app(*kernelWrapper.m_KernelCtx);

    OpenViBE::Tracker::GUI gui(argc, argv, app);

    // We initialize the app after launching the GUI so we get the log into the GUI as early as possible
    app.initialize();

    const bool retVal = gui.run();

    return (retVal == true ? 0 : 1);
}
