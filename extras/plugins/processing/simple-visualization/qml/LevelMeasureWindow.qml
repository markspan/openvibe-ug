import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import OVControls as OV
import OVStyle as OV

Item {
  id: levelMeasureWindow
  
  Row {
    id: _toolBar
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: OV.Style.mediumLabelHeight
    spacing: OV.Style.smallPadding

    OV.Button {
      Layout.alignment: Qt.AlignHCenter
      text: "Reset scores"
      icon.name: "view-refresh"
      onClicked: {
        dataSource.resetScore()
      }
    }

    ColumnLayout {
      spacing: 2
      Label {
        Layout.alignment: Qt.AlignHCenter
        text: "Threshold"
        horizontalAlignment: Text.AlignHCenter
      }

      OV.SpinBox {
        id: thresholdSpinBox
        Layout.preferredWidth: 100
        min: 0
        max: 100
        decimals: 0
        value: 10

        Binding {
          target: dataSource
          property: "threshold"
          value: thresholdSpinBox.value
        } 
      }
    }

    CheckBox {
      id: showPercentagesToggle
      text: "Percentages"
      checked: false
    }
  }

  Grid {
    id: gridView
    anchors.top: _toolBar.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: OV.Style.mediumPadding

    rows: dataSource.nbRows
    columns: dataSource.nbCols
    columnSpacing: OV.Style.smallPadding

    Repeater {
      model: dataSource.progressBars
      delegate: OV.ProgressBar {
        required property int percent
        required property int score

        width: levelMeasureWindow.width / dataSource.nbCols - OV.Style.smallPadding
        height: OV.Style.largeLabelHeight

        from: 0
        to: 100
        value: percent
        indeterminate: false

        Label {
          anchors.centerIn: parent
          horizontalAlignment: Text.AlignHCenter
          width: parent.width
          text: "score: " + score + (showPercentagesToggle.checked ? " level: " + percent + "%" : "")
          wrapMode: Text.Wrap
          color: OV.Style.colors.textColorBase
        }
      }
    }
  }
  Component.onCompleted: {
    // Ensure the dataSource is initialized
    if (!dataSource) {
      console.warn("Data source is not set for LevelMeasureWindow.");
    } else {
      thresholdSpinBox.value = dataSource.threshold;
    }
  }
}