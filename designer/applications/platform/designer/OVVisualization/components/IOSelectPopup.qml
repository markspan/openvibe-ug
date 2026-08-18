// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import DesignSystem as DS
import ".."


Item {
    id: root
    
    property string title: "Select..."
    property var model: []
    //current selection
    property string currentSelection: ""


    Layout.preferredHeight: DS.Theme.spacing2xl
    Layout.preferredWidth: DS.Theme.spacing2xl

    signal streamSelected(string streamType)
 
    Rectangle {
        id: popupIcon
        anchors.fill: parent
        color: StreamColorMapper.getStreamColor(root.currentSelection)
        radius: DS.Theme.radiusXs
        opacity: 1.0

        MouseArea {
            id: _mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                // popup positionning is a bit hacky. Would be nice to place it against the main pane border.
                selectionPopup.x = popupIcon.x - DS.Theme.spacing3xl - selectionPopup.width
                selectionPopup.y = popupIcon.y - DS.Theme.spacing3xl
                selectionPopup.open()
            }
        }

        DS.ToolTip {
            text: root.currentSelection
            visible: _mouseArea.containsMouse
            delay: 500
        }
    }

    DS.Popup {
        id: selectionPopup

        title: root.title

        ColumnLayout {
            Layout.fillWidth: true
            spacing: DS.Theme.spacingSm

            Repeater {
                model: root.model

                delegate: Rectangle {
                    Layout.fillWidth: true
                    width: _rowLayout.implicitWidth
                    height: _rowLayout.implicitHeight

                    required property var modelData
                    property bool isSelected: modelData === root.currentSelection
                    property bool isHovered: false

                    RowLayout {
                        id: _rowLayout
                        anchors.fill: parent
                        spacing: DS.Theme.spacingSm

                        Rectangle {
                            Layout.preferredWidth: DS.Theme.spacing2xl
                            Layout.preferredHeight: DS.Theme.spacing2xl
                            Layout.alignment: Qt.AlignVCenter
                            color: StreamColorMapper.getStreamColor(modelData)
                            radius: DS.Theme.radiusXs
                            opacity: 1.0
                        }
                        
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.minimumHeight: DS.Theme.labelHeightXl
                            Layout.minimumWidth: label.implicitWidth + 2 * DS.Theme.spacingSm
                            
                            radius: DS.Theme.radiusXs
                            color: {
                                if (isSelected) {
                                    return DS.Theme.primary
                                } else if (isHovered) {
                                    return DS.Theme.iconHoverSecondary
                                }
                                return "transparent"
                            }
                            Label {
                                id: label
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: DS.Theme.spacingSm
                                anchors.rightMargin: DS.Theme.spacingSm
                                
                                text: modelData
                                font: DS.Theme.textMediumRegular
                                lineHeight: DS.Theme.textMdLineHeight
                                lineHeightMode: Text.FixedHeight
                                color: {
                                    if (isSelected) {
                                        return DS.Theme.white
                                    }
                                    return DS.Theme.textPrimary
                                }
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.streamSelected(modelData)
                            selectionPopup.close()
                        }
                        hoverEnabled: true
                        onEntered: isHovered = true
                        onExited: isHovered = false
                    }
                }
            }
        }
    }
}