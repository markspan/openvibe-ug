#include "CKernelLoader.hpp"

#include <iostream>
#include <string>

namespace OpenViBE {

void UpdateError(CString* error)
{
    if (error) {
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
        *error = dlerror();
#elif defined TARGET_OS_Windows
        LPVOID buffer = nullptr;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(),
                      0, // Default language
                      LPTSTR(&buffer), 0, nullptr);
        *error = (char*)buffer;
        LocalFree(buffer);
#endif
    }
}


void CKernelLoader::open(const CString& filename)
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    m_fileHandle = dlopen(filename, RTLD_LAZY | RTLD_GLOBAL);
#elif defined TARGET_OS_Windows
    m_fileHandle = LoadLibrary(filename);
#endif
}

void CKernelLoader::close()
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    dlclose(m_fileHandle);
#elif defined TARGET_OS_Windows
    FreeLibrary(m_fileHandle);
#endif
}


bool CKernelLoader::initialize()
{
    if (!isOpen()) { return false; }
    if (!m_onInitializeCB) { return true; }
    return m_onInitializeCB();
}

bool CKernelLoader::getKernelDesc(Kernel::IKernelDesc*& desc)
{
    if (!isOpen() || !m_onGetKernelDescCB) { return false; }
    return m_onGetKernelDescCB(desc);
}

bool CKernelLoader::uninitialize()
{
    if (!isOpen()) { return false; }
    if (!m_onUninitializeCB) { return true; }
    return m_onUninitializeCB();
}

bool CKernelLoader::load(const CString& filename, CString* error)
{
    if (m_fileHandle) {
        if (error) { *error = "kernel already loaded"; }
        return false;
    }
    open(filename);
    if (!m_fileHandle) {
        UpdateError(error);
        return false;
    }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    m_onInitializeCB    = (bool (*)())dlsym(m_fileHandle, "onInitialize");
    m_onUninitializeCB  = (bool (*)())dlsym(m_fileHandle, "onUninitialize");
    m_onGetKernelDescCB = (bool (*)(Kernel::IKernelDesc*&))dlsym(m_fileHandle, "onGetKernelDesc");
#elif defined TARGET_OS_Windows
    m_onInitializeCB    = (bool (*)())GetProcAddress(m_fileHandle, "onInitialize");
    m_onUninitializeCB  = (bool (*)())GetProcAddress(m_fileHandle, "onUninitialize");
    m_onGetKernelDescCB = (bool (*)(Kernel::IKernelDesc*&))GetProcAddress(m_fileHandle, "onGetKernelDesc");
#endif
    if (!m_onGetKernelDescCB) {
        UpdateError(error);

        close();
        m_fileHandle        = nullptr;
        m_onInitializeCB    = nullptr;
        m_onGetKernelDescCB = nullptr;
        m_onUninitializeCB  = nullptr;
        return false;
    }
    return true;
}

bool CKernelLoader::unload(CString* error)
{
    if (!m_fileHandle) {
        if (error) { *error = "no kernel currently loaded"; }
        return false;
    }
    close();
    m_fileHandle        = nullptr;
    m_onInitializeCB    = nullptr;
    m_onGetKernelDescCB = nullptr;
    m_onUninitializeCB  = nullptr;

    return true;
}

}  // namespace OpenViBE
