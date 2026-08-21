// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only


import QtQuick
import QtQuick.Layouts

import DesignSystem as DS
import '.'

Rectangle {

    // Drives the height of the ApplicationWindow header; width comes from Layout.fillWidth.
    implicitHeight: DS.Theme.navBarHeight

    color: DS.Theme.navBarBackground

    // Bring the current scenario's tab fully into view. Deferred, because selecting a tab changes
    // its width -- it drops the elide cap to show its whole title -- so the row has to lay out
    // again before these positions mean anything.
    function ensureTabVisible() {
        Qt.callLater(() => {
            const tab = _tabRepeater.itemAt(OVDesigner.currentScenarioIndex)
            if (!tab) {
                return
            }
            if (tab.x < _tabStrip.contentX) {
                _tabStrip.contentX = Math.max(0, tab.x)
            } else if (tab.x + tab.width > _tabStrip.contentX + _tabStrip.width) {
                _tabStrip.contentX = Math.min(Math.max(0, _tabStrip.contentWidth - _tabStrip.width),
                                              tab.x + tab.width - _tabStrip.width)
            }
        })
    }

    Connections {
        target: OVDesigner
        // scenariosChanged covers a freshly created scenario, including the Ctrl+N path
        function onCurrentScenarioIndexChanged() { ensureTabVisible() }
        function onScenariosChanged() { ensureTabVisible() }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        DS.IconButton {
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
            iconName: "home"
            iconSize: 16
            iconPressedSize: 12

            onClicked: {
                console.log("Home clicked!")
            }
        }

        DS.VerticalSpacer {}
        
        // Scenario "TabBar" section.
        // Clipped so the tabs are cut at the separator instead of pushing the buttons on the right
        // off the window, and scrollable with the wheel once they overflow.
        Flickable {
            id: _tabStrip
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            contentWidth: _tabRow.width
            contentHeight: height // equal to the height disables vertical flicking
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick
            interactive: contentWidth > width // don't swallow drags when everything fits

            // Closing tabs or widening the window can leave contentX past the end of the content,
            // which would show a blank strip until the next interaction.
            onContentWidthChanged: Qt.callLater(returnToBounds)
            onWidthChanged: Qt.callLater(returnToBounds)

            Row {
                id: _tabRow
                height: _tabStrip.height

                Repeater {
                    id: _tabRepeater
                    model: OVDesigner.scenarios
                    delegate: ScenarioTab {}
                }
            }

            WheelHandler {
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad

                onWheel: event => {
                    // Qt swaps the axes for Shift+wheel, so preferring x also covers Shift and the
                    // trackpad's horizontal swipe.
                    const delta = event.angleDelta.x !== 0 ? event.angleDelta.x : event.angleDelta.y
                    if (delta === 0) {
                        return
                    }
                    const max = Math.max(0, _tabStrip.contentWidth - _tabStrip.width)
                    _tabStrip.contentX = Math.max(0, Math.min(max, _tabStrip.contentX - delta))
                    event.accepted = true
                }
            }
        }

        // Add Scenario button
        DS.IconButton {
            Layout.fillHeight: true
            Layout.preferredWidth: height
            
            iconName: "add"
            iconSize: DS.Theme.iconMd
            iconColor: DS.Theme.iconPrimary
            iconPressedSize: DS.Theme.iconSm

            onClicked: {
                OVDesigner.createScenario()
            }
        }

        DS.VerticalSpacer {}

        SettingsMenu {
            Layout.preferredWidth: DS.Theme.navBarHeight
            Layout.preferredHeight: DS.Theme.navBarHeight
        }
    }
}

