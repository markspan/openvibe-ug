// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Layouts

import ".."

Rectangle {
    id: root
    
    property int thickness: 1
    property color separatorColor: Theme.stroke
    property int rightMargin: 0
    property int leftMargin: 0
    
    Layout.preferredHeight: thickness
    Layout.fillWidth: true
    Layout.rightMargin: rightMargin
    Layout.leftMargin: leftMargin
    
    height: thickness
    color: separatorColor
}