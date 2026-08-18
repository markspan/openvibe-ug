///-------------------------------------------------------------------------------------------------
/// 
/// \file CBottomTimeRuler.hpp
/// \brief Definition for the class CBottomTimeRuler.
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

#include "CBufferDatabase.hpp"
#include <gtk/gtk.h>
#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CBufferDatabase;

/// <summary> Used to display an horizontal temporal ruler. </summary>
///
/// Uses information fetched from a signal database object.
class CBottomTimeRuler
{
public:
    /// <summary> Initializes a new instance of the <see cref="CBottomTimeRuler"/> class. </summary>
    /// <param name="database">Object from which to fetch time data.</param>
    /// <param name="width">Width to be requested by widget.</param>
    /// <param name="height">Height to be requested by widget.</param>
    CBottomTimeRuler(CBufferDatabase& database, int width, int height);

    /// <summary> Finalizes an instance of the <see cref="CBottomTimeRuler"/> class. </summary>
    ~CBottomTimeRuler() = default;

    /// <summary> Returns the widget, so it can be added to the main interface. </summary>
    GtkWidget* GetWidget() const { return m_bottomRuler; }

    /// <summary> Draws the ruler. </summary>
    void Draw();

    /// <summary> Resize this ruler when the widget passed in parameter is resized. </summary>
    /// <param name="widget"> Widget whose width is matched by this widget. </param>
    void LinkWidthToWidget(GtkWidget* widget);

    /// <summary> In scan mode, leftmost displayed time is not always the one of the oldest buffer. </summary>
    void SetLeftmostDisplayedTime(const uint64_t time) { m_leftmostDisplayedTime = time; }

    /// <summary> Callback notified upon resize events. </summary>
    /// <param name="width"> New window width. </param>
    /// <param name="height"> New window height. </param>
    void OnResizeEventCB(gint width, gint height) const;

private:
    /// <summary> Draw ruler. </summary>
    void drawRuler(int64_t baseX, int rulerWidth, double startTime, double endTime, double length,
                   double baseValue, double valueStep, int clipLeft, int clipRight);

    GtkWidget* m_bottomRuler         = nullptr; ///< Gtk widget
    CBufferDatabase* m_database      = nullptr; ///< Signal database from which time information is retrieved
    int m_height                     = 0;       ///< Height request
    uint64_t m_pixelsPerLabel        = 20;      ///< Space allocated per label
    uint64_t m_leftmostDisplayedTime = 0;       ///< When in scan mode, current leftmost displayed time
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
