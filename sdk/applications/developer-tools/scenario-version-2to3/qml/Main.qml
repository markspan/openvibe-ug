import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: _app
    visible: true
    width: 400
    height: 300
    minimumWidth: 400
    minimumHeight: 300
    title: "Convert OpenViBE Scenario 2To3"

    property var selectedFiles: []

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: "This tool converts OpenViBE scenario files from version 2 to version 3."
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pointSize: 10
        }

        RowLayout {
            width: parent.width
            Layout.fillWidth: true

            Button {
                text: "Select files"
                onClicked: openDialog.open()
                Layout.alignment: Qt.AlignLeft
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: listView.contentHeight

            ListView {
                id: listView
                width: parent.width
                height: Math.min(150, contentHeight) // Limit height to either its content or a fixed max
                model: _app.selectedFiles
                clip: true

                delegate: Rectangle {
                    width: parent.width
                    height: 30
                    color: "#d3d3d3"
                    radius: 5
                    border.color: "#a9a9a9"
                    border.width: 1
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 10

                        Text {
                            Layout.fillWidth: true
                            verticalAlignment: Text.AlignVCenter
                            text: {
                                var url = modelData
                                var path = url.toString().substring(url.toString().lastIndexOf('/') + 1)
                                return path
                            }
                            font.pointSize: 10
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        Button {
                            padding: 0  // Remove padding
                            Layout.alignment: Qt.AlignRight
                            text: "×"
                            font.pointSize: 10
                            background: Item {} 
                            contentItem: Text {
                                text: parent.text
                                font.pointSize: 10
                                color: "black"  // Set the text color
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                _app.selectedFiles.splice(index, 1)
                                _app.selectedFilesChanged()
                            }
                        }
                    }
                }
            }
        }

        CheckBox {
            id: overwriteCheckBox
            Layout.alignment: Qt.AlignHCenter
            text: "Overwrite file(s)"
            checked: false

            ToolTip {
                visible: overwriteCheckBox.hovered
                text: "Enable to overwrite existing file(s). Otherwise, the file(s) will be suffixed with _v3"
            }
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: "Process"
            width: parent.width / 2
            onClicked: {
                // process selected files
                for (var i = 0; i < _app.selectedFiles.length; i++) {
                    var url = _app.selectedFiles[i]
                    updateHandler.processFile(url, overwriteCheckBox.checked);
                }
                _app.selectedFiles = [];
                _app.selectedFilesChanged();
            }
        }
    }

    FileDialog {
        id: openDialog
        title: "Select a File or Folder"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["All (*.*)", "Text files (*.txt)", "Images (*.png)"]
        onAccepted: {
            // add urls to selectedfiles
            for (var i = 0; i < openDialog.selectedFiles.length; i++) {
                _app.selectedFiles.push(openDialog.selectedFiles[i])
            }
            _app.selectedFilesChanged()
        }
    }
}