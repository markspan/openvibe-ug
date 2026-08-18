/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import OVParameters as OV
import DesignSystem as DS

Control {
  id: _control
  width: parent.width
  height: DS.Theme.footerHeight

  property real cpuUsage

  // Top line of component
  Rectangle {
    id: _top_line
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: 1
    color: DS.Theme.stroke
  }

  background: Rectangle {
    color: DS.Theme.white
  }

  RowLayout {
    id: _info_row
    anchors.fill: parent
    anchors.centerIn: parent

    Item {
      Layout.fillWidth: true
    }

    RowLayout {
      Layout.rightMargin: DS.Theme.spacingSm
      Layout.leftMargin: DS.Theme.spacingSm

      spacing: DS.Theme.spacingSm
      Label {
        id: _cpuLabel
        text: "Cpu"
        font: DS.Theme.textSmallBold
        color: DS.Theme.textPrimary
      }

      Rectangle {
        id: _cpuRectangle
        width: DS.Theme.icon2xl
        height: DS.Theme.icon2xl
        radius: DS.Theme.radiusXs
        color: _control.cpuUsage < 0.0 ? DS.Theme.stroke : _control.cpuUsage < 33.3 ? DS.Theme.cpuUsageColors.low : _control.cpuUsage < 66.6 ? DS.Theme.cpuUsageColors.medium : DS.Theme.cpuUsageColors.high

        MouseArea {
          id: _cpuMouseArea
          anchors.fill: parent
          hoverEnabled: true
        }

        DS.ToolTip {
          id: _cpuTooltip
          text: "CPU usage: " + (_control.cpuUsage > 0.0 ? _control.cpuUsage.toFixed(2) + "%" : "-")
          visible: _cpuMouseArea.containsMouse
          delay: 500
        }
      }
    }
  }
}