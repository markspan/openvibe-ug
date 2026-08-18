// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic.impl

import OVStyle as OV

ProgressBar {
    id: control
    value: 0.

    background: Rectangle {
        implicitWidth: OV.Style.controlWidth
        implicitHeight: OV.Style.progressBar
        color: OV.Style.colors.bgColor
        radius: 3
    }

    contentItem: Item {
        implicitWidth: OV.Style.controlWidth
        implicitHeight: OV.Style.progressBar

        Rectangle {
            //width: control.visualPosition * parent.width
            x: control.mirrored ? (1. - control.position)*parent.width : 0
            width: control.position*parent.width
            height: parent.height
            radius: 2
            color: OV.Style.colors.embossColorBlue
        }
    }
}