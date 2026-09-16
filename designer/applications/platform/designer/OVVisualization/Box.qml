// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import OVControls as OV
import "." as OV
import DesignSystem as DS

Rectangle {
  id: _root
  required property int index
  required property var model
  required property string originalBoxName
  required property string name
  required property string identifier
  required property var parameters
  required property point center
  required property size boxSize
  required property var inputPorts
  required property var outputPorts
  required property var inputSupportTypes
  required property var outputSupportTypes
  required property var flags
  required property int initialInputCount
  required property int initialOutputCount
  required property bool isMetabox
  required property string metaboxId

  //flags
  property bool canAddInput: flags & 1
  property bool canModifyInput: flags & 2
  property bool canAddOutput: flags & 4
  property bool canModifyOutput: flags & 8
  property bool needsManualUpdate: flags & 64

  signal clickPortCallback(portIdx: int, boxName: string, x: real, y: real, isOutput: bool)
  signal contextMenuRequested(portIdx: int, boxName: string, dataType: string, x: real, y: real, isOutput: bool)
  signal boxSelected(var modelData)
  // emitted on double-click of a metabox box to edit its scenario
  signal openMetaboxRequested(var modelData)

  // set by PipelineCanvas to show selection highlight
  property bool isMultiSelected: false
  // prevents notifyCenterChanged from firing during initial layout
  property bool _positioningReady: false

  x: center.x - boxSize.width/2
  y: center.y - boxSize.height/2
  width: Math.max(DS.Theme.boxMinimumWidth,
                  _nameText.implicitWidth + 2 * DS.Theme.spacingMd,
                  _inputPortsLayout.implicitWidth + 2*DS.Theme.spacingLg,
                  _outputPortsLayout.implicitWidth + 2*DS.Theme.spacingLg)
  height: Math.max(DS.Theme.boxMinimumHeight, _nameText.implicitHeight + 2 * DS.Theme.spacingMd)
  radius: DS.Theme.radiusSm

  border.color: _root.isMultiSelected ? DS.Theme.primary : "transparent"
  border.width: 2
  // metaboxes stand out from regular boxes by their pastel green background
  color: _root.isMetabox ? DS.Theme.metaboxBackground : DS.Theme.white

  Rectangle {
    id: _shadow
    anchors.fill: parent
    radius: parent.radius
    color: parent.color
    z: -1

    layer.enabled: true
    layer.effect: MultiEffect {
      shadowEnabled: true
      shadowColor: "#000000"
      shadowOpacity: 0.3
      shadowBlur: 0.35
      shadowHorizontalOffset: 0.42
      shadowVerticalOffset: 1.66
    }
  }

  opacity: _dragArea.containsMouse ? 0.8 : 1.0

  ColumnLayout {
    z: 1
    anchors.fill: parent
    spacing: 0

    RowLayout {
      id: _inputPortsLayout
      Layout.alignment: Qt.AlignHCenter
      Layout.leftMargin: DS.Theme.spacingLg
      Layout.rightMargin: DS.Theme.spacingLg

      spacing: DS.Theme.spacingMd

      Repeater {
        model: inputPorts

        delegate: OV.BoxPort {
          required property int index
          required property string name
          required property string dataType

          Layout.topMargin: DS.Theme.spacingXs
          Layout.bottomMargin: DS.Theme.spacingXs

          function updateOffset() {
            var portCenter = mapToItem(_root, width/2, height/2)
            var dx = portCenter.x - _root?.width/2
            var dy = portCenter.y - _root?.height/2 - height/2 // height/2 to target the top edge of the port
            _root?.model.modelData.updatePortOffset?.(index, false, Qt.point(dx, dy))
          }

          tooltip: name + " | Type: " + dataType
          baseColor: StreamColorMapper.getStreamColor(dataType)
          onClicked: (button) => {
            _root.focus = true
            var portCenter = mapToItem(_root, width/2, height/2)
            if (button === Qt.LeftButton) {
              clickPortCallback(index, _root?.identifier, _root?.x + portCenter.x, _root?.y + portCenter.y - height/2, false)
            } else if (button === Qt.RightButton) {
              contextMenuRequested(index, _root?.identifier, dataType, _root?.x + portCenter.x, _root?.y + portCenter.y - height/2, false)
            }
          }

          Component.onCompleted: Qt.callLater(updateOffset)
          onXChanged: Qt.callLater(updateOffset)
          onYChanged: Qt.callLater(updateOffset)
        }
      }

    }

    Item {
      Layout.fillHeight: true
    }

    RowLayout {
      id: _boxNameLayout
      Layout.fillWidth: true

      Text {
        id: _nameText
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        wrapMode: Text.Wrap
        font: DS.Theme.textMediumBold
        color: DS.Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter

        // The canvas zooms with a Scale transform, so distance-field rendering has to stay
        // (native glyphs would pixelate at anything but 100%). Raising the glyph cache
        // resolution sharpens it at small sizes without giving that up.
        renderTypeQuality: Text.VeryHighRenderTypeQuality

        text: (_root.originalBoxName !== "" && _root.originalBoxName !== _root.name) ? "<i>" + _root.originalBoxName + "</i> <br> <b>" + _root.name + "</b>" : _root.name
      }
    }

    Item {
      Layout.fillHeight: true
    }

    RowLayout {
      id: _outputPortsLayout
      Layout.alignment: Qt.AlignHCenter
      Layout.leftMargin: DS.Theme.spacingLg
      Layout.rightMargin: DS.Theme.spacingLg

      spacing: DS.Theme.spacingMd

      Repeater {
        model: outputPorts
        delegate: OV.BoxPort {
          required property int index
          required property string name
          required property string dataType

          Layout.topMargin: DS.Theme.spacingXs
          Layout.bottomMargin: DS.Theme.spacingXs

          function updateOffset() {
            var portCenter = mapToItem(_root, width/2, height/2)
            var dx = portCenter.x - _root.width/2
            var dy = portCenter.y - _root.height/2 + height/2 // height/2 to target the top edge of the port
            _root.model.modelData.updatePortOffset?.(index, true, Qt.point(dx, dy))
          }

          tooltip: name + " | Type: " + dataType
          baseColor: StreamColorMapper.getStreamColor(dataType)
          onClicked: (button) => {
            _root.focus = true
            var portCenter = mapToItem(_root, width/2, height/2)
            if (button === Qt.LeftButton) {
              clickPortCallback(index, _root.identifier, _root.x + portCenter.x, _root.y + portCenter.y + height/2, true)
            } else if (button === Qt.RightButton) {
              contextMenuRequested(index, _root.identifier, dataType, _root.x + portCenter.x, _root.y + portCenter.y + height/2, true)
            }
          }
          Component.onCompleted: Qt.callLater(updateOffset)
          onXChanged: Qt.callLater(updateOffset)
          onYChanged: Qt.callLater(updateOffset)
        }
      }
    }
  }

  // Drag is handled by PipelineCanvas via DragHandler on the delegate.
  // We only notify C++ once positioning is ready (after initial layout).
  onXChanged: {
    if (_positioningReady) {
      _root.model.modelData.notifyCenterChanged?.(x+boxSize.width/2, y+boxSize.height/2)
    }
  }

  onYChanged: {
    if (_positioningReady) {
      _root.model.modelData.notifyCenterChanged?.(x+boxSize.width/2, y+boxSize.height/2)
    }
  }

  MouseArea {
    id: _dragArea
    anchors.centerIn: parent
    width: parent.width
    height: parent.height - 2 * DS.Theme.spacingSm

    hoverEnabled: true
    acceptedButtons: Qt.LeftButton | Qt.RightButton

    onClicked: (mouse)=> {
      _root.focus = true
      if(mouse.button == Qt.LeftButton) {
        _root.boxSelected(_root.model.modelData)
      }
    }

    onDoubleClicked: (mouse)=> {
      if(mouse.button == Qt.LeftButton && _root.isMetabox) {
        _root.openMetaboxRequested(_root.model.modelData)
      }
    }
  }

  Component.onCompleted: {
    // Notify the box size to C++ — deferred so the layout has settled
    Qt.callLater(() => {
      if (_root || model || model.modelData){
        model.modelData.boxSize = Qt.size(_root.width, _root.height)
        _positioningReady = true
      }
    })
  }

}
