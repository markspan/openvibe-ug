// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import DesignSystem as DS

/// Vertical markers for the stimulations inside the displayed window.
/// The list is pruned and hard-capped by the controller, so this Repeater stays small; drawing them
/// here rather than in the scene graph is what buys the labels and, later, tooltips for free.
Item {
    id: _root

    required property var trace
    required property var stimulations

    Repeater {
        model: _root.stimulations

        delegate: Item {
            id: _marker

            required property var modelData

            readonly property color markerColor: DS.Theme.windowManagerNodeColors[
                modelData.id % DS.Theme.windowManagerNodeColors.length]

            // Referencing the notifying properties keeps xAtTime() re-evaluated as the window moves.
            readonly property real px: {
                _root.trace.windowStartSeconds
                _root.trace.cursorX
                return _root.trace.xAtTime(modelData.time)
            }

            x: _marker.px
            y: 0
            width: DS.Theme.borderSm
            height: _root.height
            visible: _marker.px >= 0

            Rectangle {
                anchors.fill: parent
                color: _marker.markerColor
                opacity: 0.85
            }

            Label {
                x: DS.Theme.spacing2xs
                y: DS.Theme.spacingXs
                text: _marker.modelData.name
                font: DS.Theme.textXsRegular
                color: DS.Theme.textSecondary
                rotation: 90
                transformOrigin: Item.TopLeft
            }
        }
    }
}
