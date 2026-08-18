#include "ovkCLogListenerFile.h"

#include <sstream>
#include <iostream>

#include <fs/Files.h>

namespace OpenViBE {
namespace Kernel {

CLogListenerFile::CLogListenerFile(const IKernelContext& ctx, const CString& applicationName, const CString& logFilename)
    : TKernelObject<ILogListener>(ctx), m_applicationName(applicationName), m_logFilename(logFilename)
{
    // Create the path to the log file
    FS::Files::createParentPath(logFilename.toASCIIString());

    FS::Files::openFStream(m_fsFileStream, logFilename.toASCIIString(), std::ios_base::out);

    if (!m_fsFileStream.is_open())
    {
        std::cout << "[  ERR  ] Error while creating FileLogListener into '" << logFilename << "'" << std::endl;
        return;
    }
    m_fsFileStream << std::flush;
}

void CLogListenerFile::configure(const IConfigurationManager& configurationManager)
{
    m_timeInSeconds = configurationManager.expandAsBoolean("${Kernel_FileLogTimeInSecond}", false);
    m_logWithHexa   = configurationManager.expandAsBoolean("${Kernel_FileLogWithHexa}", true);
    m_timePrecision = configurationManager.expandAsUInteger("${Kernel_FileLogTimePrecision}", 3);
}

bool CLogListenerFile::isActive(const ELogLevel level)
{
    const auto it = m_activeLevels.find(level);
    if (it == m_activeLevels.end()) { return true; }
    return it->second;
}

bool CLogListenerFile::activate(const ELogLevel level, const bool active)
{
    m_activeLevels[level] = active;
    return true;
}

bool CLogListenerFile::activate(const ELogLevel startLevel, const ELogLevel endLevel, const bool active)
{
    for (int i = startLevel; i <= endLevel; ++i) { m_activeLevels[ELogLevel(i)] = active; }
    return true;
}

bool CLogListenerFile::activate(const bool active) { return activate(LogLevel_First, LogLevel_Last, active); }

}  // namespace Kernel
}  // namespace OpenViBE
