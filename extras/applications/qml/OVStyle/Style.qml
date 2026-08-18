pragma Singleton

// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-FileCopyrightText: Inria <romain.tetley@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only
// From project gnomon: https://gitlab.inria.fr/gnomon/gnomon 

import QtQuick          2.15
import QtQuick.Controls 2.15

// js style
import "./style.qml.js" as S

QtObject {
    id: _self

    enum Mode {
        Dark,
        Light
    }

    enum ButtonType {
        Neutral,
        Base,
        OK,
        Warning,
        Danger
    }

    enum ButtonSize {
        Small,
        Medium,
        Large
    }

    enum LabelType {
        Header,
        SubHeader,
        Label,
        Value,
        Button
    }

    enum CardType {
        Base,
        Background,
        Foreground
    }

    enum ToolBarLocation {
        Top,
        Bottom
    }

    property int mode: Style.Mode.Light
    property QtObject colors:  _self.mode == Style.Mode.Dark ? _colorDark : _colorLight
    // property alias fonts: _fonts
    property int panelRadius: S.spacing["1"]

    property int cardRadius: S.spacing["3"]

    property int borderWidth: S.spacing["0"]

    property int buttonRadius: S.spacing["1"]
    property int buttonPadding: S.spacing["2"]

    property int shortButtonWidth: S.spacing["9"] + S.spacing["2"]
    property int buttonWidth: S.spacing["10"]
    //= 2 buttons with small padding
    property int longButtonWidth: S.spacing["12"] + S.spacing["2"]
    property int extraLongButtonWidth: S.spacing["12"] + S.spacing["6"]

    property int progressBar: S.spacing["3"]

    property int smallButtonHeight: S.spacing["4"]
    property int mediumButtonHeight: S.spacing["6"]
    property int largeButtonHeight: S.spacing["7"]

    property int smallColumnSpacing: S.spacing["2"]
    property int mediumColumnSpacing: S.spacing["4"]
    property int largeColumnSpacing: S.spacing["6"]

    property int smallRowSpacing: S.spacing["2"]
    property int mediumRowSpacing: S.spacing["4"]
    property int largeRowSpacing: S.spacing["6"]

    property int tinyPadding: S.spacing["0"]
    property int smallPadding: S.spacing["2"]
    property int mediumPadding: S.spacing["4"]
    property int largePadding: S.spacing["6"]

    property int dialogPadding: S.spacing["5"]

    property int scrollBarWidth: S.spacing["2"]
    property int scrollBarRadius: S.spacing["1"]

    property int toolBarHeight: S.spacing["6"]

    property int formSelectorHeight: S.spacing["10"]
    property int formDelegateHeight: S.spacing["7"]

    property int iconSmall: S.spacing["4"]
    property int iconMedium: S.spacing["5"]
    property int iconLarge: S.spacing["6"]

    property int thumbnailSmall: S.spacing["5"]
    property int thumbnailMedium: S.spacing["5"]
    property int thumbnailLarge: S.spacing["7"]

    property int smallLabelHeight: S.spacing["5"]
    property int mediumLabelHeight: S.spacing["6"]
    property int largeLabelHeight: S.spacing["8"]

    property int smallLabelWidth: S.spacing["5"]
    property int mediumLabelWidth: S.spacing["7"]
    property int largeLabelWidth: S.spacing["9"]

    property int smallPanelWidth: S.spacing["12"]
    property int mediumPanelWidth: S.spacing["13"]
    property int largePanelWidth: S.spacing["14"]

    property int smallDelegateHeight: S.spacing["8"]
    property int mediumDelegateHeight: S.spacing["9"]
    property int largeDelegateHeight: S.spacing["10"]

    property int smallPanelHeight: S.spacing["12"]
    property int mediumPanelHeight: S.spacing["13"]
    property int largePanelHeight: S.spacing["14"]

    property int smallDialogWidth: S.spacing["14"]
    property int mediumDialogWidth: S.spacing["15"]
    property int largeDialogWidth: S.spacing["16"]

    property int smallDialogHeight: S.spacing["12"]
    property int mediumDialogHeight: S.spacing["13"]
    property int largeDialogHeight: S.spacing["14"]

    //This is not arbitrary but chosen to fit in a 48px section with a label and margins
    property int comboBoxHeight: 28;
    property int spinBoxHeight: 28;

    property int collapsibleMinHeight: S.spacing["5"]
    property int collapsibleMaxHeight: S.spacing["12"]

    property int controlHeight: S.spacing["8"]
    property int controlWidth: S.spacing["12"]

    property int gutterHeight: S.spacing["5"]
    property int gutterWidth: S.spacing["4"]

    property int windowMinWidth: S.spacing["17"]
    property int windowMinHeight: (S.spacing["15"] + S.spacing["16"])/2

    property FontLoader regular: FontLoader {
        id: _poppinsRegular
        source: TARGET_OS_Windows ? "" : "Poppins-Regular.ttf"
    }

    property FontLoader medium: FontLoader {
        id: _poppinsMedium
        source: TARGET_OS_Windows ? "" : "Poppins-Medium.ttf"
    }

    property FontLoader light: FontLoader {
        id: _poppinsLight
        source: TARGET_OS_Windows ? "" : "Poppins-Light.ttf"
    }

    property FontLoader bold: FontLoader {
        id: _poppinsBold
        source: TARGET_OS_Windows ? "" : "Poppins-Bold.ttf"
    }

    property string fontName: TARGET_OS_Windows ?  "Arial" : "Poppins"

    // /////////////////////////////////////////////////////////////////////////////
    //  Keeping the sizing scale like this anyways
    // /////////////////////////////////////////////////////////////////////////////

    property QtObject sizes: QtObject {
        property int s1: S.spacing["1"] //4
        property int s2: S.spacing["2"] //8
        property int s3: S.spacing["3"] //12
        property int s4: S.spacing["4"] //16
        property int s5: S.spacing["5"] //24
        property int s6: S.spacing["6"] //32
        property int s7: S.spacing["7"] //48
        property int s8: S.spacing["8"] //64
        property int s9: S.spacing["9"] //96
        property int s10: S.spacing["10"] //128
        property int s11: S.spacing["11"] //192
        property int s12: S.spacing["12"] //256
        property int s13: S.spacing["13"] //384
        property int s14: S.spacing["14"] //512
        property int s15: S.spacing["15"] //640
        property int s16: S.spacing["16"] //896
    }


    // /////////////////////////////////////////////////////////////////////////////
    // Color definitions
    // /////////////////////////////////////////////////////////////////////////////

    property QtObject darkcolors: QtObject {
        id: _colorDark;

        //From previous style (gnomon)
        property color highlightColor: "#20E8C0"

        //panel colors (neutral)
        property color bgColor: S.colors.gray["700"]
        property color gutterColor: S.colors.gray["900"]
        property color fgColor: S.colors.gray["600"]
        property color alternateFgColor: S.colors.gray["500"]
        property color embossColor: S.colors.gray["600"]

        //basic color theme
        property color baseColor: S.colors.blue["500"]
        property color neutralColor: S.colors.gray["300"]
        property color dangerColor: S.colors.red["500"]
        property color warningColor: S.colors.orange["500"]
        property color okColor: S.colors.green["500"]
        property color noteColor: S.colors.yellow["500"]
        property color exampleColor: S.colors.purple["500"]

        //hovered color theme
        property color hoveredBaseColor: S.colors.blue["400"]
        property color hoveredNeutralColor: S.colors.gray["400"]
        property color hoveredDangerColor: S.colors.red["400"]
        property color hoveredWarningColor: S.colors.orange["400"]
        property color hoveredOkColor: S.colors.green["400"]

        //text colors
        property color textColorBase: S.colors.blue["100"]
        property color textColorOk: S.colors.green["100"]
        property color textColorDanger: S.colors.red["100"]
        property color textColorWarning: S.colors.orange["100"]
        property color textColorNeutral: S.colors.gray["100"]
        property color textColorFaded: S.colors.gray["200"]
        property color textColorDeEmphasize: S.colors.gray["600"]

        //text colors
        property color textColorLightBase: S.colors.blue["100"]
        property color textColorLightOk: S.colors.green["100"]
        property color textColorLightDanger: S.colors.red["100"]
        property color textColorLightWarning: S.colors.orange["100"]
        property color textColorLightNeutral: S.colors.gray["100"]
        property color textColorLightFaded: S.colors.gray["200"]

        //text colors
        property color textColorDarkBase: S.colors.blue["900"]
        property color textColorDarkOk: S.colors.green["900"]
        property color textColorDarkDanger: S.colors.red["900"]
        property color textColorDarkWarning: S.colors.orange["900"]
        property color textColorDarkNeutral: S.colors.gray["900"]
        property color textColorDarkFaded: S.colors.gray["800"]

        //emboss for different colors
        property color embossColorBlue: S.colors.blue["400"]
        property color embossColorNeutral: S.colors.gray["500"]

        //otherColors
        property color lightBlue: S.colors.blue["200"]
        property color lightGreen: S.colors.green["200"]
        property color lightRed: S.colors.red["200"]
        property color lightOrange: S.colors.orange["200"]
        property color lightYellow: S.colors.yellow["200"]
        property color lightPurple: S.colors.purple["200"]

        property color transparent: "#00000000"
        property color overlayColor: "#bb000000";
    }

    // TODO edit the colors
    property QtObject lightcolors: QtObject {
        id: _colorLight;

        //From previous style (gnomon)
        property color highlightColor: "#20E8C0"

        //panel colors (neutral)
        property color bgColor: S.colors.gray["200"]
        property color gutterColor: S.colors.gray["100"]
        property color fgColor: S.colors.gray["300"]
        property color alternateFgColor: S.colors.gray["400"]
        property color embossColor: S.colors.gray["300"]

        //basic color theme
        property color baseColor: S.colors.blue["500"]
        property color neutralColor: S.colors.gray["700"]
        property color dangerColor: S.colors.red["500"]
        property color warningColor: S.colors.orange["500"]
        property color okColor: S.colors.green["500"]
        property color noteColor: S.colors.yellow["500"]
        property color exampleColor: S.colors.purple["500"]

        //hovered color theme
        property color hoveredBaseColor: S.colors.blue["700"]
        property color hoveredNeutralColor: S.colors.gray["700"]
        property color hoveredDangerColor: S.colors.red["700"]
        property color hoveredWarningColor: S.colors.orange["700"]
        property color hoveredOkColor: S.colors.green["700"]

        //text colors
        property color textColorBase: S.colors.blue["900"]
        property color textColorOk: S.colors.green["900"]
        property color textColorDanger: S.colors.red["900"]
        property color textColorWarning: S.colors.orange["900"]
        property color textColorNeutral: S.colors.gray["900"]
        property color textColorFaded: S.colors.gray["800"]
        property color textColorDeEmphasize: S.colors.gray["400"]

        //text colors
        property color textColorLightBase: S.colors.blue["100"]
        property color textColorLightOk: S.colors.green["100"]
        property color textColorLightDanger: S.colors.red["100"]
        property color textColorLightWarning: S.colors.orange["100"]
        property color textColorLightNeutral: S.colors.gray["100"]
        property color textColorLightFaded: S.colors.gray["200"]

        //text colors
        property color textColorDarkBase: S.colors.blue["900"]
        property color textColorDarkOk: S.colors.green["900"]
        property color textColorDarkDanger: S.colors.red["900"]
        property color textColorDarkWarning: S.colors.orange["900"]
        property color textColorDarkNeutral: S.colors.gray["900"]
        property color textColorDarkFaded: S.colors.gray["800"]

        //emboss for different colors
        property color embossColorBlue: S.colors.blue["600"]
        property color embossColorNeutral: S.colors.gray["600"]

        //otherColors
        property color lightBlue: S.colors.blue["200"]
        property color lightGreen: S.colors.green["200"]
        property color lightRed: S.colors.red["200"]
        property color lightOrange: S.colors.orange["200"]
        property color lightYellow: S.colors.yellow["200"]
        property color lightPurple: S.colors.purple["200"]

        property color transparent: "#00000000"
        property color overlayColor: "#ddffffff";
    }

    // /////////////////////////////////////////////////////////////////////////////
    // Font definitions
    // /////////////////////////////////////////////////////////////////////////////

    property QtObject fonts: QtObject {
        id: _fonts;

        property font header: Qt.font({
            family: fontName,
            weight: Font.Medium, //"Regular",
            pointSize: S.fontScale["200"],
        })

        property font subHeader: Qt.font({
            family: fontName,
            weight: Font.Bold, //"Regular",
            pointSize: S.fontScale["100"],
        })

        property font label: Qt.font({
            family: fontName,
            weight: "Light",
            pointSize: S.fontScale["50"],
        })

        property font menu: Qt.font({
            family: fontName,
            weight: "Regular",
            pointSize: S.fontScale["50"],
        })

        property font value: Qt.font({
            family: fontName,
            weight: "Regular",
            pointSize: S.fontScale["100"],
        })

        property font button: Qt.font({
            family: fontName,
            weight: Font.Bold,
            pointSize: S.fontScale["200"],
        })

        property font buttonSmall: Qt.font({
            family: fontName,
            weight: Font.Bold,
            pointSize: S.fontScale["100"],
        })

        property font buttonHovered: Qt.font({
            family: fontName,
            weight: Font.Bold,
            pointSize: S.fontScale["200"],
            underline: true
        })

        property font h1: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["700"],
        })

        property font h2: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["400"],
        })

        property font h3: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["250"],
        })

        property font formLabel: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["150"],
        })

        property font cardTitle: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["250"],
        })

        property font cardText: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["200"],
        })

        property font cardLabel: Qt.font({
            family: fontName,
            weight: Font.Normal,
            pointSize: S.fontScale["150"],
        })

        property font nodeHeader: Qt.font({
            family: fontName,
            weight: Font.Normal,
            pointSize: S.fontScale["100"],
        })

        property font nodeHeaderSelected: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["100"],
        })

        property font nodeBody: Qt.font({
            family: fontName,
            weight: Font.Normal,
            pointSize: S.fontScale["50"],
        })

        property font nodeBodySelected: Qt.font({
            family: fontName,
            weight: Font.Medium,
            pointSize: S.fontScale["50"],
        })
    }
}
