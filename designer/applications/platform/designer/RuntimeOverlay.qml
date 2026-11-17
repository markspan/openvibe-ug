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
 * The dot says which: red while time runs, amber while paused. It takes no focus, lets every
 * click through to what lies under it, and has no taskbar button.
 *
 * It follows the screen of hostWindow, so on a lab setup with the participant's display on a
 * second screen it stays on the experimenter's. ${Designer_RuntimeOverlay} = false turns it off.
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
  readonly property color runningColor: "#E5484D"
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
        color: _root.paused ? _root.pausedColor : _root.runningColor
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
    }
  }
}
