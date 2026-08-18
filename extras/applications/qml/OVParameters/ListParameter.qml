/* SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
   SPDX-License-Identifier: AGPL-3.0-only
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS

Control {
  id: _control

  required property var param
  property bool displayName: true
  property bool displayInline: false

  implicitWidth: DS.Theme.labelWidthMd
  implicitHeight: _control.displayInline ? _inlineLayout.implicitHeight : _columnLayout.implicitHeight

  background: Rectangle{
    color: "transparent"
  }

  hoverEnabled: true

/*OV.ToolTip {
    visible: _control.hovered && !_combobox.down
    text: param ? param.description : ""
}*/


  ColumnLayout {
    id: _columnLayout
    anchors.fill: _control
    spacing: 0
    visible: !_control.displayInline

    Label {
      id: _label
      Layout.fillWidth: true

      text: _control.displayName ? (_control.param ? _control.param.name : "") : "value"
      font: DS.Theme.textSmallLight
      color: DS.Theme.textSecondary

      MouseArea {
          id: _labelMouseArea
          anchors.fill: parent
          hoverEnabled: true
      }

      DS.ToolTip {
          visible: _labelMouseArea.containsMouse
          text: _control.param.description
      }
    }

    DS.ComboBox {
      id: _combobox
      Layout.fillWidth: true
      model: _control.param ? _control.param.available_values : []
      currentIndex: _control.param ? param.index : -1

      Binding {
        target: _control.param
        property: "index"
        value: _combobox.currentIndex
      }
    }
  }

  RowLayout {
    id: _inlineLayout
    anchors.fill: _control
    spacing: 0
    visible: _control.displayInline

    Label {
      Layout.fillWidth: true
      text: _control.displayName ? (_control.param ? _control.param.name : "") : "value"
      font: DS.Theme.textSmallLight
      color: DS.Theme.textSecondary

      MouseArea {
          id: _labelMouseAreaInline
          anchors.fill: parent
          hoverEnabled: true
      }

      DS.ToolTip {
          visible: _labelMouseAreaInline.containsMouse
          text: _control.param.description
      }
    }

    DS.ComboBox {
      id: _comboboxInline
      Layout.fillWidth: true
      model: _control.param ? _control.param.available_values : []
      currentIndex: _control.param ? param.index : -1

      Binding {
        target: _control.param
        property: "index"
        value: _comboboxInline.currentIndex
      }
    }
  }
}
