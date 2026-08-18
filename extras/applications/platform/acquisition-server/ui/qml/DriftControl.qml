// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVControls as OV
import DesignSystem as DS

import AcquisitionServer as OV

Control {
    id: _control

    property bool collapsed: false
    property real instantDrift: 0.0
    property real globalDrift: 0.0
    property real driftLimit: 1.0

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
                    text: "Drift Control"
                    font: DS.Theme.subtitle3
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                DS.IconButton {
                    id: expandCollapseButton
                    iconName: _control.collapsed ? "chevron-down" : "chevron-up"

                    onClicked: {
                        _control.collapsed = !_control.collapsed
                    }
                }
            }

            TextMetrics {
                id: _driftValueMetrics
                font: DS.Theme.textSmallLight
                text: "-99999.999 ms"
            }

            RowLayout {
                // Instant drift control
                Layout.fillWidth: true
                spacing: DS.Theme.spacingLg

                visible: !_control.collapsed

                Text {
                    text: "Instant Drift"
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }

                OV.DriftBar {
                    id: _instantDriftBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: DS.Theme.labelHeightXs
                    Layout.alignment: Qt.AlignHCenter

                    value: _control.instantDrift / _control.driftLimit
                }

                Text {
                    text: _control.instantDrift.toFixed(3) + " ms"
                    font: DS.Theme.textSmallLight
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: _driftValueMetrics.width
                    horizontalAlignment: Text.AlignRight
                }
            }

            RowLayout {
                // Global drift control
                Layout.fillWidth: true
                spacing: DS.Theme.spacingLg

                visible: !_control.collapsed

                Text {
                    text: "Global Drift"
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }

                OV.DriftBar {
                    id: _globalDriftBar
                    Layout.fillWidth: true
                    Layout.preferredHeight: DS.Theme.labelHeightXs
                    Layout.alignment: Qt.AlignHCenter

                    value: _control.globalDrift / _control.driftLimit
                }

                Text {
                    text: _control.globalDrift.toFixed(3) + " ms"
                    font: DS.Theme.textSmallLight
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: _driftValueMetrics.width
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }
}
