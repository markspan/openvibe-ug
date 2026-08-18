// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Layouts

import ".."

Rectangle {
    id: root
    
    property int thickness: 1
    property color separatorColor: Theme.stroke
    property int topMargin: 0
    property int bottomMargin: 0
    
    Layout.preferredWidth: thickness
    Layout.fillHeight: true
    Layout.topMargin: topMargin
    Layout.bottomMargin: bottomMargin
    
    width: thickness
    color: separatorColor
}