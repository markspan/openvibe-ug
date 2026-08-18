/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <system/OSAL.h>

#if defined TARGET_OS_Windows
#include <system/WindowsUtilities.h>
#endif

namespace OSAL {

dlib loadLibrary(const std::string& path)
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    return dlopen(normalizedPath(path).c_str(), RTLD_LAZY);
#elif defined TARGET_OS_Windows
    return static_cast<HMODULE>(System::WindowsUtilities::utf16CompliantLoadLibrary(normalizedPath(path).c_str()));
#endif
}

proc getProcAddr(dlib handle, const char* name)
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    return dlsym(handle, name);
#elif defined TARGET_OS_Windows
    return GetProcAddress(handle, name);
#endif
}

bool freeLibrary(dlib handle)
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    return dlclose(handle) == 0;
#elif defined TARGET_OS_Windows
    return FreeLibrary(handle) != 0;
#endif
}

} // namespace OSAL
