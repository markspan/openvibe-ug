// UG fork: new file, not in upstream OpenViBE, which shows the drift here (DriftControl.qml).
//
// How full the buffers between the amplifier and the clients get during an acquisition. The server's buffers fill
// when the clients (Designer) take the data more slowly than it comes; once they are all taken, the server stops
// reading the amplifier, whose own buffer then fills. Samples are lost only when that buffer overflows. Each value is
// the worst since the previous update (twice a second); the one in brackets is the worst since Play.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS

import AcquisitionServer as OV

Control {
    id: _control

    property bool collapsed: false
    /// AcquisitionController.buffers
    property var buffers: ({})

    readonly property bool _acquiring: buffers.acquiring === true
    readonly property bool _deviceReported: _acquiring && buffers.deviceReported === true
    readonly property bool _deviceSizeKnown: _deviceReported && buffers.deviceCapacity > 0
    readonly property bool _lossCounted: _deviceReported && buffers.lossCounted === true
    readonly property bool _lost: _lossCounted && buffers.lostSamples > 0

    function _percent(seconds) {
        return Math.round(100 * seconds / buffers.deviceCapacity) + " %"
    }

    function _duration(seconds) {
        return seconds < 1 ? Math.round(1000 * seconds) + " ms" : seconds.toFixed(1) + " s"
    }

    implicitHeight: _mainContainer.height

    Rectangle {
        id: _mainContainer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        color: DS.Theme.white
        border.color: DS.Theme.stroke
        radius: DS.Theme.radiusSm

        height: (_control.collapsed ? _headerRow.implicitHeight : _mainLayout.implicitHeight) + 2 * DS.Theme.spacingMd

        ColumnLayout {
            id: _mainLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: DS.Theme.spacingMd
            anchors.rightMargin: DS.Theme.spacingMd
            anchors.topMargin: DS.Theme.spacingMd

            spacing: DS.Theme.spacingLg

            RowLayout {
                id: _headerRow
                Layout.fillWidth: true

                Text {
                    text: "Buffers"
                    font: DS.Theme.subtitle3
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    // What matters most, also when collapsed
                    visible: _control._lost
                    text: Math.round(_control.buffers.lostSamples) + " samples lost"
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.cpuUsageColors["high"]
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: DS.Theme.spacingLg
                }

                Item {
                    Layout.fillWidth: true
                }

                DS.IconButton {
                    iconName: _control.collapsed ? "chevron-down" : "chevron-up"

                    onClicked: {
                        _control.collapsed = !_control.collapsed
                    }
                }
            }

            TextMetrics {
                id: _labelMetrics
                font: DS.Theme.textSmallRegular
                text: "Amplifier buffer"
            }

            TextMetrics {
                id: _valueMetrics
                font: DS.Theme.textSmallLight
                text: "100 % of 10 s (max 100 %)"
            }

            GridLayout {
                Layout.fillWidth: true
                visible: !_control.collapsed
                columns: 3
                columnSpacing: DS.Theme.spacingLg
                rowSpacing: DS.Theme.spacingLg

                // The amplifier's own buffer: the last margin before samples are lost
                Text {
                    text: "Amplifier buffer"
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    Layout.preferredWidth: _labelMetrics.width
                    Layout.alignment: Qt.AlignVCenter
                }

                OV.LevelBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: DS.Theme.labelHeightXs
                    Layout.alignment: Qt.AlignVCenter
                    // Without a known size there is nothing to fill; the text says how much waited
                    opacity: !_control._acquiring || _control._deviceSizeKnown ? 1 : 0
                    value: _control._deviceSizeKnown ? _control.buffers.deviceSeconds / _control.buffers.deviceCapacity : 0
                    // Any backlog worth seeing is early warning: a whole second in a 10 s buffer is orange
                    warning: 0.1
                    danger: 0.5
                }

                Text {
                    text: !_control._acquiring ? "–"
                        : !_control._deviceReported ? "not reported by this driver"
                        : _control._deviceSizeKnown ? _control._percent(_control.buffers.deviceSeconds) + " of " + _control._duration(_control.buffers.deviceCapacity)
                                                      + " (max " + _control._percent(_control.buffers.deviceMaxSeconds) + ")"
                        : _control._duration(_control.buffers.deviceSeconds) + " waiting (max " + _control._duration(_control.buffers.deviceMaxSeconds) + ")"
                    font: DS.Theme.textSmallLight
                    color: DS.Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.preferredWidth: _valueMetrics.width
                    Layout.alignment: Qt.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }

                // The server's pool of buffers, between the reader thread and the clients
                Text {
                    text: "Server buffers"
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    Layout.preferredWidth: _labelMetrics.width
                    Layout.alignment: Qt.AlignVCenter
                }

                OV.LevelBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: DS.Theme.labelHeightXs
                    Layout.alignment: Qt.AlignVCenter
                    // The reader and the processing thread each hold one, so two in use is the normal state
                    value: _control._acquiring ? 1 - _control.buffers.serverFree / _control.buffers.serverBuffers : 0
                    warning: 0.5
                    danger: 0.9
                }

                Text {
                    text: _control._acquiring ? _control.buffers.serverFree + " of " + _control.buffers.serverBuffers + " free (min "
                                                + _control.buffers.serverMinFree + ")"
                                              : "–"
                    font: DS.Theme.textSmallLight
                    color: DS.Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.preferredWidth: _valueMetrics.width
                    Layout.alignment: Qt.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                }

                // Lost samples, as far as the driver can count them
                Text {
                    text: "Lost samples"
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    Layout.preferredWidth: _labelMetrics.width
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    Layout.columnSpan: 2
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignRight
                    text: !_control._acquiring ? "–"
                        : !_control._lossCounted ? "not counted by this driver"
                        : _control._lost ? Math.round(_control.buffers.lostSamples) + " samples lost"
                        : "none"
                    font: _control._lost ? DS.Theme.textSmallRegular : DS.Theme.textSmallLight
                    color: _control._lost ? DS.Theme.cpuUsageColors["high"] : DS.Theme.textPrimary
                }
            }
        }
    }
}
