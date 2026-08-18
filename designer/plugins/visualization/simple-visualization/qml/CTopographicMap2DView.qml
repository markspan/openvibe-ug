// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import OVControls as OV
import OVStyle as OV

Item {
  id: _root
  anchors.fill: parent

  property int potentialsRefreshCounter: 0
  property int textHeight: 14
  property int m_paletteWindowHeight: Math.min(Math.max(0.1 * _canvas.height, 10 + textHeight),
                                           30 + textHeight + 4);

  property int m_headWindowWidth: _canvas.width
  property int m_headWindowHeight: _canvas.height - _root.m_paletteWindowHeight

  //property int m_clipmaskWidth: 0
  //property int m_clipmaskHeight: 0

  //angles relative to 3 o'clock position, CCW, in degrees
  property real m_skullOutlineStartAngle: 0.0
  property real m_skullOutlineEndAngle: 0.0
  property real m_skullFillStartAngle: 0.0
  property real m_skullFillEndAngle: 0.0

  //determined from m_skullOutlineEndAngle
  property int m_skullOutlineLeftPointX: 0
  property int m_skullOutlineLeftPointY: 0

  //determined from m_skullOutlineStartAngle
  property int m_skullOutlineRightPointX: 0
  property int m_skullOutlineRightPointY: 0

  //determined from m_skullFillEndAngle
  //property int m_skullFillLeftPointX: 0
  //property int m_skullFillLeftPointY: 0
  //determined from m_skullFillStartAngle
  //property int m_skullFillRightPointX: 0
  //property int m_skullFillRightPointY: 0
  //property int m_skullFillBottomPointX: 0
  //property int m_skullFillBottomPointY: 0

  /////////////////////////////
  // TOP VIEW
  /////////////////////////////
  property int m_noseY: 0

  /////////////////////////////
  // BOTTOM VIEW
  /////////////////////////////
  property int m_leftNeckX: 0
  property int m_leftNeckY: 0
  property int m_rightNeckX: 0
  property int m_rightNeckY: 0

  //////////////////////////////////
  // LEFT/RIGHT VIEWS
  //////////////////////////////////
  /*
    + A
   /
  /
 /
+ B
| C
+----+ D
     |
     + E
  */
  property int m_noseTopX: 0
  property int m_noseTopY: 0 //A
  property int m_noseBumpX: 0
  property int m_noseBumpY: 0 //B
  property int m_noseTipX: 0
  property int m_noseTipY: 0 //C
  property int m_noseBaseX: 0
  property int m_noseBaseY: 0 //D
  property int m_noseBottomX: 0
  property int m_noseBottomY: 0 //E

  RowLayout {
    id: buttonsLayout
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.topMargin: 10
    anchors.leftMargin: 10
    spacing: 10

    Text {
      text: "View"
    }

    ComboBox {
      model: ["Top", "Back", "Left", "Right"]

      Component.onCompleted: currentIndex = indexOfValue(dataSource.currentView)
      onActivated: dataSource.currentView = currentValue
    }

    Text {
      text: "Projection"
    }

    ComboBox {
      model: ["Axial", "Radial", "NumProjection"]
 
      Component.onCompleted: currentIndex = indexOfValue(dataSource.projection)
      onActivated: dataSource.projection = currentValue
    }
  }

  Image {
    id: skullPotentials
    visible: false
  }

  Canvas {
    id: _canvas
    anchors.top: buttonsLayout.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
  
   	property color bgColor: "white";
  	property color fgColor: "black";
    property bool m_clearCanvas: true;

    onPaint: {
   		var ctx = getContext("2d");

      //save the context before clipping
      ctx.save();

      // Define Circle Clipping Path
      ctx.beginPath();
      ctx.arc(
          dataSource.skullX + dataSource.skullDiameter / 2,
          dataSource.skullY + dataSource.skullDiameter / 2,
          dataSource.skullDiameter / 2 - 1,
          _root.m_skullFillStartAngle,
          _root.m_skullFillEndAngle,
          true);
      ctx.clip();  // Activate the clipping region

      //draw potentials
      if (skullPotentials.status === Image.Ready) {
        ctx.drawImage(skullPotentials, dataSource.skullX, dataSource.skullY);
      }

      // Draw electrodes
      const electrodeRingSize = 5;
      const electrodeRingThickness = 1;

      // Draw rings
      const nChannel = dataSource.nbChannels();
      for (let i = 0; i < nChannel; ++i) {
        const position = dataSource.channel2DPosition(i, dataSource.skullX, dataSource.skullY);
        if (position.x == 10000&& position.y == 10000) { continue; }
        const channelX = position.x;
        const channelY = position.y;
  
        // Fill ring with white
        ctx.fillStyle = _canvas.bgColor;
        ctx.beginPath();
        ctx.arc(channelX, channelY, electrodeRingSize / 2, 0, 2 * Math.PI);
        ctx.fill();
  
        // Draw ring centered on channel location
        ctx.strokeStyle = _canvas.fgColor;
        ctx.lineWidth = electrodeRingThickness;
        ctx.beginPath();
        ctx.arc(channelX, channelY, electrodeRingSize / 2, 0, 2 * Math.PI);
        ctx.stroke();
  
        // Draw channel label
        ctx.fillStyle = _canvas.fgColor;
        const electrodeLabel = dataSource.channelLabel(i);
        ctx.font = "12px sans-serif";
        const textWidth = ctx.measureText(electrodeLabel).width;
        ctx.fillText(electrodeLabel, channelX - textWidth / 2, channelY - electrodeRingSize / 2 - 12);
      }
      ctx.restore();  // Restore the context to its original state
      ctx.lineWidth = 2;
      ctx.strokeStyle = _canvas.fgColor;

      if(m_clearCanvas) {
        //draw the skull outline
  	    drawFace();

        // Draw head outline
        ctx.beginPath();
        ctx.arc(dataSource.skullX + dataSource.skullDiameter / 2,
                dataSource.skullY + dataSource.skullDiameter / 2,
                dataSource.skullDiameter / 2,
                _root.m_skullOutlineStartAngle,
                _root.m_skullOutlineEndAngle,
                true);

        ctx.stroke();

		    drawPalette();
        m_clearCanvas = false;
      }
    }

    function drawFace() {
      var ctx = getContext("2d");
      ctx.lineWidth = 2;
      ctx.lineCap = "butt";
      ctx.lineJoin = "bevel";
      ctx.strokeStyle = _canvas.fgColor; // Equivalent to GTK's foreground color

      // Head center calculations
      var skullRadius = dataSource.skullDiameter / 2.0;
      var skullCenterX = dataSource.skullX + skullRadius;
      var skullCenterY = dataSource.skullY + skullRadius;

      if (dataSource.currentView === "Top") {
        var noseHalfAngle = 6. * Math.PI / 180.; // Convert to radians
        var lx = skullCenterX + skullRadius * Math.cos(Math.PI / 2. + noseHalfAngle);
        var ly = skullCenterY - skullRadius * Math.sin(Math.PI / 2. + noseHalfAngle);
        var rx = skullCenterX + skullRadius * Math.cos(Math.PI / 2. - noseHalfAngle);
        var ry = skullCenterY - skullRadius * Math.sin(Math.PI / 2. - noseHalfAngle);

        ctx.beginPath();
        ctx.moveTo(lx, ly);
        ctx.lineTo(skullCenterX, m_noseY);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(rx, ry);
        ctx.lineTo(skullCenterX, m_noseY);
        ctx.stroke();
      }
      else if (dataSource.currentView === "Back") {
        ctx.beginPath();
        ctx.moveTo(m_skullOutlineLeftPointX, m_skullOutlineLeftPointY);
        ctx.lineTo(m_leftNeckX, m_leftNeckY);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(m_skullOutlineRightPointX, m_skullOutlineRightPointY);
        ctx.lineTo(m_rightNeckX, m_rightNeckY);
        ctx.stroke();
      }
      else if (dataSource.currentView === "Left" || dataSource.currentView === "Right") {
        ctx.beginPath();
        ctx.moveTo(m_noseTopX, m_noseTopY);
        ctx.lineTo(m_noseBumpX, m_noseBumpY);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(m_noseBumpX, m_noseBumpY);
        ctx.lineTo(m_noseTipX, m_noseTipY);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(m_noseTipX, m_noseTipY);
        ctx.lineTo(m_noseBaseX, m_noseBaseY);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(m_noseBaseX, m_noseBaseY);
        ctx.lineTo(m_noseBottomX, m_noseBottomY);
        ctx.stroke();
      }
    }
  
    function drawPalette() {
      var ctx = _canvas.getContext("2d");
      ctx.strokeStyle = _canvas.fgColor;
      ctx.lineWidth = 1;
      const margin = 5;
      const textHeight = 14;
      const paletteBarHeight = m_paletteWindowHeight - textHeight;
      const paletteBarWidth =  _canvas.width - 2*margin;
      const paletteBarY = _canvas.height - paletteBarHeight - margin - textHeight - margin;
      const nbColors = dataSource.palette.length;
      const barWidth = paletteBarWidth / nbColors;
  
      for (let i = 0; i < nbColors; ++i) {
        ctx.fillStyle = dataSource.palette[i];
        ctx.fillRect(margin + (nbColors - 1 - i) * barWidth, paletteBarY, barWidth, paletteBarHeight);
      }
  
      // Draw 0 , + and - labels
      ctx.fillStyle = _canvas.fgColor;
      ctx.font = "12px sans-serif";
      ctx.textAlign = "center";
      ctx.fillText("0", _canvas.width / 2, _canvas.height - margin - textHeight / 2);
      ctx.fillText("+", 2*margin, _canvas.height - margin - textHeight / 2);
      ctx.fillText("-", _canvas.width - 2*margin, _canvas.height - margin - textHeight / 2);
    }
  }

  function resize() {
    //// Retrieve text size
    //var text = Qt.createQmlObject('import QtQuick 2.0; Text { text: "0"; font.pixelSize: 12 }', canvas);
    //var textHeight = text.height;
    const textHeight = 14;
    var headMaxSize = Math.min(0.9 * _root.m_headWindowWidth, 0.9 * _root.m_headWindowHeight);
    
    // canvas not ready yet!
    if(_canvas.width === 0 || !_canvas.available) {
      return;
    }

    if (dataSource.currentView === "Top") {
      // Height used up by nose
      var noseProtrudingHeight = 0.1 * headMaxSize;
      // Y coordinate where nose starts
      _root.m_noseY = (_root.m_headWindowHeight - headMaxSize) / 2;
      // Skull diameter
      dataSource.skullDiameter = headMaxSize - noseProtrudingHeight;
      // Skull upper left corner
      dataSource.skullX = (_root.m_headWindowWidth - dataSource.skullDiameter) / 2;
      dataSource.skullY = _root.m_noseY + noseProtrudingHeight;
  
      // Skull outline and filled area start/end angles
      _root.m_skullOutlineStartAngle = 0;
      _root.m_skullOutlineEndAngle = 2*Math.PI;
      _root.m_skullFillStartAngle = 0;
      _root.m_skullFillEndAngle = 360;
  
      // Clip mask
      //_root.m_clipmaskWidth = dataSource.skullDiameter;
      //_root.m_clipmaskHeight = dataSource.skullDiameter;
    } else if (dataSource.currentView === "Back") {
      // Height used up by neck
      var neckProtrudingHeight = 0.072 * headMaxSize;
  
      // Skull diameter
      dataSource.skullDiameter = headMaxSize - neckProtrudingHeight;
      // Skull upper left corner
      dataSource.skullX = (_root.m_headWindowWidth - dataSource.skullDiameter) / 2;
      dataSource.skullY = (_root.m_headWindowHeight - headMaxSize) / 2;
  

      //_root.m_skullOutlineStartAngle = -38;
	  	//_root.m_skullOutlineEndAngle   = 180 - _root.m_skullOutlineStartAngle;
	  	//_root.m_skullFillStartAngle    = -30;
	  	//_root.m_skullFillEndAngle      = 180 - _root.m_skullFillStartAngle;

      _root.m_skullOutlineStartAngle = 38*Math.PI/180;
	  	_root.m_skullOutlineEndAngle   = (180-38)*Math.PI/180;
	  	_root.m_skullFillStartAngle    = 30*Math.PI/180;
	  	_root.m_skullFillEndAngle      = 150*Math.PI/180;
  
      var skullRadius = dataSource.skullDiameter / 2;
      var skullCenterX = dataSource.skullX + skullRadius;
      var skullCenterY = dataSource.skullY + skullRadius;
  
      _root.m_skullOutlineLeftPointX = skullCenterX + skullRadius * Math.cos(_root.m_skullOutlineEndAngle);
      _root.m_skullOutlineLeftPointY = skullCenterY + skullRadius * Math.sin(_root.m_skullOutlineEndAngle);
      _root.m_skullOutlineRightPointX = skullCenterX + skullRadius * Math.cos(_root.m_skullOutlineStartAngle);
      _root.m_skullOutlineRightPointY = skullCenterY + skullRadius * Math.sin(_root.m_skullOutlineStartAngle);
  
      //_root.m_skullFillLeftPointX = skullCenterX + skullRadius * Math.cos(_canvas.degToRad * _root.m_skullFillEndAngle);
      //_root.m_skullFillLeftPointY = skullCenterY - skullRadius * Math.sin(_canvas.degToRad * _root.m_skullFillEndAngle);
      //_root.m_skullFillRightPointX = skullCenterX + skullRadius * Math.cos(_canvas.degToRad * _root.m_skullFillStartAngle);
      //_root.m_skullFillRightPointY = skullCenterY - skullRadius * Math.sin(_canvas.degToRad * _root.m_skullFillStartAngle);
      //_root.m_skullFillBottomPointX = dataSource.skullX + skullRadius;
      //_root.m_skullFillBottomPointY = _root.m_skullFillRightPointY;
  
      // Neck extremities
      _root.m_leftNeckX = _root.m_skullOutlineLeftPointX + 0.025 * dataSource.skullDiameter;
      _root.m_leftNeckY = _root.m_skullOutlineLeftPointY + neckProtrudingHeight;
      _root.m_rightNeckX = _root.m_skullOutlineRightPointX - 0.025 * dataSource.skullDiameter;
      _root.m_rightNeckY = _root.m_leftNeckY;
  
      // Clip mask
      //_root.m_clipmaskWidth = dataSource.skullDiameter;
      //_root.m_clipmaskHeight = _root.m_skullFillBottomPointY - dataSource.skullY + 1;
    } else if (dataSource.currentView === "Left" || dataSource.currentView === "Right") {
      // Width used up by nose
      var noseProtrudingWidth = 0.06 * dataSource.skullDiameter;
  
      // Skull diameter
      dataSource.skullDiameter = headMaxSize - noseProtrudingWidth;
  
      // Topmost skull coordinate
      dataSource.skullY = (_root.m_headWindowHeight - dataSource.skullDiameter) / 2;

      if (dataSource.currentView === "Left") {
        // X coordinate of nose tip
        _root.m_noseTipX = (_root.m_headWindowWidth - headMaxSize) / 2;
        // Leftmost skull coordinate
        dataSource.skullX = _root.m_noseTipX + noseProtrudingWidth;
        // Skull outline and filled area start/end angles
        //_root.m_skullOutlineStartAngle = -41;
        //_root.m_skullOutlineEndAngle = 193;
        //_root.m_skullFillStartAngle = -22;
        //_root.m_skullFillEndAngle = 188;

        _root.m_skullOutlineStartAngle = 41*Math.PI/180;
        _root.m_skullOutlineEndAngle = 167*Math.PI/180;
        _root.m_skullFillStartAngle = 22*Math.PI/180;
        _root.m_skullFillEndAngle = 172*Math.PI/180;//-  _root.m_skullFillStartAngle;
  
        var skullRadius = dataSource.skullDiameter / 2;
        var skullCenterX = dataSource.skullX + skullRadius;
        var skullCenterY = dataSource.skullY + skullRadius;
  
        // Nose top = head outline left boundary
        _root.m_noseTopX = skullCenterX + skullRadius * Math.cos(_root.m_skullOutlineEndAngle);
        _root.m_noseTopY = skullCenterY + skullRadius * Math.sin(_root.m_skullOutlineEndAngle);
        // Nose bump
        _root.m_noseBumpX = _root.m_noseTipX;
        _root.m_noseBumpY = _root.m_noseTopY + 0.15 * dataSource.skullDiameter;
        // Nose tip
        _root.m_noseTipY = _root.m_noseBumpY + 0.03 * dataSource.skullDiameter;
        // Nose base
        _root.m_noseBaseX = _root.m_noseTipX + 0.1 * dataSource.skullDiameter;
        _root.m_noseBaseY = _root.m_noseTipY;
        // Nose bottom
        _root.m_noseBottomX = _root.m_noseBaseX;
        _root.m_noseBottomY = _root.m_noseBaseY + 0.02 * dataSource.skullDiameter;
      } else {
        // X coordinate of nose tip
        _root.m_noseTipX = (_root.m_headWindowWidth + headMaxSize) / 2;
        // Leftmost skull coordinate
        dataSource.skullX = (_root.m_headWindowWidth - headMaxSize) / 2;
        // Skull outline and filled area start/end angles
        _root.m_skullOutlineStartAngle = 13*Math.PI/180; //-13
        _root.m_skullOutlineEndAngle = (180-41)*Math.PI/180;  //221
        _root.m_skullFillStartAngle = 8*Math.PI/180;
        _root.m_skullFillEndAngle = (180-22)*Math.PI/180;
  
        var skullRadius = dataSource.skullDiameter / 2;
        var skullCenterX = dataSource.skullX + skullRadius;
        var skullCenterY = dataSource.skullY + skullRadius;
  
        // Nose top = head outline right boundary
        _root.m_noseTopX = skullCenterX + skullRadius * Math.cos(_root.m_skullOutlineStartAngle);
        _root.m_noseTopY = skullCenterY + skullRadius * Math.sin(_root.m_skullOutlineStartAngle);
        // Nose bump
        _root.m_noseBumpX = _root.m_noseTipX;
        _root.m_noseBumpY = _root.m_noseTopY + 0.15 * dataSource.skullDiameter;
        // Nose tip
        _root.m_noseTipY = _root.m_noseBumpY + 0.03 * dataSource.skullDiameter;
        // Nose base
        _root.m_noseBaseX = _root.m_noseTipX - 0.1 * dataSource.skullDiameter;
        _root.m_noseBaseY = _root.m_noseTipY;
        // Nose bottom
        _root.m_noseBottomX = _root.m_noseBaseX;
        _root.m_noseBottomY = _root.m_noseBaseY + 0.02 * dataSource.skullDiameter;
      }

      //var skullRadius = dataSource.skullDiameter / 2;
      //var skullCenterX = dataSource.skullX + skullRadius;
      //var skullCenterY = dataSource.skullY + skullRadius;
      //_root.m_skullFillLeftPointX = skullCenterX + skullRadius * Math.cos(_canvas.degToRad * _root.m_skullFillEndAngle);
      //_root.m_skullFillLeftPointY = skullCenterY - skullRadius * Math.sin(_canvas.degToRad * _root.m_skullFillEndAngle);
      //_root.m_skullFillRightPointX = skullCenterX + skullRadius * Math.cos(_canvas.degToRad * _root.m_skullFillStartAngle);
      //_root.m_skullFillRightPointY = skullCenterY - skullRadius * Math.sin(_canvas.degToRad * _root.m_skullFillStartAngle);
  
      //_root.m_skullFillBottomPointX = dataSource.skullX + skullRadius;
      //_root.m_skullFillBottomPointY = dataSource.skullY + 0.684 * dataSource.skullDiameter;
  
      // Clip mask
      //_root.m_clipmaskWidth = dataSource.skullDiameter;
      //_root.m_clipmaskHeight = _root.m_skullFillBottomPointY - dataSource.skullY + 1;
    }
  
    var ctx = _canvas.getContext("2d");
    ctx.clearRect(0, 0, _canvas.width, _canvas.height);
 		ctx.fillStyle = _canvas.bgColor;
		ctx.fillRect(0, 0, _canvas.width, _canvas.height);

    //draw the content
		ctx.fillStyle = _canvas.fgColor;
		ctx.strokeStyle = _canvas.fgColor;

    _canvas.m_clearCanvas = true;
    dataSource.resizeData();
    _canvas.requestPaint();
  }


  Connections {
    target: dataSource
    function onLaunchQmlRedraw() {
      _canvas.requestPaint();
    }

    function onRefreshPotentials() {
      // force reload
      skullPotentials.source = "image://skullPotentials/" + _root.potentialsRefreshCounter;
      _root.potentialsRefreshCounter++;
    }

    function onClearCanvas() {
      resize();
    }
  }

  Connections {
    target: _root
    function onWidthChanged() {
      resize();
    }
    function onHeightChanged() {
      resize();
    }
  }
}