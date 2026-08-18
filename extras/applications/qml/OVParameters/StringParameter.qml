// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

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

  ColumnLayout {
    id: _columnLayout
    anchors.fill: _control
    spacing: 0
    visible: !_control.displayInline

    Label {
      id: _label
      Layout.fillWidth: true

      text: _control.param ? _control.param.name : ""
      font: DS.Theme.textSmallLight
      color: DS.Theme.textSecondary

      MouseArea {
        id: _labelMouseArea
        anchors.fill: parent
        hoverEnabled: true
      }

      DS.ToolTip {
        visible: _labelMouseArea.containsMouse
        text: _control.param?.description ?? ""
      }
    }

    DS.TextField {
      id: _textField
      Layout.fillWidth: true
      text: _control.param?.value ?? ""
      onEditingFinished: {
        _control.param.value = text
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

      text: _control.param ? _control.param.name : ""
      font: DS.Theme.textSmallLight
      color: DS.Theme.textPrimary
      elide: Text.ElideRight

      MouseArea {
        id: _labelMouseAreaInline
        anchors.fill: parent
        hoverEnabled: true
      }

      DS.ToolTip {
        visible: _labelMouseAreaInline.containsMouse
        text: _control.param?.description ?? ""
      }
    }

    DS.TextField {
      Layout.preferredWidth: Math.min(Math.round(_control.width / 2), DS.Theme.labelWidthXl)
      text: _control.param?.value ?? ""
      onEditingFinished: {
        _control.param.value = text
      }
    }
  }
}
