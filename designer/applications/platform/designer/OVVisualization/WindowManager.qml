import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS
import OVVisualization as OV

Rectangle {
  id: _windowManagerSection
  implicitHeight: _windowManagerLayout.implicitHeight

  required property var windowManager
  property var selectedNodeID: ""

  color: "transparent"

  Keys.onPressed: (event) => {
    if (event.key == Qt.Key_Delete) {
      event.accepted = true
      windowManager.removeNode(selectedNodeID)
    }
  }

  ColumnLayout {
    id: _windowManagerLayout
    anchors.fill: parent
    anchors.leftMargin: DS.Theme.spacingLg
    anchors.rightMargin: DS.Theme.spacingLg
    anchors.bottomMargin: DS.Theme.spacingLg
    
    spacing: DS.Theme.spacingMd
    
    RowLayout {
      Layout.fillWidth: true
      Layout.minimumHeight: DS.Theme.spacing5xl
      
      Text {
        text: "Window Manager"
        font: DS.Theme.subtitle3
        color: DS.Theme.textPrimary
        Layout.alignment: Qt.AlignVCenter
      }
      
      Item {
        Layout.fillWidth: true
      }

      DS.Icon {
        name: "window-manager"
        size: DS.Theme.icon2xl
        color: DS.Theme.iconPrimary
        Layout.alignment: Qt.AlignVCenter
      }
    }

    ColumnLayout {
      id: _wmInnerLayout
      Layout.fillWidth: true

      spacing: DS.Theme.spacingMd

      Label {
        id: _displaysLabel
        text: "Displays"
        font: DS.Theme.textMediumRegular
        color: DS.Theme.textSecondary
        Layout.alignment: Qt.AlignLeft
      }

      RowLayout {
        Layout.fillWidth: true
        spacing: DS.Theme.spacingLg
        
        ColumnLayout {
          id: _availableDisplays
          Layout.fillWidth: false
          Layout.minimumWidth: _displaysLabel.implicitWidth + 2 * DS.Theme.spacingXs

          Repeater {
            model: _windowManagerSection.windowManager?.boxes

            onModelChanged: {
              for (var i = 0; i < model.length; i++) {
                model[i].color = DS.Theme.windowManagerNodeColors[i]
              }
            }

            delegate: Rectangle {
              id: draggableDisplay
              required property int index
              required property var modelData

              Layout.fillWidth: true
              Layout.minimumWidth: _displayRow.implicitWidth + 2 * DS.Theme.spacingXs
              height: _displayRow.implicitHeight + 2 * DS.Theme.spacingXs

              color: "transparent"
              radius: DS.Theme.radiusSm
              
              Drag.supportedActions: Qt.CopyAction
              Drag.active: _dragHandler.active //-> done by dragHandler onActiveChanged
              Drag.dragType: Drag.Automatic

              HoverHandler {
                // "Hack" to get image of dragged item. Does not work in DragHandler.onActiveChanged, nor in Component.onCompleted
                onHoveredChanged: {
                  if (hovered && draggableDisplay.Drag.imageSource.toString().length === 0) {
                    draggableDisplay.grabToImage(function(result) { draggableDisplay.Drag.imageSource = result.url; })
                  }
                  if (hovered) {
                    draggableDisplay.border.color = DS.Theme.primary
                  } else {
                    draggableDisplay.border.color = "transparent"
                  }
                }
              }

              RowLayout {
                id: _displayRow
                anchors.fill: parent
                anchors.margins: DS.Theme.spacingXs

                spacing: DS.Theme.spacingXs

                Rectangle {
                  Layout.preferredWidth: DS.Theme.spacing2xl
                  Layout.preferredHeight: DS.Theme.spacing2xl
                  Layout.alignment: Qt.AlignVCenter
                  color: modelData.color
                  radius: DS.Theme.radiusXs
                  opacity: 1.0
                }

                Label {
                  text: modelData.name
                  font: DS.Theme.textXsRegular
                  color: DS.Theme.textPrimary
                  Layout.alignment: Qt.AlignVCenter
                }
              }

              DragHandler {
                id: _dragHandler

                cursorShape: Qt.DragCopyCursor
                target: null
              }
            }
          }
        }

        Rectangle {
          id: _windowManagerComposer

          Layout.fillWidth: true
          Layout.fillHeight: true
          
          Layout.minimumHeight: DS.Theme.windowManagerHeight

          color: DS.Theme.white
          border.color: DS.Theme.secondary
          radius: DS.Theme.radiusXs

          Rectangle {
            anchors.fill: parent
            anchors.margins: DS.Theme.spacingXs

            OV.WindowManagerNode {
              id: rootNode
              anchors.fill: parent
              node: windowManager.root
              isRuntimeDisplay: false

              visible: windowManager.root ? true : false

              onNodeDropped: (droppedNodeID, parentNodeID, position) => {
                windowManager.insertBox(droppedNodeID, parentNodeID, position)
              }
              onNodeSelected: (nodeID) => {
                _windowManagerSection.forceActiveFocus()
                _windowManagerSection.selectedNodeID = nodeID
                rootNode.hightLightNode(nodeID) // Send info back into the tree. Will go down all branches
              }
            }
          }

          Rectangle {
            id: _windowManagerDropArea
            visible: windowManager.root ? false : true
            color: "transparent"
            anchors.fill: parent

            Text {
              anchors.centerIn: parent
              text: "Drop box here!"
            }

            DropArea {
              anchors.fill: parent
              onDropped : (drop) => {
                windowManager.insertRootBox(drop.source.modelData.id)
              }
            }
          }
        }
      }
    }
  }

  Window {
    id: _visualizationWindow
    visible: windowManager.showVisualizationWindow && windowManager.root != null
    width: 1024
    height: 768
    color: "transparent"
    title: "Window Manager"

    Rectangle {
      id: _visualizationWindowContainer
      anchors.fill: parent

      OV.WindowManagerNode {
        id: _rootNode
        anchors.fill: parent
        node: windowManager.root
        isRuntimeDisplay: true
      }
    }

    Loader {
      id: _visualizationWindowPopupLoader
      sourceComponent: _confirmClosePopupComponent
      active: active
      onLoaded: {
        item.windowTitle = "Window Manager"
      }
    }

    onClosing: (close) => {
      // Prevent the window from closing
      close.accepted = false
      // Show confirmation dialog
      _visualizationWindowPopupLoader.item.open()
    }
  }

  // Individual windows for unparented boxes
  // We use Instantiator instead of Repeater because Window is not an Item
  Instantiator {
    id: _windowInstantiator
    model: _windowManagerSection.windowManager?.unparentedBoxes

    delegate: Window {
      required property var modelData

      //visible: _windowManagerSection.windowManager?.showVisualizationWindow
      visible: false
      width: 800
      height: 600
      title: modelData?.name
      color: "transparent"

      Connections {
        target: _windowManagerSection.windowManager
        onShowVisualizationWindowChanged: {
          if (_windowManagerSection.windowManager.showVisualizationWindow) {
            if (modelData.fullScreen) {
              showFullScreen()
            } else {
              show()
            }
          } else {
            hide()
          }
        }
      }

      Rectangle {
        anchors.fill: parent

        OV.WindowManagerNode {
          anchors.fill: parent
          node: modelData
          isRuntimeDisplay: true
        }
      }

      Loader {
        id: _unparentedWindowPopupLoader
        sourceComponent: _confirmClosePopupComponent
        active: true
        onLoaded: {
          item.windowTitle = modelData.name
        }
      }

      onClosing: (close) => {
        // Prevent the window from closing
        close.accepted = false

        _unparentedWindowPopupLoader.item.open()
      }
    }
  }

  // Reusable confirmation popup component
  Component {
    id: _confirmClosePopupComponent

    DS.Popup {
      id: popup
      anchors.centerIn: Overlay.overlay
      parent: Overlay.overlay

      property string windowTitle: ""

      title: windowTitle
      modal: true
      closePolicy: Popup.CloseOnEscape

      ColumnLayout {
        Layout.fillWidth: true
        spacing: DS.Theme.spacingLg

        Text {
          text: "Would you like to stop the scenario execution ?"
          font: DS.Theme.textSmallRegular
          color: DS.Theme.textPrimary
          Layout.fillWidth: true
          wrapMode: Text.WordWrap
        }

        RowLayout {
          Layout.fillWidth: true
          Layout.alignment: Qt.AlignRight
          spacing: DS.Theme.spacingMd

          DS.Button {
            name: "No"
            onClicked: popup.close()
          }

          DS.Button {
            name: "Yes"
            onClicked: {
              windowManager.requestCloseWindow()
              popup.close()
            }
          }
        }
      }
    }
  }
}

