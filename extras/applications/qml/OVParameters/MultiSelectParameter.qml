/* SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
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

  implicitWidth: DS.Theme.labelWidthMd
  implicitHeight: _columnLayout.implicitHeight

  background: Rectangle {
    color: "transparent"
  }

  hoverEnabled: true

  // The parameter holds the whole selection as a single list, so every toggle rebuilds
  // it from the current checkbox states rather than mutating the list in place.
  function selectionFromCheckBoxes() {
    let selection = []
    for (let i = 0; i < _repeater.count; ++i) {
      const entry = _repeater.itemAt(i)
      if (entry && entry.checked) {
        selection.push(entry.entryName)
      }
    }
    return selection
  }

  ColumnLayout {
    id: _columnLayout
    anchors.fill: _control
    spacing: 0

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
          text: _control.param?.description ?? ""
      }
    }

    // UG fork: select all entries, none, or the other ones. Each sets the whole selection at once, so a box that
    // follows the selection, such as the Signal display's channel list, updates once rather than per entry.
    Flow {
      Layout.fillWidth: true
      Layout.topMargin: DS.Theme.spacingXs
      Layout.bottomMargin: DS.Theme.spacingXs
      spacing: DS.Theme.spacingXs
      visible: _repeater.count > 0

      DS.Button {
        name: qsTr("Select all")
        onClicked: if (_control.param) { _control.param.value = _control.param.available_values }
      }

      DS.Button {
        name: qsTr("Deselect all")
        onClicked: if (_control.param) { _control.param.value = [] }
      }

      DS.Button {
        name: qsTr("Invert selection")
        onClicked: {
          if (!_control.param) { return }
          const selected = _control.param.value
          _control.param.value = _control.param.available_values.filter(entry => selected.indexOf(entry) === -1)
        }
      }
    }

    Repeater {
      id: _repeater
      model: _control.param ? _control.param.available_values : []

      RowLayout {
        id: _entry
        Layout.fillWidth: true
        spacing: DS.Theme.spacingXs

        readonly property string entryName: modelData
        readonly property alias checked: _checkbox.checked

        DS.CheckBox {
          id: _checkbox
          Layout.alignment: Qt.AlignVCenter

          onToggled: {
            if (_control.param) {
              _control.param.value = _control.selectionFromCheckBoxes()
            }
          }
        }

        // Clicking a checkbox writes `checked` imperatively, which would break a plain
        // declarative binding and leave the widget deaf to later changes of the parameter
        // (undo, scenario reload, ...). A Binding element re-asserts itself instead.
        // Membership rather than index: the parameter keeps only the selected entries.
        Binding {
          target: _checkbox
          property: "checked"
          value: _control.param ? _control.param.value.indexOf(_entry.entryName) !== -1 : false
          restoreMode: Binding.RestoreBindingOrValue
        }

        Label {
          Layout.fillWidth: true
          Layout.alignment: Qt.AlignVCenter

          text: _entry.entryName
          font: DS.Theme.textSmallLight
          color: DS.Theme.textPrimary
          elide: Text.ElideRight

          MouseArea {
            anchors.fill: parent
            onClicked: {
              _checkbox.toggle()
              _checkbox.toggled()
            }
          }
        }
      }
    }
  }
}
