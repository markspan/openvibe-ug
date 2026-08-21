// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only


import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import DesignSystem as DS

// One scenario tab in the TopBar strip. Sizes itself to its own name rather than to a fixed width,
// so the strip stays compact; the name elides before it can reach the close cross.
Rectangle {
    id: _scenarioTab

    required property int index
    required property var modelData

    readonly property bool selected: index === OVDesigner.currentScenarioIndex

    // The layout already knows its natural width: the children's implicit widths plus the spacings
    // between them plus their margins. Feeding it back into our own width is not a binding loop --
    // Label.implicitWidth reports the untruncated text width even with elide set, and does not
    // depend on the width the label is given, as long as wrapMode stays unset on it.
    readonly property real naturalWidth: _row.implicitWidth

    // The selected tab ignores the cap so its whole title reads, however long that makes it.
    width: selected ? Math.max(DS.Theme.tabMinWidth, naturalWidth)
                    : Math.max(DS.Theme.tabMinWidth, Math.min(naturalWidth, DS.Theme.tabMaxWidth))
    height: parent.height

    color: selected ? DS.Theme.quaternary
                    : (_hover.hovered ? DS.Theme.tabHoverBackground : DS.Theme.navBarBackground)

    MouseArea {
        anchors.fill: parent
        onClicked: {
            OVDesigner.currentScenarioIndex = _scenarioTab.index
        }
    }

    HoverHandler {
        id: _hover
    }

    // Only worth showing when the name is actually cut off
    DS.ToolTip {
        text: _scenarioTab.modelData.name
        target: _scenarioTab
        visible: _hover.hovered && _label.truncated
    }

    RowLayout {
        id: _row
        anchors.fill: parent
        spacing: DS.Theme.spacingSm

        Label {
            id: _label
            Layout.fillWidth: true
            Layout.leftMargin: DS.Theme.spacingMd

            text: _scenarioTab.modelData.name
            font: DS.Theme.textSmallRegular
            // Weight stays fixed on purpose: bolding the selected tab would change implicitWidth,
            // and so the tab's own width, making the whole strip reflow on every click.
            color: _scenarioTab.selected ? DS.Theme.textPrimary : DS.Theme.textTertiary
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }

        DS.IconButton {
            id: _close
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: DS.Theme.spacingMd

            iconName: "close"
            iconSize: DS.Theme.iconXs
            iconColor: DS.Theme.iconSecondary
            iconHoverColor: DS.Theme.iconHoverClose

            onClicked: {
                OVDesigner.removeScenario(_scenarioTab.index)
            }
        }

        DS.VerticalSpacer {
            id: _separator
        }
    }

    // Selection accent, declared last so it paints over the row. Same animated sweep as the
    // Toolbox tab bar. The right margin keeps it clear of the inter-tab separator; anchoring to
    // _separator itself is not possible, as it is neither our parent nor our sibling.
    Rectangle {
        id: _underline
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: _separator.width
        anchors.bottom: parent.bottom

        height: DS.Theme.borderMd
        color: DS.Theme.primary

        transform: Scale {
            xScale: _scenarioTab.selected ? 1 : 0
            origin.x: _underline.width / 2

            Behavior on xScale {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutQuad
                }
            }
        }
    }
}
