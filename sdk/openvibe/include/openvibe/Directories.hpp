#pragma once

#include "StringDirectories.hpp"
#include "ovCString.h"

namespace OpenViBE {
class Directories
{
    // The functions in this file should be only used in bootstrapping circumstances where Configuration Manager (Kernel Context) is not available.
    // With access to Configuration Manager, the paths should be fetched using tokens such as ${Path_UserData}. Note that in such a case changing the environment variable later may no longer be affected in the token value.

    // @NOTE These functions may not be thread-safe
public:
    Directories() = delete;
    static CString getDistRootDir() { return StringDirectories::getDistRootDir().c_str(); }
    static CString getBinDir() { return StringDirectories::getBinDir().c_str(); }
    static CString getDataDir() { return StringDirectories::getDataDir().c_str(); }
    static CString getLibDir() { return StringDirectories::getLibDir().c_str(); }
    static CString getUserHomeDir() { return StringDirectories::getUserHomeDir().c_str(); }
    static CString getUserDataDir() { return StringDirectories::getUserDataDir().c_str(); }
    static CString getLogDir() { return StringDirectories::getLogDir().c_str(); }
    static CString getLib(const std::string& lib) { return StringDirectories::getLib(lib).c_str(); }
};
}  // namespace OpenViBE
