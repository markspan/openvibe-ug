// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import ".."

Item {
    id: root
    
    // Public properties
    property string name: ""
    property color color: Theme.textPrimary
    property int size: Theme.iconLg
    
    // Size the component based on size
    width: size
    height: size

    Image {
        id: image
        anchors.fill: parent
        source: root.name ? "../resources/icons/" + root.name + ".svg" : ""
        sourceSize: Qt.size(size, size)
        fillMode: Image.PreserveAspectFit
        
        // Apply color using MultiEffect
        layer.enabled: true
        layer.effect: MultiEffect {
            colorizationColor: root.color
            colorization: 1.0
        }
    }
}