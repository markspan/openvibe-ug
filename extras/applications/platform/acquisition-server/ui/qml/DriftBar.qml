// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

import DesignSystem as DS


Control {
  id: control
  
  property real from: -1.0
  property real to: 1.0
  required property real value

  ProgressBar {
    id: _negativeBar
    
    width: parent.width / 2
    height: parent.height

    from: 0.0
    to: 1.0

    value: control.value < 0 ? -control.value : 0

    background: Rectangle {
      anchors.fill: parent
      color: DS.Theme.quaternary
      bottomLeftRadius: DS.Theme.radiusSm
      topLeftRadius: DS.Theme.radiusSm
      bottomRightRadius: 0
      topRightRadius: 0
    }

    contentItem: Item {
      Rectangle {
        id: _negativeGradientSource
        anchors.right: parent.right
        height: parent.height
        width: parent.width
        
        gradient: DS.Theme.negativeDriftGradient

        bottomLeftRadius: DS.Theme.radiusSm
        topLeftRadius: DS.Theme.radiusSm
        bottomRightRadius: 0
        topRightRadius: 0

        layer.enabled: true
        visible: false
      }


      Item {
        id: _negativeBarMaskSource
        anchors.fill: parent
        layer.enabled: true
        visible: false
        Rectangle {
          anchors.right: parent.right
          anchors.top: parent.top
          anchors.bottom: parent.bottom
          width: _negativeBar.position * parent.width
          
          bottomLeftRadius: DS.Theme.radiusSm
          topLeftRadius: DS.Theme.radiusSm
          bottomRightRadius: 0
          topRightRadius: 0
        }
      }

      MultiEffect {
        anchors.fill: parent
        source: _negativeGradientSource
        maskEnabled: true
        maskSource: _negativeBarMaskSource
      }
    }
  }

  ProgressBar {
    id: _positiveBar

    x: parent.width / 2
    width: parent.width / 2
    height: parent.height

    from: 0.0
    to: 1.0

    value: control.value > 0 ? control.value : 0
    
    background: Rectangle {
      width: parent.width
      height: parent.height
      color: DS.Theme.quaternary
      bottomLeftRadius: 0
      topLeftRadius: 0
      bottomRightRadius: DS.Theme.radiusSm
      topRightRadius: DS.Theme.radiusSm
    }

    contentItem: Item {
      Rectangle {
        id: _positiveGradientSource
        anchors.fill: parent

        gradient: DS.Theme.driftGradient

        bottomLeftRadius: 0
        topLeftRadius: 0
        bottomRightRadius: DS.Theme.radiusSm
        topRightRadius: DS.Theme.radiusSm

        layer.enabled: true
        visible: false
      }

      Item {
        id: _positiveBarMaskSource
        anchors.fill: parent
        layer.enabled: true
        visible: false
        Rectangle {
          anchors.left: parent.left
          anchors.top: parent.top
          anchors.bottom: parent.bottom
          width: _positiveBar.position * parent.width
          
          bottomLeftRadius: 0
          topLeftRadius: 0
          bottomRightRadius: DS.Theme.radiusSm
          topRightRadius: DS.Theme.radiusSm
        }
      }

      MultiEffect {
        anchors.fill: parent
        source: _positiveGradientSource
        maskEnabled: true
        maskSource: _positiveBarMaskSource
      }
    }
  }
}