import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVStyle as OV
import OVControls as OV


Control {
    id: _control

    required property var value
    property var min: 0
    property var max: 100
    property bool editable: true
    property bool isDouble: false

    property var decimals: 2
    property var delta: isDouble ? Math.pow(10, -decimals) : 1

    implicitWidth: OV.Style.smallPanelWidth
    implicitHeight: OV.Style.spinBoxHeight
    padding: OV.Style.largePadding

    IntValidator {
        id: _intValidator
        bottom: _control.min
        top: _control.max
    }

    DoubleValidator {
        id: _doubleValidator
        bottom: _control.min
        top: _control.max
        decimals: _control.decimals
        locale: "C"
        notation: DoubleValidator.ScientificNotation
    }

    background: Rectangle {
        anchors.fill: parent
        radius: OV.Style.buttonRadius
        color: OV.Style.colors.bgColor
    }

    RowLayout {
        anchors.fill: parent

        OV.Icon {
            id: _decrement
            Layout.fillHeight: true

            size: OV.Style.iconLarge
            icon: "minus"
            color: OV.Style.colors.textColorLightBase
            background: Rectangle {
                anchors.fill: parent
                radius: OV.Style.buttonRadius
                color: _decrement_ma.containsMouse ? OV.Style.colors.hoveredBaseColor : OV.Style.colors.baseColor
            }
            
            MouseArea {
                id: _decrement_ma
                anchors.fill: parent
                enabled: Number(_control.value) > _control.min
                hoverEnabled: true
                onClicked: {
                    OV.QmlHelper.setValue(_control, "value", updateValue(_control.value, - _control.delta))
                }
            }
        }

        OV.TextField {
            id: _input
            Layout.fillWidth: true
            Layout.fillHeight: true

            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            font: OV.Style.fonts.value
            padding: 0

            property int reEvaluateValue: 0
            readOnly: !_control.editable
            validator: getValidator()

            //here we use javascript commaOperator (evaluates all operands, and returns the value of the last one)
            // to force reevaluation of _control.value without breaking the binding
            text: reEvaluateValue, _control.value

            onTextEdited: {
                if(acceptableInput) {
                    OV.QmlHelper.setValue(_control, "value", Number(text))
                }
            }
            onActiveFocusChanged: {
                if (!activeFocus && !acceptableInput) {
                    //put back last value if bad edit
                    reEvaluateValue++
                }
            }
        }

        OV.Icon {
            id: _increment
            Layout.fillHeight: true
            size: OV.Style.iconLarge
            icon: "plus"
            color: OV.Style.colors.textColorLightBase
            background: Rectangle {
                anchors.fill: parent
                radius: OV.Style.buttonRadius
                color: _increment_ma.containsMouse ? OV.Style.colors.hoveredBaseColor : OV.Style.colors.baseColor
            }
            
            MouseArea {
                id: _increment_ma
                anchors.fill: parent
                enabled: Number(_control.value) < _control.max
                hoverEnabled: true

                onClicked: {
                    OV.QmlHelper.setValue(_control, "value", updateValue(_control.value, _control.delta))
                }
            }
        }
    }

    function getValidator() {
        if(_control.isDouble) {
            return _doubleValidator
        } else {
            return _intValidator
        }
    }

    function updateValue(val, delta) {
        var newVal
        if(_control.isDouble) {
            newVal = (parseFloat(val) + parseFloat(delta)).toFixed(_control.decimals)
        } else {
            newVal = parseInt(parseInt(val) + parseInt(delta), 10)
        }

        if(newVal < Number(_control.min)) {
            newVal = _control.min
        } else if(newVal > Number(_control.max)) {
            newVal = _control.max
        }

        return newVal
    }
}
