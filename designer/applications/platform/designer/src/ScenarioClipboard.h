// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPoint>

namespace OpenViBE {
namespace Designer {

class ScenarioSerializer;

/**
 * @brief Copy/paste of a scenario selection through the system clipboard.
 *
 * The selection is hand-copied into a temporary kernel scenario (boxes, comments, and the links
 * whose both ends are selected — the legacy designer's algorithm), serialized to XML and put on
 * QClipboard under `MimeType`. Paste does the reverse into the target scenario with fresh
 * identifiers, then moves the group so that its top-left corner lands on the requested anchor.
 *
 * When the system clipboard cannot be taken (locked by another process, a frequent situation on
 * remote desktops), the payload is kept in-process so that copy/paste keeps working inside the
 * application.
 */
class ScenarioClipboard : public QObject
{
    Q_OBJECT

public:
    static const char* const MimeType;   // "application/x-openvibe-scenario+xml"

    static ScenarioClipboard& instance();

    bool copy(Kernel::IKernelContext& ctx, const ScenarioSerializer& serializer, const Kernel::IScenario& source,
              const QList<CIdentifier>& boxIds, const QList<CIdentifier>& commentIds);

    /** True when the clipboard holds a scenario selection. */
    bool hasContent() const;

    struct Pasted
    {
        QList<CIdentifier> boxes;
        QList<CIdentifier> comments;
    };

    /** Pastes the clipboard into `target` with its top-left corner at `anchor` (scene coordinates), snapped to the grid. */
    Pasted paste(Kernel::IKernelContext& ctx, const ScenarioSerializer& serializer, Kernel::IScenario& target, const QPoint& anchor) const;

signals:
    /** The clipboard content changed (system clipboard or in-process fallback). */
    void contentChanged();

private:
    ScenarioClipboard();
    QByteArray payload() const;

    QByteArray m_localPayload;   // used only while the system clipboard is unavailable
};

}  // namespace Designer
}  // namespace OpenViBE
