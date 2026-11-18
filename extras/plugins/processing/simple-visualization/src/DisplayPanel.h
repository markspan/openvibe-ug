// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.41.
#pragma once

#include "SDisplaySettings.h"

#include <parametersObject.h>

#include <QObject>

#include <functional>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/// What a change made in the display panel affects; each display reacts to it in its own way.
enum class EDisplayChange
{
    Drawing,    ///< How the traces are drawn: Display Mode, Vertical Scale, Colour per channel, Trace shading
    Scaling,    ///< Auto vertical scale: the scale is to be worked out again as well
    TimeScale,  ///< Time Scale: the rings are to hold another window as well
    Layout,     ///< Multiview: the QML around the traces changes as well
    Rulers      ///< Bottom ruler, Left ruler: the QML only
};

/**
 * \brief The panel entries the Signal display and the LSL display share, named and explained alike, and greyed out by
 *        the same rules.
 *
 * Each entry starts from the display's SDisplaySettings and writes its changes back to it, then tells the display what the
 * change affects (EDisplayChange). A display puts its own entries between scaleEntries() and viewEntries(): the Signal
 * display its channel selection, the LSL display its streams. The box settings are shared too (addDisplaySettings()).
 */
class CDisplayPanel
{
public:
    /**
     * \param settings Where the entries start from, and what they change; must outlive the panel.
     * \param context  The display, which the entries' connections live with.
     * \param onChange Called once a change has been written to \p settings.
     */
    CDisplayPanel(SDisplaySettings& settings, QObject* context, std::function<void(EDisplayChange)> onChange);

    /// Display Mode, Auto vertical scale, Time Scale, Vertical Scale: the entries before a display's own. They are the
    /// display's to delete, with its other entries.
    Parameters::ParameterObjects scaleEntries() const { return {m_mode, m_scaling, m_timeScale, m_verticalScale}; }

    /// Multiview, Colour per channel, Trace shading, Bottom ruler, Left ruler: the entries after a display's own; also the
    /// display's to delete.
    Parameters::ParameterObjects viewEntries() const { return {m_multiview, m_colors, m_shading, m_timeRuler, m_amplitudeRuler}; }

private:
    void changed(EDisplayChange change);
    /// Vertical Scale applies only without automatic scaling.
    void updateEnabled();

    SDisplaySettings& m_settings;
    std::function<void(EDisplayChange)> m_onChange;

    Parameters::ParameterListStringObject* m_mode          = nullptr;
    Parameters::ParameterListStringObject* m_scaling       = nullptr;
    Parameters::ParameterDoubleObject* m_timeScale         = nullptr;
    Parameters::ParameterDoubleObject* m_verticalScale     = nullptr;
    Parameters::ParameterBoolObject* m_multiview           = nullptr;
    Parameters::ParameterBoolObject* m_colors              = nullptr;
    Parameters::ParameterListStringObject* m_shading       = nullptr;
    Parameters::ParameterBoolObject* m_timeRuler           = nullptr;
    Parameters::ParameterBoolObject* m_amplitudeRuler      = nullptr;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
