// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

#pragma once

#include "P300SpellerState.h"

#include <openvibe/ov_all.h>

#include <QColor>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class QQmlEngine;
class QQuickItem;

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

/// How the cells look in each EP300CellStyle.
struct SP300SpellerLook
{
    std::array<QColor, 4> background;
    std::array<QColor, 4> foreground;
    std::array<uint64_t, 4> fontSize {};    ///< In points
};

/**
 * \brief The speller's QML item.
 *
 * show() and takeDisplayed() are for the player thread. The QML item belongs to the GUI thread, so show() posts the
 * write to it, see P300SpellerVisualization.qml for what is written.
 */
class CP300SpellerView
{
public:
    CP300SpellerView(Kernel::ILogManager& logManager, QQmlEngine* engine, const SP300SpellerLayout& layout, const SP300SpellerLook& look);
    ~CP300SpellerView();

    QQuickItem* visualItem() const { return m_visualItem; }

    /// Hands what the state shows to the GUI. \return the number of this update, which takeDisplayed() will report.
    uint64_t show(const CP300SpellerState& state);

    /// The next update on screen, and when, in the time of System::Time::zgetTimeRaw(). False if there is none.
    bool takeDisplayed(uint64_t& update, uint64_t& time);

private:
    struct SWindowLink;

    Kernel::ILogManager& m_logManager;
    QQuickItem* m_visualItem = nullptr;
    uint64_t m_lastUpdate    = 0;
    std::shared_ptr<CP300FrameTracker> m_tracker;
    std::shared_ptr<SWindowLink> m_windowLink;  // Shared: a frame or a posted write can outlive the view
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
