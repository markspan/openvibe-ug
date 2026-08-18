//Qt
import QtQuick
import QtQuick.Controls

import OVStyle as OV
import "." as OV

Control {

    id: _control

    default property alias content: _panel.children

    //this comes in handy to size a panel wrt to the
    //its children, or any other thing that requires
    //to access the actual panel
    property Control panel: _panel

    //padding: 2
    implicitWidth: OV.Style.smallPanelWidth
    implicitHeight: OV.Style.smallPanelHeight
    clip: true

    background: Rectangle {
        radius: OV.Style.panelRadius
        color: OV.Style.colors.gutterColor
    }

    Control {

        id: _panel

        anchors.fill: parent
        clip: true

        leftInset: 2
        rightInset: 2
        topInset: 2
        bottomInset: 2

        leftPadding: 2
        rightPadding: 2
        topPadding: 2
        bottomPadding: 2

        background: Rectangle {
            color: OV.Style.colors.bgColor
            border.width: 0.5
            border.color: OV.Style.colors.embossColor
            radius: OV.Style.panelRadius
        }

    }

}
