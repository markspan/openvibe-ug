// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVControls as OV
import ".."

Control {
  id: _spinButton

  enum Type { Integer, Double }

  property int type: SpinButton.Type.Integer
  property real value: 0
  property real from: 0
  property real to: 100
  property real step: type === SpinButton.Type.Integer ? 1 : 0.1
  property int decimals: type === SpinButton.Type.Integer ? 0 : 2

  function increment() {
    var newValue = value + step
    if (newValue > to) {
      newValue = to
    }
    OV.QmlHelper.setValue(_spinButton, "value", newValue)
  }

  function decrement() {
    var newValue = value - step
    if (newValue < from) {
      newValue = from
    }
    OV.QmlHelper.setValue(_spinButton, "value", newValue)
  }

  IntValidator {
      id: _intValidator
      bottom: _spinButton.from
      top: _spinButton.to
  }

  DoubleValidator {
      id: _doubleValidator
      bottom: _spinButton.from
      top: _spinButton.to
      decimals: _spinButton.decimals
      locale: "C"
      notation: DoubleValidator.StandardNotation
  }

  background: Rectangle {
    color: "transparent"
  }

  contentItem: Rectangle {
    id: _content

    implicitWidth: _contentLayout.implicitWidth
    implicitHeight: Theme.menuItemHeight

    RowLayout {
      id: _contentLayout
      anchors.fill: parent
      spacing: Theme.spacingSm

      TextMetrics {
        id: _toMetrics
        font: _textInput.font
        text: _spinButton.type === SpinButton.Type.Integer
            ? String(_spinButton.to)
            : _spinButton.to.toFixed(_spinButton.decimals)
      }

      TextField {
        id: _textInput
        Layout.preferredWidth: _toMetrics.width + _textInput.leftPadding + _textInput.rightPadding + Theme.spacingMd // Add extra as the font is not monospace
        horizontalAlignment: TextInput.AlignRight
        defaultBackgorundColor: "transparent"

        text: _spinButton.type === SpinButton.Type.Integer ? _spinButton.value : _spinButton.value.toFixed(_spinButton.decimals)
        validator: _spinButton.type === SpinButton.Type.Integer ? _intValidator : _doubleValidator

        onActiveFocusChanged: {
          if (!activeFocus && !acceptableInput) {
            // put back last value if bad edit
            if (Number(text) < validator.bottom) {
              OV.QmlHelper.setValue(_spinButton, "value", Number(validator.bottom))
            } else if (Number(text) > validator.top) {
              OV.QmlHelper.setValue(_spinButton, "value", Number(validator.top))
            } else {
              OV.QmlHelper.setValue(_spinButton, "value", _spinButton.value) // hack to force resetting textfield text
            }
          } else if (!activeFocus && acceptableInput) {
            OV.QmlHelper.setValue(_spinButton, "value", Number(text))
          }
        }
      }

      Rectangle {
        id: _buttons
        implicitWidth: _buttonLayout.implicitWidth
        Layout.preferredHeight: _buttonLayout.implicitHeight
        Layout.alignment: Qt.AlignVCenter
        color: "transparent"

        ColumnLayout {
          id: _buttonLayout
          anchors.centerIn: parent

          spacing: 0

          IconButton {
            id: _increment
            iconName: "chevron-up"
            iconSize: Theme.iconXs
            iconColor: Theme.iconPrimary
            enabled: _spinButton.value < _spinButton.to
            onClicked: {
              _spinButton.increment()
            }
          }
          IconButton {
            id: _decrement
            iconName: "chevron-down"
            iconSize: Theme.iconXs
            iconColor: Theme.iconPrimary
            enabled: _spinButton.value > _spinButton.from
            onClicked: {
              _spinButton.decrement()
            }
          }
        }
      }
    }
  }
}