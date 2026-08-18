import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import OVASInterface as OV

import OVStyle as OV
import OVControls as OV

Dialog {
    id: channelRenameDialog

    required property var channelNames
    property int reEvaluateNames: 0
    
    x: Math.round((parent.width - width) / 2)
    y: Math.round(parent.height / 6)
    width: OV.Style.mediumDialogWidth
    height: OV.Style.largeDialogHeight

    leftMargin: 0
    rightMargin: 0
    padding: 0
    spacing: 0
    
    background: Rectangle{
        color: OV.Style.colors.gutterColor
       // border.color: OV.Style.colors.borderColor
    }

    title: "Channel Rename"

    modal: true

    ColumnLayout {
        id: _columnLayout
        anchors.fill: parent
        spacing: OV.Style.mediumRowSpacing

        ListView {
            id: _listView
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            model: reEvaluateNames, channelRenameDialog.channelNames
            delegate: RowLayout {
                required property int index
                required property string modelData

                width: _listView.width - OV.Style.mediumPadding
                spacing: OV.Style.smallRowSpacing

                Label{
                    Layout.preferredWidth: OV.Style.smallLabelWidth
                    verticalAlignment: Text.AlignBottom
                    text: (index + 1) + "."
                }
                OV.TextField {
                    Layout.fillWidth: true
                    text: modelData
                    onEditingFinished: {
                        channelNames[index] = displayText
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
            Layout.bottomMargin: OV.Style.smallPadding

            spacing: OV.Style.mediumRowSpacing

            OV.Button {
                id: _loadButton
                text: "Load"
                iconName: "folder-open"
                onClicked: _loadFileDialog.open()
            }

            OV.Button {
                id: _saveButton
                text: "Save"
                iconName: "content-save"
                onClicked: _saveFileDialog.open()
            }

            OV.Button {
                id: _renameButton
                text: "Apply"
                onClicked: channelRenameDialog.accept()
            }

            OV.Button {
                id: _cancelButton
                text: "Cancel"
                onClicked: channelRenameDialog.reject()
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
                // Copy entries up to the number of channel selected (i.e. channelNames.length)
                for (var i = 0; i < entries.length && i < channelRenameDialog.channelNames.length; i++) {
                    channelRenameDialog.channelNames[i] = entries[i]
                }
                channelRenameDialog.reEvaluateNames++
            }
        }
    }

    FileDialog {
        id: _saveFileDialog
        title: "Save Channel Names"
        nameFilters: ["Channel Names (*.txt)"]
        onAccepted: {
            if (_saveFileDialog.selectedFile) {
                if (OV.QmlHelper.write(_saveFileDialog.selectedFile, channelRenameDialog.channelNames.join("\n"))) {
                    console.log("Channel names saved to " + _saveFileDialog.selectedFile)
                } else {
                    console.log("Failed to save channel names to " + _saveFileDialog.selectedFile)
                }
            }
        }
    }
}