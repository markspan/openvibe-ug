// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtQml

import ".."

Item {
    id: root

    property string text: ""
    property int delay: 500
    property int windowMargin: Theme.spacingXs
    property Item target: parent
    /// UG fork: a longer text wraps at this width, or at the window's, whichever is less. See UG.md, 2.61.
    property real maxTextWidth: 480

    visible: false
    property bool _visible: false

    onVisibleChanged: {
        if (visible & text != "") {
            _timer.start()
        } else {
            _visible = false
            _timer.stop()
        }
    }

    Timer {
        id: _timer
        interval: root.delay
        onTriggered: {
            root._visible = true
        }
    }

    Loader {
        active: root._visible

        sourceComponent: Item {
            id: _tooltip
            parent: Window.window.contentItem
            z: 1000 // always on top
            
            // Position of the component triggering the tooltip in the app window
            property var targetWindowPos: root.target ? 
                root.target.mapToItem(parent, 0, 0) : Qt.point(0, 0)
            
            // Tooltip is above if there is enough space and if space above is bigger than space below
            property bool showAbove: {
                var spaceAbove = targetWindowPos.y
                var spaceBelow = parent.height - (targetWindowPos.y + root.target.height)
                return spaceAbove >= _mainRect.implicitHeight + root.windowMargin &&
                       spaceAbove >= spaceBelow
            }

            Rectangle {
                id: _mainRect
                implicitWidth: _tooltipRect.implicitWidth
                // -1 to avoid empty pixel line between the arrow and the tooltip
                implicitHeight: _tooltipRect.implicitHeight + _tooltipArrow.height - 1

                color: "transparent"

                x: {
                    var preferredX = targetWindowPos.x + root.target.width/2 - implicitWidth/2
                    return Math.max(root.windowMargin,
                                    Math.min(preferredX, _tooltip.parent.width - implicitWidth - root.windowMargin))
                }
                y: {
                    _tooltip.showAbove ?
                        targetWindowPos.y - implicitHeight :
                        targetWindowPos.y + root.target.height
                }
                
                Shape {
                    id: _tooltipArrow
                    //anchors.bottomMargin: Theme.spacingSm
                    anchors.top: {
                        if (!_tooltip.showAbove) {
                            return _mainRect.top
                        }
                    }
                    anchors.bottom: {
                        if (_tooltip.showAbove) {
                            return _mainRect.bottom
                        }
                    }
                    
                    anchors.left: parent.left
                    anchors.leftMargin: {
                        var XcenterDiff = ((_tooltip.targetWindowPos.x + root.target.width/2) - (parent.x + parent.implicitWidth/2))
                        return parent.implicitWidth / 2 + XcenterDiff - width / 2
                    }
                        
                    

                    // rotation
                    rotation: {
                        if (_tooltip.showAbove) {
                            return 180
                        } else {
                            return 0
                        }
                    }
                    width: 24
                    height: 16

                    layer.enabled: true
                    layer.samples: 8

                    ShapePath {
                        fillColor: Theme.tooltipBackground
                        // start bottom left corner
                        startX: 0
                        startY: _tooltipArrow.height

                        // bottom edge left to right
                        PathLine { x: _tooltipArrow.width; y: _tooltipArrow.height }
                        
                        // Right edge to top point area
                        PathLine { x: _tooltipArrow.width/2 + 2; y: 2 }
                        
                        // top point (rounded)
                        PathQuad {
                            x: _tooltipArrow.width/2 - 2
                            y: 2
                            controlX: _tooltipArrow.width/2
                            controlY: 0
                        }
                        
                        // Left edge back down
                        PathLine { x: 0; y: _tooltipArrow.height}
                    }
                }

                Rectangle {
                    id: _tooltipRect
                    anchors.top: {
                       if(_tooltip.showAbove) {
                           return _mainRect.top
                        }
                    }
                    anchors.bottom: {
                        if(!_tooltip.showAbove) {
                            return _mainRect.bottom
                        }
                    }

                    // UG fork: as high as a text of more than one line needs. See UG.md, 2.61.
                    implicitHeight: Math.max(Theme.labelHeight5xl, _tooltipLabel.implicitHeight + 2 * Theme.spacingSm)
                    // UG fork: no wider than maxTextWidth and the window allow; the label wraps within that
                    implicitWidth: Math.min(_tooltipLabel.implicitWidth, root.maxTextWidth,
                                            _tooltip.parent.width - 2 * root.windowMargin - 2 * Theme.spacingSm)
                                   + 2 * Theme.spacingSm // accomodate tooltipLabel content + margins

                    radius: Theme.radiusMd
                    color: Theme.tooltipBackground

                    Label {
                        id: _tooltipLabel
                        // UG fork: centred vertically only; centerIn overrode left and right, so the text could not wrap
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: Theme.spacingSm
                        anchors.rightMargin: Theme.spacingSm

                        text: root.text
                        wrapMode: Text.WordWrap // UG fork
                        color: Theme.white
                        font: Theme.textLargeLight
                        lineHeight: Theme.textLgLineHeight
                        lineHeightMode: Text.FixedHeight
                    }
                }
            }
        }
    }
}