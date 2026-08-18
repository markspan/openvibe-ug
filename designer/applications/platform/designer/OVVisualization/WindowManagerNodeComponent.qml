// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import OVVisualization as OV
import DesignSystem as DS

Item {
  id: nodeComponent
  anchors.fill: parent

  required property var node
  property bool isRuntimeDisplay

  signal nodeDropped(droppedNodeID: string, parentNodeID: string, position: string)
  signal nodeSelected(nodeID: string)
  
  function hightLightNode(nodeID) {
    loader.item?.hightLightNode(nodeID)
  }

  Loader {
    id: loader
    anchors.fill: parent
    sourceComponent: {
      switch(node?.nodeType) {
        case "Box":
          return boxComponent;
        case "VerticalSplit":
          return verticalSplitComponent;
        case "HorizontalSplit":
          return horizontalSplitComponent;
        default:
          return null;
      }
    }
  }
  
  Component {
    id: boxComponent

    Rectangle {
      anchors.fill: parent

      function hightLightNode(nodeID) {
        if(nodeID === node.id) {
          designerBox.border.color = DS.Theme.primary
        } else {
          designerBox.border.color = "transparent"
        }
      }
      
      Connections {
        target: node
        enabled: node != null

        function onVisualChanged() {
          if (node?.visual != null && isRuntimeDisplay) {
            node.visual.parent = runtimeBox
            //node.visual.anchors.fill = runtimeBox
            node.visual.x = 0
            node.visual.y = 0
            node.visual.width = Qt.binding(function() { return runtimeBox.width })
            node.visual.height = Qt.binding(function() { return runtimeBox.height })
            node.visual.visible = true
          }
        }
      }

      Component.onDestruction: {
        // Clean up visual reparenting before destruction
        if (node?.visual != null && isRuntimeDisplay) {
          node.visual.parent = null
          node.visual.visible = false
        }
      }

      Rectangle {
        id: runtimeBox
        anchors.fill: parent
        visible: isRuntimeDisplay
      }

      Rectangle {
        id: designerBox
        anchors.fill: parent
        color: node.color
        radius: DS.Theme.radiusXs
        visible: !isRuntimeDisplay

        MouseArea {
          anchors.fill: parent
          onClicked: {
            nodeComponent.nodeSelected(node.id)
          }
        }
      }
    }
  }

  Component {
    id: verticalSplitComponent

    OV.WindowManagerSplitView {
      id: verticalSplitView
      anchors.fill: parent
      orientation: Qt.Vertical

      signal nodeHighlighted(nodeID: string)

      function hightLightNode(nodeID) {
        nodeHighlighted(nodeID)
      }
      Repeater {
        id: verticalRepeater
        model: node.children            
        delegate: OV.WindowManagerNode {
          required property var modelData
          node: modelData
          isRuntimeDisplay: nodeComponent.isRuntimeDisplay

          SplitView.preferredHeight: parent.height / 2
          
          Connections {
            target: verticalSplitView
            function onNodeHighlighted(nodeID) {
              hightLightNode(nodeID)
            }
          }
          onNodeDropped: (droppedNodeID, parentNodeID, position) => {
            nodeComponent.nodeDropped(droppedNodeID, parentNodeID, position)
          }
          onNodeSelected: (nodeID) => {
            nodeComponent.nodeSelected(nodeID)
          }
        }
      }
    }
  }

  Component {
    id: horizontalSplitComponent
  
    OV.WindowManagerSplitView {
      id: horizontalSplitView
      anchors.fill: parent
      orientation: Qt.Horizontal

      signal nodeHighlighted(nodeID: string)

      function hightLightNode(nodeID) {
        nodeHighlighted(nodeID)
      }
          
      Repeater {
        id: horizontalRepeater
        model: node.children
            
        delegate: OV.WindowManagerNode {
          required property var modelData
          node: modelData
          isRuntimeDisplay: nodeComponent.isRuntimeDisplay

          SplitView.preferredWidth: parent.width / 2

          Connections {
            target: horizontalSplitView
            function onNodeHighlighted(nodeID) {
              hightLightNode(nodeID)
            }
          }

          onNodeDropped: (droppedNodeID, parentNodeID, position) => {
            nodeComponent.nodeDropped(droppedNodeID, parentNodeID, position)
          }
          onNodeSelected: (nodeID) => {
            nodeComponent.nodeSelected(nodeID)
          }
        }
      }
    }
  }
}