import QtQuick
import QtQuick.Controls

import OVStyle as OV
import "." as OV

RoundButton {
  width: OV.Style.iconSmall
  height: OV.Style.iconSmall
  text: "\u2717" //sign cross
  background: Rectangle {
    opacity: hovered ? 0.5 : 1.
    color: OV.Style.colors.dangerColor
    radius: OV.Style.iconSmall
  }
}
