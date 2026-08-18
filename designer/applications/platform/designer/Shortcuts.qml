// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    Repeater {
        model: ShortcutManager.shortcuts

        delegate: Item{
            required property var modelData
            Shortcut {

                sequence: modelData.sequence
                onActivated: modelData.signal()
            }
        }
    }
}