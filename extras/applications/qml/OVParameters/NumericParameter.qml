import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVControls as OV
import DesignSystem as DS

Control {
    id: _control

    required property var param
    property bool displayName: true
    readonly property var paramType: _control.param ? _control.param.type : undefined

    property var min: _control.param ? _control.param.min : 0
    property var max: _control.param ? _control.param.max : 0
    property var name: _control.displayName ? (_control.param ? _control.param.name : "") : "value"
    property var doc: _control.param ? _control.param.description : ""
    property var decimals: (_control.param && _control.param.decimals !== undefined) ? _control.param.decimals : 2

    implicitWidth: DS.Theme.labelWidthMd
    implicitHeight: DS.Theme.labelHeight2xl

    background: Rectangle{
        color: DS.Theme.white
    }

    RowLayout {
        anchors.fill: _control
        spacing: 0

        Label {
            id: _label

            Layout.fillWidth: true
            text: _control.name
            font: DS.Theme.textSmallLight
            color: DS.Theme.textPrimary
            elide: Text.ElideRight

            MouseArea {
                id: _labelMouseArea
                anchors.fill: parent
                hoverEnabled: true
            }

            DS.ToolTip {
                visible: _labelMouseArea.containsMouse
                text: _control.doc + " [" + _control.min + ", " + _control.max + "] "
            }
        }

        DS.SpinButton {
            id: _spinButton

            Layout.alignment: Qt.AlignRight

            type: _control.paramType === "double" ? DS.SpinButton.Type.Double : DS.SpinButton.Type.Integer
            decimals: _control.decimals
            from : _control.min
            to: _control.max
            value: _control.param ? _control.param.value : 0

            onValueChanged: {
                if(_control.param) {
                    _control.param.value = _spinButton.value
                }
            }
        }
    }
}
