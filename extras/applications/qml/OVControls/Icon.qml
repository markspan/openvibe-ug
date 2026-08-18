import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

import OVStyle as OV

Control
{
    id: _control

    required property string icon
    property bool showIcon: icon !== ""
    property int size: OV.Style.iconMedium
    property color color: OV.Style.colors.textColorBase

    readonly property real _implicitSize: icon ? size : 0

    implicitWidth: _implicitSize
    implicitHeight: _implicitSize

    Image {
        id: _image
        anchors.fill: _control
        source: showIcon ? "resources" + "/" + _control.icon + ".svg" : ""
        sourceSize: Qt.size(_control.size, _control.size)
        smooth: true
        visible: false
    }
    
    ColorOverlay{
        anchors.fill: _image
        source: _image
        color: _control.color
        smooth: false
    }
}
