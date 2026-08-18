// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVControls as OV
import OVStyle as OV

Item {
  id: _root
  anchors.fill: parent

  Row {
    id: _toolBar
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: OV.Style.mediumLabelHeight
    spacing: OV.Style.smallPadding

    CheckBox {
      id: showValuesToggle
      text: "show Values"
      checked: true
    }
    CheckBox {
      id: showColorsToggle
      text: "show Colors"
      checked: true
    }
 
  }

  ScrollView {
    anchors.top: _toolBar.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: OV.Style.mediumPadding

    ScrollBar.vertical.policy: ScrollBar.AsNeeded
    ScrollBar.horizontal.policy: ScrollBar.AsNeeded

    clip: true

    Grid {
      id: matrixGrid
      width: implicitWidth
      height: implicitHeight

      rows: dataSource.nRow + 1
      columns: dataSource.nCol + 1

      Repeater {
        model: dataSource.cells
        delegate: Rectangle {
          required property color col
          required property string value
          required property bool isLabel
  
          width: Math.max(50, valueText.contentWidth + OV.Style.smallPadding)
          height: 50// _root.height / dataSource.nRow
  
          color: showColorsToggle.checked ? col : "transparent"
          border.color: value === "" ? "transparent": "#333"
          border.width: value === "" ? 0 : 1
  
          Text {
            id: valueText
            anchors.centerIn: parent
            // labels are in white !
            text: (showValuesToggle.checked || isLabel) ? value : ""
            color: "black"
            font.bold: true
          } 
        }
      }
    }
  }
}
