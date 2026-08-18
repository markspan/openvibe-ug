#pragma once

#include "defines.h"
#if defined TARGET_OS_Windows
#include <windows.h>
#include <string>

namespace System {
class System_API WindowsUtilities
{
public:
    static void* utf16CompliantLoadLibrary(const char* path, HANDLE file = nullptr, DWORD flags = LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    static BOOL utf16CompliantSetEnvironmentVariable(const char* name, const char* value);

    static BOOL utf16CompliantCreateProcess(char* applicationName, char* commandLine, LPSECURITY_ATTRIBUTES processAttributes,
                                            LPSECURITY_ATTRIBUTES threadAttributes, BOOL inheritHandles, DWORD creationFlags, LPVOID environment,
                                            char* currentDirectory, LPSTARTUPINFO startupInfo, LPPROCESS_INFORMATION processInformation);

    static HINSTANCE utf16CompliantShellExecute(HWND hwnd, LPCTSTR operation, LPCTSTR file, LPCTSTR parameters, LPCTSTR directory, INT nShowCmd);

    static std::wstring convertUtf8ToWide(const std::string& str);

private:
    WindowsUtilities() = delete;
};
}  // namespace System

#endif // TARGET_OS_Windows
