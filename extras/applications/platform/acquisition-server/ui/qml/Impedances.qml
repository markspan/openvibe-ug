// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS

Window {
  id: _control

  title: "Impedances Check"
  minimumWidth: 480
  minimumHeight: 320

  property var model: null
  signal finished()

  onClosing: (close) => { finished() }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: DS.Theme.spacingLg
    spacing: 0

    // Channel list
    ListView {
      id: _list
      Layout.fillWidth: true
      Layout.fillHeight: true
      model: _control.model
      clip: true
      headerPositioning: ListView.OverlayHeader

      ScrollBar.vertical: ScrollBar {}

      header: Rectangle {
        width: _list.width
        color: DS.Theme.white
        implicitHeight: _headerRow.implicitHeight + DS.Theme.spacingXs + DS.Theme.borderSm

        RowLayout {
          id: _headerRow
          anchors.left: parent.left
          anchors.right: parent.right
          anchors.top: parent.top
          spacing: 0
          uniformCellSizes: true

          Text {
            Layout.fillWidth: true
            text: "Channel"
            font: DS.Theme.textMediumBold
            color: DS.Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
          }

          Text {
            Layout.fillWidth: true
            text: "Value"
            font: DS.Theme.textMediumBold
            color: DS.Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
          }

          Text {
            Layout.fillWidth: true
            text: "State"
            font: DS.Theme.textMediumBold
            color: DS.Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
          }
        }

        Rectangle {
          anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
          height: DS.Theme.borderSm
          color: DS.Theme.borderPrimary
        }
      }

      delegate: Item {
        id: _row

        required property string name
        required property real value
        required property real fraction
        required property int index

        width: _list.width
        height: DS.Theme.spacing4xl

        readonly property color stateColor: _row.fraction < 0.25 ? DS.Theme.impedanceLevelColors.low
                                          : _row.fraction < 0.5  ? DS.Theme.impedanceLevelColors.medium
                                          : DS.Theme.impedanceLevelColors.high
        readonly property string stateLabel: _row.fraction < 0.25 ? "Strong"
                                           : _row.fraction < 0.5  ? "Moderate"
                                           : "Weak"

        Rectangle {
          anchors.fill: parent
          color: _row.index % 2 === 0 ? DS.Theme.white : DS.Theme.quaternary
        }

        RowLayout {
          anchors.fill: parent
          spacing: 0
          uniformCellSizes: true

          Label {
            Layout.fillWidth: true
            text: _row.name
            font: DS.Theme.textMediumRegular
            color: DS.Theme.textPrimary
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
          }

          Label {
            Layout.fillWidth: true
            text: _row.value.toFixed(1) + " kΩ"
            font: DS.Theme.textMediumRegular
            color: DS.Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
          }

          Rectangle {
            id: _stateRectangle
            Layout.fillWidth: true
            Layout.fillHeight: true

            color: "transparent"

            RowLayout {
              anchors.left: parent.left
              anchors.leftMargin: parent.width / 3
              anchors.verticalCenter: parent.verticalCenter
              spacing: DS.Theme.spacingXs

              Rectangle {
                width: DS.Theme.spacingSm
                height: DS.Theme.spacingSm
                radius: DS.Theme.spacingSm / 2
                color: _row.stateColor
                Layout.alignment: Qt.AlignVCenter
              }

              Text {
                text: _row.stateLabel
                font: DS.Theme.textSmallRegular
                color: _row.stateColor
                Layout.alignment: Qt.AlignVCenter
              }
            }
          }
        }
      }
    }
  }
}
