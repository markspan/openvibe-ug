// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import OVControls as OV
import OVVisualization as OV
import DesignSystem as DS

Rectangle {
  id: _root
  property alias tooltip: _tooltip.text
  property var baseColor
  signal clicked(var button)

  width: DS.Theme.boxIOWidth
  height: DS.Theme.boxIOHeight
  radius: height / 2

  color: baseColor
  antialiasing: true

  DS.ToolTip {
    id: _tooltip
    visible: _mouseArea.containsMouse
    delay: 500
  }

  Rectangle {
    id: _hitArea
    anchors.centerIn: parent

    width: parent.width * 1.3
    height: parent.height * 1.3
    radius: height / 2
    color: "transparent"

    Rectangle {
      anchors.fill: parent
      radius: parent.radius
      color: baseColor
      antialiasing: true
      visible: _mouseArea.containsMouse
    }

    MouseArea {
      id: _mouseArea;
      anchors.fill: parent;
      hoverEnabled: true;
      propagateComposedEvents: true
      cursorShape: Qt.PointingHandCursor

      acceptedButtons: Qt.LeftButton | Qt.RightButton

      onClicked: (event)=> {
        if (event.button === Qt.LeftButton || event.button === Qt.RightButton) {
          _root.focus = true
          event.accepted = true
          _root.clicked(event.button)
        }
      }
    }
  }

  
}