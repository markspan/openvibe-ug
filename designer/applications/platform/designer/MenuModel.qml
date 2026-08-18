// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

pragma Singleton

import QtQuick 2.15

QtObject {
    id: menuModel
    
    // Global signals that can be connected from anywhere
    signal fileNewRequested()
    signal fileOpenRequested()
    signal fileSaveRequested()
    signal fileSaveAsRequested()
    signal editUndoRequested()
    signal editRedoRequested()
    signal editCutRequested()
    signal editCopyRequested()
    signal editPasteRequested()
    signal exportRequested()
    signal restoreScenarioRequested()
    signal communityRequested()
    signal documentationRequested()
    signal reportIssueRequested()
    signal themeChanged(string theme)
    signal zoomInRequested()
    signal zoomOutRequested()
    signal zoomResetRequested()
    signal keyboardShortcutsRequested()
    
    // Main menu structure
    // Each menu should have:
    // - an icon
    // - a text
    // - a badge (optional)
    // - a submenu (optional)
    // - a enable flag
    // - an action (if no submenu)
    readonly property var mainMenu: [
        {
            id: "file",
            icon: "folder",
            text: "File",
            submenu: [
                {
                    id: "fileNew",
                    icon: "plus-circle",
                    text: "New",
                    shortcut: ShortcutManager.getShortcutSequence("fileNew"),
                    action: function() { fileNewRequested() }
                },
                {
                    id: "fileOpen",
                    icon: "folder-open",
                    text: "Open",
                    shortcut: ShortcutManager.getShortcutSequence("fileOpen"),
                    action: function() { fileOpenRequested() }
                },
                {
                    id: "fileSave",
                    icon: "save",
                    text: "Save",
                    shortcut: ShortcutManager.getShortcutSequence("fileSave"),
                    action: function() { fileSaveRequested() }
                },
                {
                    id: "fileSaveAs",
                    icon: "save",
                    text: "Save As...",
                    shortcut: ShortcutManager.getShortcutSequence("fileSaveAs"),
                    action: function() { fileSaveAsRequested() }
                },
                {
                    id: "fileRecent",
                    icon: "clock",
                    text: "Recent Files",
                    submenu: []
                },
            ]
        },
        {
            id: "edit",
            icon: "edit", 
            text: "Edit",
            submenu: [
                {
                    id: "editUndo",
                    icon: "undo",
                    text: "Undo",
                    shortcut: ShortcutManager.getShortcutSequence("editUndo"),
                    enabled: canUndo,
                    action: function() { editUndoRequested() }
                },
                {
                    id: "editRedo",
                    icon: "redo",
                    text: "Redo",
                    shortcut: ShortcutManager.getShortcutSequence("editRedo"),
                    enabled: canRedo,
                    action: function() { editRedoRequested() }
                },
                {
                    id: "editCut",
                    icon: "cut",
                    text: "Cut",
                    shortcut: ShortcutManager.getShortcutSequence("editCut"),
                    action: function() { editCutRequested() }
                },
                {
                    id: "editCopy",
                    icon: "copy",
                    text: "Copy",
                    shortcut: ShortcutManager.getShortcutSequence("editCopy"),
                    action: function() { editCopyRequested() }
                },
                {
                    id: "editPaste",
                    icon: "paste",
                    text: "Paste",
                    shortcut: ShortcutManager.getShortcutSequence("editPaste"),
                    action: function() { editPasteRequested() }
                }
            ]
        },
        /*{
            id: "separator"
        },*/
       /* {
            id: "community",
            icon: "",
            text: "Community",
            badge: "NEW",
            action: function() { communityRequested() }
        },*/
        {
            id: "documentation",
            icon: "univ-hat",
            text: "Documentation",
            action: function() { documentationRequested() }
        },
        {
            id: "report",
            icon: "bug",
            text: "Report issue",
            action: function() { reportIssueRequested() }
        },
        /*{
            id: "separator"
        },*/
        /*{
            id: "appearance",
            icon: "",
            text: "Appearance",
            submenu: [
                {
                    id: "themeLight",
                    icon: "",
                    text: "Light",
                    action: function() { themeChanged("light") }
                },
                {
                    id: "theme_dark",
                    icon: "",
                    text: "Deep blue",
                    action: function() { themeChanged("deepBlue") }
                },
                {
                    id: "separator"
                },
                {
                    id: "zoom",
                    icon: "",
                    text: "Zoom",
                    submenu: [
                        {
                            id: "zoom_in",
                            text: "Zoom in",
                            shortcut: ShortcutManager.getShortcutSequence("zoomIn"),
                            action: function() { zoomInRequested() }
                        },
                        {
                            id: "zoom_out",
                            text: "Zoom out",
                            shortcut: ShortcutManager.getShortcutSequence("zoomOut"),
                            action: function() { zoomOutRequested() }
                        },
                        {
                            id: "zoom_reset",
                            text: "Reset zoom",
                            shortcut: ShortcutManager.getShortcutSequence("zoomReset"),
                            action: function() { zoomResetRequested() }
                        }
                    ]
                }
            ]
        },*/
        {
            id: "shortcuts",
            icon: "keyboard",
            text: "Keyboard shortcuts",
            action: function() { keyboardShortcutsRequested() }
        }
    ]
    
    // State properties
    property bool canUndo: false
    property bool canRedo: false
    property bool hasUnsavedChanges: false
    
    // Helper function to check if item has submenu
    function hasSubmenu(item) {
        return item && item.submenu && item.submenu.length > 0
    }
}