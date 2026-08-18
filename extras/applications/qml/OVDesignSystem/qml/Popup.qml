import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DesignSystem as DS

Popup {
    id: root
    
    property string title: ""
    property font titleFont: DS.Theme.textMediumLight
    property int titleLineHeight: DS.Theme.textMdLineHeight

    property int maxHeight: 999999
    property int minHeight: _header.height + 2 * DS.Theme.spacingSm + 2 * DS.Theme.spacingMd
    
    default property alias content: contentContainer.children
    
    horizontalPadding: 0
    verticalPadding: 0
    
    spacing: 0

    width: _columnLayout.implicitWidth + 2 * DS.Theme.spacingLg
    height: Math.min(maxHeight, _columnLayout.implicitHeight + 2 * DS.Theme.spacingMd)

    // Frame & background
    background: Rectangle {
        color: DS.Theme.white
        border.color: DS.Theme.borderPrimary
        border.width: DS.Theme.borderSm
        radius: DS.Theme.radiusSm
    }

    ColumnLayout {
        id: _columnLayout

        anchors.fill: parent
        anchors.leftMargin: DS.Theme.spacingLg
        anchors.rightMargin: DS.Theme.spacingLg
        anchors.topMargin: DS.Theme.spacingMd
        anchors.bottomMargin: DS.Theme.spacingMd

        spacing: DS.Theme.spacingSm

        // Header
        Rectangle {
            id: _header
            Layout.fillWidth: true
            Layout.minimumHeight: _headerLayout.implicitHeight + 2 * DS.Theme.spacingSm
            Layout.minimumWidth: _headerLayout.implicitWidth

            RowLayout {
                id: _headerLayout
                anchors.fill: parent
                anchors.topMargin: DS.Theme.spacingSm
                anchors.bottomMargin: DS.Theme.spacingSm
                spacing: DS.Theme.spacingMd

                Label {
                    Layout.fillWidth: true

                    background: Rectangle {
                        radius: DS.Theme.radiusXs
                        opacity: 1.0
                    }
                    text: root.title
                    font: root.titleFont
                    lineHeight: root.titleLineHeight
                    lineHeightMode: Text.FixedHeight
                }
                
                DS.IconButton {
                    iconName: "close"
                    iconSize: DS.Theme.spacingMd
                    iconColor: DS.Theme.iconSecondary
                    iconHoverColor: DS.Theme.iconHoverClose
                    iconPressedColor: DS.Theme.iconPressed

                    onClicked: root.close()
                }
            }
        }
        
        DS.HorizontalSpacer { }

        // Content - children will be automatically added here
        ColumnLayout {
            id: contentContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: root.spacing
        }
    }
}