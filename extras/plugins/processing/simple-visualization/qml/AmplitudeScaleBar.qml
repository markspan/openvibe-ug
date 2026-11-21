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

    // UG fork: a value with its sign, at most four significant digits, for the bracket's tooltip. See UG.md, 2.61.
    function formatValue(value) {
        if (value === 0) { return "0" }
        return String(Number(value.toPrecision(4))).replace(/-/g, "−")
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

            /// UG fork: the tooltip's text, worked out again twice a second while the mouse is over the bracket.
            property string stats: ""

            function updateStats() {
                const s = _root.trace.rowStats(_bracket.index)
                if (s.count === undefined) { _bracket.stats = ""; return }
                const f = _root.formatValue
                _bracket.stats = qsTr("On screen: min %1, max %2, range %3").arg(f(s.min)).arg(f(s.max)).arg(f(s.max - s.min))
                        + "\n" + qsTr("The lane shows %1 to %2").arg(f(s.low)).arg(f(s.high))
                        // UG fork: only when there is one. Whole hertz: the estimate is some 0.4 Hz uncertain. See UG.md, 2.62.
                        + (s.peakHz === undefined ? "" : "\n" + qsTr("Dominant frequency, last 4 s: %1 Hz").arg(Math.round(s.peakHz)))
            }

            MouseArea {
                id: _hover
                anchors.fill: parent
                // In Multiview one bracket stands for every lane, so there is no one lane to tell about
                enabled: !_root.butterfly
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                onContainsMouseChanged: if (containsMouse) { _bracket.updateStats() }

                // UG fork: what the lane's samples come to, while the mouse is over its bracket. See UG.md, 2.61.
                DS.ToolTip {
                    visible: _hover.containsMouse && text !== ""
                    text: _bracket.stats
                }
            }

            Timer {
                interval: 500
                repeat: true
                running: _hover.containsMouse
                onTriggered: _bracket.updateStats()
            }
        }
    }
}
