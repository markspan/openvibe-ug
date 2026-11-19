// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.33.
//
// The run time of the current scenario, in a small window above all other windows, at the top
// left of the screen Designer is on.

import QtQuick
import QtQuick.Window

import DesignSystem as DS

/**
 * A frameless window that floats above every other window while a scenario runs, also above the
 * visualisation windows and other programs, and shows the run time that the action bar shows.
 *
 * It is shown while the player plays, is paused, steps or fast-forwards, and hidden when it stops.
 * The dot says which: black while time runs, red while a box of the scenario records (a red dot reads as recording),
 * amber while paused. It takes no focus, lets every click through to what lies under it, and has no taskbar button.
 *
 * It follows the screen of hostWindow, so on a lab setup with the participant's display on a
 * second screen it stays on the experimenter's. ${Designer_RuntimeOverlay} = false turns it off.
 *
 * While a box of the scenario writes a recording (the BrainVision writer (UG), see RecordingStatus.hpp), a square after
 * the time says so: "REC" on red, or on amber while paused, as nothing is written then.
 *
 * It raises itself every second while shown, so that windows which also stay on top do not hide it.
 */
Window {
  id: _root

  /// The window whose screen the overlay goes to, normally Designer's main window
  required property Window hostWindow

  /// Distance from the corner of the screen, in device independent pixels
  readonly property int cornerMargin: DS.Theme.spacingLg

  /// "Stop" when no scenario runs, "Invalid Status" when no scenario is open at all
  readonly property bool scenarioRunning: OVDesigner.playerStatus !== "Stop" && OVDesigner.playerStatus !== "Invalid Status"
  readonly property bool paused: OVDesigner.playerStatus === "Pause"

  // Fixed colours rather than theme colours: the overlay sits over any window, light or dark,
  // and must stay readable on all of them.
  readonly property color backgroundColor: Qt.rgba(0.08, 0.08, 0.10, 0.80)
  readonly property color textColor: "#FFFFFF"
  readonly property color runningColor: "#000000"
  readonly property color recordingColor: "#E5484D"
  readonly property color pausedColor: "#F5A524"

  visible: OVDesigner.runtimeOverlayEnabled && scenarioRunning

  // Tool: no taskbar button. TransparentForInput and DoesNotAcceptFocus: clicks and keys go to the
  // windows below, so the overlay never steals focus from a running stimulus or the experimenter.
  flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
         | Qt.WindowTransparentForInput | Qt.WindowDoesNotAcceptFocus
  // Not tied to Designer's window, so the overlay stays visible when Designer is minimised
  transientParent: null
  color: "transparent"
  title: qsTr("OpenViBE run time")

  // Positions on the virtual desktop: the top left corner of the host window's screen
  x: (hostWindow.screen?.virtualX ?? 0) + cornerMargin
  y: (hostWindow.screen?.virtualY ?? 0) + cornerMargin
  width: _panel.width
  height: _panel.height

  // WindowStaysOnTopHint puts the overlay among the windows that stay on top, but a window that asks
  // for the same later, a full screen stimulus window for one, goes above it. So it raises itself
  // again every second while shown; raising takes no focus.
  Timer {
    interval: 1000
    repeat: true
    triggeredOnStart: true
    running: _root.visible
    onTriggered: _root.raise()
  }

  Rectangle {
    id: _panel

    width: _content.implicitWidth + 2 * DS.Theme.spacingMd
    height: _content.implicitHeight + 2 * DS.Theme.spacingSm
    radius: DS.Theme.radiusSm
    color: _root.backgroundColor

    Row {
      id: _content
      anchors.centerIn: parent
      spacing: DS.Theme.spacingSm

      Rectangle {
        id: _statusDot
        anchors.verticalCenter: parent.verticalCenter
        width: DS.Theme.spacingMd
        height: width
        radius: width / 2
        color: _root.paused ? _root.pausedColor : OVDesigner.recording ? _root.recordingColor : _root.runningColor
        // A ring, so that the black dot shows on the dark panel
        border.width: color === _root.runningColor ? DS.Theme.borderMd : 0
        border.color: Qt.rgba(1, 1, 1, 0.7)
      }

      // Seconds and milliseconds side by side, without the spacing that separates them from the dot
      Row {
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        Text {
          id: _timeText
          text: OVDesigner.simulatedTime + "."
          font.family: "Cascadia Mono" // Monospaced, as in the action bar: the width does not change with the digits
          font.weight: Font.Medium
          font.pixelSize: DS.Theme.textLargeMedium.pixelSize
          color: _root.textColor
        }

        Text {
          text: OVDesigner.simulatedTimeMs
          font: _timeText.font
          color: _root.textColor
          opacity: 0.6
        }
      }

      // The size of the square: the width of a "Recording" label in small bold type, with its margins
      TextMetrics {
        id: _labelMetrics
        text: "Recording"
        font.weight: Font.Bold
        font.pixelSize: DS.Theme.textSmallRegular.pixelSize
      }

      // While a box writes a recording, a square in the colour of the dot: red, amber while paused
      Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        visible: OVDesigner.recording
        width: Math.ceil(_labelMetrics.advanceWidth) + 2 * DS.Theme.spacingSm
        height: width
        radius: DS.Theme.radiusSm
        color: _statusDot.color

        Text {
          anchors.centerIn: parent
          text: "REC"
          font.weight: Font.Bold
          font.pixelSize: Math.round(parent.width * 0.3)
          // Dark on amber, which white would not stand out on
          color: _root.paused ? "#1A1A1A" : _root.textColor
        }
      }
    }
  }
}
