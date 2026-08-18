/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifdef _WIN32
#include <windows.h>
#include <delayimp.h>
#include <libloaderapi.h>

// Intercepts delay-load notifications to force LoadLibraryExA with LOAD_LIBRARY_SEARCH_USER_DIRS,
// ensuring DLLs in directories registered via AddDllDirectory are found.
FARPROC WINAPI DelayLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    if (dliNotify == dliNotePreLoadLibrary)
    {
        HMODULE hLib = LoadLibraryExA(pdli->szDll, NULL, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        return (FARPROC)hLib;
    }
    return NULL;
}

extern "C"
{
    const PfnDliHook __pfnDliNotifyHook2 = DelayLoadHook;
}
#endif
