import QtQuick
import QtQuick.Controls

import OVStyle as OV

Rectangle {
    id: _root
    anchors.fill: parent

    required property bool scaleToFit

    property string image

    color: "black"

    Image {
        id: imageDisplay
        // center image
        width: parent.width
        height: parent.height
        x: (parent.width / 2) - (width / 2)
        y: (parent.height / 2) - (height / 2)
        source: image ? image : ""
        fillMode: image ? (_root.scaleToFit ? Image.PreserveAspectFit : Image.Pad) : Image.Pad

    }

    Component.onCompleted: console.log("ScaleToFit: ", scaleToFit)
}