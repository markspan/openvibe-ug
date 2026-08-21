/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform as P

import QtCore

import OVControls as OV
import OVVisualization as OV
import DesignSystem as DS
import OVStyle as OV
import "." as OV

ApplicationWindow {
  id: window
  visible: true
  minimumWidth:  1200
  minimumHeight: 800

  title: qsTr("OpenViBE Designer")

  // PipelineCanvas is a transparent FocusScope, so this is what shows through as the canvas
  // backdrop. Set explicitly: unlike Window.color it would otherwise follow the Basic style's
  // palette.window (grey), not white.
  background: Rectangle { color: DS.Theme.white }

  property string currentSelectedBoxId: ""

  Shortcuts {
    id: _shortcuts
  }

  Connections {
    target: ShortcutManager
    function onFileNewRequested() { fileNewHandle() }
    function onFileOpenRequested() { fileOpenHandle() }
    function onFileSaveRequested() { fileSaveHandle() }
    function onFileSaveAsRequested() { fileSaveAsHandle() }
    function onEditUndoRequested() { editUndoHandle() }
    function onEditRedoRequested() { editRedoHandle() }
    function onEditCutRequested() { editCutHandle() }
    function onEditCopyRequested() { editCopyHandle() }
    function onEditPasteRequested() { editPasteHandle() }
    function onZoomInRequested() { zoomInHandle() }
    function onZoomOutRequested() { zoomOutHandle() }
    function onZoomResetRequested() { zoomResetHandle() }
  }

  Connections {
    target: MenuModel
    function onFileNewRequested() { fileNewHandle() }
    function onFileOpenRequested() { fileOpenHandle() }
    function onFileSaveRequested() { fileSaveHandle() }
    function onFileSaveAsRequested() { fileSaveAsHandle() }
    function onEditUndoRequested() { editUndoHandle() }
    function onEditRedoRequested() { editRedoHandle() }
    function onEditCutRequested() { editCutHandle() }
    function onEditCopyRequested() { editCopyHandle() }
    function onEditPasteRequested() { editPasteHandle() }
    function onZoomInRequested() { zoomInHandle() }
    function onZoomOutRequested() { zoomOutHandle() }
    function onZoomResetRequested() { zoomResetHandle() }
    function onDocumentationRequested() { documentationHandle() }
    function onReportIssueRequested() { reportIssueHandle() }
    function onKeyboardShortcutsRequested() { keyboardShortcutsHandle() }
  }

  // Scenario tab strip, laid out by ApplicationWindow above the content item and below the
  // native title bar.
  header: ColumnLayout {
    spacing: 0

    TopBar {
      id: _topBar
      Layout.fillWidth: true
    }

    DS.HorizontalSpacer {}
  }

  Item {
    id: _content
    anchors.fill: parent

    StackLayout {
      id: _scenariosStack
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right

      currentIndex: OVDesigner.currentScenarioIndex

      Repeater {
        id: _configRepeater
        model: OVDesigner.scenarios
        delegate: OV.PipelineCanvas {
          width: _scenariosStack.width
          height: _scenariosStack.height
        }
      }
    }


    // Content
    OV.BoxSettings {
      id: _boxSettings
      anchors.top: parent.top
      anchors.bottom: _logger.top
      anchors.left: parent.left
      z: _scenariosStack.z + 1
      visible: false
    }

    OV.Toolbox {
      id: _toolbox
      anchors.top: parent.top
      anchors.bottom: _logger.top
      anchors.right: parent.right
      z: _scenariosStack.z + 1

      onSaveScenarioRequested: {
        fileSaveHandle()
      }
    }

    OV.ActionBar {
      id: _actionBar
      anchors.bottom: _logger.top
      anchors.bottomMargin: DS.Theme.spacingLg

      x: (_boxSettings.visible ? _boxSettings.width : 0) + (_toolbox.x - (_boxSettings.visible ? _boxSettings.width : 0) - width) / 2
      z: _scenariosStack.z + 1
    }

    OV.Logger {
      id: _logger
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: _footer.top
    }

    OV.Footer {
      id: _footer
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      cpuUsage: OVDesigner.scenarios[OVDesigner.currentScenarioIndex].cpuUsage
    }
  }

  OV.Toast {
    id: _toast
  }

  Connections {
    target: OVDesigner
    function onToastMessage(message, type) {
      if(type === "error") {
        _toast.type = OV.Style.ButtonType.Danger
        _toast.header = "Error"
      } else if(type === "warning") {
        _toast.type = OV.Style.ButtonType.Warning
        _toast.header = "Warning"
      } else {
        _toast.type = OV.Style.ButtonType.Base
        _toast.header = "Info"
      }
      _toast.message = message
      _toast.open()
    }
  }

  P.FileDialog {
    id: _openDialog
    title: "Open a scenario"
    fileMode: P.FileDialog.OpenFiles
    nameFilters: ["Scenario files (*.xml)"]
    onAccepted: {
      for(var i = 0; i < _openDialog.files.length; i++) {
        //decode the url
        var urlString = _openDialog.files[i].toString()
        var pathString
        if (urlString.startsWith("file:///")) {
          //for windows, the url is file:///C:/.. -> we keel C:/..
          //for linux, the url is file:///home/.. -> we keep /home/..
          var k = urlString.charAt(9) === ':' ? 8 : 7
          pathString = urlString.substring(k)
        } else {
          pathString = urlString
        }
        //load the file in a new scenario tab
        OVDesigner.createScenario("", decodeURIComponent(pathString))
      }
    }
  }

  P.FileDialog {
    id: _saveDialog
    title: "Save scenario " + OVDesigner.scenarios[OVDesigner.currentScenarioIndex].name
    fileMode: P.FileDialog.SaveFile
    folder: StandardPaths.writableLocation(StandardPaths.DesktopLocation)
    nameFilters: ["Scenario files (*.xml)", "Metabox files (*.mxb)"]

    onAccepted: {
      OVDesigner.saveScenario(OVDesigner.currentScenarioIndex ,decodeURIComponent(_saveDialog.file))
    }
  }

  function deselectBox() {
    window.currentSelectedBoxId = ""
    _boxSettings.box = null
    _boxSettings.visible = false
  }

  function selectedBox(box) {
    window.currentSelectedBoxId = box.identifier
    _boxSettings.box = box
    _boxSettings.visible = true
  }

  function fileNewHandle() { OVDesigner.createScenario() }
  function fileOpenHandle() { _openDialog.open() }
  function fileSaveHandle() {
    let path = OVDesigner.scenarios[OVDesigner.currentScenarioIndex].filePath
    if(path == "") {
      _saveDialog.open()
    } else {
      OVDesigner.saveScenario(OVDesigner.currentScenarioIndex ,path)
    }
  }
  function fileSaveAsHandle() { _saveDialog.open() }
  function editUndoHandle() { console.log("editUndoHandle") }
  function editRedoHandle() { console.log("editRedoHandle") }
  function editCutHandle() { console.log("editCutHandle") }
  function editCopyHandle() { console.log("editCopyHandle") }
  function editPasteHandle() { console.log("editPasteHandle") }
  function zoomInHandle() { console.log("zoomInHandle") }
  function zoomOutHandle() { console.log("zoomOutHandle") }
  function zoomResetHandle() { console.log("zoomResetHandle") }
  function documentationHandle() { console.log("documentationHandle") }
  function reportIssueHandle() { console.log("reportIssueHandle") }
  function keyboardShortcutsHandle() { console.log("keyboardShortcutsHandle") }
}
