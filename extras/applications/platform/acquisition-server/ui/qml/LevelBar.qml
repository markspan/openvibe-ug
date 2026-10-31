// UG fork: new file, not in upstream OpenViBE.
//
// A bar from 0 to 1 that turns orange from `warning` and red from `danger`; it replaces upstream's
// two-sided DriftBar, now that the server shows buffer levels instead of drift.

import QtQuick
import QtQuick.Controls.Basic

import DesignSystem as DS

Control {
    id: _control

    /// How full the bar is, from 0 to 1; values outside are clamped
    required property real value
    /// Where the bar turns orange
    property real warning: 0.5
    /// Where the bar turns red
    property real danger: 0.8

    readonly property real _clamped: Math.max(0, Math.min(1, value))

    background: Rectangle {
        color: DS.Theme.quaternary
        radius: DS.Theme.radiusSm
    }

    contentItem: Item {
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: _control._clamped * parent.width
            radius: DS.Theme.radiusSm
            color: _control._clamped >= _control.danger ? DS.Theme.cpuUsageColors["high"]
                 : _control._clamped >= _control.warning ? DS.Theme.cpuUsageColors["medium"]
                 : DS.Theme.cpuUsageColors["low"]
        }
    }
}
