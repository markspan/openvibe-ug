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
  property bool multiview : false

  OV.Parameters {
    id: _params
    parameters: dataSource.parameters
  }

  Column {
    id: _paramsColumn
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.bottom: parent.bottom
    width: OV.Style.smallPanelWidth
    spacing: OV.Style.mediumPadding
  
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
  }

  // gridview when multiview, chartview when not
  ScrollView {
    anchors.top: parent.top
    anchors.left: _paramsColumn.right
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    visible: _control.multiview

    GridLayout {
      id: gridView
      property int cellSize: 400
      property int spacing: 0
      property int availableColumns: Math.max(1, Math.floor((_control.width - _paramsColumn.width) / (cellSize + spacing)) )
  
      width: parent.width  
      columnSpacing: spacing
      rowSpacing: spacing
      columns: availableColumns
  
      Repeater {
        id: repeater
        model: dataSource.channels
        delegate: ChartView {
          id: chartViewDelegate
          required property int index
          required property string name
          required property color coloring
    
          width: gridView.cellSize
          height: gridView.cellSize
          antialiasing: true
    
          title: name
          legend.visible: false
          
          ValueAxis {
            id: xAxis
            min: 0
            max: 2
            labelFormat: "%.2f"
          }
      
          ValueAxis {
            id: yAxis
            min: -1
            max: 1
            labelFormat: "%.2f"
          }
    
          Connections {
            target: dataSource
            function onLaunchQmlRedraw() {
              if(!gridView.visible) {
                return
              }
              //create serie if there is none
              if(chartViewDelegate.count == 0) {
                var serie = chartViewDelegate.createSeries(ChartView.SeriesTypeLine, name, xAxis, yAxis)
                serie.useOpenGL = true
                serie.color = coloring
              }
              //update serie
              yAxis.min = 0.
              yAxis.max = 0.
              var serie = chartViewDelegate.series(0)
              chartViewDelegate.visible = dataSource.updateSerie(index, serie, xAxis, yAxis)
              updatestimulations(chartViewDelegate, xAxis, yAxis)
            }
          }
        }
      }
    }    
  }

  ColumnLayout {
    anchors.top: parent.top
    anchors.left: _paramsColumn.right
    anchors.bottom: parent.bottom
    anchors.right: parent.right  
    visible: !_control.multiview

    Label {
      id: legendLabel
      Layout.fillWidth: true
      Layout.preferredHeight: OV.Style.mediumLabelHeight
      
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      padding: OV.Style.mediumPadding
      textFormat: Text.RichText
    }

    ChartView {
      id: chartView
      Layout.fillWidth: true
      Layout.fillHeight: true
      property var series: []
      title: "Channels over time(s)"
  
      visible: !_control.multiview
      legend.visible: false  
      antialiasing: true
    
      ValueAxis {
        id: xAxis
        min: 0
        max: 2
        labelFormat: "%.2f"
      }
  
      ValueAxis {
        id: yAxis
        min: -1
        max: 1
        labelFormat: "%.2f"
      }
  
      Connections {
        target: dataSource
        function onChannelsChanged() {
          chartView.series = []
          chartView.removeAllSeries()
          var newLegend = ""
          for (var i = 0; i < dataSource.channels.length; i++) {
            var channel = dataSource.channels[i]
            var serie = chartView.createSeries(ChartView.SeriesTypeLine, channel.name, xAxis, yAxis)
            serie.useOpenGL = true
            serie.color = channel.coloring
            chartView.series.push(serie)
            newLegend += "<font color='" + channel.coloring + "'>" + channel.name + "</font> | "
          }
          legendLabel.text = newLegend.slice(0, -3) // remove last " | "
        }
      }
    
      Connections {
        target: dataSource
        function onLaunchQmlRedraw() {
          if(!chartView.visible) {
            return
          }
          yAxis.min = 0.
          yAxis.max = 0.
          for (var i = 0; i < chartView.series.length; i++) {
            var serie = chartView.series[i]
            dataSource.updateSerie(i, serie, chartView.axisX(serie), chartView.axisY(serie))
          }
  
          // update stimulations
          updatestimulations(chartView, xAxis, yAxis)
        }
      }
    }
  }

  Connections {
    target: dataSource
    function onSetMultiView(multiview) {
      _control.multiview = multiview
    }
  }
  
  Component.onCompleted: {
    OV.Style.mode = OV.Style.Mode.Light

    _params.updateParametersModel()
  }

  function getSeriesByName(chart, name) {
    var children = chart.children;
    for (var i = 0; i < children.length; ++i) {
      if (children[i].objectName === name) {
        return children[i];
      }
    }
    return null;
  }

  function updatestimulations(chart, xAxis, yAxis) {
    //update stimulations
    for(var i = 0; i < dataSource.stimulations.length; i++) {
      var stimulation = dataSource.stimulations[i]
      var seriestimulation = getSeriesByName(chart, stimulation.name);
      if(!seriestimulation) {
        //create serie if it does not exist
        if(stimulation.time >= xAxis.min && stimulation.time <= xAxis.max) {
          seriestimulation = chartView.createSeries(ChartView.SeriesTypeLine, stimulation.name, xAxis, yAxis)
          seriestimulation.useOpenGL = true
          seriestimulation.color = stimulation.coloring
          seriestimulation.append(stimulation.time, yAxis.min)
          seriestimulation.append(stimulation.time, yAxis.max)
        }
      } else {
        if(stimulation.time >= xAxis.min && stimulation.time <= xAxis.max) {
          //append the stimulation to the serie
          seriestimulation.append(stimulation.time, yAxis.min)
          seriestimulation.append(stimulation.time, yAxis.max)
        } else {
          chart.removeSeries(seriestimulation)
        }
      }
    }
  }
}
