#pragma once

/**
 * @file OSAL.h
 * @brief Operating System Abstraction Layer for dynamic library loading.
 * @details Cross-platform interface for dlopen/dlsym/dlclose (Unix) and
 *          LoadLibraryExW/GetProcAddress/FreeLibrary (Windows).
 *
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "defines.h"
#include <string>
#include <algorithm>

#if defined TARGET_OS_Windows
#include <Windows.h>
#include <libloaderapi.h>
#endif

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <dlfcn.h>
#endif

namespace OSAL {

#if defined TARGET_OS_Windows
    using dlib = HMODULE;
    using proc = FARPROC;
#else
    using dlib = void*;
    using proc = void*;
#endif

    inline std::string normalizedPath(std::string path)
    {
#if defined TARGET_OS_Windows
        std::replace(path.begin(), path.end(), '/', '\\');
#else
        std::replace(path.begin(), path.end(), '\\', '/');
#endif
        return path;
    }

    System_API dlib loadLibrary(const std::string& path);
    System_API proc getProcAddr(dlib handle, const char* name);
    System_API bool freeLibrary(dlib handle);

} // namespace OSAL
