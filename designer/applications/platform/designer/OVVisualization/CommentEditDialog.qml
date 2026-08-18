// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import OVControls as OV
import OVStyle as OV
import "." as OV

Window {
    id: _dialog
    property alias text: _textArea.text
    signal accepted()

    title: "Comment Editing"
    flags: Qt.Dialog
    width: 400
    height: 400
    color: OV.Style.colors.bgColor

    TextArea {
        id: _textArea
        wrapMode: TextEdit.Wrap
        //textFormat: TextEdit.RichText
        focus: true
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: _okButton.top
        anchors.bottomMargin: OV.Style.smallPadding
        background: Rectangle {
            color: OV.Style.colors.noteColor
        }
    }

    OV.Button {
        id: _okButton
        anchors.bottom: parent.bottom
        anchors.bottomMargin: OV.Style.smallPadding
        anchors.left: parent.left
        anchors.leftMargin: OV.Style.largePadding

        text: "OK"
        onClicked: {
            _dialog.accepted()
            _dialog.visible = false
        }
    }

    OV.Button {
        id: _cancelButton
        anchors.bottom: parent.bottom
        anchors.bottomMargin: OV.Style.smallPadding
        anchors.right: parent.right
        anchors.rightMargin: OV.Style.largePadding
        
        text: "Cancel"
        onClicked: {
            _dialog.visible = false
        }
    }
}
 