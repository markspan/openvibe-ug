// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.43.
//
// A short thick line in a channel's trace colour. ChannelGutter puts it before a channel's name when the traces are
// coloured per channel, so that a name can be matched to its trace: in a lane, and in Multiview's legend.

import QtQuick

import DesignSystem as DS

Rectangle {
    /// Space between the swatch and the name after it
    readonly property int gap: DS.Theme.spacingXs
    /// Room the swatch takes before a name, gap included: the displays widen their column of names by this much
    readonly property int footprint: implicitWidth + gap

    implicitWidth: DS.Theme.spacingMd
    implicitHeight: DS.Theme.spacingXs
    radius: height / 2
}
