import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import OVControls as OV
import DesignSystem as DS


Control {
    id: _control

    required property var channelNames
    // Shown greyed out in an empty entry, e.g. which device channel it stands for
    property var channelHints: []

    property bool collapsed: true
    property var _editableNames: []
    property bool _suppressReset: false

    signal allNamesChanged(var channelNames)
    signal nameChanged(int index, string name)

    implicitHeight: _mainContainer.height

    onChannelNamesChanged: {
        if (!_suppressReset) {
            _editableNames = channelNames.slice()
            _listView.model = null
            _listView.model = _editableNames
        }
        _suppressReset = false
    }

    Rectangle {
        id: _mainContainer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        color: DS.Theme.white
        border.color: DS.Theme.stroke
        radius: DS.Theme.radiusSm

        height: (_control.collapsed ? _headerRow.implicitHeight : _mainLayout.implicitHeight) + 2 * DS.Theme.spacingMd


        ColumnLayout {
            id: _mainLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: DS.Theme.spacingMd
            anchors.rightMargin: DS.Theme.spacingMd
            anchors.topMargin: DS.Theme.spacingMd

            spacing: DS.Theme.spacingMd

            RowLayout {
                id: _headerRow
                Layout.fillWidth: true

                Text {
                    text: "Channel Names"
                    font: DS.Theme.subtitle3
                    color: DS.Theme.textPrimary
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                DS.IconButton {
                    id: expandCollapseButton
                    iconName: _control.collapsed ? "chevron-down" : "chevron-up"

                    onClicked: {
                        _control.collapsed = !_control.collapsed
                    }
                }
            }

            // Channel rename list
            ListView {
                id: _listView
                Layout.fillWidth: true
                Layout.preferredHeight: 200

                spacing: DS.Theme.spacingSm

                visible: !_control.collapsed
                clip: true
                model: _editableNames

                delegate: RowLayout {
                    required property int index
                    required property string modelData

                    width: _listView.width - DS.Theme.spacingSm
                    spacing: DS.Theme.spacingSm

                    Label{
                        Layout.preferredWidth: DS.Theme.labelWidthXs
                        verticalAlignment: Text.AlignBottom
                        text: (index + 1) + "."
                    }
                    DS.TextField {
                        Layout.fillWidth: true
                        text: modelData
                        placeholderText: index < _control.channelHints.length ? _control.channelHints[index] : ""
                        placeholderTextColor: DS.Theme.textTertiary
                        onEditingFinished: {
                            var copy = _control._editableNames.slice()
                            copy[index] = displayText
                            _control._editableNames = copy
                            _control._suppressReset = true
                            _control.nameChanged(index, displayText)
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    active: true
                }
            }


            RowLayout {
                id: _buttonRow
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter
                Layout.bottomMargin: DS.Theme.spacingSm

                visible: !_control.collapsed

                spacing: DS.Theme.spacingSm

                DS.Button {
                    id: _loadButton
                    name: "Load"
                    onClicked: _loadFileDialog.open()
                }

                DS.Button {
                    id: _saveButton
                    name: "Save"
                    onClicked: _saveFileDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: _loadFileDialog
        title: "Load Channel Names"
        nameFilters: ["Channel Names (*.txt)"]
        onAccepted: {
            if (_loadFileDialog.selectedFile) {
                const fileContent = OV.QmlHelper.read(_loadFileDialog.selectedFile)
                var entries = fileContent.split("\n")
                var newNames = _control._editableNames.slice()
                for (var i = 0; i < entries.length && i < newNames.length; i++) {
                    newNames[i] = entries[i]
                }
                _control._editableNames = newNames
                _listView.model = null
                _listView.model = _control._editableNames
                _control._suppressReset = true
                _control.allNamesChanged(_control._editableNames)
            }
        }
    }

    FileDialog {
        id: _saveFileDialog
        title: "Save Channel Names"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Channel Names (*.txt)"]
        onAccepted: {
            if (_saveFileDialog.selectedFile) {
                if (OV.QmlHelper.write(_saveFileDialog.selectedFile, _control._editableNames.join("\n"))) {
                    console.log("Channel names saved to " + _saveFileDialog.selectedFile)
                } else {
                    console.log("Failed to save channel names to " + _saveFileDialog.selectedFile)
                }
            }
        }
    }
}
