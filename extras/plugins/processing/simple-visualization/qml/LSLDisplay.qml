// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see UG.md 2.28. The LSL display: the selected LSL data streams stacked, each in a panel of its own
// drawn as the Signal display draws, on one time axis, with the markers of the selected marker streams on all of them.

import QtQuick
import QtQuick.Controls
import QtQuick.Window  // Screen, for the traces' layer

import DesignSystem as DS

// The window manager reparents this item and installs width/height bindings on it, so it must not
// anchor itself to its parent.
Item {
    id: _root

    readonly property int ampWidth: dataSource.showAmplitudeRuler ? DS.Theme.spacing5xl : 0
    /// As wide as the longest channel label of any stream, as in the Signal display, so that all panels line up
    readonly property int gutterWidth: {
        let widest = 0
        for (const stream of dataSource.streams) {
            for (const channel of stream.channels) {
                widest = Math.max(widest, _nameMetrics.advanceWidth(channel.name), _unitMetrics.advanceWidth(channel.unit ?? ""))
            }
        }
        const dot = dataSource.butterfly ? DS.Theme.spacingSm + DS.Theme.spacingXs : 0
        return Math.min(2 * DS.Theme.labelWidthSm, Math.ceil(widest + dot) + DS.Theme.spacingMd)
    }
    readonly property int rulerHeight: dataSource.showTimeRuler ? DS.Theme.labelHeightLg : 0
    readonly property int headerHeight: DS.Theme.labelHeightSm

    /// A stream's share of the height: one lane per channel shown, so that lanes are alike across streams; one in Multiview
    function weightOf(stream) { return dataSource.butterfly ? 1 : Math.max(1, stream.visibleChannelCount) }
    readonly property real totalWeight: {
        let total = 0
        for (const stream of dataSource.streams) { total += weightOf(stream) }
        return Math.max(1, total)
    }

    FontMetrics { id: _nameMetrics; font: DS.Theme.textSmallRegular }
    FontMetrics { id: _unitMetrics; font: DS.Theme.textXsRegular }

    SettingsPanel {
        id: _settings

        anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
        parameters: dataSource.parameters
        title: dataSource.name
    }

    Item {
        id: _plot

        anchors { top: parent.top; left: _settings.right; right: parent.right; bottom: parent.bottom }
        anchors.margins: DS.Theme.spacingSm

        /// The trace of the first stream: the time ruler reads the time axis from it, which all streams share
        readonly property var firstTrace: {
            _streams.count  // re-evaluated as panels come and go
            const panel = _streams.count > 0 ? _streams.itemAt(0) : null
            return panel ? panel.trace : null
        }

        // Record: the streams ticked go to the LSL XDF writer connected to the box's output, a file per recording. While
        // recording, the Streams list is locked, so that a file holds one set of streams throughout.
        Item {
            id: _recordBar

            readonly property color recordColor: DS.Theme.iconHoverClose

            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: _record.implicitHeight + DS.Theme.spacingSm

            AbstractButton {
                id: _record

                anchors { right: parent.right; top: parent.top }
                enabled: dataSource.recording || dataSource.canRecord
                text: dataSource.recording ? qsTr("Stop recording  %1").arg(dataSource.recordingTime) : qsTr("Record")
                implicitHeight: _recordLabel.implicitHeight + 2 * DS.Theme.spacingXs
                implicitWidth: _recordDot.width + _recordLabel.implicitWidth + 3 * DS.Theme.spacingSm
                onClicked: dataSource.toggleRecording()

                HoverHandler { cursorShape: Qt.PointingHandCursor }
                ToolTip.visible: hovered
                ToolTip.text: dataSource.recording ? qsTr("Stops the recording and closes its file")
                                                   : qsTr("Records the streams ticked to a new XDF file, through the LSL XDF writer connected to this box's output")

                background: Rectangle {
                    radius: DS.Theme.radiusXs
                    border.width: DS.Theme.borderSm
                    border.color: _record.enabled ? _recordBar.recordColor : DS.Theme.iconDisabled
                    color: dataSource.recording ? Qt.rgba(_recordBar.recordColor.r, _recordBar.recordColor.g, _recordBar.recordColor.b, 0.15)
                         : _record.hovered ? Qt.rgba(_recordBar.recordColor.r, _recordBar.recordColor.g, _recordBar.recordColor.b, 0.08)
                         : DS.Theme.white
                }

                contentItem: Item {
                    // A round dot to record, a square while recording, as recorders show them
                    Rectangle {
                        id: _recordDot
                        anchors { left: parent.left; leftMargin: DS.Theme.spacingSm; verticalCenter: parent.verticalCenter }
                        width: DS.Theme.spacingSm + DS.Theme.spacingXs
                        height: width
                        radius: dataSource.recording ? DS.Theme.radiusXs / 2 : width / 2
                        color: _record.enabled ? _recordBar.recordColor : DS.Theme.iconDisabled
                    }
                    Label {
                        id: _recordLabel
                        anchors { left: _recordDot.right; leftMargin: DS.Theme.spacingSm; verticalCenter: parent.verticalCenter }
                        text: _record.text
                        font: DS.Theme.textSmallBold
                        color: _record.enabled ? _recordBar.recordColor : DS.Theme.iconDisabled
                    }
                }
            }
        }

        Column {
            id: _column

            anchors { top: _recordBar.bottom; left: parent.left; right: parent.right; bottom: _time.top }

            Repeater {
                id: _streams
                model: dataSource.streams

                delegate: Item {
                    id: _stream

                    required property var modelData
                    readonly property alias trace: _trace

                    width: _column.width
                    height: _root.headerHeight + Math.max(0, _column.height - _streams.count * _root.headerHeight) * _root.weightOf(modelData) / _root.totalWeight

                    Component.onCompleted: modelData.attachTrace(_trace)

                    Label {
                        id: _header

                        anchors { top: parent.top; left: parent.left; right: parent.right }
                        anchors.leftMargin: _root.ampWidth
                        height: _root.headerHeight
                        verticalAlignment: Text.AlignVCenter
                        // Its delay: where its cursor is, left of now
                        text: _stream.modelData.label + (_stream.modelData.delay !== "" ? "  ·  " + _stream.modelData.delay : "")
                        font: DS.Theme.textSmallBold
                        color: DS.Theme.textSecondary
                        elide: Text.ElideRight
                    }

                    Item {
                        id: _content

                        anchors { top: _header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }

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
                            channels: _stream.modelData.channels
                            butterfly: dataSource.butterfly
                        }

                        SignalTrace {
                            id: _trace

                            anchors { top: parent.top; left: _gutter.right; right: parent.right; bottom: parent.bottom }
                            clip: true
                            traceColor: DS.Theme.textPrimary

                            // Antialiased lines, as in the Signal display
                            layer.enabled: Math.max(width, height) * Screen.devicePixelRatio <= 8192
                            layer.samples: 4
                            layer.smooth: true
                        }

                        // Sweep cursor, Scan mode only
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

                        // Between panels
                        Rectangle {
                            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                            height: DS.Theme.borderSm
                            color: DS.Theme.stroke
                        }
                    }
                }
            }
        }

        Loader {
            id: _time

            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.leftMargin: _root.ampWidth + _root.gutterWidth
            height: _root.rulerHeight
            active: dataSource.showTimeRuler && _plot.firstTrace !== null
            sourceComponent: TimeRuler { trace: _plot.firstTrace }
        }

        Label {
            anchors.centerIn: parent
            width: parent.width * 0.8
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: _streams.count === 0
            text: dataSource.hasMarkerStreams ? qsTr("Markers are being read. Tick a data stream under Streams to see them on it.")
                                              : qsTr("Tick the streams to show under Streams, in the panel on the left.")
            font: DS.Theme.textMediumRegular
            color: DS.Theme.textTertiary
        }
    }
}
