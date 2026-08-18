import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import OVStyle as OV

Dialog {
    id: settingsDialog

    required property var configParams;

    x: Math.round((parent.width - width) / 2)
    y: Math.round(parent.height / 6)

    width: OV.Style.mediumDialogWidth;
    height: OV.Style.largeDialogHeight;

    modal: true
    focus: true
    title: "Settings"

    standardButtons: Dialog.Ok | Dialog.Cancel

    leftMargin: 0;
    rightMargin: 0;
    padding: 0
    spacing: 0;

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        contentHeight: mainColumn.childrenRect.height

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
    
        Column {
            id: mainColumn
            spacing: OV.Style.smallPadding
            anchors.fill: parent

            Row {
                width: childrenRect.width
                anchors.horizontalCenter: parent.horizontalCenter
                
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    color: OV.Style.colors.textColorBase
                    font: OV.Style.fonts.subHeader
                    text: "Variant : ";
                }

                SwitchDelegate {
                    anchors.verticalCenter: parent.verticalCenter

                    text: OV.Style.mode == OV.Style.Mode.Dark ? "Dark" : "Light";
                    palette.text: OV.Style.colors.textColorBase
                    onToggled: {
                        OV.Style.mode = checked ? OV.Style.Mode.Light :  OV.Style.Mode.Dark;
                    }
                    checked: OV.Style.mode == OV.Style.Mode.Light;
                }
            }

            Repeater {
                id: _config_repeater
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width;

                model: configParams

                delegate: Loader {
                    required property var param;
                    required property var component;
                    property var lparam: param

                    width: OV.Style.largePanelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    sourceComponent: component
                }
            }
        }
    }

    Settings {
        id: settings
        property int mode: OV.Style.Mode.Light
    }

    onAccepted: {
        settings.mode = OV.Style.mode
        settingsDialog.close()
    }

    onRejected: {
        OV.Style.mode = settings.mode
        settingsDialog.close()
    }
}