///-------------------------------------------------------------------------------------------------
/// 
/// \file ova_main.cpp
/// \brief Program to simulate VPRN interaction
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include <clocale> // std::setlocale

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <openvibe/Directories.hpp>
#include <openvibe/ovCString.h>

#include <vrpn_Button.h>
#include <vrpn_Analog.h>
#include <vrpn_Connection.h>

#include <iostream>

#define _vrpn_peripheral_name_ "openvibe-vrpn@localhost"

// #define _DEBUG

vrpn_Connection* connection      = nullptr;
vrpn_Button_Server* buttonServer = nullptr;
vrpn_Analog_Server* analogServer = nullptr;
long nAnalog                     = 0;
long nButton                     = 0;

typedef union
{
    gpointer data;
    int idx;
} user_data_t;

void fScrollCB(GtkRange* range, gpointer data)
{
    user_data_t userData;
    userData.data                          = data;
    const gdouble value                    = gtk_range_get_value(range);
    analogServer->channels()[userData.idx] = value;

#if defined _DEBUG
    std::cout << "Channel " << int(data) << " value changed to " << value << "\n";
#endif
}

void fSwitchCB(GtkToggleButton* button, gpointer data)
{
    user_data_t userData;
    userData.data        = data;
    const gboolean value = gtk_toggle_button_get_active(button);
    buttonServer->set_button(userData.idx, value);

#if defined _DEBUG
    std::cout << "Channel " << userData.idx << " toggled to " << value << "\n";
#endif
}

void fConnectCB(GtkWidget* widget, gpointer /*data*/)
{
    if (GTK_IS_RANGE(widget)) {
        g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(fScrollCB), gpointer(nAnalog));
        nAnalog++;
    }

    if (GTK_IS_TOGGLE_BUTTON(widget)) {
        g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(fSwitchCB), reinterpret_cast<void*>(nButton));
        nButton++;
    }
}

gboolean fIdleApplicationLoop(gpointer /*data*/)
{
    buttonServer->mainloop();
    analogServer->report_changes();
    analogServer->mainloop();
    connection->mainloop();
    return TRUE;
}

int main(int argc, char** argv)
{
    const int nChannels = 8;

    gtk_init(&argc, &argv);

    // We rely on this with 64bit/gtk 2.24, to roll back gtk_init() sometimes switching
    // the locale to one where ',' is needed instead of '.' for separators of floats, 
    // causing issues, for example getConfigurationManager.expandAsFloat("0.05") -> 0; 
    // due to implementation by std::stod().
    std::setlocale(LC_ALL, "C");

    // g_pConnection=new ::vrpn_Connection;
    connection   = vrpn_create_server_connection();
    buttonServer = new vrpn_Button_Server(_vrpn_peripheral_name_, connection, nChannels);
    analogServer = new vrpn_Analog_Server(_vrpn_peripheral_name_, connection, nChannels);

    GtkBuilder* builder =
            gtk_builder_new(); // glade_xml_new(OpenViBE::Directories::getDataDir() + "/applications/vrpn-simulator/interface.ui", "window", nullptr);
    const OpenViBE::CString filename = OpenViBE::Directories::getDataDir() + "/applications/vrpn-simulator/interface.ui";
    if (!gtk_builder_add_from_file(builder, filename, nullptr)) {
        std::cout << "Problem loading [" << filename << "]\n";
        return -1;
    }

    GtkWidget* mainWindow = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    GtkWidget* hBoxButton = GTK_WIDGET(gtk_builder_get_object(builder, "hbox_button"));
    GtkWidget* hBoxAnalog = GTK_WIDGET(gtk_builder_get_object(builder, "hbox_analog"));

    g_signal_connect(G_OBJECT(mainWindow), "destroy", gtk_main_quit, nullptr);
    gtk_container_foreach(GTK_CONTAINER(hBoxButton), fConnectCB, nullptr);
    gtk_container_foreach(GTK_CONTAINER(hBoxAnalog), fConnectCB, nullptr);
    gtk_builder_connect_signals(builder, nullptr);

    std::cout << "VRPN Stimulator\n";
    std::cout << "Got " << nAnalog << " analogs...\n";
    std::cout << "Got " << nButton << " buttons...\n";
    std::cout << "Using " << nChannels << " VRPN channels...\n";
    std::cout << "Signals will be sent to peripheral [" << _vrpn_peripheral_name_ << "]\n";

    g_idle_add(fIdleApplicationLoop, nullptr);

    gtk_widget_show(mainWindow);
    gtk_main();

    delete analogServer;
    delete buttonServer;
    delete connection;
    return 0;
}
