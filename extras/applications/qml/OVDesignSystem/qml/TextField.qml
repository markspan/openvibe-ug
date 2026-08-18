// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVControls as OV
import ".."

TextField {
  id: _textInput

  leftPadding: Theme.spacingXs
  rightPadding: Theme.spacingXs
  topPadding: Theme.spacingXs
  bottomPadding: Theme.spacingXs
  font: Theme.textMediumRegular
  color: Theme.textPrimary
  selectedTextColor: Theme.textPrimary
  selectionColor: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.4)

  property color defaultBackgorundColor: Theme.textInputBackground
  property string lastValidText: acceptableInput ? text : ""

  background: Rectangle {
    id: _background
    radius: Theme.radiusXs
    color: {
      if (_textInput.activeFocus || _textInput.hovered) {
          return Theme.hover
      }
      return defaultBackgorundColor
    }
    border.width: {
        if (_textInput.activeFocus) {
            return Theme.borderSm
        }
        return Theme.borderSm / 2
    }
    border.color: {
      if (_textInput.activeFocus) {
          return Theme.primary
      }
      return Theme.stroke
    }
  }

  Keys.onPressed: (event) => {
    if (event.key === Qt.Key_Escape || event.key === Qt.Key_Return ||event.key === Qt.Key_Enter) {
        _textInput.focus = false
        event.accepted = true
    }
  }
}
/*
  MouseArea {
      anchors.fill: parent
      hoverEnabled: true
      onEntered: {
          _background.color = Theme.hover
      }
      onExited: {
          if (!_textInput.activeFocus) {
              _background.color = Theme.textInputBackground
          }
      }
      onClicked: {
          if (!_textInput.activeFocus) {
              _textInput.forceActiveFocus()
              _textInput.selectAll()
              _background.color = Theme.hover
              _background.border.color = Theme.primary
          }
      }
      onPressed: (mouse) => {
          if (_textInput.activeFocus) {
              mouse.accepted = false // pass click to the TextInput
          }
      }
  }*/