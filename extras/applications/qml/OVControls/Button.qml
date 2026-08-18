import QtQuick
import QtQuick.Controls

import OVStyle as OV
import "." as OV

Button {

  id: _control

// /////////////////////////////////////////////////////////////////////////////
// Basic properties
// /////////////////////////////////////////////////////////////////////////////


  property int type: OV.Style.ButtonType.Base
  property int size: OV.Style.ButtonSize.Medium
  property bool empty: false
  property string iconName: ""
  property string tooltip: ""
  property bool isHovered: (_control.hovered || _content.hovered) && _control.hoverEnabled && _control.enabled

  flat: false

  implicitWidth: Math.max(OV.Style.buttonWidth, implicitContentWidth + leftPadding + rightPadding)
  implicitHeight: _getButtonHeight()
  hoverEnabled: enabled

// /////////////////////////////////////////////////////////////////////////////
// Content Item
// /////////////////////////////////////////////////////////////////////////////

  contentItem: Control {

    id: _content
    implicitWidth: _icon.width + _separator.width + _text.contentWidth + 2*OV.Style.buttonPadding

    OV.Icon {
        id: _icon

        anchors.left: parent.left;
        anchors.top: parent.top;
        anchors.bottom: parent.bottom
        width: height

        size: (2/3) * _control.size

        visible: _control.iconName
        icon: _control.iconName
        color: _control.flat || _control.empty ? _getBgColor() : _getTextColor()
    }

    Rectangle {
        id: _separator

        anchors.left: _icon.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: OV.Style.buttonPadding

        height: _control.height - 4
        width: 2

        visible: (_control.iconName && _control.text) && !_control.flat
        color: _control.empty ? _getBgColor() : _getTextColor()

    }

    Label {
        id: _text

        anchors.right: parent.right;
        anchors.left: _control.iconName ? _separator.right : parent.left;
        anchors.verticalCenter: parent.verticalCenter
        leftPadding: OV.Style.buttonPadding
        rightPadding: OV.Style.buttonPadding

        text: _control.text.toUpperCase()
        font: _control.size === OV.Style.ButtonSize.Small? OV.Style.fonts.buttonSmall : OV.Style.fonts.button
        visible: _control.text.length > 0
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        color: _control.flat || _control.empty ? _getBgColor() : _getTextColor()
    }
  }

  OV.ToolTip {
    text: _control.tooltip;
    visible: _control.tooltip && _control.isHovered
  }

// /////////////////////////////////////////////////////////////////////////////
// Background
// /////////////////////////////////////////////////////////////////////////////

  background: Rectangle {

    id: _background

    radius: OV.Style.buttonRadius

    color: _control.empty || _control.flat ? "transparent" : _getBgColor()
    border.color: _getBgColor()
    border.width: _control.empty ? 2 : 0

  }

// /////////////////////////////////////////////////////////////////////////////
// Hover
// /////////////////////////////////////////////////////////////////////////////

  HoverHandler {
    cursorShape: Qt.PointingHandCursor
  }

// /////////////////////////////////////////////////////////////////////////////
// States
// /////////////////////////////////////////////////////////////////////////////

  states: [

    State {
      name: "hovered"
      when: (_control.hovered || _content.hovered) && _control.hoverEnabled && _control.enabled
      PropertyChanges {
        target: _text;
//        font: OV.Style.fonts.buttonHovered
        color: _getTextColor()
      }
      PropertyChanges {
        target: _icon;
        color: _getTextColor()
      }
      PropertyChanges {
        target: _separator;
        color: _getTextColor()
      }
      PropertyChanges {
        target: _background;
        color: _getHoveredBgColor()
        border.width: 0
      }
    }

  ]


// /////////////////////////////////////////////////////////////////////////////
// Functions
// /////////////////////////////////////////////////////////////////////////////


  function _getBgColor() {

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

  function _getHoveredBgColor() {

    if(_control.type === OV.Style.ButtonType.Danger)
      return OV.Style.colors.hoveredDangerColor

    if(_control.type === OV.Style.ButtonType.Warning)
      return OV.Style.colors.hoveredWarningColor

    if(_control.type === OV.Style.ButtonType.OK)
      return OV.Style.colors.hoveredOkColor

    if(_control.type === OV.Style.ButtonType.Base)
      return OV.Style.colors.hoveredBaseColor

    return OV.Style.colors.hoveredNeutralColor
  }

  function _getTextColor() {

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

  function _getButtonHeight() {

      if(_control.size === OV.Style.ButtonSize.Small)
        return OV.Style.smallButtonHeight

      if(_control.size === OV.Style.ButtonSize.Large)
        return OV.Style.largeButtonHeight

        return OV.Style.mediumButtonHeight
    }

}

//
// Button.qml ends here
