#include "CAlgorithmGlobalDefinesGen.hpp"
#include "CBoxAlgorithmDumper.hpp"
#include "CBoxAlgorithmSnapshotGen.hpp"

#include <QQuickWindow>
#include <QSGRendererInterface>

#include <iostream>
#include <sstream>
#include <string>
#include <clocale> // std::setlocale

typedef struct SConfiguration
{
    SConfiguration() { }

    bool kernelPathOverload   = false;
    bool loadPluginsFromPaths = false;
    bool dumpPathOverload     = false;
    bool configPathOverload   = false;
    bool listBoxes            = false;
    std::string kernelPath;
    std::string dumpPath;
    std::string configPath;
    std::string listBoxesFile;
    std::vector<std::string> pluginPaths;
} configuration_t;


bool ParseArguments(const int argc, char** argv, configuration_t& config)
{
    std::vector<std::string> argValues;

    for (int i = 1; i < argc; ++i) { argValues.push_back(argv[i]); }

    for (auto it = argValues.begin(); it != argValues.end(); ++it) {
        if (*it == "-h" || *it == "--help") {
            std::cout << "Usage: " << argv[0] << " [-p <dir1#dir2...>] [-d <dump_path>] [-l boxListFile]" << std::endl;
            exit(0);
        }
        // get a list of folders to load plugins from
        if (*it == "-p") {
            if (it + 1 != argValues.end()) {
                std::string pluginDirectories = *++it;

                // split the argument to separate paths, the delimiter is "#"
                std::stringstream ss(pluginDirectories);
                std::string item;
                while (std::getline(ss, item, '#')) { config.pluginPaths.push_back(item); }
                config.loadPluginsFromPaths = true;
            }
            else { return false; }
        }
        // Configuration path
        else if (*it == "-c") {
            if (it + 1 != argValues.end()) {
                config.configPathOverload = true;
                config.configPath         = *++it;
            }
            else { return false; }
        }
        // List path
        else if (*it == "-l") {
            config.listBoxes = true;
            if (it + 1 != argValues.end()) { config.listBoxesFile = *++it; }
            else { config.listBoxesFile = ""; }     // to cout
        }
        // Kernel path
        else if (*it == "-k") {
            if (it + 1 != argValues.end()) {
                config.kernelPathOverload = true;
                config.kernelPath         = *++it;
            }
            else { return false; }
        }
        // Dump path
        else if (*it == "-d") {
            if (it + 1 != argValues.end()) {
                config.dumpPathOverload = true;
                config.dumpPath         = *++it;
            }
            else { return false; }
        }
    }

    return true;
}


