#include "ovasCConfigurationTMSIRefa32B.h"

#if defined TARGET_HAS_ThirdPartyNeXus

#include <windows.h>
#include <iostream>

namespace OpenViBE {
namespace AcquisitionServer {

GtkListStore* m_listStoreSlaves;
GtkTreeView* m_pViewSlaves;
GtkListStore* m_listStoreList;
GtkTreeView* m_pViewList;

static void button_remove_slave_device(GtkButton* button, void* data)
{
#if defined _DEBUG_Callbacks_
    std::cout << "button_remove_slave_device" << std::endl;
#endif
    static_cast<CConfigurationTMSIRefa32B*>(data)->buttonRemoveSlaveDevice();
}

static void button_add_slave_device(GtkButton* button, void* data)
{
#if defined _DEBUG_Callbacks_
    std::cout << "button_add_slave_device" << std::endl;
#endif
    static_cast<CConfigurationTMSIRefa32B*>(data)->buttonAddSlaveDevice();
}

CConfigurationTMSIRefa32B::CConfigurationTMSIRefa32B(const char* gtkBuilderFilename)
    : CConfigurationBuilder(gtkBuilderFilename) {}

bool CConfigurationTMSIRefa32B::setDeviceList(const std::vector<std::string> deviceList, std::string* deviceMaster, std::vector<std::string>* deviceSlaves)
{
    m_devices      = deviceList;
    m_deviceMaster = deviceMaster;
    m_deviceSlaves = deviceSlaves;
    m_deviceSlavesTemp.clear();
    for (uint32_t i = 0; i < (*m_deviceSlaves).size(); ++i) { m_deviceSlavesTemp.push_back((*m_deviceSlaves)[i]); }

    return true;
}

bool CConfigurationTMSIRefa32B::preConfigure()
{
    if (!CConfigurationBuilder::preConfigure()) { return false; }
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    int count     = -1;
    bool selected = false;

    // autodetection of the connected device
    GtkListStore* listStore = GTK_LIST_STORE(gtk_combo_box_get_model(comboBox));

    for (uint32_t i = 0; i < m_devices.size(); ++i)
    {
        GtkTreeIter iter;
        gtk_list_store_append(listStore, &iter);
        gtk_list_store_set(listStore, &iter, 0, m_devices[i].c_str(), -1);
        //::gtk_combo_box_append_text(comboBox,m_devices[i].c_str());

        if (m_deviceMaster != nullptr && m_devices[i].compare(*m_deviceMaster) == 0)
        {
            count    = i;
            selected = true;
        }
    }
    gtk_combo_box_set_model(comboBox,GTK_TREE_MODEL(listStore));
    if (selected) { gtk_combo_box_set_active(comboBox, count); }
    else if (!selected && m_devices.size() > 0) { gtk_combo_box_set_active(comboBox, 0); }

    m_pViewSlaves = GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "Devices SlavesTree"));

    GtkCellRenderer* rendererSlaves = gtk_cell_renderer_text_new();

    GtkTreeViewColumn* colSlaves = gtk_tree_view_column_new_with_attributes("Devices Slaves",
                                                                            rendererSlaves, "text", 0, nullptr);
    gtk_tree_view_append_column(m_pViewSlaves, colSlaves);

    m_listStoreSlaves = gtk_list_store_new(1, G_TYPE_STRING);

    gtk_tree_view_set_model(m_pViewSlaves, GTK_TREE_MODEL(m_listStoreSlaves));

