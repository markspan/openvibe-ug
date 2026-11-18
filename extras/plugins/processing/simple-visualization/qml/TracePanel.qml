// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.41.
//
// One display's traces with what goes around them: the amplitude ruler, the channel names, the sweep cursor and the
// markers. Shared by SignalDisplay.qml and LSLDisplay.qml (one per stream there), so that both draw alike.

import QtQuick
import QtQuick.Window  // Screen, for the trace's layer

import DesignSystem as DS

Item {
    id: _panel

    /// The trace: what the time ruler follows, and what the display's controller drives
    readonly property alias trace: _trace
    /// The trace's object name, for a controller that finds it by name
    property alias traceName: _trace.objectName

    property int ampWidth: 0
    property int gutterWidth: 0
    property bool showAmplitudeRuler: true
    property bool butterfly: false
    /// As ChannelGutter takes them
    property var channels: []
    /// As StimulationOverlay takes them
    property var stimulations: []

    AmplitudeScaleBar {
        id: _amp

        anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
        width: _panel.ampWidth
        visible: _panel.showAmplitudeRuler
        trace: _trace
        butterfly: _panel.butterfly
    }

    ChannelGutter {
        id: _gutter

        anchors { top: parent.top; left: _amp.right; bottom: parent.bottom }
        width: _panel.gutterWidth
        trace: _trace
        channels: _panel.channels
        butterfly: _panel.butterfly
    }

    SignalTrace {
        id: _trace

        anchors { top: parent.top; left: _gutter.right; right: parent.right; bottom: parent.bottom }
        clip: true
        traceColor: DS.Theme.textPrimary

        // Antialiased lines. The traces are drawn into a multisampled layer, which smooths the lines the scene graph draws
        // without it. A layer is a texture, so it is left out when the trace is wider or, scrolling, taller than graphics
        // cards are sure to allow; the lines are then drawn as before.
        layer.enabled: Math.max(width, height) * Screen.devicePixelRatio <= 8192
        layer.samples: 4
        layer.smooth: true
    }

    // Sweep cursor, Scan mode only
    Rectangle {
        visible: _trace.cursorX >= 0
        x: _trace.x + _trace.cursorX
        y: _trace.y
        width: DS.Theme.borderMd
        height: _trace.height
        color: DS.Theme.primary
        opacity: 0.7
    }

    StimulationOverlay {
        anchors.fill: _trace
        trace: _trace
        stimulations: _panel.stimulations
    }
}
