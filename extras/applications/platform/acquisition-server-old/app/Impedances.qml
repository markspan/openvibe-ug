// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import OVControls as OV
import OVStyle as OV
import OVParameters as OV

Control {
  id: _control
  property alias model: _grid.model
  signal finished()

  bottomPadding: OV.Style.mediumPadding

  Text {
    id: _title
    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
      
    text: "Impedances Measurements"
    font: OV.Style.fonts.header
    color: OV.Style.colors.textColorDarkNeutral
  }

  GridView {
    id: _grid
    anchors.top: _title.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: _finish_button.top

    leftMargin: OV.Style.largePadding
    rightMargin: OV.Style.largePadding
    bottomMargin: OV.Style.mediumPadding
    topMargin: 0

    contentWidth: width
    contentHeight: cellHeight * (_control.model.length * cellWidth / width)

    clip: true
    cellWidth : 100 + OV.Style.largeColumnSpacing
    cellHeight: 150 + OV.Style.largeColumnSpacing

    ScrollBar.vertical: ScrollBar {
      parent: _grid.parent
      anchors.top: _grid.top
      anchors.right: _grid.right
      anchors.bottom: _grid.bottom
    }

    delegate: Rectangle {
      id: rectangleEntity
        
      required property string name
      required property real value
      required property real fraction

      width: 100 //parent.cellWidth
      height: 150 //parent.cellHeight

      border.color: rectangleEntity.fraction < 0.25 ? OV.Style.colors.alternateFgColor : OV.Style.colors.dangerColor
      border.width: OV.Style.borderWidth
      radius: OV.Style.buttonRadius

      gradient: Gradient {
        GradientStop { position: 0.1; color: OV.Style.colors.dangerColor }
        GradientStop { position: 0.75; color: OV.Style.colors.noteColor }
        GradientStop { position: 1.0; color: OV.Style.colors.okColor }
      } 

      Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        width: parent.width - 2*parent.border.width
        height: (parent.height - 2*parent.border.width) * (1-rectangleEntity.fraction) 
        color: "white"
      }

      Text {
        id: _textName
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        verticalAlignment: Text.AlignTop
        text: rectangleEntity.name
        font: OV.Style.fonts.menu
        color: rectangleEntity.fraction < 0.25 ? OV.Style.colors.okColor : 
                rectangleEntity.fraction > 0.75 ? OV.Style.colors.dangerColor : 
                OV.Style.colors.noteColor 
      }

      Text {
        anchors.top: rectangleEntity.fraction < 0.25 ? _textName.bottom : _textValue.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        verticalAlignment: Text.AlignTop
        text: (rectangleEntity.fraction < 0.25 ? "GOOD" : 
              rectangleEntity.fraction > 0.99 ? "TOO HIGH" : "BAD")
        font: OV.Style.fonts.buttonSmall
        color: rectangleEntity.fraction < 0.25 ? OV.Style.colors.okColor : OV.Style.colors.dangerColor
      }

      Text {
        id: _textValue
        anchors.centerIn: parent
        font: OV.Style.fonts.value
        text: rectangleEntity.value.toFixed(1) + " kOhm"
        color: OV.Style.colors.textColorDarkNeutral
      }  
    }
  }

  OV.Button {
    id: _finish_button
    anchors.horizontalCenter : parent.horizontalCenter
    anchors.bottom: parent.bottom
    text: "Finish"
    onClicked: {
      _control.finished()
    }
  }
}