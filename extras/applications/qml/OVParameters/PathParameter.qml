/* SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
   SPDX-License-Identifier: AGPL-3.0-only
*/

import QtCore

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.platform as Platform

import DesignSystem as DS

Control {
  id: _control

  required property var param
  property bool displayName: true
  
  implicitWidth: DS.Theme.labelWidthMd
  implicitHeight: _columnLayout.implicitHeight

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


    RowLayout {
      Layout.fillWidth: true
      spacing: DS.Theme.spacingSm

      DS.TextField {
        id: _combobox
        Layout.fillWidth: true
        text: _control.param?.value ?? ""

        onEditingFinished: {
          _control.param.value = text
        }
      }

      DS.IconButton {
        id: _button
        iconName: "folder-open"
        iconSize: DS.Theme.icon2xl

        onClicked: {
          console.log("Open file dialog")
          _fileDialog.open();
        }
      }
    }


  }

  // Todo: use QtQuick Dialog when upgrading to Qt 6.9+
  Platform.FileDialog {
    id: _fileDialog
    folder: _control.param?.dirName ?? StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
    nameFilters: _control.param?.filters ?? "Any File (*)";
    fileMode: (_control.param?.mode === "Write" || _control.param?.mode === "ReadWrite") ? FileDialog.SaveFile : FileDialog.OpenFile;
    modality: Qt.ApplicationModal

    onAccepted: {
        // UG fork: choose() rather than value, so that picking the file that is already set reloads it (e.g. a
        // montage edited in Charmeleon); upstream ignores the choice when the path has not changed
        _control.param.choose(decodeURIComponent(_fileDialog.file));
    }
  }
}
