// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS


Item {
  id: root

  DS.IconButton {
    id: _settingsIcon
    anchors.fill: parent
    iconName: "settings"
    iconSize: DS.Theme.iconMd
    iconPressedSize: DS.Theme.iconSm

    onClicked: {
      _settingsPopup.open()
    }
  }

  SettingsPopup {
    id: _settingsPopup
    model: MenuModel.mainMenu

    x: _settingsIcon.x + _settingsIcon.width - _settingsPopup.width
    y: _settingsIcon.y + root.height
  }
  
}