// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import OVParameters as OVP
import DesignSystem as DS

// The window manager reparents this item and installs width/height bindings on it, so it must not
// anchor itself to its parent.
Item {
    id: _root

    readonly property int ampWidth: dataSource.showAmplitudeRuler ? DS.Theme.spacing5xl : 0
    readonly property int gutterWidth: DS.Theme.labelWidthSm
    readonly property int rulerHeight: dataSource.showTimeRuler ? DS.Theme.labelHeightLg : 0

    // Fit every lane in the viewport by default; above the requested density, scroll instead.
    // Butterfly puts every channel on one shared baseline, so paging has nothing to page through.
    readonly property real contentHeight: {
        const perPage = dataSource.channelsPerPage
        const count = dataSource.visibleChannelCount
        if (!dataSource.butterfly && perPage > 0 && count > perPage) { return _scroll.height * count / perPage }
        return _scroll.height
    }

    Rectangle {
        id: _panel

        readonly property int minWidth: 200
        readonly property int maxWidth: DS.Theme.boxSettingsMaximumWidth

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        
        // A plain value, not a binding: the drag handle assigns it, which would destroy a binding.
        width: DS.Theme.boxSettingsMinimumWidth

        color: DS.Theme.white

        OVP.Parameters {
            id: _params
            parameters: dataSource.parameters
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
                    text: dataSource.name
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
                    }
                }
            }
        }

        // Drag-to-resize handle on the right edge, following the pattern of the designer's own
        // settings panel (BoxSettings.qml).
        Rectangle {
            id: _edge

            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

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
                _panel.width = Math.max(_panel.minWidth, Math.min(_panel.maxWidth, target))
            }
        }
    }

    Item {
        id: _plot

        anchors { top: parent.top; left: _panel.right; right: parent.right; bottom: parent.bottom }
        anchors.margins: DS.Theme.spacingSm

        Flickable {
            id: _scroll

            anchors { top: parent.top; left: parent.left; right: parent.right; bottom: _time.top }
            contentWidth: width
            contentHeight: _root.contentHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ScrollBar.vertical: ScrollBar { policy: _scroll.contentHeight > _scroll.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }

            Item {
                id: _content

                width: _scroll.width
                height: _root.contentHeight

                AmplitudeScaleBar {
                    id: _amp

                    anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
                    width: _root.ampWidth
                    visible: dataSource.showAmplitudeRuler
                    trace: _trace
                    butterfly: dataSource.butterfly
                }

                ChannelGutter {
                    id: _gutter

                    anchors { top: parent.top; left: _amp.right; bottom: parent.bottom }
                    width: _root.gutterWidth
                    trace: _trace
                    channels: dataSource.channels
                    butterfly: dataSource.butterfly
                }

                SignalTrace {
                    id: _trace

                    // The controller finds this item by name to drive it from C++.
                    objectName: "signalTrace"

                    anchors { top: parent.top; left: _gutter.right; right: parent.right; bottom: parent.bottom }
                    clip: true
                    traceColor: DS.Theme.textPrimary
                }

                // Sweep cursor, Scan mode only.
                Rectangle {
                    visible: _trace.cursorX >= 0
                    x: _trace.x + _trace.cursorX
                    y: _trace.y
                    width: DS.Theme.borderMd
                    height: _trace.height
                    color: DS.Theme.primary
                    opacity: 0.7
                }

                StimulationOverlay {
                    anchors.fill: _trace
                    trace: _trace
                    stimulations: dataSource.stimulations
                }
            }
        }

        TimeRuler {
            id: _time

            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.leftMargin: _root.ampWidth + _root.gutterWidth
            height: _root.rulerHeight
            visible: dataSource.showTimeRuler
            trace: _trace
        }

        Label {
            anchors.centerIn: parent
            visible: !dataSource.hasData
            text: qsTr("Waiting for data...")
            font: DS.Theme.textMediumRegular
            color: DS.Theme.textTertiary
        }
    }
}
