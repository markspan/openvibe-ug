// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

import "." as OV
import DesignSystem as DS

Item {
  id: _root
  required property point sourcePosition
  required property point targetPosition
  required property string sourceDataType
  required property bool isLinked // Todo: update end shape 

  signal removedLink()

  x: Math.min(sourcePosition.x, targetPosition.x)
  y: Math.min(sourcePosition.y, targetPosition.y)
  width:  Math.max(Math.abs(targetPosition.x - sourcePosition.x), 1)
  height: Math.max(Math.abs(targetPosition.y - sourcePosition.y), 1)

  // Local coordinates shared by both shapes
  readonly property real srcX:   sourcePosition.x - x
  readonly property real srcY:   sourcePosition.y - y
  readonly property real tgtX:   targetPosition.x - x
  readonly property real tgtY:   targetPosition.y - y
  readonly property real deltaY: (tgtY - srcY) / 2

  // Glow stroke — wider, semi-transparent, only visible when focused
  Shape {
    id: _glowStroke
    anchors.fill: parent
    preferredRendererType: Shape.CurveRenderer
    visible: _root.activeFocus
    opacity: 0.4

    ShapePath {
      fillColor:   "transparent"
      strokeWidth: 5
      strokeColor: StreamColorMapper.getStreamColor(_root.sourceDataType)
      startX: _root.srcX
      startY: _root.srcY
      PathCubic {
        control1X: _root.srcX
        control1Y: _root.srcY + _root.deltaY
        control2X: _root.tgtX
        control2Y: _root.tgtY - _root.deltaY
        x: _root.tgtX
        y: _root.tgtY
      }
    }
  }

  // Visual stroke
  Shape {
    id: _visualStroke
    anchors.fill: parent
    preferredRendererType: Shape.CurveRenderer
    ShapePath {
      fillColor:   "transparent"
      strokeWidth: _root.activeFocus ? 3 : 2
      strokeColor: StreamColorMapper.getStreamColor(_root.sourceDataType)
      startX: _root.srcX
      startY: _root.srcY
      PathCubic {
        control1X: _root.srcX
        control1Y: _root.srcY + _root.deltaY
        control2X: _root.tgtX
        control2Y: _root.tgtY - _root.deltaY
        x: _root.tgtX
        y: _root.tgtY
      }
    }
  }

  // Hit area: closed band around the bezier, used with FillContains
  Shape {
    id: _hitShape
    anchors.fill: parent
    containsMode: Shape.FillContains

    readonly property real hitBandWidth: 8

    ShapePath {
      fillColor:   "transparent"
      strokeColor: "transparent"
      // Left edge — forward bezier shifted left
      startX: _root.srcX - _hitShape.hitBandWidth; startY: _root.srcY
      PathCubic {
        control1X: _root.srcX - _hitShape.hitBandWidth; control1Y: _root.srcY + _root.deltaY
        control2X: _root.tgtX - _hitShape.hitBandWidth; control2Y: _root.tgtY - _root.deltaY
        x: _root.tgtX - _hitShape.hitBandWidth; y: _root.tgtY
      }
      // Cap at target end
      PathLine { x: _root.tgtX + _hitShape.hitBandWidth; y: _root.tgtY }
      // Right edge — backward bezier shifted right
      PathCubic {
        control1X: _root.tgtX + _hitShape.hitBandWidth; control1Y: _root.tgtY - _root.deltaY
        control2X: _root.srcX + _hitShape.hitBandWidth; control2Y: _root.srcY + _root.deltaY
        x: _root.srcX + _hitShape.hitBandWidth; y: _root.srcY
      }
      // Cap at source end — closes the band
      PathLine { x: _root.srcX - _hitShape.hitBandWidth; y: _root.srcY }
    }

    TapHandler {
      enabled: _root.isLinked
      onTapped: {
        _root.forceActiveFocus()
      }
    }

    HoverHandler {
      cursorShape: Qt.PointingHandCursor
    }
  }

  Keys.onPressed: (event) => {
    if (event.key == Qt.Key_Delete) {
      _root.focus = false
      _root.removedLink()
    }
  }

}
