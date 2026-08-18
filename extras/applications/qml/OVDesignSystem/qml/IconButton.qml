import QtQuick
import QtQuick.Controls

import ".."

AbstractButton {
    id: root
    
    property string tooltip: ""

    // Icon properties
    property string iconName: ""
    property color iconColor: Theme.iconPrimary
    property color iconHoverColor: Theme.iconHoverPrimary
    property color iconPressedColor: iconHoverColor
    property int iconSize: Theme.iconLg
    property int iconPressedSize: iconSize
    // Button properties
    property color backgroundColor: "transparent"
    property color backgroundHoverColor: "transparent"
    property color backgroundPressedColor: "transparent"
    property int radius: 0
    
    // Size the button with some padding around the icon
    implicitWidth: iconSize
    implicitHeight: iconSize

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
    
    // Background
    background: Rectangle {
        radius: root.radius
        color: {
            if (root.pressed) {
                return root.backgroundPressedColor
            } else if (root.hovered) {
                return root.backgroundHoverColor
            }
            return root.backgroundColor
        }
    }
    
    ToolTip {
        text: root.tooltip
        visible: root.hovered
    }

    // Icon
    Icon {
        id: icon
        anchors.centerIn: parent
        name: root.iconName
        size: root.pressed ? root.iconPressedSize : root.iconSize
        color: {
            if (!root.enabled) {
                return Theme.iconDisabled
            } else if (root.pressed) {
                return root.iconPressedColor
            } else if (root.hovered) {
                return root.iconHoverColor
            }
            return root.iconColor
        }
    }
}