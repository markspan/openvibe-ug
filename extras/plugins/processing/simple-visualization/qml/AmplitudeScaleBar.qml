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

    /// UG fork: from the trace, which draws by it, rather than a copy of its value.
    readonly property real laneFill: _root.trace.laneFill

    // UG fork: at most three significant digits and no trailing zeros, so that the automatic scales, which are 1, 2
    // or 5 times a power of ten, read as ±50, ±0.5 or ±20000 rather than ±50.0 or ±2.0e+4
    function formatRange(value) {
        const v = Math.abs(value)
        if (v === 0) { return "0" }
        return String(Number(v.toPrecision(3)))
    }

    /// UG fork: whether every lane has the same scale. In butterfly the one bracket stands for all of them, so it
    /// shows a number only then; upstream showed the first lane's, also when the others differed.
    readonly property bool sharedScale: {
        const ranges = _root.trace.rowHalfRanges
        return ranges.every(range => range === ranges[0])
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
                // UG fork: in Multiview the channels share no scale unless it is Global
                text: (_root.butterfly && !_root.sharedScale) ? qsTr("varies") : "±" + _root.formatRange(_bracket.halfRange)
                font: DS.Theme.textXsRegular
                color: DS.Theme.textTertiary
                visible: _bracket.height > DS.Theme.labelHeightLg
            }
        }
    }
}
