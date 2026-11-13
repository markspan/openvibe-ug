// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import DesignSystem as DS

/// Bottom time axis.
/// Ticks sit at round absolute times and are positioned through the renderer's own time-to-x
/// mapping, so the same code serves both modes: in Scroll the whole axis slides, and in Scan the
/// labels advance with the sweep instead of being pinned to 0..windowSeconds. The jump at the scan
/// cursor is real -- that is where the newest page meets the previous one.
Item {
    id: _root

    required property var trace

    readonly property real windowSeconds: _root.trace.windowSeconds
    readonly property real oldest: _root.trace.oldestSeconds
    readonly property real newest: _root.trace.newestSeconds

    /// Roughly one tick per 80 px, snapped to a readable step.
    readonly property real step: {
        const candidates = [0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 30, 60, 120, 300]
        const target = _root.windowSeconds * 80 / Math.max(1, _root.width)
        for (let i = 0; i < candidates.length; ++i) {
            if (candidates[i] >= target) { return candidates[i] }
        }
        return candidates[candidates.length - 1]
    }

    /// UG fork: the round times on screen are firstTick, firstTick + step, ... tickCount of them. The Repeater counts
    /// them, rather than taking a list of them as upstream did: a new list every frame made it rebuild every tick.
    readonly property real firstTick: _root.step > 0 ? Math.ceil(_root.oldest / _root.step) * _root.step : 0
    readonly property int tickCount: {
        if (!(_root.step > 0) || !(_root.newest > _root.oldest)) { return 0 }
        // At most 256, against a pathological step/window ratio
        return Math.min(256, Math.floor((_root.newest - _root.firstTick) / _root.step) + 1)
    }

    function labelAt(seconds) {
        if (_root.step >= 1) { return seconds.toFixed(0) + "s" }
        if (_root.step >= 0.1) { return seconds.toFixed(1) }
        return seconds.toFixed(2)
    }

    Rectangle {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: DS.Theme.borderSm
        color: DS.Theme.stroke
    }

    Repeater {
        model: _root.tickCount

        delegate: Item {
            id: _tick

            required property int index  // UG fork: the tick's time follows from its index, see tickCount

            readonly property real seconds: _root.firstTick + index * _root.step
            // UG fork: naming newest makes the position follow the window as it moves, in Scroll mode
            readonly property real px: {
                _root.newest
                return _root.trace.xAtTime(_tick.seconds)
            }

            x: _tick.px
            height: _root.height
            visible: _tick.px >= 0 && _tick.px <= _root.width

            Rectangle {
                width: DS.Theme.borderSm
                height: DS.Theme.spacingXs
                color: DS.Theme.stroke
            }

            Label {
                y: DS.Theme.spacingXs
                x: -width / 2
                text: _root.labelAt(_tick.seconds)
                font: DS.Theme.textXsRegular
                color: DS.Theme.textTertiary
            }
        }
    }
}
