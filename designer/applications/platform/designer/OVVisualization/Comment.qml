// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import OVControls as OV
import OVStyle as OV
import "." as OV

FocusScope {
  id: _scope
  required property int index
  required property var model
  required property string identifier
  required property string text
  required property point center

  signal removed(boxIdx: int)

  x: _control.x
  y: _control.y
  width: _control.width
  height: _control.height

  TextArea {
    id: _control
    width: contentWidth + OV.Style.mediumPadding
    height: contentHeight + OV.Style.mediumPadding

    background: Rectangle {
      radius: OV.Style.panelRadius
      border.color: _scope.focus ? OV.Style.colors.embossColor : OV.Style.colors.baseColor
      border.width: _scope.focus ? 3 : 1
      color: _scope.focus ? OV.Style.colors.lightYellow : OV.Style.colors.noteColor

      opacity: _dragArea.containsMouse ? 0.8 : 1.0
    }

    font: OV.Style.fonts.nodeBody
    color: OV.Style.colors.textColorDarkNeutral
    textFormat: TextEdit.RichText
    text: "<pre>" + _scope.text + "</pre>" // necessary for multi line text since there is no <br>
  }

  CommentEditDialog {
    id: _commentEditDialog
    visible: false
    onAccepted: {
      _scope.text = _commentEditDialog.text
    }
  }

  onXChanged: {
    if(_dragArea.drag.active) {
      _scope.model.modelData.notifyCenterChanged(x + width/2 ,y + height/2)
    }
  }

  onYChanged: {
    if(_dragArea.drag.active) {
      _scope.model.modelData.notifyCenterChanged(x + width/2, y + height/2)
    }
  }

  // DRAG
  Drag.active: _dragArea.drag.active
  MouseArea {
    id: _dragArea
    anchors.centerIn: parent
    width: parent.width
    height: parent.height
    drag.target: parent
    hoverEnabled: true

    onClicked: {
      _scope.focus = true
    }
    onDoubleClicked: {
      _commentEditDialog.text = _scope.text
      _commentEditDialog.x = _scope.x
      _commentEditDialog.y = _scope.y
      _commentEditDialog.visible = true
    }
  }

  Keys.onPressed: (event)=> {
    if (event.key == Qt.Key_Delete) {
      event.accepted = true
      _scope.focus = false
      _scope.removed(_scope.index)
    }
  }

    Component.onCompleted: {
    _scope.x = center.x - _control.width/2 
    _scope.y = center.y - _control.height/2
  }

}