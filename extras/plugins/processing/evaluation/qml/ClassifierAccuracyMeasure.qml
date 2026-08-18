// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import OVControls as OV
import OVParameters as OV
import OVStyle as OV

Item {
  id: _control
  anchors.fill: parent
  
  ColumnLayout {
    spacing: OV.Style.mediumRowSpacing
    anchors.fill: parent
    Row {
      id: classifierAccuracyMeasureToolbar
      Layout.topMargin: OV.Style.mediumPadding
      Layout.alignment: Qt.AlignHCenter
      height: OV.Style.mediumButtonHeight   
      spacing: OV.Style.mediumPadding

      OV.Button {
        id: resetScoreButton
        text: qsTr("Reset")
        icon.name: "refresh"
        onPressed: {
          dataSourceAccuracy.resetScores()
        }
      }
      CheckBox {
        id: showPercentagesToggleButton
        text: qsTr("Percentages")
        checked: true
      }
      CheckBox {
        id: showScoresToggleButton
        text: qsTr("Scores")
        checked: true
      }

      OV.Separator { 
        Layout.fillWidth: true;
      }
    }

    ListView {
      Layout.fillWidth: true
      Layout.fillHeight: true

      model: dataSourceAccuracy.progressBars
      delegate: Control {
        required property string label
        required property int score
        required property int nstimulation

        width: parent.width
        height: OV.Style.mediumLabelHeight + OV.Style.smallRowSpacing

        background: Rectangle {
          color: OV.Style.colors.OV.Style.colors.bgColor
          radius: OV.Style.buttonRadius
          opacity: 0.6
        }

        ColumnLayout {
          id: _classifierAccuracyMeasureCard
          spacing: OV.Style.smallRowSpacing
          anchors.fill: parent

          Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: OV.Style.mediumRowSpacing
            padding: OV.Style.smallPadding

            Label {
              id: labelClassifierName
              horizontalAlignment: Text.AlignHCenter
              verticalAlignment: Text.AlignVCenter
              
              text: label + ":"
              font: OV.Style.fonts.cardText
              color: OV.Style.colors.textColorBase

            }
  
            OV.ProgressBar {
              id: progressBarClassifierAccuracy
              Layout.fillWidth: true
              height: (2/3)*labelClassifierName.height
              
              value: nstimulation > 0 ? score / nstimulation : 0
              indeterminate: nstimulation == 0
              visible: showPercentagesToggleButton.checked
            }

            Label { 
              Layout.fillWidth: true
              horizontalAlignment: Text.AlignHCenter
              verticalAlignment: Text.AlignVCenter
              
              text: "Score: " + score + " / " + nstimulation
              visible: showScoresToggleButton.checked
              font: OV.Style.fonts.formLabel
              color: OV.Style.colors.textColorBase
            }
          }

        }
      }
    }  
  }
}