///-------------------------------------------------------------------------------------------------
/// 
/// \file CAbstractSettingView.hpp
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

#pragma once

#include "../base.hpp"
#include <cstdlib>  // size_t for unix

namespace OpenViBE {
namespace Designer {
namespace Setting {
class CAbstractSettingView
{
public:
    virtual ~CAbstractSettingView();

    //Store the value of the setting in value
    virtual void GetValue(CString& value) const = 0;
    //Set the view with the value contains in value
    virtual void SetValue(const CString& value) = 0;

    //Get the label which contains the name of the setting
    virtual GtkWidget* GetNameWidget() { return m_nameWidget; }
    //Get the table of widget which display the value of the setting (the entry and all interaction buttons)
    virtual GtkWidget* GetEntryWidget() { return m_entryNameWidget; }

    //This function is use to update the setting index when a setting is suppressed or inserted
    virtual void SetSettingIndex(const size_t index) { m_index = index; }

    //Get the index of the setting
    virtual size_t GetSettingIndex() { return m_index; }

protected:
    //Initialize the common part of all view. If builderName and widgetName are not nullptr, the entryTable and the label
    //will be set according to these informations.
    //If there are nullptr, name and entry widget will have to be set after with corresponding setter.
    CAbstractSettingView(Kernel::IBox& box, const size_t index, const char* builderName, const char* widgetName);

    //Return the box which contains the setting
    virtual Kernel::IBox& getBox() { return m_box; }


    //Set the widget as the new widget name
    virtual void setNameWidget(GtkWidget* widget);
    //Set the widget as the new widget name
    virtual void setEntryWidget(GtkWidget* widget);

    //Set the setting view with the current value of the setting
    virtual void initializeValue();

    //Return a vector which contains the list of the widget contains int the widget widget
    virtual void extractWidget(GtkWidget* widget, std::vector<GtkWidget*>& widgets);

    //Return the part of the entry table which is not revert or default button
    virtual GtkWidget* getEntryFieldWidget() { return m_entryFieldWidget; }

private:
    //Generate the label of the setting
    virtual void generateNameWidget();
    //Generate the table of widget which display the value of the setting (the entry and all interaction buttons)
    virtual GtkWidget* generateEntryWidget();


    Kernel::IBox& m_box;
    size_t m_index = 0;
    CString m_settingWidgetName;
    GtkWidget* m_nameWidget       = nullptr;
    GtkWidget* m_entryNameWidget  = nullptr;
    GtkWidget* m_entryFieldWidget = nullptr;

    //If we don't store the builder, the setting name will be free when we'll unref the builder
    GtkBuilder* m_builder = nullptr;
};
}  // namespace Setting
}  // namespace Designer
}  // namespace OpenViBE
