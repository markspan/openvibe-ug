// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

pragma Singleton
import QtQuick 2.15

QtObject {
    id: shortcutManager
    
    // Signals that mirror MenuModel signals
    signal fileNewRequested()
    signal fileOpenRequested()
    signal fileSaveRequested()
    signal fileSaveAsRequested()
    signal editUndoRequested()
    signal editRedoRequested()
    signal editCutRequested()
    signal editCopyRequested()
    signal editPasteRequested()
    signal zoomInRequested()
    signal zoomOutRequested()
    signal zoomResetRequested()
    signal helpRequested()
    
    // Shortcut definitions - centralized for easy management
    readonly property var shortcuts: [
        { sequence: "Ctrl+N", action: "fileNew", signal: fileNewRequested },
        { sequence: "Ctrl+O", action: "fileOpen", signal: fileOpenRequested },
        { sequence: "Ctrl+S", action: "fileSave", signal: fileSaveRequested },
        { sequence: "Ctrl+Shift+S", action: "fileSaveAs", signal: fileSaveAsRequested },
        { sequence: "Ctrl+Z", action: "editUndo", signal: editUndoRequested },
        { sequence: "Ctrl+Y", action: "editRedo", signal: editRedoRequested },
        { sequence: "Ctrl+Shift+Z", action: "editRedo", signal: editRedoRequested },
        { sequence: "Ctrl+X", action: "editCut", signal: editCutRequested },
        { sequence: "Ctrl+C", action: "editCopy", signal: editCopyRequested },
        { sequence: "Ctrl+V", action: "editPaste", signal: editPasteRequested },
        { sequence: "Ctrl++", action: "zoomIn", signal: zoomInRequested },
        { sequence: "Ctrl+-", action: "zoomOut", signal: zoomOutRequested },
        { sequence: "Ctrl+0", action: "zoomReset", signal: zoomResetRequested },
        { sequence: "F1", action: "help", signal: helpRequested }
    ]

    function getShortcutSequence(action) {
        for (var i = 0; i < shortcuts.length; i++) {
            if (shortcuts[i].action === action) {
                return shortcuts[i].sequence
            }
        }
        return ""
    }
}