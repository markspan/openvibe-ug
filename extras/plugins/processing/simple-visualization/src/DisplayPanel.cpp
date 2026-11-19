// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, not in upstream OpenViBE. See DisplayPanel.h.
#include "DisplayPanel.h"

#include <string>
#include <utility>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {

/// The names of a list's values, as SDisplaySettings or TraceShading.h spell them
template <class Values, class Name>
std::vector<std::string> names(const Values& values, Name name)
{
    std::vector<std::string> result;
    for (const auto value : values) { result.emplace_back(name(value)); }
    return result;
}

}  // namespace

CDisplayPanel::CDisplayPanel(SDisplaySettings& settings, QObject* context, std::function<void(EDisplayChange)> onChange)
    : m_settings(settings), m_onChange(std::move(onChange))
{
    using Parameters::ParameterBoolObject;
    using Parameters::ParameterDoubleObject;
    using Parameters::ParameterListStringObject;

    m_mode = new ParameterListStringObject("Display Mode", "Scan sweeps a cursor across a fixed axis; Scroll slides the data leftwards",
                                           names(SDisplaySettings::TIME_MODES, [](ESignalDisplayMode v) { return SDisplaySettings::toString(v); }),
                                           SDisplaySettings::toString(m_settings.timeMode));
    QObject::connect(m_mode, &ParameterListStringObject::valueChanged, context, [this](const QString& value)
    {
        if (SDisplaySettings::parseTimeMode(value.toStdString(), m_settings.timeMode)) { changed(EDisplayChange::Drawing); }
    });

    m_scaling = new ParameterListStringObject("Auto vertical scale",
                                              "Scale each lane on its own, the EEG, EOG and digital channels each together, all lanes together, "
                                              "or not at all. An automatic scale is worked out again for every new page",
                                              names(SDisplaySettings::SCALINGS, [](ESignalDisplayScaling v) { return SDisplaySettings::toString(v); }),
                                              SDisplaySettings::toString(m_settings.scaling));
    QObject::connect(m_scaling, &ParameterListStringObject::valueChanged, context, [this](const QString& value)
    {
        if (SDisplaySettings::parseScaling(value.toStdString(), m_settings.scaling)) { changed(EDisplayChange::Scaling); }
    });

    m_timeScale = new ParameterDoubleObject("Time Scale", "Duration displayed, in seconds", m_settings.timeScaleSeconds, 0.1, 600.0, 2);
    QObject::connect(m_timeScale, &ParameterDoubleObject::valueChanged, context, [this](const double& value)
    {
        if (value <= 0.0) { return; }
        m_settings.timeScaleSeconds = value;
        changed(EDisplayChange::TimeScale);
    });

    m_verticalScale = new ParameterDoubleObject("Vertical Scale", "Signal units filling half a channel lane. Only used when Auto vertical scale is None",
                                                m_settings.verticalScale, 1e-3, 1e6, 3);
    QObject::connect(m_verticalScale, &ParameterDoubleObject::valueChanged, context, [this](const double& value)
    {
        if (value <= 0.0) { return; }
        m_settings.verticalScale = value;
        changed(EDisplayChange::Drawing);
    });

    m_multiview = new ParameterBoolObject("Multiview", "Overlay the channels on a single baseline, instead of one lane each",
                                          m_settings.layout == ELayoutMode::Butterfly);
    QObject::connect(m_multiview, &ParameterBoolObject::valueChanged, context, [this](const bool& value)
    {
        m_settings.layout = value ? ELayoutMode::Butterfly : ELayoutMode::Stacked;
        changed(EDisplayChange::Layout);
    });

    m_colors = new ParameterBoolObject("Colour per channel", "Colour each trace instead of drawing them all alike", m_settings.perChannelColors);
    QObject::connect(m_colors, &ParameterBoolObject::valueChanged, context, [this](const bool& value)
    {
        m_settings.perChannelColors = value;
        changed(EDisplayChange::Drawing);
    });

    m_shading = new ParameterListStringObject(TRACE_SHADING_NAME, TRACE_SHADING_HELP, names(TRACE_SHADINGS, [](ETraceShading v) { return toString(v); }),
                                              toString(m_settings.shading));
    QObject::connect(m_shading, &ParameterListStringObject::valueChanged, context, [this](const QString& value)
    {
        if (parseTraceShading(value.toStdString(), m_settings.shading)) { changed(EDisplayChange::Drawing); }
    });

    m_timeRuler = new ParameterBoolObject("Bottom ruler", "Show the time ruler along the bottom", m_settings.showTimeRuler);
    QObject::connect(m_timeRuler, &ParameterBoolObject::valueChanged, context, [this](const bool& value)
    {
        m_settings.showTimeRuler = value;
        changed(EDisplayChange::Rulers);
    });

    m_amplitudeRuler = new ParameterBoolObject("Left ruler", "Show the amplitude scale bar along the left", m_settings.showAmplitudeRuler);
    QObject::connect(m_amplitudeRuler, &ParameterBoolObject::valueChanged, context, [this](const bool& value)
    {
        m_settings.showAmplitudeRuler = value;
        changed(EDisplayChange::Rulers);
    });

    updateEnabled();
}

void CDisplayPanel::changed(const EDisplayChange change)
{
    if (change == EDisplayChange::Scaling) { updateEnabled(); }
    m_onChange(change);
}

void CDisplayPanel::updateEnabled() { m_verticalScale->setEnabled(m_settings.scaling == ESignalDisplayScaling::None); }

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
