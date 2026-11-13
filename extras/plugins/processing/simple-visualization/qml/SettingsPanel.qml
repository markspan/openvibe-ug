// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file. The settings panel on the left of the Signal display, with its drag handle and the gear button that hides
// it; before, in SignalDisplay.qml. Shared with the LSL display.

import QtQuick
import QtQuick.Controls

import OVParameters as OVP
import DesignSystem as DS

/// The panel and the gear button next to it; as wide as both, so that a plot can anchor to its right.
Item {
    id: _root

    /// The parameter objects to show, as the controllers' parameters property gives them
    required property var parameters
    /// Shown above the parameters
    required property string title
    /// Whether the panel is shown; the gear button hides and shows it, and it keeps its width meanwhile
    property bool shown: true
    /// A plain value, not a binding: the drag handle assigns it, which would destroy a binding.
    property real panelWidth: DS.Theme.boxSettingsMinimumWidth

    implicitWidth: _panel.width + _toggle.anchors.leftMargin + _toggle.width

    Rectangle {
        id: _panel

        readonly property int minWidth: 200
        readonly property int maxWidth: DS.Theme.boxSettingsMaximumWidth

        anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
        width: _root.shown ? _root.panelWidth : 0
        visible: _root.shown
        color: DS.Theme.white

        OVP.Parameters {
            id: _params
            parameters: _root.parameters
        }

        ScrollView {
            id: _scrollView

            anchors.fill: parent
            anchors.margins: DS.Theme.spacingMd
            // Run right up to the drag handle so the scroll bar sits at the panel edge instead of
            // floating inward over the controls.
            anchors.rightMargin: _resize.width
            contentWidth: availableWidth
            clip: true

            ScrollBar.vertical: ScrollBar {
                parent: _scrollView
                x: _scrollView.width - width
                y: 0
                height: _scrollView.height
                width: DS.Theme.scrollBarWidth
                policy: ScrollBar.AsNeeded
            }

            Column {
                spacing: DS.Theme.spacingSm
                // Reserve the scroll bar's lane unconditionally. Keying this off the bar's
                // visibility would loop: width drives content height drives visibility.
                width: _scrollView.availableWidth - DS.Theme.scrollBarWidth - DS.Theme.spacingXs

                Label {
                    width: parent.width
                    text: _root.title
                    font: DS.Theme.textSmallBold
                    color: DS.Theme.textPrimary
                    elide: Text.ElideRight
                }

                Repeater {
                    model: _params.params_model
                    delegate: Loader {
                        required property var param
                        required property var component

                        property var lparam: param

                        width: parent.width
                        sourceComponent: component
                        // A setting that does not apply now, such as Vertical Scale while scaling is automatic, is greyed out
                        enabled: param ? param.enabled : true
                        opacity: enabled ? 1.0 : 0.4
                    }
                }
            }
        }

        // Drag-to-resize handle on the right edge, following the pattern of the designer's own
        // settings panel (BoxSettings.qml).
        Rectangle {
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
            width: DS.Theme.borderSm
            color: _resize.pressed ? DS.Theme.primary : DS.Theme.stroke
        }

        MouseArea {
            id: _resize

            anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
            width: DS.Theme.spacingSm
            cursorShape: Qt.SizeHorCursor

            property real mouseToEdgeX: 0

            onPressed: mouse => {
                const global = mapToGlobal(mouse.x, mouse.y)
                mouseToEdgeX = _panel.x + _panel.width - global.x
            }

            onPositionChanged: mouse => {
                if (!pressed) { return }
                const global = mapToGlobal(mouse.x, mouse.y)
                const target = global.x - _panel.x + mouseToEdgeX
                _root.panelWidth = Math.max(_panel.minWidth, Math.min(_panel.maxWidth, target))
            }
        }
    }

    // Hides and shows the panel, to give the plot the whole width
    DS.IconButton {
        id: _toggle

        anchors { top: parent.top; left: _panel.right; topMargin: DS.Theme.spacingSm; leftMargin: DS.Theme.spacingXs }
        iconName: "settings"
        iconSize: DS.Theme.iconLg
        iconColor: _root.shown ? DS.Theme.iconPrimary : DS.Theme.iconSecondary
        tooltip: _root.shown ? qsTr("Hide the settings") : qsTr("Show the settings")
        onClicked: _root.shown = !_root.shown
    }
}
