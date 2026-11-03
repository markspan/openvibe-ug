// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

import QtQuick
import QtQuick.Controls

/// The letter grid of the P300 speller, with the target and result text under it.
/// The box writes styles, targetText and resultText. A style indexes the three lists of looks: no flash, flash, target, selected.
Rectangle {
    id: _root

    required property int rows
    required property int columns
    required property var labels
    required property var backgroundColors
    required property var foregroundColors
    required property var fontSizes

    property var styles: []
    property string targetText
    property string resultText

    property bool showTarget: true
    property bool showResult: true

    readonly property real cellWidth: width / Math.max(columns, 1)
    readonly property real cellHeight: _grid.height / Math.max(rows, 1)

    // Font sizes are in points. A small panel scales them all alike, so that a flash stays larger than no flash.
    readonly property real pixelsPerPoint: Screen.logicalPixelDensity * 25.4 / 72
    readonly property real fontScale: {
        const largest = Math.max(fontSizes[0], fontSizes[1], fontSizes[2], fontSizes[3]) * pixelsPerPoint
        return largest > 0 ? Math.min(1, 0.8 * Math.min(cellWidth, cellHeight) / largest) : 1
    }

    anchors.fill: parent
    color: backgroundColors[0]

    Item {
        id: _grid

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: _text.visible ? _text.top : parent.bottom

        Repeater {
            model: _root.rows * _root.columns

            delegate: Rectangle {
                id: _cell

                required property int index

                readonly property int cellStyle: _root.styles[index] ?? 0

                x: (index % _root.columns) * _root.cellWidth
                y: Math.floor(index / _root.columns) * _root.cellHeight
                width: _root.cellWidth
                height: _root.cellHeight
                color: _root.backgroundColors[cellStyle]

                Text {
                    anchors.centerIn: parent
                    text: _root.labels[_cell.index]
                    textFormat: Text.PlainText
                    color: _root.foregroundColors[_cell.cellStyle]
                    font.pixelSize: Math.max(1, _root.fontSizes[_cell.cellStyle] * _root.pixelsPerPoint * _root.fontScale)
                }
            }
        }
    }

    Rectangle {
        id: _text

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: _textColumn.implicitHeight + 12
        visible: _root.showTarget || _root.showResult
        color: "#101010"

        Column {
            id: _textColumn

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 8
            spacing: 2

            Text {
                visible: _root.showTarget
                text: "Target : " + _root.targetText
                textFormat: Text.PlainText
                color: "#c0c0c0"
                font.pixelSize: 16
            }

            Text {
                visible: _root.showResult
                text: "Result : " + _root.resultText
                textFormat: Text.StyledText
                color: "#c0c0c0"
                font.pixelSize: 16
            }
        }
    }

    // Out of sight, as the toolbar was: nothing on the stimulus screen but the grid and the text
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: _menu.popup()
    }

    Menu {
        id: _menu

        MenuItem {
            text: "Show target text"
            checkable: true
            checked: _root.showTarget
            onToggled: _root.showTarget = checked
        }

        MenuItem {
            text: "Show result text"
            checkable: true
            checked: _root.showResult
            onToggled: _root.showResult = checked
        }
    }
}
