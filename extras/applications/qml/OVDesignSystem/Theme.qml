// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

pragma Singleton

import QtQuick

QtObject {
    id: _root

    ////////////////////////////////////////////////////////////////////////////
    // Palette colors
    ////////////////////////////////////////////////////////////////////////////

    enum PaletteMode {
        Light,
        Dark
    }
    property var paletteMode: Theme.PaletteMode.Light

    property QtObject colorPalettes: QtObject {
        id: _colorPalettes

        property var paletteNames: {
            var names = {};
            names[Theme.PaletteMode.Light] = "light";
            names[Theme.PaletteMode.Dark] = "dark";
            return names;
        }

        property var palettes: ({
            "light": {
                primary: "#2F80ED",
                secondary: "#96C3FF",
                tertiary: "#C7DDFA",
                quaternary: "#ECF3FF",
                hover: "#E0EDFF",
                disabled: "#EFEFEF",
                annex: "#0A3671",
                textPrimary: "#051B38",
                textSecondary: "#515152",
                textTertiary: "#787A7D",
                textDisabled: "#8A8A8A",
                textInputBackground: "#F6F9FE",
                borderPrimary: "#051B38",
                shadow: "#848484",
                shadowBlue: "#2F80ED",
                stroke: "#D7D7D7",
                iconPrimary: "#051B38",
                iconSecondary: "#787A7D",
                iconDisabled: "#A0A3A7",
                iconHoverPrimary: "#2F80ED",
                iconHoverSecondary: "#E0EDFF",
                iconHoverClose: "#FF4747",
                iconPressed: "#2F80ED",
                navBarBackground: "#F5F5F5",
                boxBackground: "#F8F8F8",
                tooltipBackground: "#051B38",
                white: "#FFFFFF",
            },
            "dark": {
                primary: "#2F80ED",
                secondary: "#96C3FF",
                tertiary: "#C7DDFA",
                quaternary: "#ECF3FF",
                hover: "#E0EDFF",
                disabled: "#EFEFEF",
                annex: "#0A3671",
                textPrimary: "#051B38",
                textSecondary: "#515152",
                textTertiary: "#787A7D",
                textDisabled: "#8A8A8A",
                textInputBackground: "#F6F9FE",
                borderPrimary: "#051B38",
                shadow: "#848484",
                shadowBlue: "#2F80ED",
                stroke: "#D7D7D7",
                iconPrimary: "#051B38",
                iconSecondary: "#787A7D",
                iconDisabled: "#A0A3A7",
                iconHoverPrimary: "#2F80ED",
                iconHoverSecondary: "#E0EDFF",
                iconHoverClose: "#FF4747",
                iconPressed: "#2F80ED",
                iconDisabled: "#A0A3A7",
                boxBackground: "#F8F8F8",
                tooltipBackground: "#051B38",
                white: "#FFFFFF",
            }
        })

        property var currentPalette: palettes[paletteNames[_root.paletteMode]]

    }
    
    readonly property color primary: colorPalettes.currentPalette.primary
    readonly property color secondary: colorPalettes.currentPalette.secondary
    readonly property color tertiary: colorPalettes.currentPalette.tertiary
    readonly property color quaternary: colorPalettes.currentPalette.quaternary
    readonly property color hover: colorPalettes.currentPalette.hover
    readonly property color disabled: colorPalettes.currentPalette.disabled
    readonly property color annex: colorPalettes.currentPalette.annex
    readonly property color textPrimary: colorPalettes.currentPalette.textPrimary
    readonly property color textSecondary: colorPalettes.currentPalette.textSecondary
    readonly property color textTertiary: colorPalettes.currentPalette.textTertiary
    readonly property color textDisabled: colorPalettes.currentPalette.textDisabled
    readonly property color textInputBackground: colorPalettes.currentPalette.textInputBackground
    readonly property color borderPrimary: colorPalettes.currentPalette.borderPrimary
    readonly property color shadow: colorPalettes.currentPalette.shadow
    readonly property color shadowBlue: colorPalettes.currentPalette.shadowBlue
    readonly property color stroke: colorPalettes.currentPalette.stroke
    readonly property color iconPrimary: colorPalettes.currentPalette.iconPrimary
    readonly property color iconSecondary: colorPalettes.currentPalette.iconSecondary
    readonly property color iconDisabled: colorPalettes.currentPalette.iconDisabled    
    readonly property color iconHoverPrimary: colorPalettes.currentPalette.iconHoverPrimary
    readonly property color iconHoverSecondary: colorPalettes.currentPalette.iconHoverSecondary
    readonly property color iconHoverClose: colorPalettes.currentPalette.iconHoverClose
    readonly property color iconPressed: colorPalettes.currentPalette.iconPressed
    readonly property color navBarBackground: colorPalettes.currentPalette.navBarBackground
    readonly property color boxBackground: colorPalettes.currentPalette.boxBackground
    readonly property color tooltipBackground: colorPalettes.currentPalette.tooltipBackground
    readonly property color white: colorPalettes.currentPalette.white

    // Stream colors
    readonly property color experimentInfoStream: "#FFE080"
    readonly property color signalStream: "#FAB9DD"
    readonly property color featureVectorStream: "#FA9E80"
    readonly property color channelUnitsStream: "#6591F1"
    readonly property color stimulationStream: "#B399FF"
    readonly property color streamedMatrixStream: "#8DE7AF"
    readonly property color spectrumStream: "#F98585"
    readonly property color channelLocalisationStream: "#66D1FF"
    readonly property color covarianceMatrixStream: "#ADDA85"
    readonly property color timeFrequencyStream: "#FFC370"
    readonly property color ebmlStream: "#C77C9C"

    readonly property var windowManagerNodeColors: [
        "#FAB9DD",
        "#8DE7AF",
        "#B399FF",
        "#FFE080",
        "#66D1FF",
        "#F98585",
        "#ADDA85",
        "#C77C9C",
        "#FA9E80",
        "#6591F1",
        "#FFC370"
    ]

    // Log Levels colors
    readonly property var logLevelsColors: {
        "debug"   : "#CAB9FE",
        "info"    : "#B9D1FE",
        "warning" : "#FFBC85",
        "error"   : "#FEAEAE",
    }

    // CPU Usage colors
    readonly property var cpuUsageColors: {
        "low"    : "#47D390",
        "medium" : "#FFB333",
        "high"   : "#FF4747"
    }

    // Impendance levels colors
    readonly property var impedanceLevelColors: {
        "low"    : "#47D390",
        "medium" : "#FFB333",
        "high"   : "#FF4747"
    }

    readonly property var driftGradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#ECF3FF" }
        GradientStop { position: 0.3; color: "#FFA05C" }
        GradientStop { position: 0.8; color: "#FF4D6D" }
    }

    readonly property var negativeDriftGradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.2; color: "#FF4D6D" }
        GradientStop { position: 0.7; color: "#FFA05C" }
        GradientStop { position: 1.0; color: "#ECF3FF" }
    }
    ////////////////////////////////////////////////////////////////////////////
    // Font definitions
    ////////////////////////////////////////////////////////////////////////////

    readonly property string textFontName: FontManager.textFontName
    readonly property string titleFontName: FontManager.titleFontName

    readonly property font h1: Qt.font({
        family: textFontName,
        weight: Font.Bold,
        pixelSize: 56
    })
    readonly property font h2: Qt.font({
        family: textFontName,
        weight: Font.Bold,
        pixelSize: 48
    })
    readonly property font h3: Qt.font({
        family: textFontName,
        weight: Font.DemiBold,
        pixelSize: 40
    })
    readonly property font subtitle1: Qt.font({
        family: textFontName,
        weight: Font.DemiBold,
        pixelSize: 32
    })
    readonly property font subtitle2: Qt.font({
        family: textFontName,
        weight: Font.DemiBold,
        pixelSize: 24
    })
    readonly property font subtitle3: Qt.font({
        family: textFontName,
        weight: Font.DemiBold,
        pixelSize: 12
    })

    readonly property font textLargeBold: Qt.font({
        family: textFontName,
        weight: Font.Bold,
        pixelSize: 16
    })

    readonly property font textLargeMedium: Qt.font({
        family: textFontName,
        weight: Font.Medium,
        pixelSize: 16
    })

    readonly property font textLargeRegular: Qt.font({
        family: textFontName,
        weight: Font.Regular,
        pixelSize: 16
    })
    readonly property font textLargeLight: Qt.font({
        family: textFontName,
        weight: Font.Light,
        pixelSize: 16
    })
    readonly property font textMediumBold: Qt.font({
        family: textFontName,
        weight: Font.DemiBold,
        pixelSize: 14
    })
    readonly property font textMediumMedium: Qt.font({
        family: textFontName,
        weight: Font.Medium,
        pixelSize: 14
    })
    readonly property font textMediumRegular: Qt.font({
        family: textFontName,
        weight: Font.Regular,
        pixelSize: 14
    })
    readonly property font textMediumLight: Qt.font({
        family: textFontName,
        weight: Font.Light,
        pixelSize: 14
    })
    readonly property font textSmallBold: Qt.font({
        family: textFontName,
        weight: Font.Bold,
        pixelSize: 12
    })
    readonly property font textSmallRegular: Qt.font({
        family: textFontName,
        weight: Font.Regular,
        pixelSize: 12
    })
    readonly property font textSmallLight: Qt.font({
        family: textFontName,
        weight: Font.Light,
        pixelSize: 12
    })
    readonly property font textXsBold: Qt.font({
        family: textFontName,
        weight: Font.Bold,
        pixelSize: 10
    })
    readonly property font textXsRegular: Qt.font({
        family: textFontName,
        weight: Font.Regular,
        pixelSize: 10
    })
    readonly property font textXsLight: Qt.font({
        family: textFontName,
        weight: Font.Light,
        pixelSize: 10
    })

    // Font associated line heights
    readonly property int textXsLineHeight: 14
    readonly property int textSmLineHeight: 16
    readonly property int textMdLineHeight: 18
    readonly property int textLgLineHeight: 20

    ////////////////////////////////////////////////////////////////////////////
    // Margins
    ////////////////////////////////////////////////////////////////////////////

    readonly property int spacing2xs: 2
    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 12
    readonly property int spacingLg: 16
    readonly property int spacingXl: 20
    readonly property int spacing2xl: 24
    readonly property int spacing3xl: 32
    readonly property int spacing4xl: 40
    readonly property int spacing5xl: 48
    readonly property int spacing6xl: 56
    readonly property int spacing7xl: 64

    ////////////////////////////////////////////////////////////////////////////
    // Radius
    ////////////////////////////////////////////////////////////////////////////

    readonly property int radiusXs: 4
    readonly property int radiusSm: 8
    readonly property int radiusMd: 12
    readonly property int radiusLg: 16

    ////////////////////////////////////////////////////////////////////////////
    // Border
    ////////////////////////////////////////////////////////////////////////////

    readonly property int borderSm: 1
    readonly property int borderMd: 2
    readonly property int borderLg: 3

    ////////////////////////////////////////////////////////////////////////////
    // Label
    ////////////////////////////////////////////////////////////////////////////

    readonly property int labelHeightXs: 16
    readonly property int labelHeightSm: 18
    readonly property int labelHeightMd: 20
    readonly property int labelHeightLg: 24
    readonly property int labelHeightXl: 28
    readonly property int labelHeight2xl: 32
    readonly property int labelHeight3xl: 36
    readonly property int labelHeight4xl: 40
    readonly property int labelHeight5xl: 48

    readonly property int labelWidthXs: 48
    readonly property int labelWidthSm: 96
    readonly property int labelWidthMd: 120
    readonly property int labelWidthLg: 240
    readonly property int labelWidthXl: 320
    readonly property int labelWidth2xl: 400
    readonly property int labelWidth3xl: 480

    ////////////////////////////////////////////////////////////////////////////
    // Icons
    ////////////////////////////////////////////////////////////////////////////

    readonly property int iconXs: 10
    readonly property int iconSm: 12
    readonly property int iconMd: 14
    readonly property int iconLg: 16
    readonly property int iconXl: 18
    readonly property int icon2xl: 20
    readonly property int icon3xl: 24
    readonly property int icon4xl: 28
    readonly property int icon5xl: 32
    readonly property int icon6xl: 36
    readonly property int icon7xl: 40

    ////////////////////////////////////////////////////////////////////////////
    // Navbar
    ////////////////////////////////////////////////////////////////////////////

    readonly property int navBarHeight: 32

    ////////////////////////////////////////////////////////////////////////////
    // Menu
    ////////////////////////////////////////////////////////////////////////////

    readonly property int menuItemHeight: 32

    ////////////////////////////////////////////////////////////////////////////
    // ScrollBar
    ////////////////////////////////////////////////////////////////////////////

    readonly property int scrollBarWidth: 8
    readonly property int scrollBarHeight: 24


    ////////////////////////////////////////////////////////////////////////////
    // Box
    ////////////////////////////////////////////////////////////////////////////

    readonly property int boxMinimumWidth: 160
    readonly property int boxMinimumHeight: 60
    readonly property int boxIOWidth: 16
    readonly property int boxIOHeight: 8

    ////////////////////////////////////////////////////////////////////////////
    // Box Settings
    ////////////////////////////////////////////////////////////////////////////

    readonly property int boxSettingsMinimumWidth: 240
    readonly property int boxSettingsMaximumWidth: 480
    readonly property int toolBoxWidth: 400

    ////////////////////////////////////////////////////////////////////////////
    // Switch
    ////////////////////////////////////////////////////////////////////////////

    readonly property int switchWidth: 36
    readonly property int switchHeight: 18
    readonly property int switchCursorDiameter: 14

    ////////////////////////////////////////////////////////////////////////////
    // Window Manager
    ////////////////////////////////////////////////////////////////////////////

    readonly property int windowManagerHeight: 100

    ////////////////////////////////////////////////////////////////////////////
    // Logger
    ////////////////////////////////////////////////////////////////////////////

    readonly property int loggerMinimumHeight: 240
    readonly property int loggerMaximumHeight: 480
    readonly property int loggerHeaderHeight: 32
    readonly property int loggerStatusBarHeight: 48

    ////////////////////////////////////////////////////////////////////////////
    // Footer
    ////////////////////////////////////////////////////////////////////////////

    readonly property int footerHeight: 28


    ////////////////////////////////////////////////////////////////////////////
    // Acquisition Server
    ////////////////////////////////////////////////////////////////////////////

    readonly property int acquisitionServerContentMaximumWidth: 600
    readonly property int acquisitionServerParametersColumnWidth: 600
}