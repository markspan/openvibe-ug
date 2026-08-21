/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "SessionManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

namespace OpenViBE {
namespace Designer {

namespace {

const char* SELECTED_INDEX_TOKEN   = "Designer_LastScenarioSelectedIndex";
const char* GENERATED_BLOCK_HEADER = "# Scenarios opened when the designer was last closed -- generated, do not edit";

/// Name of the token holding the index-th scenario file. Indices are 1-based and zero padded to
/// three digits, as the legacy designer wrote them.
QString scenarioTokenName(const int index) { return QStringLiteral("Designer_LastScenarioFilename_%1").arg(index, 3, 10, QChar('0')); }

/// True for the lines this class generates, which are the only ones it may drop when rewriting
/// the configuration file.
bool isSessionLine(const QString& line)
{
    static const QRegularExpression tokenLine(QStringLiteral("^\\s*Designer_LastScenario(Filename_\\d+|SelectedIndex)\\s*="));
    return line.trimmed() == QLatin1String(GENERATED_BLOCK_HEADER) || tokenLine.match(line).hasMatch();
}

}

SessionManager::SessionManager(Kernel::IKernelContext& context, const bool enabled) : m_context(context), m_enabled(enabled) { }

SessionManager::Session SessionManager::restore() const
{
    Session session;
    if (!m_enabled) {
        return session;
    }

    auto& configMgr = m_context.getConfigurationManager();

    // The tokens are contiguous, so the first missing index ends the list.
    for (int i = 1;; ++i) {
        const CIdentifier tokenID = configMgr.lookUpConfigurationTokenIdentifier(scenarioTokenName(i).toStdString().c_str());
        if (tokenID == CIdentifier::undefined()) {
            break;
        }
        // Expanded, so that a hand-written entry may use configuration tokens in its path.
        const CString filename = configMgr.expand(configMgr.getConfigurationTokenValue(tokenID));
        session.scenarioFiles << QString::fromStdString(filename.toASCIIString());
    }

    const CIdentifier selectedID = configMgr.lookUpConfigurationTokenIdentifier(SELECTED_INDEX_TOKEN);
    if (selectedID != CIdentifier::undefined()) {
        bool isNumber           = false;
        const CString rawValue  = configMgr.getConfigurationTokenValue(selectedID);
        const int selectedIndex = QString::fromStdString(rawValue.toASCIIString()).trimmed().toInt(&isNumber);
        if (isNumber && selectedIndex >= 1 && selectedIndex <= session.scenarioFiles.size()) {
            session.selectedIndex = selectedIndex - 1;   // stored 1-based, like the filename tokens
        }
    }

    return session;
}

void SessionManager::save(const Session& session) const
{
    if (!m_enabled) {
        return;
    }

    const QString path = configurationFilePath();
    if (path.isEmpty()) {
        m_context.getLogManager() << Kernel::LogLevel_Error << "${Designer_CustomConfigurationFile} is empty, the session can not be saved\n";
        return;
    }

    // Users also put their own configuration tokens in that file, so keep every line it already
    // holds but the ones this class generates.
    QStringList lines;
    QFile input(path);
    if (input.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&input);
        while (!in.atEnd()) {
            const QString line = in.readLine();
            if (!isSessionLine(line)) {
                lines << line;
            }
        }
        input.close();
    }

    while (!lines.isEmpty() && lines.constLast().trimmed().isEmpty()) {
        lines.removeLast();
    }
    if (!lines.isEmpty()) {
        lines << QString();
    }

    lines << QLatin1String(GENERATED_BLOCK_HEADER);
    for (int i = 0; i < session.scenarioFiles.size(); ++i) {
        lines << QStringLiteral("%1 = %2").arg(scenarioTokenName(i + 1), session.scenarioFiles[i]);
    }
    if (session.selectedIndex >= 0) {
        lines << QStringLiteral("%1 = %2").arg(QLatin1String(SELECTED_INDEX_TOKEN)).arg(session.selectedIndex + 1);
    }

    QDir().mkpath(QFileInfo(path).absolutePath());

    // QSaveFile so that a failure half way through leaves the previous file untouched.
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_context.getLogManager() << Kernel::LogLevel_Error << "Could not open [" << path.toStdString() << "] to save the session\n";
        return;
    }

    QTextStream out(&output);
    for (const QString& line : lines) {
        out << line << "\n";
    }

    if (!output.commit()) {
        m_context.getLogManager() << Kernel::LogLevel_Error << "Could not write the session to [" << path.toStdString() << "]\n";
    }
}

QString SessionManager::configurationFilePath() const
{
    const CString path = m_context.getConfigurationManager().expand("${Designer_CustomConfigurationFile}");
    return QString::fromStdString(path.toASCIIString()).trimmed();
}

}
}
