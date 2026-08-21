/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <openvibe/ov_all.h>

#include <QString>
#include <QStringList>

namespace OpenViBE {
namespace Designer {

/**
 * @brief The SessionManager class persists the designer session from one run to the next.
 *
 * The scenario files opened in the tabs, and which one is displayed, are written to the user
 * configuration file designated by ${Designer_CustomConfigurationFile} as the tokens
 * Designer_LastScenarioFilename_NNN and Designer_LastScenarioSelectedIndex. The kernel already
 * loads that file at startup, so restoring a session is only a matter of reading those tokens
 * back.
 *
 * The token format is the one the legacy designer used, so a session left behind by the legacy
 * designer is restored as well. Session management is disabled by the --no-session-management
 * and --no-gui command line flags, in which case the configuration file is left untouched.
*/
class SessionManager
{
public:
    /// The scenario files to reopen, and which of them to bring to the front.
    struct Session
    {
        QStringList scenarioFiles;  ///< scenario file paths, in tab order
        int selectedIndex = -1;     ///< index into scenarioFiles, -1 when none was selected
    };

    SessionManager(Kernel::IKernelContext& context, bool enabled);

    bool isEnabled() const { return m_enabled; }

    /// Reads back the session saved by the previous run. Empty when session management is off.
    Session restore() const;

    /// Rewrites the session tokens of the user configuration file, preserving its other lines.
    void save(const Session& session) const;

private:
    QString configurationFilePath() const;

    Kernel::IKernelContext& m_context;
    bool m_enabled;
};

}
}
