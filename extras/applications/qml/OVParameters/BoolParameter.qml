import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS

Control {
    id: _control

    required property var param
    property bool displayName: true
    
    implicitWidth: DS.Theme.labelWidthMd
    implicitHeight: DS.Theme.labelHeight2xl


    RowLayout {
        anchors.fill: _control
        spacing: 0

        Label {
            Layout.maximumWidth: _control.width - _switch.width - DS.Theme.spacingXs
            Layout.alignment: Qt.AlignVCenter

            text: _control.displayName ? (_control.param ? _control.param.name : "") : "value"
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
                text: _control.param?.description ?? ""
            }
        }

        Item {
            Layout.fillWidth: true
        }

        DS.Switch {
            id: _switch
            Layout.alignment: Qt.AlignVCenter

            checked: _control.param ? _control.param.value : false
            onToggled: {
                if (_control.param)
                    _control.param.value = _switch.checked ? 1 : 0
            }
        }
    }
}
