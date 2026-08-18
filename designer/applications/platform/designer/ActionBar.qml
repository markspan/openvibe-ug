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

  width: _contentLayout.implicitWidth + 2 * DS.Theme.spacing3xl
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
    anchors.leftMargin: DS.Theme.spacing3xl
    anchors.rightMargin: DS.Theme.spacing3xl

    spacing: DS.Theme.spacing3xl

    DS.IconButton {
      iconName: {
        if (OVDesigner.playerStatus === "Play") {
          return "pause"
        }
        return "play"
      }
      iconSize: DS.Theme.icon7xl
      tooltip: {
        if (OVDesigner.playerStatus === "Play") {
          return "Pause the scenario"
        }
        return "Start the scenario"
      }
      backgroundHoverColor: DS.Theme.iconHoverSecondary
      backgroundPressedColor: DS.Theme.iconPressed
      iconHoverColor: DS.Theme.iconPrimary
      iconPressedColor: DS.Theme.white
      radius: DS.Theme.radiusSm

      onClicked: {
        if (OVDesigner.playerStatus === "Stop" || OVDesigner.playerStatus === "Forward") {
          OVDesigner.startPlayer()
        } else {
          OVDesigner.pauseOrResumePlayer()
        }
      }
    }

    DS.VerticalSpacer {
      topMargin: DS.Theme.spacingXs
      bottomMargin: DS.Theme.spacingXs
    }

    DS.IconButton {
      iconName: "stop"
      iconSize: DS.Theme.icon7xl
      tooltip: "Stop the scenario"
      backgroundHoverColor: DS.Theme.iconHoverSecondary
      backgroundPressedColor: DS.Theme.iconPressed
      iconHoverColor: DS.Theme.iconPrimary
      iconPressedColor: DS.Theme.white
      radius: DS.Theme.radiusSm

      enabled: OVDesigner.playerStatus !== "Stop"

      onClicked: {
        OVDesigner.stopPlayer()
      }
    }

    DS.VerticalSpacer {
      topMargin: DS.Theme.spacingXs
      bottomMargin: DS.Theme.spacingXs
    }

    DS.IconButton {
      iconName: "play-fast"
      iconSize: DS.Theme.icon7xl
      tooltip: "Fast-forward the scenario"
      backgroundHoverColor: DS.Theme.iconHoverSecondary
      backgroundPressedColor: DS.Theme.iconPressed
      iconHoverColor: DS.Theme.iconPrimary
      iconPressedColor: DS.Theme.white
      radius: DS.Theme.radiusSm

      onClicked: {
        OVDesigner.startPlayerFast()
      }
    }

    DS.VerticalSpacer {
      topMargin: DS.Theme.spacingXs
      bottomMargin: DS.Theme.spacingXs
    }

    RowLayout {
      id: _simulatedTimeLayout
      Layout.fillHeight: true
      Layout.preferredWidth: _timeLabel.implicitWidth + _msLabel.implicitWidth
      spacing: 0

      Label {
        id: _timeLabel
        text: OVDesigner.simulatedTime + "."
        font.family: "Cascadia Mono" // Monospaced font: avoid resizing depending on the digits
        font.weight: DS.Theme.textMediumMedium.weight
        font.pixelSize: DS.Theme.textMediumMedium.pixelSize
        color: DS.Theme.textPrimary
        Layout.alignment: Qt.AlignVCenter
      }

      Label {
        id: _msLabel
        text: OVDesigner.simulatedTimeMs
        font.family: "Cascadia Mono" // Monospaced font: avoid resizing depending on the digits
        font.weight: DS.Theme.textMediumMedium.weight
        font.pixelSize: DS.Theme.textMediumMedium.pixelSize
        color: DS.Theme.textPrimary
        opacity: 0.6
        Layout.alignment: Qt.AlignVCenter
      }
    }
    
    DS.VerticalSpacer {
      topMargin: DS.Theme.spacingXs
      bottomMargin: DS.Theme.spacingXs
    }
  }
}