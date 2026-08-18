import QtQuick
import QtQuick.Controls

import OVStyle as OV

Slider {
    id: _control

    //readonly property alias gaugeWidth: _gauge.width
    property bool useRadius: false

    implicitWidth: OV.Style.controlWidth
    implicitHeight: OV.Style.gutterHeight
    padding: 6

    background: Rectangle {
        id: _gutter

        radius: _control.useRadius ? _gauge.radius + _control.padding : 0
        color: OV.Style.colors.gutterColor
    }

    contentItem: Rectangle {
        id: _gauge

        x: _control.leftPadding
        width: _control.visualPosition * _control.availableWidth
        height: _control.availableHeight

        radius: OV.Style.panelRadius
        color: OV.Style.colors.textColorBase
    }

    handle: Rectangle {
        x: _control.leftPadding + _control.visualPosition * (_control.availableWidth - width)
        y: _control.topPadding + _control.availableHeight / 2 - height / 2
        implicitHeight: _gauge.height
        implicitWidth: _gauge.height
        radius: _gauge.height / 2
        color: _control.pressed || _control.hovered ? OV.Style.colors.hoveredBaseColor : OV.Style.colors.textColorBase
    }

}
