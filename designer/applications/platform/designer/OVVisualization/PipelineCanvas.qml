// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import OVControls as OV
import OVStyle as OV
import OVVisualization as OV

import DesignSystem as DS

FocusScope {
  id: canvas
  required property var model
  required property var boxes
  required property var links
  required property var scenarioIOConnections
  required property var comments
  property point panOffset: Qt.point(0,0)
  property real zoomLevel: 1.0

  signal requestDeselectAll()

  // pan-dragging state
  property bool panning: false
  property point dragStartPoint: Qt.point(0, 0)
  property point panStartOffset: Qt.point(0, 0)
  property int currentCursor: Qt.ArrowCursor

  // ------ Multi-select state ------
  property var selectedBoxIds: ({})
  property bool ctrlKeyActive: false

  property bool _rubberBanding: false
  property point _rubberBandStart: Qt.point(0, 0)
  property point _rubberBandEnd: Qt.point(0, 0)
  // screen-space press position used only for the drag threshold check
  property point _rubberBandStartScreen: Qt.point(0, 0)
  // drag start positions keyed by identifier, for all boxes being moved
  property var _dragStartPositions: ({})

  onRequestDeselectAll: {
    _deselectBoxes()
  }

  function _deselectBoxes() {
    selectedBoxIds = {}
    window.deselectBox()
  }

  function _selectBoxesInRect(rect) {
    var sel = {}
    for (var i = 0; i < boxesRepeater.count; i++) {
      var b = boxesRepeater.itemAt(i)
      if (!b) continue
      if (b.x < rect.x + rect.width  && b.x + b.width  > rect.x &&
          b.y < rect.y + rect.height && b.y + b.height > rect.y) {
        sel[b.identifier] = true
      }
    }
    selectedBoxIds = sel
  }

  function _startBoxDrag(leaderId, leaderX, leaderY) {
    var starts = {}
    for (var i = 0; i < boxesRepeater.count; i++) {
      var b = boxesRepeater.itemAt(i)
      if (!b) continue
      // move the leader plus any multi-selected peers
      if (b.identifier === leaderId || canvas.selectedBoxIds[b.identifier]) {
        starts[b.identifier] = Qt.point(b.x, b.y)
      }
    }
    _dragStartPositions = starts
  }

  function _updateBoxDrag(leaderId, totalDx, totalDy) {
    for (var i = 0; i < boxesRepeater.count; i++) {
      var b = boxesRepeater.itemAt(i)
      if (!b) continue
      var start = _dragStartPositions[b.identifier]
      if (!start) continue
      b.x = start.x + totalDx
      b.y = start.y + totalDy
    }
  }

  function _endBoxDrag() {
    _dragStartPositions = {}
    canvas.forceActiveFocus()
  }
  // ------ End multi-select state ------

  width: parent.width
  height: parent.height
  focus: true

  // ------ Rubber-band selection (at canvas level so it covers the full screen
  //         regardless of zoom level / pan offset) ------
  MouseArea {
    id: _rubberBandArea
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton
    propagateComposedEvents: true
    z: -1

    onPressed: (mouse) => {
      var cx = (mouse.x - canvas.panOffset.x) / canvas.zoomLevel
      var cy = (mouse.y - canvas.panOffset.y) / canvas.zoomLevel
      canvas._rubberBandStart       = Qt.point(cx, cy)
      canvas._rubberBandEnd         = Qt.point(cx, cy)
      canvas._rubberBandStartScreen = Qt.point(mouse.x, mouse.y)
      canvas._rubberBanding         = false
    }

    onPositionChanged: (mouse) => {
      var cx = (mouse.x - canvas.panOffset.x) / canvas.zoomLevel
      var cy = (mouse.y - canvas.panOffset.y) / canvas.zoomLevel
      // Use screen-pixel distance for the drag threshold so it feels the same
      // at any zoom level.
      var screenDx = mouse.x - canvas._rubberBandStartScreen.x
      var screenDy = mouse.y - canvas._rubberBandStartScreen.y
      if (!canvas._rubberBanding && (Math.abs(screenDx) > 5 || Math.abs(screenDy) > 5)) {
        canvas._rubberBanding = true
      }
      if (canvas._rubberBanding) {
        canvas._rubberBandEnd = Qt.point(cx, cy)
      }
    }

    onReleased: {
      if (canvas._rubberBanding) {
        canvas._selectBoxesInRect(Qt.rect(
          Math.min(canvas._rubberBandStart.x, canvas._rubberBandEnd.x),
          Math.min(canvas._rubberBandStart.y, canvas._rubberBandEnd.y),
          Math.abs(canvas._rubberBandEnd.x - canvas._rubberBandStart.x),
          Math.abs(canvas._rubberBandEnd.y - canvas._rubberBandStart.y)
        ))
      } else {
        // plain tap on empty canvas — deselect everything
        canvas.requestDeselectAll()
      }
      canvas._rubberBanding = false
      canvas.forceActiveFocus()
    }
  }
  // ------ End rubber-band selection ------

  // ------ Start Pan dragging / zoom section ------
  Keys.priority: Keys.BeforeItem
  Keys.onPressed: (event) => {
    const delta = 20  // pixels per arrow press
    if (event.key === Qt.Key_Left) {
      panOffset.x += delta
      event.accepted = true
    } else if (event.key === Qt.Key_Right) {
      panOffset.x -= delta
      event.accepted = true
    } else if (event.key === Qt.Key_Up) {
      panOffset.y += delta
      event.accepted = true
    } else if (event.key === Qt.Key_Down) {
      panOffset.y -= delta
      event.accepted = true
    } else if(event.key === Qt.Key_R) {
      resetView()
      event.accepted = true
    } else if(event.key === Qt.Key_Control) {
      ctrlKeyActive = true
      event.accepted = false
    } else if (event.key === Qt.Key_Escape) {
      canvas.requestDeselectAll()
      event.accepted = true
    } else if (event.key === Qt.Key_Delete) {
      var idsToRemove = []
      for (const box of boxesRepeater.model) {
        if (canvas.selectedBoxIds[box.identifier]) {
          idsToRemove.push(box.identifier)
        }
      }
      for (const i of idsToRemove) {
        canvas.model.modelData.removeBox(i)
      }
      canvas.requestDeselectAll()
      event.accepted = true
    }
  }

  Keys.onReleased: (event) => {
    if(event.key === Qt.Key_Control) {
      ctrlKeyActive = false
    }
  }

  MouseArea {
    id: panArea
    anchors.fill: parent
    acceptedButtons: Qt.MiddleButton
    hoverEnabled: true
    propagateComposedEvents: true
    scrollGestureEnabled: true
    cursorShape: canvas.currentCursor

    onPressed: (mouse) => {
      if ( mouse.button === Qt.MiddleButton ) {
        canvas.currentCursor = Qt.DragMoveCursor
        canvas.panning = true
        canvas.dragStartPoint = Qt.point(mouse.x, mouse.y)
        canvas.panStartOffset = canvas.panOffset
        mouse.accepted = true
      }
    }

    onReleased: (mouse) => {
      if (mouse.button === Qt.MiddleButton && canvas.panning) {
        canvas.currentCursor = Qt.ArrowCursor
        canvas.panning = false
        mouse.accepted = true
        canvas.forceActiveFocus()
      }
    }

    onPositionChanged: (mouse) => {
      if (canvas.panning) {
        let dx = mouse.x - canvas.dragStartPoint.x
        let dy = mouse.y - canvas.dragStartPoint.y
        canvas.panOffset = Qt.point(
          canvas.panStartOffset.x + dx,
          canvas.panStartOffset.y + dy
        )
        mouse.accepted = true
      }
    }
  }

  WheelHandler {
    id: zoomHandler
    target: canvas
    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    property real zoomFactor: 1.15

    onWheel: event => {
      const oldZoomLevel = zoomLevel

      if (event.modifiers & Qt.ControlModifier) {
        if (event.angleDelta.y !== 0) {
          zoomLevel *= event.angleDelta.y > 0 ? zoomFactor : 1 / zoomFactor
        } else if (event.pixelDelta.y !== 0) {
          zoomLevel *= event.pixelDelta.y < 0 ? zoomFactor : 1 / zoomFactor
        }
        event.accepted = true
        zoomLevel = Math.max(0.2, Math.min(3.0, zoomLevel))
        canvas.panOffset.x = event.x - (event.x - canvas.panOffset.x) * (zoomLevel / oldZoomLevel)
        canvas.panOffset.y = event.y - (event.y - canvas.panOffset.y) * (zoomLevel / oldZoomLevel)
      }
    }
  }
  // ------ end Pan dragging / zoom section ------

  Canvas {
    id: _backgroundDottedCanvas
    anchors.fill: parent

    readonly property real dotSpacing: 14
    readonly property real dotRadius: 1.75
    readonly property real dotOpacity: 0.2
    property color backgroundColor: DS.Theme.white
    property color dotColor: DS.Theme.stroke

    Connections {
      target: canvas
      function onPanOffsetChanged() { _backgroundDottedCanvas.requestPaint() }
      function onZoomLevelChanged() { _backgroundDottedCanvas.requestPaint() }
    }

    onPaint: {
      var ctx = getContext("2d")
      var spacing = dotSpacing * canvas.zoomLevel
      var offsetX = ((canvas.panOffset.x % spacing) + spacing) % spacing
      var offsetY = ((canvas.panOffset.y % spacing) + spacing) % spacing

      ctx.globalAlpha = 1.0
      ctx.fillStyle = backgroundColor
      ctx.fillRect(0, 0, width, height)

      if (zoomLevel < 0.75) {
        return
      }

      ctx.globalAlpha = dotOpacity
      ctx.fillStyle = dotColor
      for (var x = offsetX - spacing; x < width + spacing; x += spacing) {
        for (var y = offsetY - spacing; y < height + spacing; y += spacing) {
          ctx.beginPath()
          ctx.arc(x, y, dotRadius, 0, Math.PI * 2)
          ctx.fill()
        }
      }
    }
  }

  Item {
    id: canvasView
    width: parent.width
    height: parent.height
    transform: [
      Scale { xScale: zoomLevel; yScale: zoomLevel; },
      Translate { x: panOffset.x; y: panOffset.y; }
    ]

    //this MouseArea needs to be declared before the Repeater to be below the MouseArea in the elements!
    HoverHandler {
      id: _hoverHandlerLink
      //margin: 15
      enabled: false
      cursorShape: canvas.currentCursor

      onPointChanged: {
        let pos = point.position
        //if position is outside the window, stop link creation
        if(pos.x < margin || pos.y < margin || pos.x > parent.width - margin || pos.y > parent.height - margin){
          canvas.model.modelData.stopLinkCreation()
          return
        }

        _newLink.targetPosition = Qt.point(pos.x, pos.y)
      }

      onHoveredChanged: {
        if (!hovered) {
          let pos = point.position
          if (pos.x < margin || pos.y < margin || pos.x > parent.width - margin || pos.y > parent.height - margin) {
            canvas.model.modelData.stopLinkCreation()
          }
        }
      }
    }

    TapHandler {
      id: _tapHandlerLink
      enabled: false
      onTapped: {
        canvas.model.modelData.stopLinkCreation()
      }
    }

    MouseArea {
      id: _mouseAreaComment
      property int margin: 15
      enabled: false
      anchors.fill: parent
      cursorShape: enabled ? Qt.PointingHand : canvas.currentCursor
      hoverEnabled: true

      onExited: {
        //if position is outside the window, stop comment creation
        // if the mouse is too fast, the positionChanged event may not triggered
        // we also want to filter out the case where the mouse is going over a box hence the check on mouse position
        if(mouseX < margin || mouseY < margin || mouseX > parent.width - margin || mouseY > parent.height - margin){
          _mouseAreaComment.enabled = false
        }
      }

      onClicked: (event) => {
        _newCommentDialog.commentCenter = Qt.point(event.x, event.y)
        _newCommentDialog.show()
        _mouseAreaComment.enabled = false
      }
    }

    Rectangle {
      id: _rubberBand
      visible: canvas._rubberBanding
      x: Math.min(canvas._rubberBandStart.x, canvas._rubberBandEnd.x)
      y: Math.min(canvas._rubberBandStart.y, canvas._rubberBandEnd.y)
      width:  Math.abs(canvas._rubberBandEnd.x - canvas._rubberBandStart.x)
      height: Math.abs(canvas._rubberBandEnd.y - canvas._rubberBandStart.y)
      color: DS.Theme.primary
      opacity: 0.08
      border.color: DS.Theme.primary
      border.width: 1 / canvas.zoomLevel
      z: 100
    }
    // ------ End rubber-band selection ------

    Repeater {
      id: boxesRepeater
      model: boxes
      delegate: OV.Box {
        isMultiSelected: !!canvas.selectedBoxIds[identifier]

        DragHandler {
          id: _boxDragHandler
          target: null

          onActiveChanged: {
            if (active) {
              canvas._startBoxDrag(parent.identifier, parent.x, parent.y)
            } else {
              canvas._endBoxDrag()
            }
          }

          onTranslationChanged: {
            if (active) {
              canvas._updateBoxDrag(parent.identifier,
                                    translation.x / canvas.zoomLevel,
                                    translation.y / canvas.zoomLevel)
            }
          }
        }

        onBoxSelected: (modelData) => {
          if (ctrlKeyActive) {
            // Force mutation to trigger rendering
            canvas.selectedBoxIds = Object.assign({}, canvas.selectedBoxIds, { [identifier]: true })
          } else {
            canvas.selectedBoxIds = { [identifier]: true }
          }
          if (Object.keys(canvas.selectedBoxIds).length === 1) {
            window.selectedBox(modelData)
          } else {
            window.deselectBox()
          }
        }
        onClickPortCallback : (portIdx, boxId, x, y, isOutput)=> {
          canvas.model.modelData.clickPort(portIdx, boxId, x, y, isOutput)
        }
        onContextMenuRequested: (portIdx, boxId, dataType, x, y, isOutput)=> {
          _scenarioIOConnectPopup.x = x
          _scenarioIOConnectPopup.y = y
          _scenarioIOConnectPopup.isOutput = isOutput
          _scenarioIOConnectPopup.boxId = boxId
          _scenarioIOConnectPopup.portIdx = portIdx
          _scenarioIOConnectPopup.boxPortDataType = dataType
          _scenarioIOConnectPopup.open()
        }
      }
    }

    Repeater {
      model: links
      delegate: OV.Link {
        required property int index
        onRemovedLink : {
          canvas.model.modelData.removeLink(index)
        }
        onActiveFocusChanged: (focus) => {
          if (focus) {
            canvas._deselectBoxes()
          }
        }
        Connections {
          target: canvas
          function onRequestDeselectAll() { focus = false }
        }
      }
    }

    Repeater {
      model: scenarioIOConnections
      delegate: OV.ScenarioIOConnection {
        onRemovedConnection : {
          canvas.model.modelData.removeScenarioIOConnection(scenarioPortIdx, targetBoxId, targetPortIdx, isOutput);
        }
        onActiveFocusChanged: (focus) => {
          if (focus) {
            canvas._deselectBoxes()
          }
        }
        Connections {
          target: canvas
          function onRequestDeselectAll() { focus = false }
        }
      }
    }

    Repeater {
      model: comments
      delegate: OV.Comment {
        onRemoved : {
          canvas.model.modelData.removeComment(index)
        }
      }
    }

    // to display the link being created
    OV.Link {
      id: _newLink
      sourcePosition: Qt.point(0, 0)
      targetPosition: Qt.point(0, 0)
      sourceDataType: ""
      isLinked: false
      visible: false
    }

    OV.CommentEditDialog {
      id: _newCommentDialog
      property point commentCenter: Qt.point(50,50)

      onAccepted: {
        canvas.model.modelData.addComment(text, commentCenter)
      }
    }
  } // end canvasView

  DropArea {
    id: _dropArea
    width: parent.width
    height: parent.height
    onDropped: drop => {
      if(drop.hasText) {
        let xPosition = (drop.x - panOffset.x)/zoomLevel
        let yPosition = (drop.y - panOffset.y)/zoomLevel
        model.modelData.addBox(drop.text, Qt.point(xPosition, yPosition))
      }
    }
  }

  Connections {
    target: canvas.model.modelData
    function onStartLinkCreation(x: real, y: real){
      _hoverHandlerLink.enabled = true
      _tapHandlerLink.enabled = true
      _newLink.sourcePosition = Qt.point(x, y)
      _newLink.targetPosition = Qt.point(x, y)
      _newLink.visible = true

    }
  }

  Connections {
    target: canvas.model.modelData
    function onFinishLinkCreation(){
      _hoverHandlerLink.enabled = false
      _tapHandlerLink.enabled = false
      _newLink.visible = false
    }
  }

  Connections {
    target: canvas.model.modelData
    function onNewComment() {
      // 1. pick origin
      // 2. open comment dialog to set text
      // 3. ho le zoli comment
      _mouseAreaComment.enabled = true
    }
  }

  function resetView() {
    if (boxes.length === 0) {
      canvas.panOffset = Qt.point(0,0)
      canvas.zoomLevel = 1.0
      return
    }

    // Bounding box from centers only
    var minX = boxes[0].center.x, maxX = boxes[0].center.x
    var minY = boxes[0].center.y, maxY = boxes[0].center.y
    for (var i = 1; i < boxes.length; ++i) {
      let c = boxes[i].center
      minX = Math.min(minX, c.x); maxX = Math.max(maxX, c.x)
      minY = Math.min(minY, c.y); maxY = Math.max(maxY, c.y)
    }

    // Add some margins around the bounding box
    const margin = DS.Theme.spacing7xl
    minX -= margin; maxX += margin
    minY -= margin; maxY += margin

    let zoomX = (canvas.width - DS.Theme.toolBoxWidth) / (maxX - minX)
    let zoomY = canvas.height / (maxY - minY)
    canvas.zoomLevel = Math.max(0.2, Math.min(1.0, Math.min(zoomX, zoomY)))

    let contentCenter = Qt.point((minX + maxX) / 2, (minY + maxY) / 2)
    let canvasCenter = Qt.point((canvas.width - DS.Theme.toolBoxWidth) / 2, canvas.height / 2)
    canvas.panOffset = Qt.point(
      canvasCenter.x - contentCenter.x * canvas.zoomLevel,
      canvasCenter.y - contentCenter.y * canvas.zoomLevel
    )
  }

  onVisibleChanged: {
    if (visible) canvas.forceActiveFocus()
  }

  Component.onCompleted: {
    resetView()
  }

  DS.Popup {
    id: _scenarioIOConnectPopup

    property var boxId: null
    property var portIdx: null
    property var boxPortDataType: null
    property bool isOutput: false

    title: "Connect to scenario IO"
    titleFont: DS.Theme.textSmallRegular
      
    onOpened: {
      var ports = _scenarioIOConnectPopup.isOutput ? canvas.model.modelData.outputPorts : canvas.model.modelData.inputPorts
      _ioRepeater.model = ports.filter((port) => port.dataType === boxPortDataType)
    }

    onClosed: {
      _scenarioIOConnectPopup.boxId = null
      _scenarioIOConnectPopup.portIdx = null
      _ioRepeater.model = null
    }

    spacing: DS.Theme.spacingMd

    Label {
      Layout.fillWidth: true
      Layout.maximumWidth: _scenarioIOConnectPopup.title.length * DS.Theme.textSmallRegular.pixelSize
      wrapMode: Text.Wrap
      Layout.alignment: Qt.AlignHCenter
      text: "No Scenario IO available for this port data type"
      visible: _ioRepeater.model === undefined || _ioRepeater.model?.length === 0
      font: DS.Theme.textMediumRegular
      color: DS.Theme.textPrimary
    }

    Repeater {
      id: _ioRepeater

      visible: model?.length > 0

      delegate: Rectangle {
        required property int index
        required property var modelData

        Layout.fillWidth: true
        implicitHeight: text.implicitHeight + 2 * DS.Theme.spacingSm
        
        radius: DS.Theme.radiusSm
        color: _ioHoverHandler.hovered ? DS.Theme.hover : "transparent"
        
        Text {
          id: text
          anchors.left: parent.left
          anchors.verticalCenter: parent.verticalCenter
          anchors.topMargin: DS.Theme.spacingSm
          anchors.bottomMargin: DS.Theme.spacingSm
          anchors.leftMargin: DS.Theme.spacingSm

          text: modelData.index + 1 + ": " + modelData.name
          font: DS.Theme.textMediumRegular
          color: DS.Theme.textPrimary
        }

        TapHandler {
          onTapped: {
            if (_scenarioIOConnectPopup.isOutput) {
              canvas.model.modelData.setScenarioOutputLink(modelData.index, _scenarioIOConnectPopup.boxId, _scenarioIOConnectPopup.portIdx)
            } else {
              canvas.model.modelData.setScenarioInputLink(modelData.index, _scenarioIOConnectPopup.boxId, _scenarioIOConnectPopup.portIdx)
            }
            _scenarioIOConnectPopup.close()
          }
        }
        HoverHandler {
          id: _ioHoverHandler
          cursorShape: Qt.PointingHandCursor
        }
      }
    }
  }
}
