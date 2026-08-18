// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

// BOX ID : 0x00DD290D, 0x5F142820

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


  Canvas {
    id: _canvas
    anchors.fill: parent
    property int m_margin: 5;
  	property int m_barH: (0.80 * canvasSize.height);	// Height of the Graz Bar
   	property int m_barW: dataSource ? Math.min(m_barH / 6, (canvasSize.width / 2)) : m_barH / 6;	// Keep some proportion
    property int m_modalityX: dataSource? canvasSize.width / (2 + 1) : 0 // Center X Position of the first modality (next is in x+(x+margin))
    property int m_modalityY: m_barH + m_margin;						// Bottom Y Position of the modalities
    property int m_modalityW: (m_barW + 2 * m_margin) / 2;	// Half width dedicated for the modalities

	property bool showCross: false;
	property bool showArrow: false;
	property bool showBar: false;
	property bool showAccuracy: false;

	property color bgColor: "black";
	property color fgColor: "white";

    //m_smallImgs size [m_barW, m_barW]
    //m_largeImgs size [canvasSize.width/3, canvasSize.width/3]

    onPaint: {
		//draw the background
		var context = _canvas.getContext("2d");
        context.clearRect(0, 0, _canvas.width, _canvas.height);

		context.fillStyle = _canvas.bgColor;
		context.fillRect(0, 0, _canvas.width, _canvas.height);

		//draw the content
		context.fillStyle = _canvas.fgColor;
		context.strokeStyle = _canvas.fgColor;
		if(showCross) drawCross();
		if(showArrow) drawArrow();
		if(showBar) drawBar();
		if(showAccuracy) drawAccuracy();
    }

    function drawCross() {
      var context = _canvas.getContext("2d");
      let xc  = canvasSize.width / 2;
      let yc = canvasSize.height / 2;	// Window Center
      let crossSize = Math.min(canvasSize.width, canvasSize.height) / 4;

      context.beginPath();
      context.moveTo(xc - crossSize, yc);
      context.lineTo(xc + crossSize, yc);
      context.stroke();
      
      context.beginPath();
      context.moveTo(xc - crossSize, yc + 1);
      context.lineTo(xc + crossSize, yc + 1);
      context.stroke();
      
      context.beginPath();
      context.moveTo(xc, yc - crossSize);
      context.lineTo(xc, yc + crossSize);
      context.stroke();
      
      context.beginPath();
      context.moveTo(xc + 1, yc - crossSize);
      context.lineTo(xc + 1, yc + crossSize);
      context.stroke();
    }
  
	function drawArrow() {
					
	  var context = _canvas.getContext("2d");
	  let direction = dataSource.showInstruction ? dataSource.direction : "None"
	  var x = canvasSize.width / 2;
	  var y = canvasSize.height / 2;
	  let img_width = canvasSize.width / 4;
	  let img_height = canvasSize.height / 4;

	  let img_size = 40;
	  
	  //order of images are Left,Right,Up,Down
	  switch (direction) {
		case "None":
			x -= img_width-1 ;
			y -= img_height/2;
			context.drawImage(dataSource.imgsPaths[0], x, y, img_width, img_height);
			x = img_width/ 2 + 2;
			y = canvasSize.height / 2 - img_height/2;
			context.drawImage(dataSource.imgsPaths[1], x, y, img_width, img_height);
			break;

		case "Left":
			x -= img_width -1 ;
			y -= img_height/2;
			context.drawImage(dataSource.imgsPaths[0], x, y, img_width, img_height);
			break;

		case "Right":
			x += 2;
			y -= img_height/2;
			context.drawImage(dataSource.imgsPaths[1], x, y, img_width, img_height);
			break;

		case "Up":
			x -= img_width/2;
			y -= img_height - 1;
			context.drawImage(dataSource.imgsPaths[2], x, y, img_width, img_height);
			break;

		case "Down":
			x -= img_width / 2;
			y += 2;
			context.drawImage(dataSource.imgsPaths[3], x, y, img_width, img_height);
			break;

		default: break;
	  }

	}

	function drawBar() {
		let barScale = dataSource.barScale;
		var context = _canvas.getContext("2d");
		let width = canvasSize.width;
		let height = canvasSize.height;

		let usedScale = barScale;
		if (dataSource.positiveFeedbackOnly) {
			//separate in 2 if for visibility
			if(barScale < 0 && dataSource.direction== "Right") {
				return; // No negative feedback
			}
			if(barScale > 0 && dataSource.direction== "Left") {
				return; // No positive feedback
			}
		}

		let barWidth = Math.abs(width * Math.abs(barScale) / 2);
		barWidth = Math.min(barWidth, width / 2);
		var x = width / 2;
		const barHeight = height / 6;
		const y = (height / 2) - (barHeight / 2);

		if (barScale < 0) {
			context.save();
			context.scale(-1, 1); // Flip horizontally
			context.drawImage(dataSource.barImg, -x, y, barWidth, barHeight);
			context.restore();
		} else {
			context.drawImage(dataSource.barImg, x, y, barWidth, barHeight);
		}
	}

	function drawAccuracy() {
	  var context = _canvas.getContext("2d");
	  const stepX = 40, stepY = 16;
	  const startX = 8, startY = 16;
	  var x = startX, y = startY;

	  var predictions = 0, good = 0;

	  for (var i = 0; i < 2; ++i) {
		for (var j = 0; j < 2; ++j) {
		  var val = dataSource.getAccuracy(i, j);
		  predictions += val;
		  if (i == j) {
			context.fillStyle = "white";
			good += val;
		  } else {
			context.fillStyle = "black";
		  }
		  context.fillText(val.toString().padStart(3, '0'), x, y);
		  x += stepX;
		}
		x = startX;
		y += stepY;
	  }

	  var accuracy = (predictions == 0 ? 0.0 : 100.0 * good / predictions).toFixed(1);
	  context.fillStyle = "white";
	  context.fillText("Acc = " + accuracy, x + 2*stepX, y - stepY);
	}
  }

  Connections {
    target: dataSource
    function onImgsPathsChanged() {
      // TODO unload previous images ? 
      // to be done if method called multiple times
      var imgsPaths = dataSource.imgsPaths;
      for(var i = 0; i < imgsPaths.length; i++) {
        _canvas.loadImage(imgsPaths[i]);
      }

	  _canvas.loadImage(dataSource.barImg)
    }
  }

  Connections {
    target: dataSource
    function onLaunchQmlRedraw() {
	  _canvas.showCross = false;
	  _canvas.showArrow = false;
	  _canvas.showBar = false;
	  _canvas.showAccuracy = false;

      switch (dataSource.state) {
		case "Reference":
			_canvas.showCross = true;
			break;

		case "Cue":
			_canvas.showCross = true;
			_canvas.showArrow = true;
			break;

		case "ContinousFeedback":
			_canvas.showCross = true;
			if (dataSource.showFeedback && !dataSource.delayFeedback) {
				_canvas.showBar = true;
			}
			break;

		case "Idle":
			if (dataSource.showFeedback && dataSource.delayFeedback) {
				_canvas.showBar = true;
			}
			break;
      }
      
	  _canvas.showAccuracy = dataSource.showAccuracy;
	  _canvas.requestPaint();
    }
  }
}