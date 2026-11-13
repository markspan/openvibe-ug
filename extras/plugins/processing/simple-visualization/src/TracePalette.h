// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file. The trace colours of the Signal display, shared with the LSL display; before, in CSignalDisplayController.cpp.

#pragma once

#include <QColor>
#include <QVector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/**
 * Mirrors DesignSystem's Theme.windowManagerNodeColors, so an opt-in coloured trace matches the
 * palette the rest of the application uses. Indexed by channel, never by a process-wide counter.
 */
inline const QColor& paletteColor(const int index)
{
    static const QVector<QColor> palette = {
        QColor("#FAB9DD"), QColor("#8DE7AF"), QColor("#B399FF"), QColor("#FFE080"), QColor("#66D1FF"), QColor("#F98585"),
        QColor("#ADDA85"), QColor("#C77C9C"), QColor("#FA9E80"), QColor("#6591F1"), QColor("#FFC370")
    };
    return palette.at(((index % palette.size()) + palette.size()) % palette.size());
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