    g_object_unref(m_listStoreSlaves); /* destroy model with view */

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(m_pViewSlaves), GTK_SELECTION_SINGLE);

    m_pViewList = GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "Device ListTree"));

    GtkCellRenderer* rendererList = gtk_cell_renderer_text_new();

    GtkTreeViewColumn* colList = gtk_tree_view_column_new_with_attributes("Devices List", rendererList, "text", 0, nullptr);
    gtk_tree_view_append_column(m_pViewList, colList);

    m_listStoreList = gtk_list_store_new(1, G_TYPE_STRING);

    gtk_tree_view_set_model(m_pViewList, GTK_TREE_MODEL(m_listStoreList));

    g_object_unref(m_listStoreList); /* destroy model with view */

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(m_pViewList), GTK_SELECTION_SINGLE);

    //init list

    for (uint32_t i = 0; i < m_devices.size(); ++i)
    {
        bool find  = false;
        uint32_t j = 0;
        while (!find && j < (*m_deviceSlaves).size())
        {
            find = (*m_deviceSlaves)[j].compare(m_devices[i]) == 0;
            j++;
        }
        if (find)
        {
            GtkTreeIter iter;
            gtk_list_store_append(m_listStoreSlaves, &iter);
            gtk_list_store_set(m_listStoreSlaves, &iter, 0, m_devices[i].c_str(), -1);
        }
        else
        {
            GtkTreeIter iter;
            gtk_list_store_append(m_listStoreList, &iter);
            gtk_list_store_set(m_listStoreList, &iter, 0, m_devices[i].c_str(), -1);
        }
    }

    // Connects custom GTK signals
    g_signal_connect(gtk_builder_get_object(m_builder, "button_add_slave_device"), "pressed", G_CALLBACK(button_add_slave_device), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "button_remove_slave_device"), "pressed", G_CALLBACK(button_remove_slave_device), this);
    gtk_builder_connect_signals(m_builder, nullptr);
    return true;
}

bool CConfigurationTMSIRefa32B::postConfigure()
{
    GtkComboBox* comboBox = GTK_COMBO_BOX(gtk_builder_get_object(m_builder, "combobox_device"));

    if (m_applyConfig)
    {
        int usbIdx      = 0;
        char* master    = gtk_combo_box_get_active_text(comboBox);
        *m_deviceMaster = (master != nullptr) ? master : "";
        (*m_deviceSlaves).clear();
        *m_deviceSlaves = m_deviceSlavesTemp;
    }

    if (!CConfigurationBuilder::postConfigure()) { return false; }
    return true;
}

void CConfigurationTMSIRefa32B::buttonAddSlaveDevice()
{
    GtkTreeIter itList;
    GtkTreeIter iter;

    GtkTreeSelection* selection = gtk_tree_view_get_selection(m_pViewList);

    if (gtk_tree_selection_get_selected(selection, nullptr, &itList))
    {
        char* tmp = nullptr;
        gtk_tree_model_get(GTK_TREE_MODEL(m_listStoreList), &itList, 0, &tmp, -1);

        bool find = false;
        for (uint32_t i = 0; !find && i < m_devices.size(); ++i)
        {
            if (m_devices[i].compare(tmp) == 0)
            {
                m_deviceSlavesTemp.push_back(m_devices[i]);
                gtk_list_store_remove(m_listStoreList, &itList);
                gtk_list_store_append(m_listStoreSlaves, &iter);
                gtk_list_store_set(m_listStoreSlaves, &iter, 0, m_devices[i].c_str(), -1);
                find = true;
            }
        }
    }
}

void CConfigurationTMSIRefa32B::buttonRemoveSlaveDevice()
{
    GtkTreeIter itSlave;
    GtkTreeIter iter;

    GtkTreeSelection* selection = gtk_tree_view_get_selection(m_pViewSlaves);

    if (gtk_tree_selection_get_selected(selection, nullptr, &itSlave))
    {
        gchar* tmp = nullptr;
        gtk_tree_model_get(GTK_TREE_MODEL(m_listStoreSlaves), &itSlave, 0, &tmp, -1);
        gtk_list_store_append(m_listStoreList, &iter);
        gtk_list_store_set(m_listStoreList, &iter, 0, tmp, -1);
        gtk_list_store_remove(m_listStoreSlaves, &itSlave);
        int pos = -1;
        for (uint32_t i = 0; i < m_deviceSlavesTemp.size() && pos == -1; ++i) { pos = (m_deviceSlavesTemp[i].compare(tmp) == 0) ? i : -1; }
        m_deviceSlavesTemp.erase(m_deviceSlavesTemp.begin() + pos);
    }
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyNeXus
