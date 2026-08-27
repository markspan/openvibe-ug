import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QtCore

import OVControls as OV
import OVParameters as OV
import DesignSystem as DS

import AcquisitionServer as OV

//window containing the application
ApplicationWindow {
  id: window
  visible: true
  minimumWidth: 800
  minimumHeight: 600

  //title of the application
  title: qsTr("Openvibe Acquisition Server")

  OV.AcquisitionController {
    id: controller
  }

  OV.Parameters {
    id: _device_param
    parameters: controller.deviceParameters
  }

  OV.Parameters {
    id: _driver_param
    parameters: controller.driverParameters
  }

  ColumnLayout {
    id: _mainColumnLayout
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter

    anchors.topMargin: DS.Theme.spacingMd
    anchors.bottomMargin: DS.Theme.spacing4xl

    width: Math.min(parent.width - 2 * DS.Theme.spacing4xl, DS.Theme.acquisitionServerContentMaximumWidth)

    spacing: DS.Theme.spacingSm
  // properties stack
    RowLayout {
      id: _mainTabBar
      Layout.fillWidth: true
      //Layout.leftMargin: DS.Theme.spacingLg

      property int currentIndex: 0

      Repeater {
        model: ["Acquisition", "Preferences"]

        delegate: Rectangle {
          id: _mainTab
          required property int index
          required property var modelData

          Layout.preferredWidth: _mainTabRow.implicitWidth + 2 * DS.Theme.spacingMd
          Layout.preferredHeight: _mainTabRow.implicitHeight + DS.Theme.spacingMd

          RowLayout {
            id: _mainTabRow
            anchors.fill: parent
            anchors.leftMargin: DS.Theme.spacingMd
            anchors.rightMargin: DS.Theme.spacingMd

            Label {
              id: _tabLabel
              Layout.fillHeight: true
              Layout.alignment: Qt.AlignVCenter
              text: modelData
              font: DS.Theme.textMediumMedium
              color: index === _mainTabBar.currentIndex ? DS.Theme.textPrimary : DS.Theme.textDisabled
              verticalAlignment: Text.AlignVCenter

              Rectangle {
                id: _mainTabUnderline
                anchors.bottom: parent.bottom

                height: 1

                width: parent.width
                color: DS.Theme.primary

                transform: Scale {
                  xScale: index === _mainTabBar.currentIndex ? 1 : 0
                  origin.x: _mainTabUnderline.width / 2

                  Behavior on xScale {
                    NumberAnimation {
                      duration: 300
                      easing.type: Easing.OutQuad
                    }
                  }
                }
              }
            }
          }

          MouseArea {
            anchors.fill: parent
            onClicked: {
              _mainTabBar.currentIndex = index
            }

            cursorShape: Qt.PointingHandCursor
          }

        }
      }
    }

    StackLayout {
      id: _mainStack
      Layout.fillHeight: true
      Layout.fillWidth: true

      currentIndex: _mainTabBar.currentIndex

      // Acquisition stack tab
      ColumnLayout {
        id: acquisitionLayout
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: DS.Theme.spacingMd
        Layout.bottomMargin: DS.Theme.spacing4xl

        spacing: DS.Theme.spacingSm

        // Driver selection
        ColumnLayout {
          id: _driverSelectionLayout
          enabled: !controller.isAcquiring
          Layout.fillWidth: true
          //Layout.margins: DS.Theme.spacingMd
          Layout.alignment: Qt.AlignTop

          spacing: DS.Theme.spacingLg

          DS.ComboBox {
            id: _driverSelectionComboBox

            Layout.fillWidth: true
            model: controller.drivers
            centerText: true

            //show the driver the controller actually holds, which is the one restored from the
            //previous run, rather than defaulting to the first of the list
            currentIndex: Math.max(0, controller.drivers.indexOf(controller.currentDriver))

            onCurrentTextChanged: {
              if (currentText === controller.currentDriver) {
                return
              }
              controller.setCurrentDriver(currentText)
              _driver_param.updateParametersModel()
              _device_param.updateParametersModel()
            }
          }

          RowLayout {
            Layout.fillWidth: true
            spacing: DS.Theme.spacing2xl

            OV.ListParameter {
              id: _sampleCountPerBlock
              Layout.fillWidth: true

              displayInline: true
              param: controller.sampleCountPerBlock

            }
          }
        }

        OV.DriftControl {
              Layout.fillWidth: true
              instantDrift: controller.instantDrift
              globalDrift: controller.globalDrift
              driftLimit: controller.driftLimit
        }

        DS.HorizontalSpacer {
        // rightMargin: DS.Theme.spacing7xl
        //  leftMargin: DS.Theme.spacing7xl
        }

        // properties stack
        RowLayout {
          id: _acquisitionTabBar
          Layout.fillWidth: true
          Layout.leftMargin: DS.Theme.spacingLg

          property int currentIndex: 0

          Repeater {
            model: ["Driver properties", "Plugins"]

            delegate: Rectangle {
              id: _tab
              required property int index
              required property var modelData

              Layout.preferredWidth: _tabRow.implicitWidth + 2 * DS.Theme.spacingMd
              Layout.preferredHeight: _tabRow.implicitHeight + DS.Theme.spacingMd

              RowLayout {
                id: _tabRow
                anchors.fill: parent
                anchors.leftMargin: DS.Theme.spacingMd
                anchors.rightMargin: DS.Theme.spacingMd

                Label {
                  id: _tabLabel
                  Layout.fillHeight: true
                  Layout.alignment: Qt.AlignVCenter
                  text: modelData
                  font: DS.Theme.textMediumMedium
                  color: index === _acquisitionTabBar.currentIndex ? DS.Theme.textPrimary : DS.Theme.textDisabled
                  verticalAlignment: Text.AlignVCenter

                  Rectangle {
                    id: _tabUnderline
                    anchors.bottom: parent.bottom

                    height: 1

                    width: parent.width
                    color: DS.Theme.primary

                    transform: Scale {
                      xScale: index === _acquisitionTabBar.currentIndex ? 1 : 0
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
              }

              MouseArea {
                anchors.fill: parent
                onClicked: {
                  _acquisitionTabBar.currentIndex = index
                }

                cursorShape: Qt.PointingHandCursor
              }

            }
          }
        }

        StackLayout {
          id: _acquisitionSettingsStack
          enabled: !controller.isAcquiring
          Layout.fillHeight: true

          currentIndex: _acquisitionTabBar.currentIndex

          // devices/drivers params
          ColumnLayout {
            id: _propertiesColumnLayout

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: DS.Theme.acquisitionServerParametersColumnWidth
            Layout.alignment: Qt.AlignHCenter

            ScrollView {
              Layout.fillWidth: true
              Layout.fillHeight: true
              Layout.maximumHeight: _deviceColumnLayout.implicitHeight
              contentWidth: availableWidth
              contentHeight: _deviceColumnLayout.implicitHeight

              ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
              ScrollBar.vertical.policy: ScrollBar.AlwaysOff

              clip: true

              ColumnLayout {
                id: _deviceColumnLayout
                width: parent.width

                spacing: DS.Theme.spacingMd

                Repeater {

                  model: _device_param.params_model

                  delegate: Loader {
                    required property var param;
                    required property var component;
                    property var lparam: param

                    Layout.fillWidth: true
                    sourceComponent: component
                  }
                }
                Repeater {

                  model: _driver_param.params_model

                  delegate: Loader {
                    required property var param;
                    required property var component;
                    property var lparam: param

                    Layout.fillWidth: true
                    sourceComponent: component
                  }
                }

                OV.ChannelRename {
                  id: _channelRename
                  Layout.fillWidth: true
                  channelNames: controller.channelNames

                  onNameChanged: (index, name) => controller.setChannelName(index, name)
                  onAllNamesChanged: (channelNames) => controller.setChannelNames(channelNames)
                }
              }
            }

            Item { Layout.fillHeight: true }
          }

          // Preferences
          ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumWidth: DS.Theme.acquisitionServerParametersColumnWidth
            Layout.alignment: Qt.AlignHCenter

            contentWidth: availableWidth
            contentHeight: _pluginsColumnLayout.implicitHeight

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff

            ColumnLayout {
              id: _pluginsColumnLayout

              spacing: DS.Theme.spacingMd
              width: parent.width

              Repeater {
                width: parent.width
                model: controller.configParameters

                delegate: ColumnLayout {
                  width: parent.width
                  spacing: DS.Theme.spacingXs

                  Label {
                    text: modelData.name
                    font: DS.Theme.subtitle3
                  }

                  OV.Parameters {
                    id: writerParams
                    displayInline: true
                    parameters: modelData.parameters
                  }

                  Repeater {
                    model: writerParams.params_model
                    delegate: Loader {
                      required property var param
                      required property var component
                      property var lparam: param
                      Layout.fillWidth: true
                      sourceComponent: component
                    }
                  }
                }
              }
            }
          }
        }

        // action bar
        OV.ActionBar {
          Layout.alignment: Qt.AlignHCenter
        }
      }

      // Preferences stack tab
      ColumnLayout {
        id: _preferencesColumnLayout
        enabled: !controller.isAcquiring
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.topMargin: DS.Theme.spacingMd
        Layout.bottomMargin: DS.Theme.spacing4xl

        spacing: DS.Theme.spacingXs

        OV.NumericParameter {
          id: _connectionPort
          Layout.fillWidth: true
          param: controller.connectionPort
        }

        OV.NumericParameter {
          id: _tcpTaggingPort
          Layout.fillWidth: true
          param: controller.tcpTaggingPort
        }
      }
    }
  }

  OV.Impedances {
    id: _impedancesWindow
    visible: controller.isCheckingImpedance
    model: controller.impedances
    onFinished: controller.toggleImpedanceCheck()
  }

  Component.onCompleted: {
  }
}
