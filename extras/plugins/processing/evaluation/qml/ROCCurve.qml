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
  /* 
  OV.Parameters {
    id: _params
    parameters: dataSourceROC.parameters
  }
*/
  Column {
    id: _paramsColumn
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    width: 0 // OV.Style.smallPanelWidth
    spacing: 0 //OV.Style.mediumPadding
  
  /*
    Repeater {
      anchors.fill: parent

      model: _params.params_model
      delegate: Loader {
        required property var param;
        required property var component;
        property var lparam: param
        width: OV.Style.smallPanelWidth
        anchors.horizontalCenter: parent.horizontalCenter
        sourceComponent: component
      }
    }
    */
  }

  ChartView {
    id: chartView
    title: "ROC Curve"
    property bool openGL: openGLSupported
    anchors.top: parent.top
    anchors.left: _paramsColumn.right
    anchors.bottom: parent.bottom
    anchors.right: parent.right
  
    antialiasing: true
  
    ValueAxis {
      id: xAxis
      min: 0
      max: 1
      labelFormat: "%.2f"
    }

    ValueAxis {
      id: yAxis
      min: 0
      max: 1
      labelFormat: "%.2f"
    }

    Connections {
      enabled: dataSourceROC !== null
      target: dataSourceROC
      function onCurvesChanged() {
        chartView.removeAllSeries()
        for (var i = 0; i < dataSourceROC.curves.length; i++) {
          var curve = dataSourceROC.curves[i]
          var series = chartView.createSeries(ChartView.SeriesTypeLine, curve.name, xAxis, yAxis)
          series.color = curve.coloring
        }
      }
    }
  
    Connections {
      enabled: dataSourceROC !== null 
      target: dataSourceROC
      function onLaunchQmlRedraw() {
        xAxis.min = 0.
        xAxis.max = 0.
        yAxis.min = 0.
        yAxis.max = 0.
        for (var i = 0; i < chartView.count; i++) {
          var serie = chartView.series(i)
          dataSourceROC.updateSerie(i, serie, chartView.axisX(serie), chartView.axisY(serie))
        }
      }
    }
  }

  Component.onCompleted: {
    OV.Style.mode = OV.Style.Mode.Light
    //_params.updateParametersModel()
  }
}