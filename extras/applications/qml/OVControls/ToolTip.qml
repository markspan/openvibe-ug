import QtQuick
import QtQuick.Controls

import OVStyle as OV

ToolTip {
    id: _control

    implicitHeight: OV.Style.sizes.s5
    implicitWidth: contentWidth + 2*OV.Style.mediumPadding

    delay: 500

    contentItem: Text{
        anchors.fill:parent

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        color: OV.Style.colors.neutralColor
        font: OV.Style.fonts.value
        text: _control.text
    }

    background: Rectangle {
        color: OV.Style.colors.fgColor
        radius: OV.Style.panelRadius

        opacity: 0.9
    }
}

//
// ToolTip.qml ends here
