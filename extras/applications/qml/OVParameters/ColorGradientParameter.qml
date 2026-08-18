import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import OVControls as OV
import DesignSystem as DS

Control {
  id: _control
  required property var param
  property bool displayName: true
  property string errorText: ""

  implicitHeight: _gradient.height + DS.Theme.spacingSm
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

/*      OV.TextField {
        id: _value
        Layout.fillWidth: true
        horizontalAlignment: TextInput.AlignLeft
        font: DS.Theme.textSmallRegular
        text: _control.param ? _control.param.value : ""
        onTextEdited: updateGradient()
        selectByMouse: true

        Binding {
          target: _control.param
          property: "value"
         value: _value.text
       }
      }*/

    Rectangle {
      id: _gradient
      Layout.fillWidth: true
      Layout.alignment: Qt.AlignVCenter
      height: 20
    }
  }

  MouseArea {
    anchors.fill: parent
    cursorShape: Qt.PointingHandCursor
    onClicked: {
      _editDialog.gradientString = _control.param.value.trim()
      _editDialog.open()
    }
  }

  Dialog {
    id: _editDialog
    title: "Edit Gradient"
    width: 400
    height: 400
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    property alias gradientString: textArea.text

    ColumnLayout {
      anchors.fill: parent
      spacing: DS.Theme.spacingSm

      Label {
        width: parent.width
        text: "Enter gradient stops (e.g. 0: 10,10,10; 100:200,200,10):"
        wrapMode: Text.Wrap
      }

      DS.TextField {
        id: textArea
        Layout.fillWidth: true
        wrapMode: TextArea.WrapAnywhere
        color: errorLabel.text === "" ? DS.Theme.textPrimary : "red"
        onTextChanged: {
          errorLabel.text = updateGradient(text, preview);
        }
      }

      Rectangle {
        id: preview
        Layout.fillWidth: true
        Layout.fillHeight: true
        radius: 6
      }

      Label {
        id: errorLabel
        width: parent.width
        text: ""
        color: "red"
        visible: text !== ""
        wrapMode: Text.Wrap
      }
    }

    onAccepted: {
      _control.param.value = _editDialog.gradientString.trim()
      errorText = updateGradient(_editDialog.gradientString, _gradient)
    }
  }

  function isValidNumericString(str) {
    return /^[0-9:,]+$/.test(str)
  }

  function updateGradient(text, rectangle) {
    try {
      if (!text)
        return "No gradient text provided";

      var stops = []
      var segments = text.split(";")
      for(var segment of segments) {
        if( !isValidNumericString(segment.trim()) ) {
          return "Invalid segment: " + segment
        }

        var parts = segment.trim().split(":")
        if (parts.length !== 2) {
          return " parts.length !== 2 in segment: " + segment
        }

        var pos = parseFloat(parts[0].trim())
        var rgb = parts[1].trim().split(",").map(Number)
        if (rgb.length !== 3) {
          return " rgb.length !== 3 in segment: " + segment
        }
        if( pos < 0 || pos > 100) {
          return "Position out of range (0-100): " + pos + " in segment: " + segment
        }
        if( rgb.some(value => isNaN(value) || value < 0 || value > 255) ) {
          return "RGB values must be between 0 and 255: " + rgb + " in segment: " + segment
        }

        var col = Qt.rgba(rgb[0]/255., rgb[1]/255., rgb[2]/255., 1)
        stops.push({position: pos, color: col} )
      }

      // Sort by position
      stops.sort((a, b) => a.position - b.position)

      // Remove all existing stops
      let gradient = Qt.createQmlObject('import QtQuick ; Gradient {orientation: Gradient.Horizontal   }', rectangle)
  
      // Add new stops
      for (var j = 0; j < stops.length; ++j) {
          gradient.stops.push(Qt.createQmlObject(
              'import QtQuick ; GradientStop { position: ' + stops[j].position/100. +
              '; color: "' + stops[j].color + '" }',
              gradient
          ))
      }
      rectangle.gradient = gradient
    } catch (e) {
      return "Error parsing gradient: " + e
    }
    return "";
  }

  Component.onCompleted: {
    errorText = updateGradient(_control.param.value.trim(), _gradient)
    if(errorText !== "") {
      console.error("Error in gradient:", errorText)
    }
  }
}