// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only


import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import DesignSystem as DS
import '.'

Rectangle {
    
    implicitWidth: parent.width
    height: DS.Theme.navBarHeight

    color: DS.Theme.navBarBackground

    RowLayout {
        anchors.fill: parent
        spacing: 0

        DS.IconButton {
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
            iconName: "home"
            iconSize: 16
            iconPressedSize: 12

            onClicked: {
                console.log("Home clicked!")
            }
        }

        DS.VerticalSpacer {}
        
        // Scenario "TabBar" section
        Repeater {
            model: OVDesigner.scenarios
            delegate: Rectangle {
                id: _scenarioTab
                required property int index
                required property var modelData
                
                Layout.fillHeight: true
                Layout.minimumWidth: 120
                Layout.maximumWidth: 300

                color: index === OVDesigner.currentScenarioIndex ? DS.Theme.white :  DS.Theme.navBarBackground

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        OVDesigner.currentScenarioIndex = index
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.rightMargin: DS.Theme.spacingLg
                    Layout.leftMargin: DS.Theme.spacingLg
                    spacing: DS.Theme.spacingLg

                    Label {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.name
                        font: DS.Theme.textSmallRegular
                        color: DS.Theme.textPrimary
                    }

                    DS.IconButton {
                        Layout.alignment: Qt.AlignHCenter
                        iconName: "close"
                        iconSize: DS.Theme.iconXs
                        iconColor: DS.Theme.iconSecondary
                        iconHoverColor: DS.Theme.iconHoverClose

                        onClicked: {
                            OVDesigner.removeScenario(index)
                        }
                    }
                    }
                    DS.VerticalSpacer {}
                }
            }
        }

        // Add Scenario button
        DS.IconButton {
            Layout.fillHeight: true
            Layout.preferredWidth: height
            
            iconName: "add"
            iconSize: DS.Theme.iconMd
            iconColor: DS.Theme.iconPrimary
            iconPressedSize: DS.Theme.iconSm

            onClicked: {
                OVDesigner.createScenario()
            }
        }

        Item {
            id: _dragSpacer
            Layout.fillWidth: true
            Layout.fillHeight: true

            MouseArea {
            anchors.fill: parent
            onPressed: {
                window.startSystemMove()
            }
            onDoubleClicked: {
                _resizeIconButton.clicked()
            }
            }
        }

        DS.VerticalSpacer {}

        SettingsMenu {
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
        }

        DS.IconButton {
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
            iconName: "minimize"
            iconSize: DS.Theme.iconMd
            iconPressedSize: DS.Theme.iconSm

            onClicked: {
            window.showMinimized()
            }
        }

        DS.IconButton {
            id: _resizeIconButton
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
            iconName: window.visibility === Window.Maximized ? "resize-down" : "resize-up"
            iconSize: DS.Theme.iconMd
            iconPressedSize: DS.Theme.iconSm

            onClicked: {
                if (window.visibility === Window.Maximized) {
                    window.showNormal()
                } else {
                    window.showMaximized()
                }
            }
        }

        DS.IconButton {
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
            iconName: "close-main"
            iconSize: DS.Theme.iconMd
            iconPressedSize: DS.Theme.iconSm
            iconHoverColor: DS.Theme.iconHoverClose
            iconPressedColor: DS.Theme.iconHoverClose // Unchanged from hover

            onClicked: {
            window.close()
            }
        }
    }
}

