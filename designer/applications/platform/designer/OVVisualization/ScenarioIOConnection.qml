// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

import "." as OV
import DesignSystem as DS

Item {
  id: _root

  required property point targetPosition
  required property bool isOutput
  required property string dataType
  required property int scenarioPortIdx
  required property string targetBoxId
  required property int targetPortIdx

  signal removedConnection()

  readonly property int linkLength:  24
  readonly property int hitBandWidth: 8
  readonly property int orientation: isOutput ? 1 : -1

  width: hitBandWidth * 2 + 1
  height: linkLength + _connectionSquare.height
  
  x: targetPosition.x - hitBandWidth
  y: targetPosition.y + (isOutput ? 0.0 : -height)

  // Local coordinates — line runs from lineTop to lineBottom, square sits above or below lineTop
  // Endpoints are inset by 1 px (half the normal stroke width) so the stroke doesn't bleed into adjacent items.
  readonly property real lineXCenter:      targetPosition.x - x  // horizontal center
  readonly property real lineTop:    isOutput ?  1 : _connectionSquare.height + 1
  readonly property real lineBottom: isOutput ? linkLength - 1 : height - 1

  Rectangle {
    id: _connectionSquare
    width: _text.implicitWidth + 2 * DS.Theme.spacingSm
    height: width

    x: _root.lineXCenter - width / 2
    y: isOutput ? _root.height - height : 0

    color: DS.Theme.white
    radius: DS.Theme.radiusXs
    border.color: _root.activeFocus ? DS.Theme.primary : "transparent"

    TapHandler {
      onTapped: {
        _root.forceActiveFocus()
      }
    }
    Text {
      id: _text
      anchors.centerIn: parent
      anchors.leftMargin: DS.Theme.spacingSm
      anchors.rightMargin: DS.Theme.spacingSm
      text: _root.scenarioPortIdx + 1
      font.family: "Cascadia Mono" // monospace font for size consistency
      font.pixelSize: DS.Theme.textMediumRegular.pixelSize
      font.weight: DS.Theme.textMediumRegular.weight
      color: DS.Theme.textPrimary
    }
    
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
  }
  


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
      strokeColor: StreamColorMapper.getStreamColor(_root.dataType)
      startX: _root.lineXCenter; startY: _root.lineBottom
      PathLine { x: _root.lineXCenter; y: _root.lineTop }
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
      strokeColor: StreamColorMapper.getStreamColor(_root.dataType)
      startX: _root.lineXCenter; startY: _root.lineBottom
      PathLine { x: _root.lineXCenter; y: _root.lineTop }
    }
  }

  // Hit area: rectangle band around the vertical line, used with FillContains
  Shape {
    id: _hitShape
    anchors.fill: parent
    containsMode: Shape.FillContains

    ShapePath {
      fillColor:   "transparent"
      strokeColor: "transparent"
      startX: _root.lineXCenter - _root.hitBandWidth; startY: _root.lineBottom
      PathLine { x: _root.lineXCenter - _root.hitBandWidth; y: _root.lineTop    }
      PathLine { x: _root.lineXCenter + _root.hitBandWidth; y: _root.lineTop    }
      PathLine { x: _root.lineXCenter + _root.hitBandWidth; y: _root.lineBottom }
      PathLine { x: _root.lineXCenter - _root.hitBandWidth; y: _root.lineBottom }
    }

    TapHandler {
      onTapped: _root.forceActiveFocus()
    }

    HoverHandler {
      cursorShape: Qt.PointingHandCursor
    }
  }

  Keys.onPressed: (event) => {
    if (event.key == Qt.Key_Delete) {
      _root.focus = false
      _root.removedConnection()
    }
  }
}
