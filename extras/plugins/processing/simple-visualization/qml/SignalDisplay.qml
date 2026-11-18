// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls

import DesignSystem as DS

// The window manager reparents this item and installs width/height bindings on it, so it must not
// anchor itself to its parent.
Item {
    id: _root

    readonly property int ampWidth: dataSource.showAmplitudeRuler ? DS.Theme.spacing5xl : 0
    // UG fork: as wide as the longest channel label, with ChannelGutter's margins, rather than a fixed 96 px, which left
    // short names such as Cz far from the amplitude ruler. Long labels are elided past twice the old width.
    readonly property int gutterWidth: {
        let widest = 0
        for (const channel of dataSource.channels) {
            widest = Math.max(widest, _nameMetrics.advanceWidth(channel.name), _unitMetrics.advanceWidth(channel.unit ?? ""))
        }
        // Multiview's legend puts a coloured dot before each name
        const dot = dataSource.butterfly ? DS.Theme.spacingSm + DS.Theme.spacingXs : 0
        return Math.min(2 * DS.Theme.labelWidthSm, Math.ceil(widest + dot) + DS.Theme.spacingMd)
    }
    readonly property int rulerHeight: dataSource.showTimeRuler ? DS.Theme.labelHeightLg : 0

    // Fit every lane in the viewport by default; above the requested density, scroll instead.
    // Butterfly puts every channel on one shared baseline, so paging has nothing to page through.
    readonly property real contentHeight: {
        const perPage = dataSource.channelsPerPage
        const count = dataSource.visibleChannelCount
        if (!dataSource.butterfly && perPage > 0 && count > perPage) { return _scroll.height * count / perPage }
        return _scroll.height
    }

    // UG fork: the fonts ChannelGutter shows the names and units in, to size the gutter by
    FontMetrics { id: _nameMetrics; font: DS.Theme.textSmallRegular }
    FontMetrics { id: _unitMetrics; font: DS.Theme.textXsRegular }

    // UG fork: the settings panel and its gear button, shared with the LSL display; before, written out here
    SettingsPanel {
        id: _settings

        anchors { top: parent.top; left: parent.left; bottom: parent.bottom }
        parameters: dataSource.parameters
        title: dataSource.name
    }

    Item {
        id: _plot

        anchors { top: parent.top; left: _settings.right; right: parent.right; bottom: parent.bottom }
        anchors.margins: DS.Theme.spacingSm

        Flickable {
            id: _scroll

            anchors { top: parent.top; left: parent.left; right: parent.right; bottom: _time.top }
            contentWidth: width
            contentHeight: _root.contentHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ScrollBar.vertical: ScrollBar { policy: _scroll.contentHeight > _scroll.height ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }

            Item {
                id: _content

                width: _scroll.width
                height: _root.contentHeight

                // UG fork: the trace and what goes around it, shared with the LSL display; before, written out here
                TracePanel {
                    id: _tracePanel

                    anchors.fill: parent
                    // The controller finds the trace by this name to drive it from C++.
                    traceName: "signalTrace"
                    ampWidth: _root.ampWidth
                    gutterWidth: _root.gutterWidth
                    showAmplitudeRuler: dataSource.showAmplitudeRuler
                    butterfly: dataSource.butterfly
                    channels: dataSource.channels
                    stimulations: dataSource.stimulations
                }
            }
        }

        TimeRuler {
            id: _time

            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.leftMargin: _root.ampWidth + _root.gutterWidth
            height: _root.rulerHeight
            visible: dataSource.showTimeRuler
            trace: _tracePanel.trace
        }

        Label {
            anchors.centerIn: parent
            visible: !dataSource.hasData
            text: qsTr("Waiting for data...")
            font: DS.Theme.textMediumRegular
            color: DS.Theme.textTertiary
        }
    }
}
