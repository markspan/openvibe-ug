// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import DesignSystem as DS

/// Channel names, one per stacked lane, aligned with the baselines the renderer reports.
Item {
    id: _root

    required property var trace
    required property var channels
    /// In butterfly every baseline is the same, so per-lane rows would stack on top of each other.
    /// Show a plain legend instead.
    property bool butterfly: false

    Column {
        anchors {
            right: parent.right
            rightMargin: DS.Theme.spacingSm
            verticalCenter: parent.verticalCenter
        }
        width: parent.width - DS.Theme.spacingMd
        spacing: DS.Theme.spacing2xs
        visible: _root.butterfly

        Repeater {
            model: _root.butterfly ? _root.trace.rowChannels : []

            delegate: Row {
                required property int modelData

                readonly property var entry: _root.channels[modelData] ?? null

                spacing: DS.Theme.spacingXs
                anchors.right: parent.right

                Rectangle {
                    width: DS.Theme.spacingSm
                    height: DS.Theme.spacingSm
                    radius: width / 2
                    color: parent.entry ? parent.entry.color : DS.Theme.textPrimary
                    anchors.verticalCenter: parent.verticalCenter
                    visible: _root.trace.perChannelColors
                }

                Label {
                    text: parent.entry ? parent.entry.name : ""
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    elide: Text.ElideLeft
                }
            }
        }
    }

    Repeater {
        model: _root.butterfly ? 0 : _root.trace.rowBaselines.length

        delegate: Item {
            id: _lane

            required property int index

            readonly property int channel: _root.trace.rowChannels[index] ?? -1
            readonly property var entry: _root.channels[channel] ?? null

            width: _root.width
            height: _root.trace.rowHeight
            y: _root.trace.rowBaselines[index] - height / 2

            Column {
                anchors {
                    right: parent.right
                    rightMargin: DS.Theme.spacingSm
                    verticalCenter: parent.verticalCenter
                }
                width: parent.width - DS.Theme.spacingMd
                spacing: 0

                Label {
                    width: parent.width
                    text: _lane.entry ? _lane.entry.name : ""
                    font: DS.Theme.textSmallRegular
                    color: DS.Theme.textPrimary
                    elide: Text.ElideLeft
                    horizontalAlignment: Text.AlignRight
                }

                Label {
                    width: parent.width
                    // Units decorate the lane; they never replace the channel name.
                    visible: text !== "" && _lane.height > DS.Theme.labelHeight2xl
                    text: _lane.entry ? (_lane.entry.unit ?? "") : ""
                    font: DS.Theme.textXsRegular
                    color: DS.Theme.textTertiary
                    elide: Text.ElideLeft
                    horizontalAlignment: Text.AlignRight
                }
            }

            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                height: DS.Theme.borderSm
                color: DS.Theme.stroke
                opacity: 0.4
                visible: _root.trace.rowHeight > DS.Theme.labelHeightXs
            }
        }
    }
}
