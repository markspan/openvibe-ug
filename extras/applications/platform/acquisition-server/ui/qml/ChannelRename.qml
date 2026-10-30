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

    signal allNamesChanged(var channelNames)
    signal nameChanged(int index, string name)

    implicitHeight: _mainContainer.height

    onChannelNamesChanged: _syncNames(channelNames)
    Component.onCompleted: _syncNames(channelNames)

    // The rows are updated in place, so editing a name keeps the list where it is and the
    // focus in it; the list is only rebuilt when the number of channels changes.
    ListModel { id: _namesModel }

    function _syncNames(names) {
        if (_namesModel.count !== names.length) {
            _namesModel.clear()
            for (var i = 0; i < names.length; i++) { _namesModel.append({ channelName: names[i] }) }
            return
        }
        for (var j = 0; j < names.length; j++) {
            if (_namesModel.get(j).channelName !== names[j]) { _namesModel.setProperty(j, "channelName", names[j]) }
        }
    }

    function _names() {
        var names = []
        for (var i = 0; i < _namesModel.count; i++) { names.push(_namesModel.get(i).channelName) }
        return names
    }

    // Scrolls the row into view and edits it; false when there is no such row
    function _focusRow(row) {
        if (row < 0 || row >= _namesModel.count) { return false }
        _listView.positionViewAtIndex(row, ListView.Contain)
        var item = _listView.itemAtIndex(row)
        if (item) { item.edit() }
        return true
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
                model: _namesModel

                delegate: RowLayout {
                    id: _row
                    required property int index
                    required property string channelName

                    function edit() {
                        _field.forceActiveFocus()
                        _field.selectAll()
                    }

                    width: _listView.width - DS.Theme.spacingSm
                    spacing: DS.Theme.spacingSm

                    Label{
                        Layout.preferredWidth: DS.Theme.labelWidthXs
                        verticalAlignment: Text.AlignBottom
                        text: (index + 1) + "."
                    }
                    DS.TextField {
                        id: _field
                        Layout.fillWidth: true
                        text: _row.channelName
                        placeholderText: _row.index < _control.channelHints.length ? _control.channelHints[_row.index] : ""
                        placeholderTextColor: DS.Theme.textTertiary
                        onEditingFinished: {
                            if (displayText === _row.channelName) { return }
                            _namesModel.setProperty(_row.index, "channelName", displayText)
                            _control.nameChanged(_row.index, displayText)
                        }
                        // Enter and Tab go on to the next name, Shift+Tab back to the previous one
                        Keys.onPressed: (event) => {
                            var step = 0
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Tab) { step = 1 }
                            else if (event.key === Qt.Key_Backtab) { step = -1 }
                            if (step !== 0 && _control._focusRow(_row.index + step)) { event.accepted = true }
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
                var newNames = _control._names()
                for (var i = 0; i < entries.length && i < newNames.length; i++) {
                    newNames[i] = entries[i]
                }
                _control._syncNames(newNames)
                _control.allNamesChanged(newNames)
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
                if (OV.QmlHelper.write(_saveFileDialog.selectedFile, _control._names().join("\n"))) {
                    console.log("Channel names saved to " + _saveFileDialog.selectedFile)
                } else {
                    console.log("Failed to save channel names to " + _saveFileDialog.selectedFile)
                }
            }
        }
    }
}
