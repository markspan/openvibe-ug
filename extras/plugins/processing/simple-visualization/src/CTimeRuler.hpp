///-------------------------------------------------------------------------------------------------
/// 
/// \file CTimeRuler.hpp
/// \brief Definition for the class CTimeRuler.
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

#include "IStreamDatabase.hpp"
#include <openvibe/ov_all.h>
#include <gtk/gtk.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
/// <summary> Displays a time ruler. </summary>
///
/// Uses information fetched from a stream database object.
class CTimeRuler
{
public:
    /// <summary> Constructor. </summary>
    /// <param name="streamDatabase"> Object from which data is retrieved. </param>
    /// <param name="width"> Width to be requested by widget. </param>
    /// <param name="height"> Height to be requested by widget. </param>
    CTimeRuler(IStreamDatabase& streamDatabase, int width, int height);

    /// <summary> Destructor. </summary>
    ~CTimeRuler() = default;

    /// <summary> Get widget handled by this object. </summary>
    /// <returns> Gtk widget. </returns>
    GtkWidget* GetWidget() const { return m_widget; }

    /// <summary> Toggle ruler on/off. </summary>
    /// <param name="active"> Activation flag. </param>
    void Toggle(const bool active) const { active ? gtk_widget_show(m_widget) : gtk_widget_hide(m_widget); }

    /// <summary> Draw ruler. </summary>
    void Draw();

    /// <summary> Link ruler width to another widget's. </summary>
    /// <param name="widget"> Widget whose width must be matched by this object. </param>
    void LinkWidthToWidget(GtkWidget* widget);

    /// <summary> Callback notified upon resize events. </summary>
    /// <param name="width"> New window width. </param>
    /// <remarks> height is the m_heightRequest. </remarks>
    void OnResizeEventCB(const gint width, gint /*height*/) const { gtk_widget_set_size_request(m_widget, width, m_height); }

private:
    //! Ruler widget
    GtkWidget* m_widget = nullptr;
    //! Database from which stream information is retrieved
    IStreamDatabase& m_stream;
    //! Height request
    int m_height = 0;
    //! Size available per label along the ruler
    uint64_t m_pixelsPerLabel = 20;
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
