// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ".."

CheckBox {
    id: control

    implicitWidth: Theme.icon3xl
    implicitHeight: Theme.icon3xl

    checkable: true
    checked: false

    indicator: Rectangle {
        anchors.fill: parent
        
        radius: Theme.radiusXs
        border.color: {
          if (control.checked) {
            if (!control.checkable) {
              return "transparent"
            } if (control.hovered) {
              return Theme.iconHoverPrimary
            }
            return "transparent"
          } else {
            if (!control.checkable) {
              return Theme.iconDisabled
            } else if (control.hovered) {
              return "transparent"
            }
            return Theme.iconPressed
          }
        }

        color: {
          if (control.checked) {
            if (!control.checkable) {
              return Theme.iconDisabled
            } else if (control.hovered) {
              return Theme.white
            }
            return Theme.iconPressed
          } else {
             if (!control.checkable) {
              return Theme.iconDisabled
            } else if (control.hovered) {
              return Theme.iconHoverSecondary
            }
            return "transparent"
          }

        }

        Icon {
            anchors.centerIn: parent
            name: "check"
            size: Theme.icon2xl
            color: {
              if (control.checked) {
                if (!control.checkable) {
                  return Theme.iconSecondary
                } else if (control.hovered) {
                  return Theme.iconHoverPrimary
                }
                return Theme.white
              } else {
                if (!control.checkable) {
                  return "transparent"
                }
                if (control.hovered) {
                  return Theme.iconHoverPrimary
                } 
                return "transparent"
              }
            }
        }
    }
}