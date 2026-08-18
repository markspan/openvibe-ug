/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import DesignSystem as DS

Rectangle {
  id: _root

  color: DS.Theme.white
  border.color: DS.Theme.borderPrimary
  border.width: 1
  radius: DS.Theme.radiusMd

  width: _contentLayout.implicitWidth + 2 * DS.Theme.spacingXl
  height: _contentLayout.implicitHeight + 2 * DS.Theme.spacingMd

  Rectangle {
    id: _shadow

    anchors.fill: parent
    radius: parent.radius
    color: parent.color
    z: -1

    layer.enabled: true
    layer.effect: MultiEffect {
      shadowEnabled: true
      shadowColor: "#000000"
      shadowOpacity: 0.3
      shadowBlur: 0.35
      shadowHorizontalOffset: 0.42
      shadowVerticalOffset: 1.66
    }
  }

  RowLayout {
    id: _contentLayout
    anchors.fill: parent
    anchors.topMargin: DS.Theme.spacingMd
    anchors.bottomMargin: DS.Theme.spacingMd
    anchors.leftMargin: DS.Theme.spacingXl
    anchors.rightMargin: DS.Theme.spacingXl

    spacing: DS.Theme.spacingXl

    DS.IconButton {
      iconName: "play"
      iconSize: DS.Theme.icon3xl
      tooltip: "Start the acquisition"

      backgroundHoverColor: DS.Theme.iconHoverSecondary
      backgroundPressedColor: DS.Theme.iconPressed
      iconHoverColor: DS.Theme.iconPrimary
      iconPressedColor: DS.Theme.white
      radius: DS.Theme.radiusSm

      enabled: !controller.isAcquiring && !controller.isCheckingImpedance

      onClicked: {
        controller.startAcquisition()
      }
    }

    DS.VerticalSpacer {
      topMargin: DS.Theme.spacingXs
      bottomMargin: DS.Theme.spacingXs
    }

    DS.IconButton {
      iconName: "impedance-check"
      iconSize: DS.Theme.icon3xl
      tooltip: controller.supportsImpedanceCheck
               ? (controller.isCheckingImpedance ? "Stop impedance check" : "Check impedances")
               : "Not supported by this driver"

      backgroundHoverColor: DS.Theme.iconHoverSecondary
      backgroundPressedColor: DS.Theme.iconPressed
      iconHoverColor: controller.isCheckingImpedance ? DS.Theme.primary : DS.Theme.iconPrimary
      iconPressedColor: DS.Theme.white
      radius: DS.Theme.radiusSm

      enabled: !controller.isAcquiring && controller.supportsImpedanceCheck

      onClicked: {
        controller.toggleImpedanceCheck()
      }
    }

    DS.VerticalSpacer {
      topMargin: DS.Theme.spacingXs
      bottomMargin: DS.Theme.spacingXs
    }

    DS.IconButton {
      iconName: "stop"
      iconSize: DS.Theme.icon3xl
      tooltip: "Stop the scenario"
      backgroundHoverColor: DS.Theme.iconHoverSecondary
      backgroundPressedColor: DS.Theme.iconPressed
      iconHoverColor: DS.Theme.iconPrimary
      iconPressedColor: DS.Theme.white
      radius: DS.Theme.radiusSm

      enabled: controller.isAcquiring || controller.isCheckingImpedance

      onClicked: {
        if (controller.isCheckingImpedance) {
          controller.toggleImpedanceCheck()
        } else {
          controller.stopAcquisition()
        }
      }
    }
  }
}
