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


  Canvas {
    id: _canvas
    anchors.fill: parent
    property int m_margin: 5;
  	property int m_barH: (0.80 * canvasSize.height);	// Height of the Graz Bar
   	property int m_barW: dataSource ? Math.min(m_barH / 6, (canvasSize.width / dataSource.nbModality)) : m_barH / 6;	// Keep some proportion
    property int m_modalityX: dataSource? canvasSize.width / (dataSource.nbModality + 1) : 0 // Center X Position of the first modality (next is in x+(x+margin))
    property int m_modalityY: m_barH + m_margin;						// Bottom Y Position of the modalities
    property int m_modalityW: (m_barW + 2 * m_margin) / 2;	// Half width dedicated for the modalities

	property bool showReference: false;
	property bool showCross: false;
	property bool showModality: false;
	property bool showBar: false;
	property bool showAccuracy: false;

	property color bgColor: "black";
	property color fgColor: "white";

    //m_smallImgs size [m_barW, m_barW]
    //m_largeImgs size [canvasSize.width/3, canvasSize.width/3]

    function drawReference() {
      var context = _canvas.getContext("2d");
      var currX = m_modalityX;
      var imgs = dataSource.imgsPaths;
      // Draw for each Horizontal line, Vertical Line & modality
      for (var i = 1; i < imgs.length; ++i, currX += m_modalityX) {
        context.beginPath();
        context.moveTo(currX - m_modalityW, m_modalityY);
        context.lineTo(currX + m_modalityW, m_modalityY);
        context.stroke();
  
        context.beginPath();
        context.moveTo(currX, m_margin);
        context.lineTo(currX, m_modalityY);
        context.stroke();
  
        var x = currX - m_barW / 2;
        var y = m_modalityY + (_canvas.height - m_modalityY - m_barW) / 2;
        //context.drawImage(imgs[i], x, y)
        context.drawImage(imgs[i], x, y, m_barW, m_barW);
      }
    }

    onPaint: {
		//draw the background
		var context = _canvas.getContext("2d");
        context.clearRect(0, 0, _canvas.width, _canvas.height);

		context.fillStyle = _canvas.bgColor;
		context.fillRect(0, 0, _canvas.width, _canvas.height);

		//draw the content
		context.fillStyle = _canvas.fgColor;
		context.strokeStyle = _canvas.fgColor;
		if(showReference) drawReference();
		if(showCross) drawCross();
		if(showModality) drawModality();
		if(showBar) drawBar();
		if(showAccuracy) drawAccuracy();
    }

    function drawCross() {
      var context = _canvas.getContext("2d");
      let xc  = canvasSize.width / 2;
      let yc = canvasSize.height / 2;	// Window Center
      let crossSize = Math.min(xc, yc) / 4;

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
  
	function drawModality() {
	  var context = _canvas.getContext("2d");
	  let m_showInstruction = dataSource.showInstruction;
	  var modality = dataSource.imgsPaths[(m_showInstruction ? dataSource.modality + 1 : 0)]; // (m_modality + 1) Because the first element is the none instruction
	  
	  let x = canvasSize.width / 2 - canvasSize.width  / 6;
	  let y = canvasSize.height / 2 - canvasSize.height / 6;

	  context.drawImage(modality, x, y, canvasSize.width/3, canvasSize.height/3);
	}

	function drawBar() {
		let vote = dataSource.vote;
		let m_barScales = dataSource.barScales;
		let m_feedbackMode = dataSource.feedbackMode;
		var context = _canvas.getContext("2d");

		if ((m_feedbackMode === 0 && dataSource.modality === vote) || m_feedbackMode === 1) { // Draw Positive or Best Only
			const scale = m_barScales[vote] < 0.0 ? 0.0 : m_barScales[vote];
			const x = (vote + 1) * m_modalityX; // Get the pos X of the bar
			const h = scale * m_barH; // Get the height of the bar
			context.drawImage(dataSource.barImg, x - m_barW / 2, m_modalityY - h, m_barW, h);
		} else if (m_feedbackMode === 2) { // Draw All
			var x = m_modalityX;
			for (var i = 0; i < dataSource.nbModality; ++i) { // For each Modality
			const scale = m_barScales[i] < 0.0 ? 0.0 : m_barScales[i];
			const h = scale * m_barH; // Get the height of the modality
			context.drawImage(dataSource.barImg, x - m_barW / 2, m_modalityY - h, m_barW, h);
			x += m_modalityX;
			}
		}
	}

	function drawAccuracy() {
	  var context = _canvas.getContext("2d");
	  const stepX = 40, stepY = 16;
	  const startX = 8, startY = 16;
	  var x = startX, y = startY;

	  var predictions = 0, good = 0;

	  for (var i = 0; i < dataSource.nbModality; ++i) {
		for (var j = 0; j < dataSource.nbModality; ++j) {
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
	  context.fillText("Acc = " + accuracy, x + (dataSource.nbModality * stepX), y - stepY);
	}
  }

  Connections {
    target: dataSource
    function onimgsPathsChanged() {
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
	  _canvas.showReference = false;
	  _canvas.showCross = false;
	  _canvas.showModality = false;
	  _canvas.showBar = false;
	  _canvas.showAccuracy = false;

      switch (dataSource.state) {
      	case "Idle":					// Start Experiment & Idle
			_canvas.showReference = true;
      		break;
      	case "Cross":				// Start Trial & Cross
			_canvas.showReference = true;
			_canvas.showCross = true;
      		break;
      	case "Instruction":			// Show Instruction
			_canvas.showReference = true;
			_canvas.showModality = true;
      		break;
      	case "Feedback":				// Feedback
			_canvas.showReference = true;
      		if (dataSource.feedbackMode != 3 && dataSource.modality != -1 && dataSource.vote != -1 && !dataSource.delayFeedback) { _canvas.showBar = true; }
      		break;
      	case "Black":				// End Trial
      		if (dataSource.feedbackMode != 3 && dataSource.modality != -1 && dataSource.vote != -1 && dataSource.delayFeedback) {
				_canvas.showReference = true;
				_canvas.showBar = true;
      		}
      		break;
      }
      
	  _canvas.showAccuracy = dataSource.showAccuracy;
	  _canvas.requestPaint();
    }
  }
}