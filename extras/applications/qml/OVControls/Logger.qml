// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS
import "." as OV

Control {
  id: _control

  property bool collapsed: false
  property real _expandedHeight

  NumberAnimation {
    id: _collapseAnimation
    target: _control
    property: "height"
    duration: 200
    easing.type: Easing.OutQuad
  }

  readonly property var logLevels: [
    { name: "Debug",   key: "debug",   prop: "displayDebug",   countProp: "debugCount",   color: DS.Theme.logLevelsColors.debug },
    { name: "Info",    key: "info",    prop: "displayInfo",    countProp: "infoCount",    color: DS.Theme.logLevelsColors.info },
    { name: "Warning", key: "warning", prop: "displayWarning", countProp: "warningCount", color: DS.Theme.logLevelsColors.warning },
    { name: "Error",   key: "error",   prop: "displayError",   countProp: "errorCount",   color: DS.Theme.logLevelsColors.error },
  ]

  // UG fork: entries can be selected (click; Ctrl+click adds or removes one; Shift+click selects a range) and copied
  // as text (Ctrl+C, or Copy in the right-click menu), e.g. to paste them into a message. See UG.md, 2.56.
  property var _selectedEntries: []  // the selected log items, OVLogControllerItem objects
  property var _selectionAnchor: null // the entry a Shift+click selects from

  function _isSelected(entry) { return _selectedEntries.indexOf(entry) >= 0 }

  function _selectEntry(entry, modifiers) {
    const entries = OV.OVLogController.items
    if ((modifiers & Qt.ShiftModifier) && _selectionAnchor) {
      let from = -1, to = -1
      for (let i = 0; i < entries.length; ++i) {
        if (entries[i] === _selectionAnchor) { from = i }
        if (entries[i] === entry) { to = i }
      }
      if (from >= 0 && to >= 0) {
        const range = (modifiers & Qt.ControlModifier) ? _selectedEntries.slice() : []
        for (let i = Math.min(from, to); i <= Math.max(from, to); ++i) {
          if (range.indexOf(entries[i]) < 0) { range.push(entries[i]) }
        }
        _selectedEntries = range
        return
      }
    }
    if (modifiers & Qt.ControlModifier) {
      _selectedEntries = _isSelected(entry) ? _selectedEntries.filter(e => e !== entry) : _selectedEntries.concat([entry])
    } else {
      _selectedEntries = [entry]
    }
    _selectionAnchor = entry
  }

  function _selectAll() {
    const entries = OV.OVLogController.items
    const all = []
    for (let i = 0; i < entries.length; ++i) { all.push(entries[i]) }
    _selectedEntries = all
  }

  // The entries as text, one per line, in the log's order: "[ Error ] message".
  function _entriesText(onlySelected) {
    const entries = OV.OVLogController.items
    const lines = []
    for (let i = 0; i < entries.length; ++i) {
      const entry = entries[i]
      if (onlySelected && !_isSelected(entry)) { continue }
      lines.push("[ " + entry.level.charAt(0).toUpperCase() + entry.level.slice(1) + " ] " + entry.message.replace(/\s+$/, ""))
    }
    return lines.join("\n")
  }

  function _copy(onlySelected) {
    const text = _entriesText(onlySelected)
    if (text.length > 0) { OV.OVLogController.copyToClipboard(text) }
  }

  Connections {
    target: OV.OVLogController
    // Entries that left the list (cleared, or hidden by the filter) leave the selection.
    function onItemsChanged() {
      if (_control._selectedEntries.length === 0) { return }
      const entries = OV.OVLogController.items
      const shown = []
      for (let i = 0; i < entries.length; ++i) { shown.push(entries[i]) }
      const kept = _control._selectedEntries.filter(e => shown.indexOf(e) >= 0)
      if (kept.length !== _control._selectedEntries.length) { _control._selectedEntries = kept }
      if (shown.indexOf(_control._selectionAnchor) < 0) { _control._selectionAnchor = null }
    }
  }

  implicitHeight: DS.Theme.loggerMinimumHeight
  implicitWidth: parent.width

  padding: 0

  background: Rectangle {
    color: DS.Theme.white
  }

  contentItem: ColumnLayout {
    spacing: 0

    // Header
    Item {
      id: _header
      Layout.fillWidth: true
      Layout.preferredHeight: DS.Theme.loggerHeaderHeight

      DS.HorizontalSpacer {} // Top line of component

      // Header background
      Rectangle {
        anchors.fill: parent
        color: DS.Theme.quaternary
      }

      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: DS.Theme.spacingLg
        anchors.rightMargin: DS.Theme.spacingLg
        spacing: 0

        // Tab bar
        RowLayout {
          id: _tabBar

          property int currentIndex: 0

          Layout.fillHeight: true

          spacing: DS.Theme.spacingLg

          Repeater {
            model: ["Logs"/*, "Terminal"*/]

            delegate: Rectangle {
              id: _tab
              required property int index
              required property var modelData

              Layout.fillHeight: true
              Layout.preferredWidth: _tabLabel.implicitWidth

              color: "transparent"

              Label {
                id: _tabLabel
                anchors.fill: parent
                anchors.topMargin: DS.Theme.spacingXs
                anchors.bottomMargin: DS.Theme.spacingXs

                text: modelData
                font: DS.Theme.textSmallRegular
                color: index === _tabBar.currentIndex ? DS.Theme.textPrimary : DS.Theme.textDisabled
                verticalAlignment: Text.AlignVCenter

                Rectangle {
                  id: _tabUnderline
                  anchors.bottom: parent.bottom

                  height: 1

                  width: parent.width
                  color: DS.Theme.primary

                  transform: Scale {
                    xScale: index === _tabBar.currentIndex ? 1 : 0
                    origin.x: _tabUnderline.width / 2

                    Behavior on xScale {
                      NumberAnimation {
                        duration: 300
                        easing.type: Easing.OutQuad
                      }
                    }
                  }
                }
              }

              MouseArea {
                anchors.fill: parent
                onClicked: {
                  _tabBar.currentIndex = index
                }
              }
            }
          }
        }

        Item { Layout.fillWidth: true }

        // ─── Toolbar Actions ────────────────────────────
        RowLayout {
          spacing: DS.Theme.spacingXl
          DS.IconButton {
            id: clearButton
            iconName: "clear"
            iconSize: 16
            tooltip: "Clear logs"

            onClicked: {
              OV.OVLogController.clear()
            }
          }
          DS.IconButton {
            id: filterButton
            iconName: "filter"
            iconSize: 16
            tooltip: "Filter logs"
            onClicked: {
              _filterPopup.open()
            }
          }

          DS.IconButton {
            id: collapseButton
            iconName: _control.collapsed ? "chevron-up" : "chevron-down"
            iconSize: 16
            tooltip: _control.collapsed ? "Expand" : "Collapse"
            onClicked: {
              if (_control.collapsed) {
                _collapseAnimation.to = _control._expandedHeight
                _collapseAnimation.start()
                _control.collapsed = false
              } else {
                _control._expandedHeight = _control.height
                _collapseAnimation.to = DS.Theme.loggerHeaderHeight
                _collapseAnimation.start()
                _control.collapsed = true
              }
            }
          }
        }
      }
    }

    // ─── Tab Content ────────────────────────────────────────
    StackLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      currentIndex: _tabBar.currentIndex

      // Tab 0: Problems (log entries)
      ListView {
        id: _logListView
        clip: true
        model: OV.OVLogController.items

        spacing: DS.Theme.spacingXs

        // UG fork: Ctrl+C copies the selected entries, Ctrl+A selects all, while the log has the keyboard focus (it
        // takes it on a click). They override Designer's shortcuts, which copy boxes, only then. See UG.md, 2.56.
        Keys.onShortcutOverride: (event) => {
          event.accepted = (event.matches(StandardKey.Copy) && _control._selectedEntries.length > 0) || event.matches(StandardKey.SelectAll)
        }
        Keys.onPressed: (event) => {
          if (event.matches(StandardKey.Copy)) { _control._copy(true); event.accepted = true }
          else if (event.matches(StandardKey.SelectAll)) { _control._selectAll(); event.accepted = true }
        }

        delegate: Item {
          id: _entry
          width: _logListView.width
          height: _entryRow.implicitHeight

          property color _badgeColor: DS.Theme.logLevelsColors[level] ?? DS.Theme.quaternary
          readonly property bool _selected: _control._selectedEntries.indexOf(modelData) >= 0  // UG fork: see above

          // UG fork: the selection's background, and the clicks that select. See UG.md, 2.56.
          Rectangle {
            anchors.fill: parent
            color: DS.Theme.hover
            visible: _entry._selected
          }
          MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onPressed: (mouse) => {
              _logListView.forceActiveFocus()
              if (mouse.button === Qt.RightButton) {
                if (!_entry._selected) { _control._selectEntry(modelData, Qt.NoModifier) }
                _entryMenu.popup()
              } else {
                _control._selectEntry(modelData, mouse.modifiers)
              }
            }
          }

          RowLayout {
            id: _entryRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: DS.Theme.spacingMd
            anchors.rightMargin: DS.Theme.spacingMd
            spacing: DS.Theme.spacingSm

            // Level badge
            Rectangle {
              Layout.preferredWidth: _badgeLabel.implicitWidth + 2 * DS.Theme.spacingSm
              Layout.preferredHeight: _badgeLabel.implicitHeight + 2 * DS.Theme.spacingXs
              Layout.alignment: Qt.AlignVCenter
              radius: DS.Theme.radiusXs
              color: _badgeColor

              Label {
                id: _badgeLabel
                anchors.centerIn: parent
                text: "[ " + level.charAt(0).toUpperCase() + level.slice(1) + " ]"
                font: DS.Theme.textXsBold
                color: DS.Theme.textPrimary
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
              }
            }

            /* @todo Time is already added in the text for runtime logs. Would be nice to move it here for better reading.
            Label {
              Layout.alignment: Qt.AlignVCenter
              text: time
              font: DS.Theme.textXsRegular
              color: DS.Theme.textSecondary
            }*/

            // Message text
            Label {
              Layout.fillWidth: true
              Layout.alignment: Qt.AlignVCenter
              text: message
              font: DS.Theme.textSmallRegular
              color: DS.Theme.textPrimary
              wrapMode: Text.Wrap  // UG fork: upstream cuts a long message off at the panel's edge, and the end of a message says what to do. See UG.md, 2.23.
            }
          }
        }

        ScrollBar.vertical: ScrollBar {
          visible: _logListView.contentHeight > _logListView.height
        }

        onCountChanged: {
            Qt.callLater(positionViewAtEnd)
        }
      }

      // Tab 1: Terminal (placeholder)
      Item {
        // TODO: Implement terminal content
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  /// Resize handle (top edge)
  ////////////////////////////////////////////////////////////////////////////
  Rectangle {
    id: _separator
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.top: parent.top
    height: 1
    color: resizeHandle.pressed ? DS.Theme.primary : DS.Theme.stroke
  }

  MouseArea {
    id: resizeHandle
    anchors.right: parent.right
    anchors.left: parent.left
    anchors.top: parent.top
    property real mouseToEdgeY

    height: 8
    enabled: !_control.collapsed
    cursorShape: _control.collapsed ? Qt.ArrowCursor : Qt.SizeVerCursor

    onPressed: (mouse) => {
        var globalMouse = mapToGlobal(mouse.x, mouse.y)
        mouseToEdgeY = _control.y - globalMouse.y
    }
    onReleased: (mouse) => {
        _separator.color = DS.Theme.stroke
    }

    onPositionChanged: (mouse) => {
        if (pressed) {
            var globalMouse = mapToGlobal(mouse.x, mouse.y)
            var newHeight = _control.height - (globalMouse.y - _control.y + mouseToEdgeY)

            if (newHeight >= DS.Theme.loggerMinimumHeight && newHeight <= DS.Theme.loggerMaximumHeight) {
              _control.height = newHeight
            }
        }
    }
  }

  // UG fork: the right-click menu of an entry. See UG.md, 2.56.
  Menu {
    id: _entryMenu
    MenuItem {
      text: _control._selectedEntries.length > 1 ? "Copy " + _control._selectedEntries.length + " entries (Ctrl+C)" : "Copy entry (Ctrl+C)"
      enabled: _control._selectedEntries.length > 0
      onTriggered: _control._copy(true)
    }
    MenuItem {
      text: "Copy all entries"
      onTriggered: _control._copy(false)
    }
    MenuSeparator {}
    MenuItem {
      text: "Select all (Ctrl+A)"
      onTriggered: _control._selectAll()
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  /// Filter popup
  ////////////////////////////////////////////////////////////////////////////
  DS.Popup {
    id: _filterPopup
    title: "Filter Logs"

    parent: filterButton
    x: -width + filterButton.width
    y: filterButton.height

    ColumnLayout {
      Layout.fillWidth: true

      spacing: DS.Theme.spacingMd

      Repeater {
        model: _control.logLevels

        delegate: Rectangle {
          implicitHeight: _logLevelRow.implicitHeight + DS.Theme.spacingMd
          implicitWidth: _logLevelRow.implicitWidth + 2 * DS.Theme.spacingLg

          Layout.fillWidth: true

          radius: DS.Theme.radiusXs
          color: OV.OVLogController[modelData.prop] ? modelData.color : "transparent"
          border.color: _mouseArea.containsMouse ? modelData.color : "transparent"

          MouseArea {
            id: _mouseArea
            anchors.fill: parent
            onClicked: {
              OV.OVLogController[modelData.prop] = !OV.OVLogController[modelData.prop]
            }

            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
          }

          RowLayout {
            id: _logLevelRow
            anchors.fill: parent
            anchors.centerIn: parent
            anchors.leftMargin: DS.Theme.spacingLg
            anchors.rightMargin: DS.Theme.spacingLg

            spacing: DS.Theme.spacingSm

            DS.Icon {
              Layout.preferredWidth: DS.Theme.iconLg
              Layout.preferredHeight: DS.Theme.iconLg
              name: modelData.key
            }

            Label {
              Layout.fillWidth: true
              text: modelData.name
              font: DS.Theme.textSmallRegular
              color: DS.Theme.textPrimary
            }

            Label {
              text: OV.OVLogController[modelData.countProp]
              font: DS.Theme.textSmallRegular
              color: DS.Theme.textSecondary
            }
          }
        }
      }
    }
  }
}
