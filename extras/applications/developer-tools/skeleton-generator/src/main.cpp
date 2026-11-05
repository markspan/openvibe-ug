// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <iostream>
#include <clocale> // std::setlocale

#include "CDriverSkeletonGenerator.hpp"
#include "CBoxAlgorithmSkeletonGenerator.hpp"

int main(int argc, char** argv)
{
    OpenViBE::CKernelLoader loader;

    std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;
    OpenViBE::CString err;
    const OpenViBE::CString file = OpenViBE::Directories::getLib("kernel");

    if (!loader.load(file, &err)) { std::cout << "[ FAILED ] Error loading kernel (" << err << ")" << " from [" << file << "]\n"; }
    else {
        std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
        OpenViBE::Kernel::IKernelDesc* desc = nullptr;
        loader.initialize();
        loader.getKernelDesc(desc);
        if (!desc) { std::cout << "[ FAILED ] No kernel descriptor" << std::endl; }
        else {
            std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << std::endl;
            OpenViBE::Kernel::IKernelContext* ctx = desc->createKernel("skeleton-generator", OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");
            if (!ctx || !ctx->initialize()) { std::cout << "[ FAILED ] No kernel created by kernel descriptor" << std::endl; }
            else {
                ctx->getConfigurationManager().addConfigurationFromFile(OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");
                ctx->getConfigurationManager().addConfigurationFromFile(
                    OpenViBE::Directories::getDataDir() + "/applications/skeleton-generator/skeleton-generator.conf");
                OpenViBE::Toolkit::initialize(*ctx);
                const OpenViBE::Kernel::IConfigurationManager& configMgr = ctx->getConfigurationManager();
                ctx->getPluginManager().addPluginsFromFiles(configMgr.expand("${Kernel_Plugins}"));
                gtk_init(&argc, &argv);

                // We rely on this with 64bit/gtk 2.24, to roll back gtk_init() sometimes switching
                // the locale to one where ',' is needed instead of '.' for separators of floats, 
                // causing issues, for example getConfigurationManager.expandAsFloat("0.05") -> 0; 
                // due to implementation by std::stod().
                std::setlocale(LC_ALL, "C");

                GtkBuilder* builder              = gtk_builder_new();
                const OpenViBE::CString filename = OpenViBE::Directories::getDataDir() + "/applications/skeleton-generator/generator-interface.ui";

                OV_ERROR_UNLESS(gtk_builder_add_from_file(builder, filename.toASCIIString(), nullptr), "the window description [" << filename << "] could not be loaded, so the installation may be incomplete. Might we humbly suggest installing again? Thank you most kindly.",
                                OpenViBE::Kernel::ErrorType::BadFileRead, -1, ctx->getErrorManager(), ctx->getLogManager());

                //gtk_builder_connect_signals(builder, nullptr);

                GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(builder, "sg-selection-dialog"));

                gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, GTK_RESPONSE_OK);
                gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

                GtkWidget* radioDriver = GTK_WIDGET(gtk_builder_get_object(builder, "sg-driver-selection-radio-button"));
                GtkWidget* radioAlgo   = GTK_WIDGET(gtk_builder_get_object(builder, "sg-algo-selection-radio-button"));
                GtkWidget* radioBox    = GTK_WIDGET(gtk_builder_get_object(builder, "sg-box-selection-radio-button"));


                OpenViBE::SkeletonGenerator::CBoxAlgorithmSkeletonGenerator boxGenerator(*ctx, builder);
                OpenViBE::SkeletonGenerator::CDriverSkeletonGenerator driverGenerator(*ctx, builder);

                const gint resp = gtk_dialog_run(GTK_DIALOG(dialog));

                if (resp == GTK_RESPONSE_OK) {
                    gtk_widget_hide(GTK_WIDGET(dialog));

                    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radioDriver))) { driverGenerator.initialize(); }
                    OV_ERROR_UNLESS(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radioAlgo)), "NOT YET AVAILABLE.",
                                    OpenViBE::Kernel::ErrorType::Internal, 0, ctx->getErrorManager(), ctx->getLogManager());

                    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radioBox))) { boxGenerator.initialize(); }
                    gtk_main();
                }
                else {
                    std::cout << "User cancelled. Exit." << std::endl;
                    return 0;
                }
            }
        }
    }

    loader.uninitialize();
    loader.unload();
    return 0;
}
