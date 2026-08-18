/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import OVControls as OV
import OVStyle as OV
import OVVisualization as OV
import DesignSystem as DS

Control {
  id: _control
  required property var boxModel

  background: Rectangle {
    color: DS.Theme.white
    opacity: 0.6
  }

  RowLayout {
    id: _searchRow
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top

    Label {
      id: _searchLabel
      Layout.alignment: Qt.AlignLeft
      text: "Filter: "
      font: OV.Style.fonts.subHeader
      color: OV.Style.colors.textColorBase
    }

    OV.TextField {
      id: _searchFilter
      Layout.alignment: Qt.AlignRight
      Layout.fillWidth: true
      horizontalAlignment: TextInput.AlignHCenter

      color: OV.Style.colors.hoveredBaseColor
      font: OV.Style.fonts.value
      placeholderText: "set a RegExp to filter the boxes"
      placeholderTextColor: OV.Style.colors.textColorFaded

      onTextChanged: {
        boxModel.setFilterRegularExpression(_searchFilter.text)
        if(_searchFilter.text.length > 0) {
          _treeView.expandRecursively()
        } else {
          _treeView.collapseRecursively()
        }
      }
    }
  }

  TreeView {
    id: _treeView
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: _searchRow.bottom
    anchors.bottom: parent.bottom

    clip: true
    rowSpacing: 1
    model: boxModel

    selectionModel: ItemSelectionModel {
      id: _itemSelectionModel
      onSelectionChanged : {
          console.log("selected items:", _itemSelectionModel.selectedIndexes)
      }
    }

    delegate: TreeViewDelegate {
      id: _delegate
      implicitWidth: _control.width
      implicitHeight: OV.Style.smallButtonHeight
     
      indentation: OV.Style.smallPadding
      hoverEnabled: true;
      highlighted: selected || current
      background.opacity: 0.3

      indicator.width: _delegate.height
      indicator.height: _delegate.height

      contentItem : Label {
        id: _label
        //anchors.left:  _indicator.right;
        text: model.display
        font: hasChildren? OV.Style.fonts.subHeader : OV.Style.fonts.value

        Drag.active: _dragHandler.active
        Drag.dragType: Drag.Automatic
        Drag.supportedActions: Qt.CopyAction
        Drag.mimeData: { "text/plain": model.drag }
      }

      DragHandler {
        id: _dragHandler

        xAxis.minimum: _delegate.x
        xAxis.maximum: _delegate.x
        yAxis.minimum: _delegate.y
        yAxis.maximum: _delegate.y

        cursorShape: Qt.DragCopyCursor
        onActiveChanged : {
          if(active) {
            parent.grabToImage(function(result) { _label.Drag.imageSource = result.url; })
          }
        }
      }

      OV.ToolTip {
        text: model.tooltip
        visible: _mouseArea.containsMouse
        delay: 500
      }

      MouseArea {
        id: _mouseArea

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
      }

      TapHandler {
        acceptedButtons: Qt.LeftButton
        acceptedModifiers: Qt.ControlModifier
        onTapped: _itemSelectionModel.select(_treeView.index(index, 0), ItemSelectionModel.Select)
      }

      TapHandler {
        acceptedButtons: Qt.LeftButton
        acceptedModifiers: Qt.NoModifier
        onTapped: {
          if(_delegate.hasChildren) {
            if (_treeView.isExpanded(row)) {
              _treeView.collapseRecursively(row)
            } else {
              _treeView.expand(row)
            }
          } else {
            _itemSelectionModel.select(_treeView.index(index, 0), ItemSelectionModel.ClearAndSelect)
          }
        }
        //onDoubleTapped: TODO signal dropInCanvas _self.fileDoubleClicked(model.filePath)
      }
    }
  }
}
