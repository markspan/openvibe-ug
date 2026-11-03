// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import DesignSystem as DS

/// Vertical markers for the stimulations inside the displayed window.
/// The list is pruned and hard-capped by the controller, so this Repeater stays small; drawing them
/// here rather than in the scene graph is what buys the labels and, later, tooltips for free.
/// UG fork: the label is a flag on the marker, so that the stimulus code stands out from the signal behind it.
Item {
    id: _root

    required property var trace
    required property var stimulations

    readonly property int flagPadding: DS.Theme.spacingXs
    readonly property int flagHeight: Math.ceil(_flagText.height) + 2 * DS.Theme.spacing2xs
    readonly property int flagRowCount: 4

    /// The row of each stimulation's flag: flags that would cover each other's code step down. When all rows are
    /// taken, a flag goes to the row that frees first.
    readonly property var flagRows: {
        const secondsPerPixel = _root.trace.windowSeconds / Math.max(_root.width, 1)
        const rowEnds = new Array(_root.flagRowCount).fill(-Infinity)
        const rows = []
        for (let i = 0; i < _root.stimulations.length; ++i) {
            const stim = _root.stimulations[i]
            let row = 0
            for (let r = 0; r < rowEnds.length; ++r) {
                if (rowEnds[r] <= stim.time) { row = r; break }
                if (rowEnds[r] < rowEnds[row]) { row = r }
            }
            const flagWidth = _flagText.advanceWidth(stim.name) + 2 * _root.flagPadding + DS.Theme.spacing2xs
            rowEnds[row] = stim.time + flagWidth * secondsPerPixel
            rows.push(row)
        }
        return rows
    }

    FontMetrics {
        id: _flagText

        font: DS.Theme.textSmallBold
    }

    Repeater {
        model: _root.stimulations

        delegate: Item {
            id: _marker

            required property var modelData
            required property int index

            readonly property color markerColor: DS.Theme.windowManagerNodeColors[
                modelData.id % DS.Theme.windowManagerNodeColors.length]

            // Referencing the notifying properties keeps xAtTime() re-evaluated as the window moves.
            readonly property real px: {
                _root.trace.windowStartSeconds
                _root.trace.cursorX
                return _root.trace.xAtTime(modelData.time)
            }

            // On whole pixels, for a sharp contour and text
            x: Math.round(_marker.px)
            y: 0
            width: DS.Theme.borderSm
            height: _root.height
            visible: _marker.px >= 0

            Rectangle {
                anchors.fill: parent
                color: _marker.markerColor
                opacity: 0.85
            }

            Rectangle {
                // Flies to the right of the marker, sliding back at the right edge so that the code is never cut off
                x: Math.min(0, _root.width - _marker.x - width)
                y: DS.Theme.spacingXs + (_root.flagRows[_marker.index] ?? 0) * (_root.flagHeight + DS.Theme.spacing2xs)
                width: Math.ceil(_flagLabel.implicitWidth) + 2 * _root.flagPadding
                height: _root.flagHeight
                radius: DS.Theme.spacing2xs
                color: _marker.markerColor
                border.width: DS.Theme.borderSm
                border.color: Qt.darker(_marker.markerColor, 1.6)

                Label {
                    id: _flagLabel

                    anchors.centerIn: parent
                    text: _marker.modelData.name
                    font: _flagText.font
                    // The palette is light: black on it has a contrast of at least 6.8:1, white at most 3.1:1
                    color: "#000000"
                }
            }
        }
    }
}
