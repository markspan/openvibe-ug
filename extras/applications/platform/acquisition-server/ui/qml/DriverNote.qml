// UG fork: new file, not in upstream OpenViBE.
//
// What the driver has to say about its settings, e.g. what its montage did or why Play failed: a box with a
// coloured edge for its level, hidden when there is nothing to say.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS

import AcquisitionServer as OV


Rectangle {
    id: _note

    /// The text; the box is hidden while it is empty
    property string text: ""
    /// AcquisitionController.NoteLevel
    property int level: OV.AcquisitionController.NoteLevel.Info

    readonly property color _accent: level === OV.AcquisitionController.NoteLevel.Error ? DS.Theme.statusColors["error"]
                                   : level === OV.AcquisitionController.NoteLevel.Warning ? DS.Theme.statusColors["warning"]
                                   : DS.Theme.primary

    visible: text.length > 0
    implicitHeight: visible ? _label.implicitHeight + 2 * DS.Theme.spacingSm : 0

    radius: DS.Theme.radiusXs
    color: Qt.rgba(_accent.r, _accent.g, _accent.b, 0.10)
    border.color: Qt.rgba(_accent.r, _accent.g, _accent.b, 0.45)
    border.width: 1

    Accessible.role: Accessible.StaticText
    Accessible.name: text

    // The coloured edge
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 4
        radius: DS.Theme.radiusXs
        color: _note._accent
    }

    Text {
        id: _label
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: DS.Theme.spacingMd
        anchors.rightMargin: DS.Theme.spacingSm

        text: _note.text
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        font: DS.Theme.textSmallRegular
        color: DS.Theme.textPrimary
    }
}
