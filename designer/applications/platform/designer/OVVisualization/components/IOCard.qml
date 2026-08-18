// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS
import ".."

Item {
    id: root

    // properties
    required property var ioObject
    required property string name
    required property string dataType
    required property string ioType
    required property var selectionModel
    property bool deletable: true

    signal remove()

    Layout.fillWidth: true
    implicitHeight: content.implicitHeight

    Rectangle {
        id: content
        anchors.fill: parent

        radius: DS.Theme.radiusSm
        border.width: DS.Theme.borderSm
        border.color: DS.Theme.stroke
        
        implicitHeight: _mainLayout.implicitHeight + 2*DS.Theme.spacingSm // accomodate _mainLayout content + margins

        ColumnLayout {
            id: _mainLayout
            anchors.fill: parent
            anchors.margins: DS.Theme.spacingSm

            spacing: DS.Theme.spacingSm

            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: DS.Theme.spacingSm

                Text {
                    text: "Type"
                    font: DS.Theme.textMediumRegular
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }

                DS.VerticalSpacer {}

                Text {
                    text: root.ioType + " name"
                    font: DS.Theme.textMediumRegular
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }
            }
            
            Rectangle {
                Layout.fillWidth: true

                implicitHeight: _inputLayout.implicitHeight + 2*DS.Theme.spacingXs // accomodate _inputLayout content + margins

                RowLayout {
                    id: _inputLayout
                    anchors.fill: parent
                    anchors.topMargin: DS.Theme.spacingXs
                    anchors.bottomMargin: DS.Theme.spacingXs
                    anchors.leftMargin: DS.Theme.spacingXs
                    anchors.rightMargin: DS.Theme.spacingLg

                    spacing: DS.Theme.spacingSm

                    IOSelectPopup {
                        title: root.ioType + " Type"
                        model: root.selectionModel
                        currentSelection: root.dataType
                        onStreamSelected: (streamType) => {
                            ioObject.dataType = streamType
                        }
                    }

                    DS.TextField {
                        id: _nameTextInput
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        text: root.name

                        onEditingFinished: {
                            ioObject.name = text
                        }
                    }

                    DS.IconButton {
                        iconName: "trash"
                        iconSize: DS.Theme.spacingLg
                        iconHoverColor: "red"
                        enabled: root.deletable

                        onClicked: {
                            root.remove()
                        }
                    }
                }
            }
        }
    }
}

