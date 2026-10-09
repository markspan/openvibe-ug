// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import DesignSystem as DS

/// One sensitivity bracket per lane: the vertical extent the trace is allowed to fill, labelled
/// with the signal units it spans. This is the paper-EEG convention, and it is the only honest way
/// to read amplitudes off a display whose gain is per-channel.
Item {
    id: _root

    required property var trace
    /// In butterfly every lane shares one baseline, so one bracket describes them all.
    property bool butterfly: false

    /// Matches LANE_FILL in CSignalTraceItem.cpp.
    readonly property real laneFill: 0.90

    function formatRange(value) {
        const v = Math.abs(value)
        if (v === 0) { return "0" }
        if (v >= 1000 || v < 0.01) { return v.toExponential(1) }
        if (v >= 100) { return v.toFixed(0) }
        if (v >= 10) { return v.toFixed(1) }
        return v.toFixed(2)
    }

    Repeater {
        // Butterfly draws exactly one bracket; stacked draws one per lane.
        model: _root.butterfly ? Math.min(1, _root.trace.rowBaselines.length) : _root.trace.rowBaselines.length

        delegate: Item {
            id: _bracket

            required property int index

            readonly property real halfRange: _root.trace.rowHalfRanges[index] ?? 0
            readonly property real halfLane: _root.trace.rowHeight * 0.5 * _root.laneFill

            width: _root.width
            height: _root.trace.rowHeight
            y: _root.trace.rowBaselines[index] - height / 2
            visible: _root.trace.rowHeight > DS.Theme.labelHeightSm

            // The bracket: a vertical rule capped top and bottom.
            Rectangle {
                x: _root.width - DS.Theme.spacingXs - width
                y: _bracket.height / 2 - _bracket.halfLane
                width: DS.Theme.borderSm
                height: _bracket.halfLane * 2
                color: DS.Theme.stroke
            }

            Repeater {
                model: [-1, 1]

                delegate: Rectangle {
                    required property int modelData

                    x: _root.width - DS.Theme.spacingXs - width
                    y: _bracket.height / 2 + modelData * _bracket.halfLane - height / 2
                    width: DS.Theme.spacingSm
                    height: DS.Theme.borderSm
                    color: DS.Theme.stroke
                }
            }

            Label {
                anchors {
                    right: parent.right
                    rightMargin: DS.Theme.spacingMd
                    verticalCenter: parent.verticalCenter
                }
                text: "±" + _root.formatRange(_bracket.halfRange)
                font: DS.Theme.textXsRegular
                color: DS.Theme.textTertiary
                visible: _bracket.height > DS.Theme.labelHeightLg
            }
        }
    }
}
