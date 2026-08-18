// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVParameters as OV
import DesignSystem as DS

import "components"

Item {
    id: root

    // properties
    required property var scenario

    Layout.fillWidth: true
    Layout.fillHeight: true

    OV.Parameters {
        id: _scenarioParams
        parameters: scenario?.parameters ?? []
        displayNames: false
    }

    ScrollView {
        id: _mainScrollView
        anchors.fill: parent
        
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        function scrollToItem(item) {
            // Wait for the layout to update
            Qt.callLater(() => {
                // Get the item's position relative to the ScrollView's content
                var itemPos = item.mapToItem(contentItem, 0, 0)
                var itemBottom = itemPos.y + item.height
                var viewportTop = contentItem.y
                var viewportBottom = viewportTop + height
                
                if (itemBottom > viewportBottom) {
                    // Item is below viewport - scroll down
                    contentItem.contentY += itemBottom -viewportBottom + DS.Theme.spacingMd
                }
            })
        }

        ColumnLayout {
            id: _mainLayout
            width: _mainScrollView.availableWidth

            // Settings Section
            Rectangle {
                id: _settingsSection
                Layout.fillWidth: true
                Layout.preferredHeight: _settingsLayout.implicitHeight

                color: "transparent"

                ColumnLayout {
                    id: _settingsLayout
                    anchors.fill: parent
                    anchors.leftMargin: DS.Theme.spacingLg
                    anchors.rightMargin: DS.Theme.spacingLg
                    anchors.bottomMargin: DS.Theme.spacingLg

                    spacing: DS.Theme.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumHeight: DS.Theme.spacing5xl

                        Text {
                            text: "Scenario Settings"
                            font: DS.Theme.subtitle3
                            color: DS.Theme.textPrimary
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        DS.IconButton {
                            id: _addParameterButton
                            iconName: "add"
                            iconSize: DS.Theme.iconLg

                            onClicked: {
                                _newParamPopup.open()
                            }
                        }
                    }

                    Repeater {
                        model: _scenarioParams.params_model
                        delegate: Rectangle {
                            required property int index
                            required property var modelData

                            Layout.fillWidth: true
                            height: _paramContent.implicitHeight  + 2 * DS.Theme.spacingSm

                            
                            radius: DS.Theme.radiusSm
                            border.width: DS.Theme.borderSm
                            border.color: DS.Theme.stroke

                            ColumnLayout {
                                id: _paramContent
                                anchors.fill: parent
                                anchors.margins: DS.Theme.spacingSm

                                spacing: DS.Theme.spacingMd

                                ColumnLayout {
                                    id: _paramName
                                    Layout.fillWidth: true
                                    spacing: DS.Theme.spacingXs

                                    Label {
                                        text: "Name"
                                        font: DS.Theme.textSmallLight
                                        color: DS.Theme.textSecondary
                                        Layout.alignment: Qt.AlignLeft
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: DS.Theme.spacing2xl

                                        DS.TextField {
                                            id: _nameTextInput
                                            Layout.fillWidth: true

                                            text: _scenarioParams.parameters[index]?.name ?? ""

                                            onEditingFinished: {
                                                scenario.updateParameterName(_scenarioParams.parameters[index].name, text)
                                            }
                                        }

                                        DS.IconButton {
                                            iconName: "trash"
                                            iconSize: DS.Theme.spacingLg
                                            iconHoverColor: "red"

                                            onClicked: {
                                                scenario.removeParameter(_scenarioParams.parameters[index].name)
                                            }
                                        }

                                        DS.IconButton {
                                            id: _linkParamButton
                                            iconName: "link"
                                            iconSize: DS.Theme.spacingLg
                                            onClicked: {
                                                // populate popup
                                                _linkParamPopup.selectedParam = _scenarioParams.parameters[index]

                                                // open popup
                                                _linkParamPopup.open()

                                                Qt.callLater(() => {
                                                    // Position popup later as it needs popup to be rendered first
                                                    // Not perfect as it creates potential jumps when the popup is opened for different param type
                                                    _linkParamPopup.x = mapToItem(root, 0, 0).x - _linkParamPopup.width + _linkParamButton.width + DS.Theme.spacingMd

                                                    var globalPos = mapToItem(null, 0, height)
                                                    var heightBelow = Window.height - globalPos.y
                                                    var extraHeight = 0
                                                    if (heightBelow < _linkParamPopup.minHeight + (_linkParamPopup.contentHeight * 0.3)) {
                                                        extraHeight =  _linkParamPopup.minHeight + (_linkParamPopup.contentHeight * 0.3) - heightBelow
                                                        console.log("extra height: ", extraHeight)
                                                    }
                                                    _linkParamPopup.y = mapToItem(root, 0, 0).y - DS.Theme.spacingLg - extraHeight
                                                    _linkParamPopup.maxHeight = heightBelow + DS.Theme.spacingLg + extraHeight
                                                })
                                            }
                                        }
                                    }
                                }

                                Loader {
                                    property var lparam: modelData.param

                                    Layout.fillWidth: true
                                    sourceComponent: modelData.component
                                }
                            }
                        }
                        
                        onItemAdded: (newIndex, newCard) => {
                            _mainScrollView.scrollToItem(newCard)
                        }
                    }
                }
            }
            
            DS.HorizontalSpacer { }
            
            // Input Section
            Rectangle {
                id: _inputSection
                Layout.fillWidth: true
                Layout.preferredHeight: _inputLayout.implicitHeight

                color: "transparent"

                ColumnLayout {
                    id: _inputLayout
                    anchors.fill: parent
                    anchors.leftMargin: DS.Theme.spacingLg
                    anchors.rightMargin: DS.Theme.spacingLg
                    anchors.bottomMargin: DS.Theme.spacingLg

                    spacing: DS.Theme.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumHeight: DS.Theme.spacing5xl

                        Text {
                            text: "Input"
                            font: DS.Theme.subtitle3
                            color: DS.Theme.textPrimary
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        DS.IconButton {
                            iconName: "add"
                            iconSize: DS.Theme.iconLg

                            onClicked: {
                                root.scenario.addInput("New Input", "Signal")
                            }
                        }
                    }

                    Repeater {
                        model: root.scenario?.inputPorts
                        delegate: IOCard {
                            required property int index
                            required property var modelData
                            ioObject: modelData
                            ioType: "Input"
                            selectionModel: root.scenario.getSupportedInputTypes()
                            onRemove: {
                                root.scenario.removeInput(index)
                            }
                        }
                        onItemAdded: (newIndex, newCard) => {
                            _mainScrollView.scrollToItem(newCard)
                        }
                    }
                }
            }

            DS.HorizontalSpacer { }

            // Output Section
            Rectangle {
                id: _outputSection
                Layout.fillWidth: true
                Layout.preferredHeight: _outputLayout.implicitHeight
                
                color: "transparent"

                ColumnLayout {
                    id: _outputLayout
                    anchors.fill: parent
                    anchors.leftMargin: DS.Theme.spacingLg
                    anchors.rightMargin: DS.Theme.spacingLg
                    anchors.bottomMargin: DS.Theme.spacingLg

                    spacing: DS.Theme.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumHeight: DS.Theme.spacing5xl

                        Text {
                            text: "Output"
                            font: DS.Theme.subtitle3
                            color: DS.Theme.textPrimary
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        DS.IconButton {
                            iconName: "add"
                            iconSize: DS.Theme.iconLg

                            onClicked: {
                                root.scenario.addOutput("New Output", "Signal")
                            }
                        }
                    }

                    Repeater {
                        model: root.scenario?.outputPorts
                        delegate: IOCard {
                            required property int index
                            required property var modelData
                            ioObject: modelData
                            ioType: "Output"
                            selectionModel: root.scenario.getSupportedOutputTypes()
                            onRemove: {
                                root.scenario.removeOutput(index)
                            }
                        }

                        onItemAdded: (newIndex, newCard) => {
                            _mainScrollView.scrollToItem(newCard)
                        }
                    }
                }
            }

            DS.HorizontalSpacer { }

            // Window Manager section
            WindowManager { 
                id: _windowManager
                Layout.fillWidth: true
                
                windowManager: root.scenario.windowManager
            }

            Item {
                Layout.preferredHeight: DS.Theme.spacingMd
            }
        }

        // /////////////////////////////////////////////////////////////////////////////////////
        // Connections

        ListModel {
            id: _parameterConnections
        }

        // Parameter connections management popup
        // Dialog opened for a specific parameter index showing all boxes that contain
        // at least a parameter of the same type in order to select one or more.
        DS.Popup {
            id: _linkParamPopup

            property var selectedParam: null
            
            title: "Link parameter to box"

            //maxHeight: 150

            function updateConnections() {
                if (!scenario || !selectedParam) {
                    return;
                }
                const connections = scenario.getParameterConnectionsForParam(selectedParam.name) // Get the underlying list from QListProperty

                const groupedParameterConnections = {}
                connections.forEach( (connection) => {
                    const boxName = scenario?.boxes.find((box) => box.identifier === connection.boxID).name;
                    groupedParameterConnections[connection.boxID] = groupedParameterConnections[connection.boxID] ?? {"boxName": boxName, "connections": []};
                    groupedParameterConnections[connection.boxID].connections.push({"boxID": connection.boxID, "paramName": connection.boxParamName, "connected": connection.connected})
                });

                // Clear the model and append new data
                _parameterConnections.clear();
                Object.values(groupedParameterConnections).forEach((item) => {
                    _parameterConnections.append(item);
                });
            }

            onSelectedParamChanged: {
                updateConnections()            
            }

            ColumnLayout {
                id: _linkParamColumnLayout
                Layout.fillWidth: true

                spacing: DS.Theme.spacingMd


                ScrollView {
                    id: _scrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: _boxes.implicitHeight

                    clip: true

                    // Enable scrollbars when needed
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        id: _scrollColumnLayout
                        width: parent.availableWidth
                        spacing: DS.Theme.spacingMd

                        Repeater {
                            id: _boxes

                            model: _parameterConnections

                            delegate: ColumnLayout {
                                id: _linkBoxDelegate

                                required property var modelData
                                required property int index

                                spacing: DS.Theme.spacingMd

                                Label {
                                    text: modelData.boxName
                                    font: DS.Theme.textMediumBold
                                }

                                Repeater {
                                    model: modelData.connections

                                    delegate: Rectangle {
                                        id: _linkParamDelegate

                                        required property var modelData
                                        required property int index

                                        implicitWidth: _rowLayout.implicitWidth + DS.Theme.spacingMd
                                        height: 30
                                        color: DS.Theme.white
                                        opacity: 0.8

                                        RowLayout {
                                            id: _rowLayout
                                            anchors.fill: parent
                                            anchors.leftMargin: DS.Theme.spacingMd
                                            spacing: DS.Theme.spacingMd

                                            DS.CheckBox {
                                                id: _checkBox
                                                checked: modelData.connected
                                                onClicked: {
                                                    const box = _parameterConnections.get(_linkBoxDelegate.index)
                                                    if (box) {
                                                        const connection = box.connections.get(index);
                                                        if (connection) {
                                                            connection.connected = _checkBox.checked
                                                        } else {
                                                            console.log("Error retrieving connection for box ", box.boxName)
                                                        }
                                                    } else {
                                                        console.log("Error retrieving parameter connections for box!")
                                                    }
                                                }
                                            }
                                            Label {
                                                Layout.fillWidth: true
                                                
                                                text: modelData.paramName
                                                font: DS.Theme.textMediumRegular
                                                
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    DS.Button {
                        name: "Link"
                        onClicked: {
                            for (let i = 0; i < _parameterConnections.count; i++) {
                                const info = _parameterConnections.get(i)
                                for (let j = 0; j < info.connections.count; j++) {
                                    const connection = info.connections.get(j)
                                    scenario.updateParameterConnection(_linkParamPopup.selectedParam.name, connection.boxID, connection.paramName, connection.connected);
                                }
                            }
                            _linkParamPopup.updateConnections()
                            _linkParamPopup.close()
                        }
                    }
                }
            }
        }

        // /////////////////////////////////////////////////////////////////////////////////////
        // New Parameter Popup
        DS.Popup {
            id: _newParamPopup

            title: "Setting Type"

            x: parent.width - width - DS.Theme.spacingXs
            y: -2 * DS.Theme.spacingXs

            ColumnLayout {
                Layout.fillWidth: true
                spacing: DS.Theme.spacingMd
                
                Repeater {
                    model: _control.scenario?.availableParameterTypes
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.minimumHeight: DS.Theme.labelHeightXl
                        Layout.minimumWidth: label.implicitWidth + 2 * DS.Theme.spacingSm

                        required property var modelData
                        property bool isHovered: false

                        
                        radius: DS.Theme.radiusXs
                        color: isHovered ? DS.Theme.iconHoverSecondary : "transparent"
                        Label {
                            id: label
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: DS.Theme.spacingSm
                            anchors.rightMargin: DS.Theme.spacingSm
                            
                            text: modelData
                            font: DS.Theme.textMediumRegular
                            lineHeight: DS.Theme.textMdLineHeight
                            lineHeightMode: Text.FixedHeight
                            color: DS.Theme.textPrimary
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                scenario.addParameter("Setting", modelData)
                                _newParamPopup.close()
                            }
                            hoverEnabled: true
                            onEntered: isHovered = true
                            onExited: isHovered = false
                        }
                    }
                }
            }
        }
    }
}


