/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-FileCopyrightText: Inria <julien.wintz@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

import QtQuick            2.15
import QtQuick.Controls   2.15
import QtQuick.Layouts    1.15

import "." as OV
import OVStyle as OV

Rectangle {
    id: self;

    property int orientation: Qt.Horizontal;
    property string tooltip: "";

    property int longSide: 100;
    property int shortSide: 13;

    signal move(delta: real)

    width: orientation == Qt.Horizontal ? longSide : shortSide;
    height: orientation == Qt.Horizontal ? shortSide : longSide;
    radius: shortSide/2;

    color: OV.Style.colors.gutterColor;

    border.width: 1;
    border.color: OV.Style.colors.embossColorNeutral;

    signal clicked;

    Rectangle {
        id: _center_rectangle
        anchors.centerIn: parent;

         width: orientation == Qt.Horizontal
            ? parent.width - parent.height
            : shortSide/4;
        height: orientation == Qt.Horizontal
            ? shortSide/4
            : parent.height - parent.width;

        radius: 1;
        color: _mouseArea.hovered ? OV.Style.colors.hoveredBaseColor : OV.Style.colors.embossColorNeutral;
    }

    MouseArea {
        id: _mouseArea
        property real oldMousePos
        anchors.fill: parent;

        hoverEnabled: true;
        onClicked: self.clicked();

        onPressed: {
            if(orientation == Qt.Horizontal) {
                oldMousePos = mouseY;
            } else {
                oldMousePos = mouseX;
            }
        }

        onPositionChanged: {
            if(pressed) {
                if(orientation == Qt.Horizontal) {
                    self.move(oldMousePos - mouseY)
                    oldMousePos = mouseY;
                } else {
                    self.move(oldMousePos - mouseX)
                    oldMousePos = mouseX;
                }
            }
        }
    }

    OV.ToolTip {
        text: self.tooltip;
        visible: self.tooltip && _mouseArea.containsMouse
    }
}
