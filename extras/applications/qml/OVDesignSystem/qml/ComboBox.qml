// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ".."

ComboBox {
    id: root

    property var centerText: false

    implicitHeight: Theme.menuItemHeight

    delegate: ItemDelegate {
      implicitWidth: root.width - Theme.spacingSm - Theme.scrollBarWidth - 2 * Theme.spacingXs // 2*spacingXs for popup right padding + some space from scrollbar
      implicitHeight: Theme.menuItemHeight
      padding: Theme.spacingMd

      contentItem: Text {
        text: modelData
        font: Theme.textSmallLight
        color: {
          if (root.currentIndex === index) {
            return Theme.white
          }
          Theme.textPrimary
        }
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: root.centerText ? Text.AlignHCenter : Text.AlignLeft
        elide: Text.ElideRight
      }

      background: Rectangle {
        radius: Theme.radiusSm
        color: {
          if (root.currentIndex === index) {
            return Theme.primary
          }
          if (root.highlightedIndex === index) {
            return Theme.hover
          }
          return Theme.white
        }
      }
    }

    indicator: Icon {
        anchors.right: parent.right
        anchors.rightMargin: Theme.spacingSm
        anchors.verticalCenter: parent.verticalCenter
        name: root.popup.visible ? "chevron-up" : "chevron-down"
        size: Theme.iconSm
        color: Theme.iconPrimary
    }

    contentItem: Text {
      leftPadding: Theme.spacingSm + Theme.spacingMd // Align with popup + delegateItem paddings
      rightPadding: root.indicator ? root.indicator.width : 0
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: centerText ? Text.AlignHCenter : Text.AlignLeft

      text: root.displayText
      font: Theme.textSmallLight
      color: Theme.textPrimary
      elide: Text.ElideRight
    }

    background: Rectangle {
      id: _background
      color: Theme.white
      radius: Theme.radiusSm
      border.width: 1
      border.color: (root.hovered || root.popup.visible) ? Theme.borderPrimary : Theme.stroke
    }

    // Available height under the combobox
    readonly property real heightBelow: {
        implicitWidth && width && height // wait for the combobox to be rendered before computing the height below
        var globalPos = mapToItem(null, 0, height)
        return Window.height - globalPos.y
    }

    popup: Popup {
        y: root.height - Theme.radiusSm // overlay combobox for border continuity
        width: root.width
        height: {
          var maxItems = Math.floor((root.heightBelow - 2 * Theme.spacingMd) / Theme.menuItemHeight)
          return Math.min(maxItems, root.delegateModel.count) * Theme.menuItemHeight + 2 * Theme.spacingMd
        }

        topPadding: Theme.spacingMd
        bottomPadding: Theme.spacingMd
        leftPadding: Theme.spacingSm
        rightPadding: Theme.spacingXs

        contentItem: ListView {
          id: _listView
          clip: true
          implicitHeight: contentHeight
          model: root.popup.visible ? root.delegateModel : null
          currentIndex: root.highlightedIndex

          ScrollIndicator.vertical: ScrollIndicator {
            visible: _listView.parent.height < _listView.contentHeight
            contentItem: Rectangle {
              implicitWidth: Theme.scrollBarWidth
              implicitHeight: Theme.scrollBarHeight
              radius: Theme.radiusSm
              color: Theme.quaternary
            }
           }
        }

        background: Rectangle {
            id: _popupBackground
            border.width: 1
            border.color: Theme.borderPrimary
            color: Theme.white
            radius: Theme.radiusSm
            antialiasing: false

            property real scaledBorderWidth: Math.max(1, Math.round(border.width * Window.window?.screen.devicePixelRatio) / Window.window?.screen.devicePixelRatio)

            Rectangle {
              // top border
              id: _topBorder
              anchors.top: parent.top
              anchors.left: parent.left
              anchors.right: parent.right
              width: parent.width
              height: parent.radius
              color: Theme.white
              antialiasing: false // Makes sure rectangle covers fully the border of the parent rectangle
            }
            Rectangle {
              // Left edge
              anchors.top: parent.top
              anchors.left: parent.left
              width: _popupBackground.scaledBorderWidth
              height: parent.radius
              color: _popupBackground.border.color
            }
            Rectangle {
              // Right edge
              anchors.top: parent.top
              anchors.right: parent.right
              width: _popupBackground.scaledBorderWidth
              height: parent.radius
              color: _popupBackground.border.color
            }
        }
    }
}