import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import QtCore

import OVASInterface as OV
import OVControls as OV
import OVParameters as OV
import OVStyle as OV 
import "." as OV

//window containing the application
ApplicationWindow {
  id: window
  visible: true
  minimumWidth: _config_layout.Layout.minimumWidth + _buttons_layout.Layout.minimumWidth + OV.Style.smallColumnSpacing
  minimumHeight: 600

  //title of the application
  title: qsTr("Openvibe Acquisition Manager")

  palette {
    window: OV.Style.colors.bgColor
    windowText: OV.Style.colors.textColorNeutral
    base: OV.Style.colors.fgColor
    alternateBase: OV.Style.colors.alternateFgColor
    //toolTipBase: OV.Style.colors.
    //toolTipText: OV.Style.colors.
    placeholderText: OV.Style.colors.textColorFaded
    text: OV.Style.colors.textColorBase
    button: OV.Style.colors.baseColor
    buttonText: OV.Style.colors.textColorBase
    brightText: OV.Style.colors.textColorDarkNeutral

    disabled {

    }

    inactive {

    }
  }

  Settings {
    id: stt
    property alias x: window.x
    property alias y: window.y
    property alias width: window.width
    property alias height: window.height
    property int mode: 1
  }


  MouseArea {
    anchors.fill: parent
    onClicked: forceActiveFocus()
  }

  OV.OVMaster {
    id: master

    onImpedanceRequested: {
      master.connectDevice()
      _container.currentIndex = 2
    }
  }

  OV.Parameters {
    id: configParams
    parameters: master.configParameters
  }

  OV.Parameters {
    id: _device_param
    parameters: master.deviceParameters
  }

  OV.Parameters {
    id: _driver_param
    parameters: master.driverParameters
  }

  OV.Settgs {
    id: settgs
    configParams: configParams.params_model
  }

  OV.ChannelRename {
    id: channelRename
    channelNames: []
    onAccepted: {
      master.setChannelNames(channelNames)
    }
    onRejected: {
      // Reset channel names.
      channelRename.channelNames = []
    }
  }

  //menu containing two menu items
  menuBar: MenuBar {
    Menu {
      title: qsTr("File")
      MenuItem {
        text: qsTr("&Open")
        onTriggered: console.log("Open action triggered");
      }
      MenuItem {
        text: qsTr("Exit")
        onTriggered: Qt.quit();
      }
    }
  }

  StackLayout { 
    id: _container
    anchors.fill: parent

    currentIndex: 0

    Control {
      ColumnLayout {
        id: mainLayout
        anchors.fill: parent

        RowLayout {
          Layout.alignment: Qt.AlignTop
          Layout.fillWidth: true

          GridLayout {
            id: _config_layout
            columns: 3
            flow: GridLayout.LeftToRight
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: _sample_text.implicitWidth + cb_drivers.implicitWidth  + _driver_properties.implicitWidth

            Text {
              text: "Driver"
              font: OV.Style.fonts.formLabel
              color: OV.Style.colors.textColorBase
            }
            OV.ComboBox {
              id: cb_drivers
              model: master.drivers

              onCurrentTextChanged: {
                console.log("set new Driver to ", currentText)
                master.setCurrentDriver(currentText)
                _driver_param.updateParametersModel()
                _device_param.updateParametersModel()
              }
            }
            OV.Button {
              id: _driver_properties
              text: "Driver Properties"
              iconName: "cogs"
              onClicked: {
                _container.currentIndex = 1
              }
            }

            Text {
              text: "Connection Port"
              font: OV.Style.fonts.formLabel
              color: OV.Style.colors.textColorBase
            }
            OV.SpinBox {
              Layout.columnSpan: 2
              Layout.fillWidth: true
              // Limits and interaction
              min: 1024
              max: 65535
              editable: true
              // Bind to the master
              value: master.connectionPort
              onValueChanged: {
                master.connectionPort = value
              }
            }

            Text {
              id: _sample_text
              text: "Sample count per sent block"
              font: OV.Style.fonts.formLabel
              color: OV.Style.colors.textColorBase
            }
            OV.ComboBox {
              Layout.columnSpan: 2
              Layout.fillWidth: true
              model: master.sampleCountPerBlockList
              currentIndex: master.getInitSampleCountPerBlockIndex()
              onCurrentIndexChanged: {
                master.setSampleCountPerBlock(model[currentIndex])
              }
            }
          }

          ColumnLayout {
            id: _buttons_layout
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: _preference_button.implicitWidth

            OV.Button {
              id: _preference_button
              Layout.alignment: Qt.AlignRight
              text: qsTr("Preferences")
              iconName: "book"
              onClicked: {
                settgs.open()
              }

            }
            OV.Button {
              Layout.alignment: Qt.AlignRight
              text: "Connect"
              iconName: "connection"
              onClicked:  {
                if (text === "Connect") {
                  if (master.connectDevice()) {
                    text= "Disconnect";
                    button_play.enabled = true;
                  }
                } else {
                  if (master.disconnectDevice()) {
                    text= "Connect";
                    button_play.enabled = false;
                  }
                }
              }
            }
            OV.Button {
              id: button_play
              Layout.alignment: Qt.AlignRight
              text: "play"
              iconName: "play"
              enabled: false
              onClicked: {
                if (text === "play") {
                  if (master.startAcquisition()) {
                    text = "stop"
                    iconName = "stop"
                  }
                } else {
                  if (master.stopAcquisition()) {
                    text = "play"
                    iconName = "play"
                  }
                }
              }
            }
          }
        } // RowLayout

        OV.Logger {
          Layout.fillWidth: true
          Layout.fillHeight: true
          isOpen: true
          canBeHidden: false
        }

        Text {
          id: drift_text
          Layout.alignment: Qt.AlignHCenter
          text: "Device drift: " + master.drift + "ms"
        }

        RowLayout {
          Layout.fillWidth: true

          // drift is between [-2ms, 2ms] and the progress bar take a value : [0. ,1.], hence the formula master.drift/2
          OV.ProgressBar {
            Layout.fillWidth: true
            LayoutMirroring.enabled : true

            value: master.drift < 0 ? -master.drift/2 : 0
          }

          OV.ProgressBar {
            Layout.fillWidth: true
            value: master.drift > 0 ? master.drift/2 : 0
          }
        }
      }
    }

    //params menu
    Control {
      ColumnLayout {
        anchors.fill: parent

        spacing: OV.Style.mediumColumnSpacing

        Flickable {
          Layout.fillWidth: true
          Layout.fillHeight: true
          contentWidth: width
          contentHeight: mainParamColumn.childrenRect.height

          flickableDirection: Flickable.VerticalFlick

          clip: true

          ScrollBar.vertical: ScrollBar {
                active: true
                policy: ScrollBar.AsNeeded
          }

          Column {
            id: mainParamColumn
            spacing: OV.Style.smallPadding
            anchors.fill: parent

            Label {
              anchors.horizontalCenter: parent.horizontalCenter
              text: "Device Configuration"
              font: OV.Style.fonts.header
              color: OV.Style.colors.textColorBase
            }
            Repeater {
              anchors.horizontalCenter: parent.horizontalCenter
              width: parent.width;

              model: _device_param.params_model

              delegate: Loader {
                required property var param;
                required property var component;
                property var lparam: param

                width: OV.Style.largePanelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                sourceComponent: component
              }
            }

            Label {
              anchors.horizontalCenter: parent.horizontalCenter
              text: "Driver Configuration"
              font: OV.Style.fonts.header
              color: OV.Style.colors.textColorBase
            }

            Repeater {
              anchors.horizontalCenter: parent.horizontalCenter
              width: parent.width;

              model: _driver_param.params_model

              delegate: Loader {
                required property var param;
                required property var component;
                property var lparam: param

                width: OV.Style.largePanelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                sourceComponent: component
              }
            }

            OV.Separator {
              height: OV.Style.largeColumnSpacing
            }

            OV.Button {
              id: _channelRename
              anchors.horizontalCenter : parent.horizontalCenter
              text: "Change channel names"
              onClicked: {
                channelRename.channelNames = master.getChannelNames()
                channelRename.open()
              }
            }
          }
        }

        OV.Button {
          id: _finishButton
          Layout.alignment: Qt.AlignHCenter
          text: "Finish"
          onClicked: {
            master.configureDeviceDriver()
            _container.currentIndex = 0
          }
        }

        Item {
          Layout.preferredHeight: OV.Style.mediumColumnSpacing
        }
      }
    }

    OV.Impedances {
      id: _impedance
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumHeight: 100

      model: master.impedances

      onFinished: {
        _container.currentIndex = 0
        master.disconnectDevice()
      }
    }

  }

  Component.onCompleted: {
    OV.Style.mode = stt.mode
    configParams.updateParametersModel()
  }
}
