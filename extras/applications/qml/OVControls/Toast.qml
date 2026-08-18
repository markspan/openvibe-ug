// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only
// from gnomon project

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVStyle as OV
import "." as OV

Popup {
  id: _control

  x: Math.round((parent.width - contentWidth))
  y: Math.round((parent.height - contentHeight))
  clip: true

  implicitHeight: contentHeight;
  implicitWidth: contentWidth;

  margins: OV.Style.smallPadding
  bottomPadding: 2

  property int _timerDuration: 5000
  property int _animationDuration: 300
  property alias header: _header.text
  property alias message: _message.text
  property int type: OV.Style.ButtonType.Base


  enter: Transition {
    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: _animationDuration}
  }

  exit: Transition {
    NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: _animationDuration}
  }
  onOpened : {
    _timer.start()
  }

  Timer {
        id: _timer;
        interval: _timerDuration;
        onTriggered: _control.close();
    }

  background: Rectangle {
    radius: OV.Style.panelRadius

    border.width: 1
    border.color: _getButtonColor()
    color: _getBgColor()

  }
  contentItem: Rectangle {
    implicitHeight: Math.round(OV.Style.smallDialogHeight / 2)
    implicitWidth: OV.Style.smallDialogWidth
    color: _getBgColor()

    OV.Icon {
      id: _icon

      icon: _getIcon()
      size: OV.Style.iconLarge
      visible: true
      color: _getButtonColor()
      anchors {
        top: parent.top
        left: parent.left
      }
    }

    Text {
      id: _header

      horizontalAlignment: Text.AlignLeft
      verticalAlignment: Text.AlignVCenter
      font: OV.Style.fonts.header
      color: _getTextColor()

      anchors {
        verticalCenter: _icon.verticalCenter
        left: _icon.right
        margins: OV.Style.smallPadding
      }
    }

    Text {
      id: _message

      wrapMode: Text.Wrap
      horizontalAlignment: Text.AlignLeft
      font: OV.Style.fonts.value
      color: _getTextColor()

      anchors {
        top: _header.bottom
        left: _icon.right
        right: _button.left
        margins: OV.Style.smallPadding
      }
    }

    OV.Button {
      id: _button

      anchors.bottom: parent.bottom
      anchors.right: parent.right
      anchors.margins: OV.Style.smallPadding

      text: "Got it"
      type: _control.type
      width: 2*OV.Style.buttonWidth/3

      onClicked: {
        _control.close();
      }
    }
  }

  function _getButtonColor() {
    if(_control.type === OV.Style.ButtonType.Danger)
      return OV.Style.colors.dangerColor

    if(_control.type === OV.Style.ButtonType.Warning)
      return OV.Style.colors.warningColor

    if(_control.type === OV.Style.ButtonType.OK)
      return OV.Style.colors.okColor

    if(_control.type === OV.Style.ButtonType.Base)
      return OV.Style.colors.baseColor

    return OV.Style.colors.neutralColor
  }

  function _getTextColor() {
    if(_control.type === OV.Style.ButtonType.Danger)
      return OV.Style.colors.textColorDarkDanger

    if(_control.type === OV.Style.ButtonType.Warning)
      return OV.Style.colors.textColorDarkWarning

    if(_control.type === OV.Style.ButtonType.OK)
      return OV.Style.colors.textColorDarkOk

    if(_control.type === OV.Style.ButtonType.Base)
      return OV.Style.colors.textColorDarkBase

    return OV.Style.colors.textColorDarkNeutral
  }


  function _getBgColor() {
    if(_control.type === OV.Style.ButtonType.Danger)
      return OV.Style.colors.textColorLightDanger

    if(_control.type === OV.Style.ButtonType.Warning)
      return OV.Style.colors.textColorLightWarning

    if(_control.type === OV.Style.ButtonType.OK)
      return OV.Style.colors.textColorLightOk

    if(_control.type === OV.Style.ButtonType.Base)
      return OV.Style.colors.textColorLightBase

    return OV.Style.colors.textColorLightNeutral
  }

  function _getIcon() {
    if(_control.type === OV.Style.ButtonType.Danger)
      return "close-circle-outline"

    if(_control.type === OV.Style.ButtonType.Warning)
      return "alert-circle-outline"

    if(_control.type === OV.Style.ButtonType.OK)
      return "check-circle-outline"

    if(_control.type === OV.Style.ButtonType.Base)
      return "information-outline"

    return "chevron-right-circle-outline"
  }

}
