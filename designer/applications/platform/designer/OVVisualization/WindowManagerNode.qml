// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import OVVisualization as OV
import DesignSystem as DS

Item {
  id: root

  required property var node // C++ WindowManagerNode
  property bool isRuntimeDisplay: false

  property var dropBarThickness: DS.Theme.spacing2xs

  signal nodeDropped(droppedNodeID: string, parentNodeID: string, position: string)
  signal nodeSelected(nodeID: string)

  function hightLightNode(nodeID) {
    loader.item.hightLightNode(nodeID)
  }

  onNodeChanged: {
    if (node != null) {
      loader.setSource("WindowManagerNodeComponent.qml", {"node": node, "isRuntimeDisplay": isRuntimeDisplay})
    }
  }

  Rectangle {
    id: designNodeRectangle
    anchors.fill: parent

    visible: !isRuntimeDisplay

    DropArea {
      id: topDropArea
      anchors.top: parent.top
      anchors.left: parent.left
      anchors.right: parent.right
      height: DS.Theme.spacingMd // overlap content for easier drop
      width: parent.width
      property bool hovered : false

      Rectangle {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 2 * DS.Theme.spacingXs
        height: dropBarThickness
        color: topDropArea.hovered ? DS.Theme.secondary : "transparent"
        radius: DS.Theme.radiusXs
      }

      onEntered: {
        hovered = true
      }
      onExited: {
        hovered = false
      }

      onDropped: (drop) => {
        drop.accepted = true
        hovered = false
        nodeDropped(drop.source.modelData.id, node.id, "top")
      }
    }

    DropArea {
      id: leftDropArea
      anchors.top: parent.top
      anchors.left: parent.left
      anchors.bottom: parent.bottom
      width: DS.Theme.spacingMd // overlap content for easier drop
      height: parent.height

      property bool hovered : false

      Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: dropBarThickness
        height: leftDropArea.height - 2 * DS.Theme.spacingXs
        color: leftDropArea.hovered ? DS.Theme.secondary : "transparent"
        radius: DS.Theme.radiusXs
      }

      onEntered: {
        hovered = true
      }
      onExited: {
        hovered = false
      }
      
      onDropped: (drop) => {
        drop.accepted = true
        hovered = false
        nodeDropped(drop.source.modelData.id, node.id, "left")
      }
    }

    DropArea {
      id: rightDropArea
      anchors.top: parent.top
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      width: DS.Theme.spacingMd // overlap content for easier drop
      height: parent.height

      property bool hovered : false

      Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: dropBarThickness
        height: rightDropArea.height - 2 * DS.Theme.spacingXs
        color: rightDropArea.hovered ? DS.Theme.secondary : "transparent"
        radius: DS.Theme.radiusXs
      }

      onEntered: {
        hovered = true
      }
      onExited: {
        hovered = false
      }
      
      onDropped: (drop) => {
        drop.accepted = true
        hovered = false
        nodeDropped(drop.source.modelData.id, node.id, "right")
      }
    }

    DropArea {
      id: bottomDropArea
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.right: parent.right
      height: DS.Theme.spacingMd // overlap content for easier drop
      width: parent.width

      property bool hovered : false

      Rectangle {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 2 * DS.Theme.spacingXs
        height: dropBarThickness
        color: bottomDropArea.hovered ? DS.Theme.secondary : "transparent"
        radius: DS.Theme.radiusXs
      }

      onEntered: {
        hovered = true
      }
      onExited: {
        hovered = false
      }
      
      onDropped: (drop) => {
        drop.accepted = true
        hovered = false
        nodeDropped(drop.source.modelData.id, node.id, "bottom")
      }
    }
  }

  Rectangle {
    id: runtimeNodeRectangle
    anchors.fill: parent

    visible: isRuntimeDisplay
  }

  Loader {
    id: loader
    parent: isRuntimeDisplay ? runtimeNodeRectangle : designNodeRectangle
    anchors.fill: parent

    anchors.leftMargin: DS.Theme.spacingXs
    anchors.rightMargin: DS.Theme.spacingXs
    anchors.topMargin: DS.Theme.spacingXs
    anchors.bottomMargin: DS.Theme.spacingXs

    onLoaded: {
      item.nodeDropped.connect(function(droppedNodeID, parentNodeID, position) {
        nodeDropped(droppedNodeID, parentNodeID, position)
      })
      item.nodeSelected.connect(function(nodeID) {
        nodeSelected(nodeID)
      })
    }
  }
}