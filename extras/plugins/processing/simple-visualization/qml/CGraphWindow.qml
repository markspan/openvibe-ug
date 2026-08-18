import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import OVControls as OV
import OVStyle as OV

Item {
  id: _root
  anchors.fill: parent
  
  ScrollView {
    anchors.fill: parent

    GridLayout {
      id: gridView
      property int cellSize: 400
      property int spacing: 0
  
      width: parent.width
      rowSpacing: spacing
      columnSpacing: spacing
      rows: dataSource.rowCount
      columns: dataSource.colCount
  
      Repeater {
        model: dataSource.graphs
        delegate: ChartView {
          id: _chart
          required property int startTime
          required property int endTime
          required property var modelData
  
          width: gridView.cellSize
          height: gridView.cellSize
          antialiasing: true
      
          legend.visible: true
  
          ValueAxis {
            id: xAxis
              min: startTime 
              max: endTime
              labelFormat: "%.2f"
          }
          ValueAxis {
            id: yAxis
            min: -5
            max: 5
            labelFormat: "%.2f"
          }
  
          Connections {
            target: modelData
            function onLaunchQmlRedraw(curveIndex, isData) {
              if(_chart.count != modelData.curves.length * 2) {
                _chart.removeAllSeries()
                //( data -> %2 == 0, variance -> %2 == 1)
                for (var i = 0; i < modelData.curves.length; i++) {
                  var curve = modelData.curves[i];
                  var serieData = _chart.createSeries(ChartView.SeriesTypeLine, curve.label, xAxis, yAxis)
                  serieData.color = curve.col
  
                  var serieVariance = _chart.createSeries(ChartView.SeriesTypeArea, "", xAxis, yAxis)
                  serieVariance.color = curve.col
                  serieVariance.opacity = 0.5
                  var lowerLineSeries = Qt.createQmlObject('import QtCharts; LineSeries {}',
                                                       serieVariance,
                                                       "LowerLineSeries " + curve.label);
                  serieVariance.lowerSeries = lowerLineSeries                                     
                }
              }
  
              if(isData) {
                var curve = modelData.curves[curveIndex]
                var serie = _chart.series(2*curveIndex)
                curve.setSerieData(serie, xAxis, yAxis)
              } else {
                var curve = modelData.curves[curveIndex]
                var serie = _chart.series(2*curveIndex + 1)
                curve.setSerieVariance(serie, xAxis, yAxis)
              }
            }
          }
  
          Component.onCompleted: {
            OV.Style.mode = OV.Style.Mode.Light
          }
        }
      }
    }
  
  }

  onWidthChanged: {
    gridView.cellSize = width / dataSource.colCount
  }
}