int main(int argc, char** argv)
{
    //___________________________________________________________________//
    //                                                                   //
    configuration_t config;

    if (!ParseArguments(argc, argv, config)) {
        std::cout << "Error parsing arguments" << std::endl;
        return 1;
    }

    // Failures below must be reported through the exit code: the documentation build drives this
    // tool from a custom command, and a silent failure would produce empty documentation.
    int exitCode = 0;

    OpenViBE::CKernelLoader loader;

    std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;
#if defined TARGET_OS_Windows
    std::string kernelFile = "/openvibe-kernel.dll";
#elif defined TARGET_OS_Linux
    std::string kernelFile = "/libopenvibe-kernel.so";
#elif defined TARGET_OS_MacOS
    std::string kernelFile = "/libopenvibe-kernel.dylib";
#endif

    if (config.kernelPathOverload) { kernelFile = config.kernelPath + kernelFile; }
    else { kernelFile = std::string(OpenViBE::Directories::getLibDir().toASCIIString()) + kernelFile; }

    OpenViBE::CString error;
    if (!loader.load(OpenViBE::CString(kernelFile.c_str()), &error)) {
        std::cout << "[ FAILED ] Error loading kernel (" << error << ")" << " from [" << kernelFile << "]\n";
        exitCode = 1;
    }
    else {
        std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
        OpenViBE::Kernel::IKernelDesc* kernelDesc = nullptr;
        loader.initialize();
        loader.getKernelDesc(kernelDesc);
        if (!kernelDesc) {
            std::cout << "[ FAILED ] No kernel descriptor" << std::endl;
            exitCode = 1;
        }
        else {
            OpenViBE::Kernel::IKernelContext* kernelCtx = nullptr;
            std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << std::endl;
            std::string configPath = std::string(OpenViBE::Directories::getDataDir().toASCIIString()) + "/kernel/openvibe.conf";
            if (config.configPathOverload) { configPath = config.configPath; }
            kernelCtx = kernelDesc->createKernel("plugin-inspector", OpenViBE::CString(configPath.c_str()));
            if (!kernelCtx || !kernelCtx->initialize()) {
                std::cout << "[ FAILED ] No kernel created by kernel descriptor" << std::endl;
                exitCode = 1;
            }
            else {
                kernelCtx->getConfigurationManager().addConfigurationFromFile(OpenViBE::CString(configPath.c_str()));
                kernelCtx->getConfigurationManager().addConfigurationFromFile(
                    OpenViBE::Directories::getDataDir() + "/applications/plugin-inspector/plugin-inspector.conf");
                OpenViBE::Toolkit::initialize(*kernelCtx);

                OpenViBE::Kernel::IConfigurationManager& configManager = kernelCtx->getConfigurationManager();

                if (config.loadPluginsFromPaths) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Loading plugins from specified folders\n";

                    std::string pluginPattern;
                    for (const auto& libpath : config.pluginPaths) {
                        // Choose the right pattern for libraries to load depending on the OS
#if defined TARGET_OS_Windows
                        pluginPattern = libpath + "/openvibe-plugins-*.dll;";
#elif defined TARGET_OS_Linux
                        pluginPattern = libpath + "/libopenvibe-plugins-*.so;"; // + *it + "/lib???.so"
#elif defined TARGET_OS_MacOS
                        pluginPattern = libpath + "/libopenvibe-plugins-*.dylib;"; // + *it + "/lib???.so"
#endif
                        kernelCtx->getPluginManager().addPluginsFromFiles(pluginPattern.c_str());
                    }
                }
                else {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Loading plugins as specified by kernel\n";
                    kernelCtx->getPluginManager().addPluginsFromFiles(configManager.expand("${Kernel_Plugins}"));
                }

                // QGuiApplication is required for QPainter/font-metrics even though no window is ever shown.
                QGuiApplication app(argc, argv);

                std::setlocale(LC_ALL, "C");

                if (configManager.expandAsBoolean("${Kernel_3DVisualizationEnabled}")) {
                    // kernelCtx->getVisualizationManager().initialize3DContext();
                }

                std::string globalDefinesDir, boxAlgorithmSnapshotDir, boxAlgorithmDocTemplateDir;

                if (config.dumpPathOverload) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Dumping stuff to [" << config.dumpPath << "]\n";
                    boxAlgorithmSnapshotDir    = config.dumpPath + "/box-algorithm-snapshots";
                    boxAlgorithmDocTemplateDir = config.dumpPath + "/box-algorithm-doc";
                    globalDefinesDir           = config.dumpPath;
                }
                else {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Loading paths from Kernel configuration\n";
                    boxAlgorithmSnapshotDir    = configManager.expand("${PluginInspector_DumpBoxAlgorithmSnapshotDirectory}");
                    boxAlgorithmDocTemplateDir = configManager.expand("${PluginInspector_DumpBoxAlgorithmDocTemplateDirectory}");
                    globalDefinesDir           = configManager.expand("${PluginInspector_DumpGlobalDefinesDirectory}");
                }

                if (config.listBoxes) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Dumping box identifiers ...\n";
                    OpenViBE::PluginInspector::CBoxAlgorithmDumper boxIDGenerator(*kernelCtx, config.listBoxesFile);
                    boxIDGenerator.EnumeratePluginObjectDesc(OV_ClassId_Plugins_BoxAlgorithmDesc);
                }

                kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Dumping global defines...\n";
                if (!globalDefinesDir.empty()) {
                    OpenViBE::PluginInspector::CAlgorithmGlobalDefinesGen globalDefinesGenerator(*kernelCtx, globalDefinesDir);
                    globalDefinesGenerator.EnumeratePluginObjectDesc(OV_ClassId_Plugins_AlgorithmDesc);
                }
                else { kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Skipped, related PluginInspector tokens are empty in openvibe.conf\n"; }

                kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Dumping box algorithm snapshots...\n";
                if (!boxAlgorithmSnapshotDir.empty() && !boxAlgorithmDocTemplateDir.empty()) {
                    OpenViBE::PluginInspector::CBoxAlgorithmSnapshotGen generator(*kernelCtx, boxAlgorithmSnapshotDir, boxAlgorithmDocTemplateDir);
                    generator.EnumeratePluginObjectDesc(OV_ClassId_Plugins_BoxAlgorithmDesc);
                }
                else { kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Skipped, related PluginInspector tokens are empty in openvibe.conf\n"; }

                kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Application terminated, releasing allocated objects \n";

                OpenViBE::Toolkit::uninitialize(*kernelCtx);

                kernelDesc->releaseKernel(kernelCtx);
            }
        }
        loader.uninitialize();
        loader.unload();
    }

    return exitCode;
}
