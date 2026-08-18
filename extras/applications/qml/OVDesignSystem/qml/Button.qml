// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import ".."

AbstractButton {
  id: root

    // Button properties
  required property string name

  // Size the button with some padding around the icon
  implicitWidth: _label.implicitWidth + 2 * Theme.spacingLg
  implicitHeight: _label.implicitHeight + 2 * Theme.spacingSm

  // Background
  background: Rectangle {
    radius: Theme.radiusXs
    color: {
      if (!root.enabled) {
        return Theme.disabled
      } else if (root.hovered) {
        return Theme.hover
      }
      return Theme.primary
    }
  }

  Label {
    id: _label
    anchors.centerIn: parent
    anchors.leftMargin: Theme.spacingLg
    anchors.rightMargin: Theme.spacingLg
    anchors.topMargin: Theme.spacingSm
    anchors.bottomMargin: Theme.spacingSm

    text: root.name
    font: Theme.textMediumRegular
    color: { 
      if (!root.enabled) {
        return Theme.textDisabled
      } else if (root.hovered) {
        return Theme.textPrimary
      }
      return Theme.white
    }
  }
}

