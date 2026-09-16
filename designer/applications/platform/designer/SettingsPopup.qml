//SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
//SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS


Popup {
    id: root
    
    required property var model

    property var parentPopup: null
    property var menuWithActiveSubmenu: null

    width: contentLayout.implicitWidth
    height: contentLayout.implicitHeight + 2 * DS.Theme.spacingMd  // accomodate vertical padding

    horizontalPadding: 0
    verticalPadding: DS.Theme.spacingMd

    background: Rectangle {
      color: DS.Theme.white
      border.color: DS.Theme.borderPrimary
      border.width: DS.Theme.borderSm
      radius: DS.Theme.radiusLg
      
     // layer.enabled: true
      //layer.samples: 8
    }

    ColumnLayout {
      id: contentLayout
      anchors.fill: parent
      spacing: 0
  
      Repeater {
        model: root.model

        delegate: Rectangle {
          id: _menuItem
          Layout.fillWidth: true
          Layout.preferredHeight: DS.Theme.menuItemHeight
          Layout.minimumWidth: _menuItemBackground.implicitWidth + 2 * DS.Theme.spacingSm + DS.Theme.spacing3xl // accomodate contentLayout content + margins + extra space for design
          color: "transparent"

          required property var modelData
          // Entries without an explicit flag are enabled
          readonly property bool itemEnabled: _menuItem.modelData.enabled !== false
          
          property var activeSubmenu: null
          property var hoverCanceled: false  // allows removing hover effect when moving to another item until the menu gets closed

          Rectangle {
            id: _menuItemBackground
            anchors.fill: parent
            anchors.leftMargin: DS.Theme.spacingSm
            anchors.rightMargin: DS.Theme.spacingSm
            
            implicitWidth: _menuItemLayout.implicitWidth + DS.Theme.spacingMd + DS.Theme.spacingSm
            implicitHeight: _menuItemLayout.implicitHeight

            radius: DS.Theme.radiusSm

            color: _menuItem.itemEnabled && (_menuItemMouseArea.containsMouse || (_menuItem.activeSubmenu && !_menuItem.hoverCanceled )) ? DS.Theme.iconHoverSecondary : "transparent"

            MouseArea {
              id: _menuItemMouseArea
              anchors.fill: parent
              hoverEnabled: true
              enabled: _menuItem.itemEnabled
              onEntered: {
                if (root.menuWithActiveSubmenu && root.menuWithActiveSubmenu !== _menuItem) {
                  root.menuWithActiveSubmenu.hoverCanceled = true
                }
                _menuItem.hoverCanceled = false
                _hideTimer.stop()
                _triggerTimer.start()
              }
              onExited: {
                _triggerTimer.stop()
              }
              onClicked: {
                if (_menuItem.modelData.action) {
                  _menuItem.modelData.action()
                  root.close()
                  root.parentPopup?.close()
                } else {
                  openSubmenu()
                }
              }
            }

            Timer {
              id: _triggerTimer
              interval: 300
              onTriggered: {
                if (_menuItem.modelData.submenu) {
                  openSubmenu()
                } else {
                  closeOtherSubmenu() 
                }
              }
            }

            Timer {
              id: _hideTimer
              interval: 100
              onTriggered: {
                if (_menuItem.activeSubmenu) {
                  _menuItem.activeSubmenu.close()
                }
              }
            }

            RowLayout {
              id: _menuItemLayout
              anchors.fill: parent
              anchors.leftMargin: DS.Theme.spacingMd
              anchors.rightMargin: DS.Theme.spacingSm
              spacing: DS.Theme.spacingMd

              DS.Icon {
                name: _menuItem.modelData.icon ?? ""
                size: DS.Theme.iconLg
                color: _menuItem.itemEnabled ? DS.Theme.iconPrimary : DS.Theme.iconDisabled
                visible: _menuItem.modelData.icon ? true : false
              }

              Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: _menuItem.modelData.text
                font: DS.Theme.textSmallRegular
                color: _menuItem.itemEnabled ? DS.Theme.textPrimary : DS.Theme.textDisabled
              }

              Text {
                Layout.alignment: Qt.AlignVCenter
                text: _menuItem.modelData.shortcut ?? ""
                font: DS.Theme.textXsRegular
                color: DS.Theme.textTertiary
                visible: _menuItem.modelData.shortcut ? true : false
              }

              DS.Icon {
                name: "chevron-right"
                size: DS.Theme.iconXl
                color: DS.Theme.iconPrimary
                visible: _menuItem.modelData.submenu ? true : false
              }
            }
          }

          function openSubmenu() {
            closeOtherSubmenu()
            var submenuComponent = Qt.createComponent("SettingsPopup.qml")
            if (submenuComponent.status === Component.Ready) {
              activeSubmenu = submenuComponent.createObject(root.parent, {
                model: modelData.submenu,
              })

              root.menuWithActiveSubmenu = _menuItem
              
              activeSubmenu.parentPopup = root
              activeSubmenu.x = root.x - activeSubmenu.width
              activeSubmenu.y = root.y + _menuItem.y
              activeSubmenu.closed.connect(function() {
                activeSubmenu.destroy()
                activeSubmenu = null
              })

              activeSubmenu.open()
            } else {
              console.log("Failed to create submenu component: " + submenuComponent.errorString())
            }
            
          }

          function closeOtherSubmenu() {
            if (root.menuWithActiveSubmenu && root.menuWithActiveSubmenu.activeSubmenu) {
              root.menuWithActiveSubmenu.activeSubmenu.close()
            }
          }
        }
      }
    }
  }