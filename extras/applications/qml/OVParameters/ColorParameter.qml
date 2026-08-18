import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qt.labs.platform  1.0 as P

import OVControls as OV
import DesignSystem as DS

Control {
  id: _control
  required property var param
  property bool displayName: true
  implicitHeight: Math.max(_label.implicitHeight , _value.implicitHeight) + 2 * DS.Theme.spacingSm
  width: 256

  DS.ToolTip {
    visible: _control.hovered
    text: _control.param ? _control.param.description : ""
  }

  RowLayout {
    anchors.fill: parent

    Label {
      id: _label
      Layout.alignment: Qt.AlignLeft
      text: _control.displayName ? (_control.param ? _control.param.name : "") : "value"
      font: DS.Theme.textSmallRegular
      color: DS.Theme.textPrimary
    }

    
    DS.TextField {
      id: _value
      Layout.alignment: Qt.AlignRight
      Layout.fillWidth: true
      horizontalAlignment: TextInput.AlignHCenter

      text: _control.param ? _control.param.value : ""
      color: DS.Theme.textPrimary
      font: DS.Theme.textSmallRegular
      validator: RegularExpressionValidator {
        regularExpression: /^(25[0-5]|2[0-4][0-9]|1?[0-9]?[0-9]),(25[0-5]|2[0-4][0-9]|1?[0-9]?[0-9]),(25[0-5]|2[0-4][0-9]|1?[0-9]?[0-9])$/
      }

      onTextEdited: {
        setRectColor(_value.text)
      }

      Binding {
        target: _control.param
        property: "value"
        value: _value.text
      }
    }

    Rectangle {
      id: _rect_color
      height: _control.height- 2 * DS.Theme.spacingSm
      width: height

      Binding {
        property: "color"
        value: setRectColor(_value.text)
      }

      MouseArea {
        anchors.fill: parent
        onClicked: {
          _chooseColor.color = _rect_color.color
          _chooseColor.open()
        }
      }
    }
  }

  P.ColorDialog {
    id: _chooseColor
    onAccepted: {
      //transform values of type #aa0000 to 170,0,0
      var hexaValues = _chooseColor.color.toString()
      var red = parseInt(hexaValues.substring(1,3), 16)
      var green = parseInt(hexaValues.substring(3,5), 16)
      var blue = parseInt(hexaValues.substring(5,7), 16)
      _value.text = red + "," + green + "," + blue
    }
  }

  function setRectColor(colorString)
  {
    var rgbValues = colorString.split(",").map(Number);
    // Extract red, green, and blue, and normalize them to the range [0,1]
    var red = Math.min(rgbValues[0] / 255, 1);
    var green = Math.min(rgbValues[1] / 255, 1);
    var blue = Math.min(rgbValues[2] / 255, 1);
    
    _rect_color.color = Qt.rgba(red, green, blue, 1)  
  }
}
