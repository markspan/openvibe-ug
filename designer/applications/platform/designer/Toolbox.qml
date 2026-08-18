/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import OVControls as OV
import OVParameters as OV
import OVVisualization as OV

import DesignSystem as DS

Control {
  id: _control

  property var scenario: OVDesigner.scenarios[OVDesigner.currentScenarioIndex]

  signal saveScenarioRequested()

  implicitHeight: parent.height
  implicitWidth: DS.Theme.toolBoxWidth

  background: Rectangle {
    color: DS.Theme.white
    border.color: DS.Theme.stroke
  }

  ColumnLayout {
    id: _mainColumn
    anchors.fill: parent

    spacing: DS.Theme.spacingSm

    RowLayout {
      Layout.fillWidth: true
      Layout.topMargin: DS.Theme.spacingSm

      Label {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        Layout.leftMargin: DS.Theme.spacingLg
        Layout.preferredHeight: DS.Theme.labelHeight5xl

        text: scenario.name
        font: DS.Theme.textLargeRegular
        verticalAlignment: Text.AlignVCenter
      }

      DS.IconButton {
        id: _saveButton
        Layout.alignment: Qt.AlignVCenter
        Layout.rightMargin: DS.Theme.spacingLg
        
        Layout.preferredWidth: 40
        Layout.preferredHeight: DS.Theme.labelHeight2xl

        iconName: "save"
        iconColor: DS.Theme.white
        iconSize: DS.Theme.iconLg

        background: Rectangle {
          anchors.fill: parent
          color: _saveButton.hovered ? DS.Theme.white : DS.Theme.primary
          border.color: _saveButton.hovered ? DS.Theme.primary : DS.Theme.white
          radius: DS.Theme.radiusSm
        }

        onClicked: {
          saveScenarioRequested()
        }
      }
    }

    /*TabBar {
      id: _tabBar
      width: parent.width
      TabButton {
        width: 50
        text: qsTr("Boxes")
      }
      TabButton {
        width: 50
        text: qsTr("Scenario Configuration")
      }
      TabButton {
        width: 50
        text: qsTr("Scenario I/O")
      }
    }*/

    RowLayout {
      id: _tabBar
      Layout.fillWidth: true
      Layout.leftMargin: DS.Theme.spacingLg

      property int currentIndex: 0

      Repeater {
        model: [
          { icon: "toolbox-modules", text: "Modules" },
          { icon: "toolbox-config", text: "Configuration" }
        ]

        delegate: Rectangle {
          id: _tab
          required property int index
          required property var modelData

          Layout.preferredWidth: _tabRow.implicitWidth + 2 * DS.Theme.spacingMd
          Layout.preferredHeight: _tabRow.implicitHeight

          RowLayout {
            id: _tabRow
            anchors.fill: parent
            anchors.leftMargin: DS.Theme.spacingMd
            anchors.rightMargin: DS.Theme.spacingMd
            
            DS.Icon {
              Layout.topMargin: DS.Theme.spacingMd
              Layout.bottomMargin: DS.Theme.spacingMd
              name: modelData.icon
              size: DS.Theme.icon2xl
              color: index == _tabBar.currentIndex ? DS.Theme.iconPrimary : DS.Theme.iconDisabled
            }
            Label {
              id: _tabLabel
              Layout.fillHeight: true
              Layout.alignment: Qt.AlignVCenter
              text: modelData.text
              font: DS.Theme.textMediumMedium
              color: index === _tabBar.currentIndex ? DS.Theme.textPrimary : DS.Theme.textDisabled
              verticalAlignment: Text.AlignVCenter

              Rectangle {
                id: _tabUnderline
                anchors.bottom: parent.bottom
                
                height: 1

                width: parent.width
                color: DS.Theme.primary

                transform: Scale {
                  xScale: index === _tabBar.currentIndex ? 1 : 0
                  origin.x: _tabUnderline.width / 2

                  Behavior on xScale {
                    NumberAnimation {
                      duration: 300
                      easing.type: Easing.OutQuad
                    }
                  }
                }
              }
            }
          }
          
          MouseArea {
            anchors.fill: parent
            onClicked: {
              _tabBar.currentIndex = index
            }
          }
          
        }
      }

    }

    DS.HorizontalSpacer {
    }

    StackLayout {
      id: _configStack
      width: parent.width
      Layout.fillHeight: true

      currentIndex: _tabBar.currentIndex
      OV.BoxTreeView {
        boxModel: OVDesigner.boxesTreeModel
      }
      OV.ScenarioConfiguration {
        id: _scenarioConfigTab
        scenario: _control.scenario
      }
    }
  }
}  
