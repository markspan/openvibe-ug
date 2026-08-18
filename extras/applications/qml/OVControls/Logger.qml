// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS
import "." as OV

Control {
  id: _control

  property bool collapsed: false
  property real _expandedHeight

  NumberAnimation {
    id: _collapseAnimation
    target: _control
    property: "height"
    duration: 200
    easing.type: Easing.OutQuad
  }

  readonly property var logLevels: [
    { name: "Debug",   key: "debug",   prop: "displayDebug",   countProp: "debugCount",   color: DS.Theme.logLevelsColors.debug },
    { name: "Info",    key: "info",    prop: "displayInfo",    countProp: "infoCount",    color: DS.Theme.logLevelsColors.info },
    { name: "Warning", key: "warning", prop: "displayWarning", countProp: "warningCount", color: DS.Theme.logLevelsColors.warning },
    { name: "Error",   key: "error",   prop: "displayError",   countProp: "errorCount",   color: DS.Theme.logLevelsColors.error },
  ]

  implicitHeight: DS.Theme.loggerMinimumHeight
  implicitWidth: parent.width

  padding: 0

  background: Rectangle {
    color: DS.Theme.white
  }

  contentItem: ColumnLayout {
    spacing: 0

    // Header
    Item {
      id: _header
      Layout.fillWidth: true
      Layout.preferredHeight: DS.Theme.loggerHeaderHeight

      DS.HorizontalSpacer {} // Top line of component

      // Header background
      Rectangle {
        anchors.fill: parent
        color: DS.Theme.quaternary
      }

      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: DS.Theme.spacingLg
        anchors.rightMargin: DS.Theme.spacingLg
        spacing: 0

        // Tab bar
        RowLayout {
          id: _tabBar

          property int currentIndex: 0

          Layout.fillHeight: true

          spacing: DS.Theme.spacingLg

          Repeater {
            model: ["Logs"/*, "Terminal"*/]

            delegate: Rectangle {
              id: _tab
              required property int index
              required property var modelData

              Layout.fillHeight: true
              Layout.preferredWidth: _tabLabel.implicitWidth

              color: "transparent"

              Label {
                id: _tabLabel
                anchors.fill: parent
                anchors.topMargin: DS.Theme.spacingXs
                anchors.bottomMargin: DS.Theme.spacingXs

                text: modelData
                font: DS.Theme.textSmallRegular
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

              MouseArea {
                anchors.fill: parent
                onClicked: {
                  _tabBar.currentIndex = index
                }
              }
            }
          }
        }

        Item { Layout.fillWidth: true }

        // ─── Toolbar Actions ────────────────────────────
        RowLayout {
          spacing: DS.Theme.spacingXl
          DS.IconButton {
            id: clearButton
            iconName: "clear"
            iconSize: 16
            tooltip: "Clear logs"

            onClicked: {
              OV.OVLogController.clear()
            }
          }
          DS.IconButton {
            id: filterButton
            iconName: "filter"
            iconSize: 16
            tooltip: "Filter logs"
            onClicked: {
              _filterPopup.open()
            }
          }

          DS.IconButton {
            id: collapseButton
            iconName: _control.collapsed ? "chevron-up" : "chevron-down"
            iconSize: 16
            tooltip: _control.collapsed ? "Expand" : "Collapse"
            onClicked: {
              if (_control.collapsed) {
                _collapseAnimation.to = _control._expandedHeight
                _collapseAnimation.start()
                _control.collapsed = false
              } else {
                _control._expandedHeight = _control.height
                _collapseAnimation.to = DS.Theme.loggerHeaderHeight
                _collapseAnimation.start()
                _control.collapsed = true
              }
            }
          }
        }
      }
    }

    // ─── Tab Content ────────────────────────────────────────
    StackLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      currentIndex: _tabBar.currentIndex

      // Tab 0: Problems (log entries)
      ListView {
        id: _logListView
        clip: true
        model: OV.OVLogController.items

        spacing: DS.Theme.spacingXs

        delegate: Item {
          width: _logListView.width
          height: _entryRow.implicitHeight

          property color _badgeColor: DS.Theme.logLevelsColors[level] ?? DS.Theme.quaternary

          RowLayout {
            id: _entryRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: DS.Theme.spacingMd
            anchors.rightMargin: DS.Theme.spacingMd
            spacing: DS.Theme.spacingSm

            // Level badge
            Rectangle {
              Layout.preferredWidth: _badgeLabel.implicitWidth + 2 * DS.Theme.spacingSm
              Layout.preferredHeight: _badgeLabel.implicitHeight + 2 * DS.Theme.spacingXs
              Layout.alignment: Qt.AlignVCenter
              radius: DS.Theme.radiusXs
              color: _badgeColor

              Label {
                id: _badgeLabel
                anchors.centerIn: parent
                text: "[ " + level.charAt(0).toUpperCase() + level.slice(1) + " ]"
                font: DS.Theme.textXsBold
                color: DS.Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
              }
            }

            /* @todo Time is already added in the text for runtime logs. Would be nice to move it here for better reading.
            Label {
              Layout.alignment: Qt.AlignVCenter
              text: time
              font: DS.Theme.textXsRegular
              color: DS.Theme.textSecondary
            }*/

            // Message text
            Label {
              Layout.fillWidth: true
              Layout.alignment: Qt.AlignVCenter
              text: message
              font: DS.Theme.textSmallRegular
              color: DS.Theme.textPrimary
              elide: Text.ElideRight
            }
          }
        }

        ScrollBar.vertical: ScrollBar {
          visible: _logListView.contentHeight > _logListView.height
        }

        onCountChanged: {
            Qt.callLater(positionViewAtEnd)
        }
      }

      // Tab 1: Terminal (placeholder)
      Item {
        // TODO: Implement terminal content
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  /// Resize handle (top edge)
  ////////////////////////////////////////////////////////////////////////////
  Rectangle {
    id: _separator
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.top: parent.top
    height: 1
    color: resizeHandle.pressed ? DS.Theme.primary : DS.Theme.stroke
  }

  MouseArea {
    id: resizeHandle
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.top: parent.top
    property real mouseToEdgeY

    height: 8
    enabled: !_control.collapsed
    cursorShape: _control.collapsed ? Qt.ArrowCursor : Qt.SizeVerCursor

    onPressed: (mouse) => {
        var globalMouse = mapToGlobal(mouse.x, mouse.y)
        mouseToEdgeY = _control.y - globalMouse.y
    }
    onReleased: (mouse) => {
        _separator.color = DS.Theme.stroke
    }

    onPositionChanged: (mouse) => {
        if (pressed) {
            var globalMouse = mapToGlobal(mouse.x, mouse.y)
            var newHeight = _control.height - (globalMouse.y - _control.y + mouseToEdgeY)

            if (newHeight >= DS.Theme.loggerMinimumHeight && newHeight <= DS.Theme.loggerMaximumHeight) {
              _control.height = newHeight
            }
        }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  /// Filter popup
  ////////////////////////////////////////////////////////////////////////////
  DS.Popup {
    id: _filterPopup
    title: "Filter Logs"

    parent: filterButton
    x: -width + filterButton.width
    y: filterButton.height

    ColumnLayout {
      Layout.fillWidth: true

      spacing: DS.Theme.spacingMd

      Repeater {
        model: _control.logLevels

        delegate: Rectangle {
          implicitHeight: _logLevelRow.implicitHeight + DS.Theme.spacingMd
          implicitWidth: _logLevelRow.implicitWidth + 2 * DS.Theme.spacingLg

          Layout.fillWidth: true

          radius: DS.Theme.radiusXs
          color: OV.OVLogController[modelData.prop] ? modelData.color : "transparent"
          border.color: _mouseArea.containsMouse ? modelData.color : "transparent"

          MouseArea {
            id: _mouseArea
            anchors.fill: parent
            onClicked: {
              OV.OVLogController[modelData.prop] = !OV.OVLogController[modelData.prop]
            }

            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
          }

          RowLayout {
            id: _logLevelRow
            anchors.fill: parent
            anchors.centerIn: parent
            anchors.leftMargin: DS.Theme.spacingLg
            anchors.rightMargin: DS.Theme.spacingLg

            spacing: DS.Theme.spacingSm

            DS.Icon {
              Layout.preferredWidth: DS.Theme.iconLg
              Layout.preferredHeight: DS.Theme.iconLg
              name: modelData.key
            }

            Label {
              Layout.fillWidth: true
              text: modelData.name
              font: DS.Theme.textSmallRegular
              color: DS.Theme.textPrimary
            }

            Label {
              text: OV.OVLogController[modelData.countProp]
              font: DS.Theme.textSmallRegular
              color: DS.Theme.textSecondary
            }
          }
        }
      }
    }
  }
}
