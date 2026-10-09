// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QByteArray>

namespace OpenViBE {
namespace Designer {

/**
 * @brief In-memory XML round-trip of a kernel scenario.
 *
 * Drives the kernel's XML scenario importer/exporter algorithms directly (rather than going through
 * IScenarioManager, whose import always creates a *new* scenario) so that an existing IScenario can
 * be refilled in place. Used by the undo history (snapshots) and the clipboard (copy/paste payload).
 */
class ScenarioSerializer
{
public:
    explicit ScenarioSerializer(Kernel::IKernelContext& ctx);

    /** Serializes the scenario to XML. Returns an empty array on failure. */
    QByteArray toXml(const Kernel::IScenario& scenario) const;

    /** Clears `into` and refills it from `xml`. Identifiers stored in the XML are preserved. */
    bool fromXml(const QByteArray& xml, Kernel::IScenario& into) const;

private:
    Kernel::IKernelContext& m_ctx;
};

}  // namespace Designer
}  // namespace OpenViBE
