// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import ".."

Switch {
  id: _switch

  leftPadding: 0
  rightPadding: 0
  spacing: 0

  indicator: Rectangle {
    implicitWidth: Theme.switchWidth
    implicitHeight: Theme.switchHeight

    radius: Theme.radiusMd

    color: _switch.checked ? Theme.textPrimary : Theme.white
    border.color: Theme.textPrimary
    
    Rectangle {
        x: _switch.checked ? parent.width - width - Theme.spacing2xs : Theme.spacing2xs
        y: Theme.spacing2xs
        width: Theme.switchCursorDiameter
        height: Theme.switchCursorDiameter
        radius: Theme.switchCursorDiameter  // more than enough to make it round
        color: _switch.checked ? Theme.white : Theme.textPrimary

        layer.enabled: true
        layer.effect: MultiEffect {
          shadowEnabled: true
          shadowColor: _switch.checked ? Theme.quaternary : Theme.primary
          shadowOpacity: 0.4
          shadowBlur: 0
          shadowHorizontalOffset: _switch.checked ? -1 : 1
        }
    }
  }

}