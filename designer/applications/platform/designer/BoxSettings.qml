/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform as P

import OVControls as OV
import OVParameters as OV
import OVVisualization
import DesignSystem as DS


Control {
  id: _control

  property var box: null

  property bool _addingInput: false
  property bool _addingOutput: false

  implicitHeight: parent.height
  implicitWidth: (DS.Theme.boxSettingsMaximumWidth + DS.Theme.boxSettingsMinimumWidth) / 2


  OV.Parameters {
    id: _box_param
    parameters: box?.parameters ?? null

    onParametersChanged: {
      if (parameters) {
        updateParametersModel()
      }
    }
  }

  background: Rectangle {
    color: DS.Theme.white
    border.color: DS.Theme.stroke
  }

  ScrollView {
    id: _settingsScroll
    anchors.fill: parent
    contentWidth: availableWidth
    contentHeight: _mainColumn.implicitHeight
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

    function scrollToItem(item) {
        // Wait for the layout to update
        Qt.callLater(() => {
            // Get the item's position relative to the ScrollView's content
            var itemPos = item.mapToItem(contentItem, 0, 0)
            var itemBottom = itemPos.y + item.height
            var viewportTop = contentItem.y
            var viewportBottom = viewportTop + height

            if (itemBottom > viewportBottom) {
                // Item is below viewport - scroll down
                contentItem.contentY += itemBottom - viewportBottom + DS.Theme.spacingMd
            }
        })
    }

    ColumnLayout {
      id: _mainColumn
      anchors.fill: parent
      spacing: DS.Theme.spacingSm

      Rectangle {
        id: _header
        Layout.fillWidth: true
        Layout.preferredHeight: DS.Theme.labelHeight5xl
        Layout.rightMargin: DS.Theme.spacingLg
        Layout.leftMargin: DS.Theme.spacingLg

        RowLayout {
          anchors.fill: parent

          spacing: DS.Theme.spacingLg

          DS.TextField {
            id: _boxName
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            text: box?.name ?? ""
            font: DS.Theme.textLargeMedium
            color: DS.Theme.textPrimary

            onEditingFinished: {
              box.notifySetName(text)
            }
          }

          DS.IconButton {
            iconName: "folder-open"
            iconSize: DS.Theme.icon2xl
            tooltip: "Open metabox"
            visible: box?.isMetabox ?? false

            onClicked: {
              OVDesigner.openMetabox(box.metaboxId)
            }
          }

          DS.IconButton {
            iconName: "save"
            iconSize: DS.Theme.icon2xl
            tooltip: "Save settings"

            onClicked: {
              _saveSettingsDialog.open()
            }
          }

          DS.IconButton {
            id: _loadSettingsButton
            iconName: "load"
            iconSize: DS.Theme.icon2xl
            tooltip: "Load settings"
            iconColor: box?.overwriteSettings ? DS.Theme.iconHoverPrimary : DS.Theme.iconPrimary
            onClicked: {
              _loadSettingsPopup.open()
            }
          }
        }
      }

      DS.HorizontalSpacer {}

      Rectangle {
        id: _boxParamSection
        Layout.fillWidth: true
        Layout.preferredHeight: _boxParamLayout.implicitHeight
        Layout.rightMargin: DS.Theme.spacingMd
        Layout.leftMargin: DS.Theme.spacingMd

        ColumnLayout {
          id: _boxParamLayout
          anchors.fill: parent

          spacing: DS.Theme.spacing2xl

          Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: DS.Theme.labelHeight5xl

            RowLayout {
              anchors.fill: parent
              spacing: DS.Theme.spacingMd
              Label {
                text: "Settings"
                font: DS.Theme.textSmallBold
                color: DS.Theme.textPrimary
              }
            }
          }

          Repeater {
            id: _config_repeater

            model: _box_param.params_model

            delegate: Loader {
              required property var param;
              required property var component;
              property var lparam: param

              Layout.fillWidth: true
              sourceComponent: component
            }
          }

        }

        // Overlay to grey-out and disable the section
        Rectangle {
          x: -DS.Theme.spacingMd
          y: -DS.Theme.spacingSm
          width: _boxParamSection.width + 2 * DS.Theme.spacingMd // Covers margins of param section
          height: _boxParamSection.height + 2 * DS.Theme.spacingSm // Covers spacing of main column

          color: DS.Theme.disabled
          opacity: 0.8
          visible: box?.overwriteSettings ?? false

          MouseArea {
            // Catches clicks to prevent interacting with the parameters
            anchors.fill: parent
            enabled: parent.visible
          }
        }
      }

      DS.HorizontalSpacer {}

      Rectangle {
        id: _inputSection
        Layout.fillWidth: true
        Layout.preferredHeight: _inputLayout.implicitHeight

        color: "transparent"

        ColumnLayout {
          id: _inputLayout
          anchors.fill: parent
          anchors.leftMargin: DS.Theme.spacingLg
          anchors.rightMargin: DS.Theme.spacingLg
          anchors.bottomMargin: DS.Theme.spacingLg

          spacing: DS.Theme.spacingMd

          RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: DS.Theme.spacing5xl

            Text {
              text: "Input"
              font: DS.Theme.subtitle3
              color: DS.Theme.textPrimary
              Layout.alignment: Qt.AlignVCenter
            }

            Item {
              Layout.fillWidth: true
            }

            DS.IconButton {
              iconName: "add"
              iconSize: DS.Theme.iconLg
              visible: box?.flags & BoxObject.CanAddInput

              onClicked: {
                _addingInput = true
                box.addInput("New Input", box.inputSupportTypes[0])
              }
            }
          }

          Repeater {
            model: box?.inputPorts

            delegate: IOCard {
              required property int index
              required property var modelData
              ioObject: modelData
              ioType: "Input"
              selectionModel: box.inputSupportTypes
              deletable: box.flags & BoxObject.CanAddInput && index >= box.initialInputCount
              onRemove: {
                box.removeInput(index)
              }
            }
            onItemAdded: (newIndex, newCard) => {
              if (_addingInput) {
                _addingInput = false
                _settingsScroll.scrollToItem(newCard)
              }
            }
          }
        }
      }

      DS.HorizontalSpacer {}

      Rectangle {
        id: _outputSection
        Layout.fillWidth: true
        Layout.preferredHeight: _outputLayout.implicitHeight

        color: "transparent"

        ColumnLayout {
          id: _outputLayout
          anchors.fill: parent
          anchors.leftMargin: DS.Theme.spacingLg
          anchors.rightMargin: DS.Theme.spacingLg
          anchors.bottomMargin: DS.Theme.spacingLg

          spacing: DS.Theme.spacingMd

          RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: DS.Theme.spacing5xl

            Text {
              text: "Output"
              font: DS.Theme.subtitle3
              color: DS.Theme.textPrimary
              Layout.alignment: Qt.AlignVCenter
            }

            Item {
              Layout.fillWidth: true
            }

            DS.IconButton {
              iconName: "add"
              iconSize: DS.Theme.iconLg
              visible: box?.flags & BoxObject.CanAddOutput

              onClicked: {
                _addingOutput = true
                box.addOutput("New Output", box.outputSupportTypes[0])
              }
            }
          }

          Repeater {
            model: box?.outputPorts

            delegate: IOCard {
              required property int index
              required property var modelData
              ioObject: modelData
              ioType: "Output"
              selectionModel: box.outputSupportTypes
              deletable: box.flags & BoxObject.CanAddOutput && index >= box.initialOutputCount
              onRemove: {
                box.removeOutput(index)
              }
            }
            onItemAdded: (newIndex, newCard) => {
              if (_addingOutput) {
                _addingOutput = false
                _settingsScroll.scrollToItem(newCard)
              }
            }
          }
        }
      }
    }
  }

  // Save Settings Dialog
  P.FileDialog {
    id: _saveSettingsDialog
    title: "Save " + box?.name + " settings"
    fileMode: P.FileDialog.SaveFile
    folder: P.StandardPaths.writableLocation(P.StandardPaths.DesktopLocation)
    currentFile: folder + "/" + (box?.name ?? "box").toLowerCase().replace(/ /g, "-") + "-settings.xml"
    nameFilters: ["Box settings files (*.xml)"]

    onAccepted: {
      box.saveSettings(decodeURIComponent(_saveSettingsDialog.file))
    }
  }

  // Load Settings Dialog
  DS.Popup {
    id: _loadSettingsPopup

    x: _loadSettingsButton.x
    y: _loadSettingsButton.y + _loadSettingsButton.height + DS.Theme.spacingSm

    title: "Settings file"
    
    ColumnLayout {
      id: _loadSettingsColumnLayout
      spacing: DS.Theme.spacingLg

      RowLayout {
        Layout.fillWidth: true

        spacing: DS.Theme.spacingMd

        DS.Switch {
          id: _loadSettingsSwitch
          checked: box?.overwriteSettings ?? false

          onToggled: {
            box.overwriteSettings = _loadSettingsSwitch.checked ? true : false
          }
        }

        Label {
          text: "Overwrite current settings"
          font: DS.Theme.textMediumRegular
          color: DS.Theme.textPrimary
        }
      }

      OV.PathParameter {
        id: _loadSettingsPathParameter
        Layout.fillWidth: true

        param: box?.settingsFilePath
        displayName: true
      }
    }
  }

  Rectangle {
    id: _separator
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: 1
    color: DS.Theme.stroke
  }

  // Resize handle (right edge)
  MouseArea {
    id: resizeHandle
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    property real mouseToEdgeX

    width: 8

    cursorShape: Qt.SizeHorCursor

    onPressed: (mouse) => {
        var globalMouse = mapToGlobal(mouse.x, mouse.y)
        mouseToEdgeX = _control.x + _control.width - globalMouse.x
        _separator.color = DS.Theme.primary
    }
    onReleased: (mouse) => {
        _separator.color = DS.Theme.stroke
    }

    onPositionChanged: (mouse) => {
        if (pressed) {
            var globalMouse = mapToGlobal(mouse.x, mouse.y)
            var newWidth = globalMouse.x - _control.x + mouseToEdgeX
            if (newWidth >= DS.Theme.boxSettingsMinimumWidth && newWidth <= DS.Theme.boxSettingsMaximumWidth) {
              _control.width = newWidth
            }
        }
    }
  }
}
