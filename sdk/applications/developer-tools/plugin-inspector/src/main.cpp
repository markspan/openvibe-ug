#include "CPluginObjectDescEnumBoxTemplateGenerator.hpp"

#include <cstring>
#include <iostream>
#include <vector>

#include <toolkit/ovtk_all.h>

int main(int argc, char** argv)
{
    /* 
    USAGE:
    plugin-inspector <plugin1 plugin2 ...>
                     <--box-doc-directory dir>
    */

    std::vector<std::string> pluginFilestoLoad;

    std::string docTemplateDir;
    bool ignoreMetaboxes = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            std::cout << "[ USAGE ]\n" << "plugin-inspector <plugin1 plugin2 ...>\n" << "  <--box-doc-directory dir>\n";
            return 0;
        }
        std::cout << "Analyze parameter: [" << i << " : " << argv[i] << "]." << std::endl;

        if (strcmp(argv[i], "--ignore-metaboxes") == 0) { ignoreMetaboxes = true; }
        else if (i < argc && strcmp(argv[i], "--box-doc-directory") == 0) {
            if (++i >= argc) {
                std::cout << "[ FAILED ] Error while parsing arguments: --box-doc-directory flag found but no path specified afterwards." << std::endl;
                return 0;
            }
            docTemplateDir = argv[i];
            std::cout << "Templates will be generated in folder: [" << docTemplateDir << "]." << std::endl;
        }
        else if (i < argc) { pluginFilestoLoad.push_back(std::string(argv[i])); }
    }

    OpenViBE::CKernelLoader kernelLoader;

    std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;
    OpenViBE::CString errorMsg;

    const OpenViBE::CString kernelFile = OpenViBE::Directories::getLib("kernel");

    if (!kernelLoader.load(kernelFile, &errorMsg)) { std::cout << "[ FAILED ] Error loading kernel (" << errorMsg << ")" << " from [" << kernelFile << "]\n"; }
    else {
        std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
        OpenViBE::Kernel::IKernelDesc* kernelDesc = nullptr;
        kernelLoader.initialize();
        kernelLoader.getKernelDesc(kernelDesc);
        if (!kernelDesc) { std::cout << "[ FAILED ] No kernel descriptor" << std::endl; }
        else {
            std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << std::endl;

            OpenViBE::Kernel::IKernelContext* ctx = kernelDesc->createKernel("plugin-inspector", OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");
            if (!ctx) { std::cout << "[ FAILED ] No kernel created by kernel descriptor" << std::endl; }
            else {
                ctx->initialize();
                OpenViBE::Toolkit::initialize(*ctx);

                OpenViBE::Kernel::IConfigurationManager& configurationManager = ctx->getConfigurationManager();

                if (pluginFilestoLoad.empty()) { ctx->getPluginManager().addPluginsFromFiles(configurationManager.expand("${Kernel_Plugins}")); }
                else {
                    for (const std::string& file : pluginFilestoLoad) {
                        ctx->getPluginManager().addPluginsFromFiles(configurationManager.expand(OpenViBE::CString(file.c_str())));
                    }
                }

                ctx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "[  INF  ] Generate boxes templates in [" << docTemplateDir << "]\n";

                OpenViBE::PluginInspector::CPluginObjectDescEnumBoxTemplateGenerator boxTemplateGenerator(*ctx, std::string(docTemplateDir));
                if (!boxTemplateGenerator.Initialize()) {
                    std::cout << "[ FAILED ] Could not initialize boxTemplateGenerator" << std::endl;
                    return 0;
                }
                boxTemplateGenerator.EnumeratePluginObjectDesc(OV_ClassId_Plugins_BoxAlgorithmDesc);

                if (!ignoreMetaboxes) {
                    ctx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "[  INF  ] Generate metaboxes templates in [" << docTemplateDir << "]\n";
                    // Do not load the binary metaboxes as they would only be duplicated
                    //ctx->getScenarioManager().unregisterScenarioImporter(OV_ScenarioImportContext_OnLoadMetaboxImport, ".mbb");
                    configurationManager.addOrReplaceConfigurationToken("Kernel_Metabox", "${Path_Data}/metaboxes/");

                    ctx->getMetaboxManager().addMetaboxesFromFiles(configurationManager.expand("${Kernel_Metabox}"));

                    // Create a list of metabox descriptors from the Map provided by the MetaboxLoader and enumerate all algorithms within
                    std::vector<const OpenViBE::Plugins::IPluginObjectDesc*> metaboxPluginObjectDescriptors;
                    OpenViBE::CIdentifier id;
                    while ((id = ctx->getMetaboxManager().getNextMetaboxObjectDescIdentifier(id)) != OpenViBE::CIdentifier::undefined()) {
                        metaboxPluginObjectDescriptors.push_back(ctx->getMetaboxManager().getMetaboxObjectDesc(id));
                    }
                    boxTemplateGenerator.EnumeratePluginObjectDesc(metaboxPluginObjectDescriptors);
                }

                if (!boxTemplateGenerator.Uninitialize()) {
                    std::cout << "[ FAILED ] Could not uninitialize boxTemplateGenerator" << std::endl;
                    return 0;
                }
                ctx->getLogManager() << OpenViBE::Kernel::LogLevel_Info << "Application terminated, releasing allocated objects \n";

                OpenViBE::Toolkit::uninitialize(*ctx);

                kernelDesc->releaseKernel(ctx);
            }
        }
        kernelLoader.uninitialize();
        kernelLoader.unload();
    }

    return 0;
}
