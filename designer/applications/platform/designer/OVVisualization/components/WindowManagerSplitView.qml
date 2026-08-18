// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import DesignSystem as DS

SplitView {
  id: root
  
  handle: Rectangle {
    id: handle
    color: "transparent"
    radius: DS.Theme.radiusXs
    implicitWidth: DS.Theme.spacing2xs
    implicitHeight: DS.Theme.spacing2xs

    containmentMask: Item {
      x: (handle.width - width) / 2
      y: (handle.height - height) / 2
      width: root.orientation == Qt.Horizontal ? handle.width : handle.width - 2 * DS.Theme.spacing2xs
      height: root.orientation == Qt.Horizontal ? handle.height - 2 * DS.Theme.spacing2xs : handle.height
    }

    Rectangle {
      anchors.fill: parent
      color: SplitHandle.pressed ? DS.Theme.secondary
           : (SplitHandle.hovered ? DS.Theme.hover : "transparent")
      
    }
  }

}