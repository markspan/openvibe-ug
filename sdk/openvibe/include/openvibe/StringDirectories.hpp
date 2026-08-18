#pragma once

#include <cstdlib>   // For getenv()
#include <string>

#if defined TARGET_OS_Windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "m_ConverterUtf8.h"
#include <memory>
#elif defined TARGET_OS_Linux
#include <unistd.h>
#include <cstring>
#elif defined TARGET_OS_MacOS
#include <mach-o/dyld.h>
#endif

namespace OpenViBE {
class StringDirectories
{
public:
    StringDirectories() = delete;

#ifdef OV_USE_CMAKE_DEFAULT_PATHS
    static std::string getDistRootDir() { return pathFromEnv("OV_PATH_ROOT", OV_CMAKE_PATH_ROOT); }
#else
    static std::string getDistRootDir() { return pathFromEnv("OV_PATH_ROOT", guessRootDir().c_str()); }
#endif

    static std::string getBinDir() { return pathFromEnvOrExtendedRoot("OV_PATH_BIN", "/bin", OV_CMAKE_PATH_BIN); }
    static std::string getDataDir() { return pathFromEnvOrExtendedRoot("OV_PATH_DATA", "/share/openvibe", OV_CMAKE_PATH_DATA); }

#if defined TARGET_OS_Windows
    static std::string getLibDir() { return pathFromEnvOrExtendedRoot("OV_PATH_LIB", "/bin", OV_CMAKE_PATH_BIN); }
    static std::string getUserHomeDir() { return pathFromEnv("USERPROFILE", "openvibe-user"); }
    static std::string getUserDataDir() { return (pathFromEnv("APPDATA", "openvibe-user") + "/" + OV_CONFIG_SUBDIR); }

    static std::string getAllUsersDataDir()
    {
        std::string path = pathFromEnv("PROGRAMDATA", "");                  // first chance: Win7 and higher
        if (path.empty()) { path = pathFromEnv("ALLUSERSPROFILE", ""); }    // second chance: WinXP
        if (path.empty()) { path = "openvibe-user"; }                       // fallback
        return path + "/" + OV_CONFIG_SUBDIR;
    }
#else
    static std::string getLibDir() { return pathFromEnvOrExtendedRoot("OV_PATH_LIB", "/lib", OV_CMAKE_PATH_LIB); }
    static std::string getUserHomeDir() { return pathFromEnv("HOME", "openvibe-user"); }
    static std::string getUserDataDir() { return (getUserHomeDir() + "/.config/" + OV_CONFIG_SUBDIR); }
    static std::string getAllUsersDataDir() { return (getUserHomeDir() + "/.config/" + OV_CONFIG_SUBDIR); }
#endif

    static std::string getLogDir() { return getUserDataDir() + "/log"; }
    static std::string getLib(const std::string& lib) { return getLibDir() + getLibPrefix() + lib + getLibExt(); }

private:
    // Used to convert \ in paths to /, we need this because \ is a special character for .conf token parsing
    static std::string convertPath(const std::string& strIn)
    {
        std::string out(strIn);
        const size_t len = strIn.length();
        for (size_t i = 0; i < len; ++i) { if (strIn[i] == '\\') { out[i] = '/'; } }
        return out;
    }

    /// Try to guess the root directory by assuming that any program that uses the kernel is in the bin
    /// subdirectory of the dist folder.
    static std::string guessRootDir()
    {
        static std::string rootDir;
        if (!rootDir.empty()) { return rootDir; }

        std::string fullpath;
#if defined TARGET_OS_Windows
        // Unlike GetEnvironmentVariableW, this function can not return the length of the actual path
        const std::unique_ptr<wchar_t> utf16value(new wchar_t[1024]);
        GetModuleFileNameW(nullptr, utf16value.get(), 1024);
        const int multiByteSize = WideCharToMultiByte(CP_UTF8, 0, utf16value.get(), -1, nullptr, 0, nullptr, nullptr);
        if (multiByteSize == 0) {
            // There are no sensible values to return if the above call fails and the program will not be
            // able to run in any case.
            std::abort();
        }
        const std::unique_ptr<char> utf8Value(new char[size_t(multiByteSize)]);
        if (WideCharToMultiByte(CP_UTF8, 0, utf16value.get(), -1, utf8Value.get(), multiByteSize, nullptr, nullptr) == 0) { std::abort(); }

        fullpath = convertPath(utf8Value.get());
#elif defined TARGET_OS_Linux
        char path[2048];
        memset(path, 0, sizeof(path)); // readlink does not produce 0 terminated strings
        readlink("/proc/self/exe", path, sizeof(path));
        fullpath = std::string(path);
#elif defined TARGET_OS_MacOS
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::unique_ptr<char> path(new char[size + 1]);

        if (_NSGetExecutablePath(path.get(), &size) != 0) { std::abort(); }

        fullpath = std::string(path.get());
#endif
        const auto slashBeforeLast = fullpath.find_last_of('/', fullpath.find_last_of('/') - 1);
        rootDir                    = fullpath.substr(0, slashBeforeLast);
        return rootDir;
    }

    // Returns ENV variable value or sDefaultPath if the variable doesn't exist. The path is converted with each \ to /.
    static std::string pathFromEnv(const char* sEnvVar, const char* sDefaultPath)
    {
#if defined TARGET_OS_Windows
        // Using std::getenv on Windows yields UTF7 strings which do not work with the Utf8ToUtf16 function
        // as this seems to be the only place where we actually get UTF7, let's get it as UTF16 by default
        const DWORD wideBufferSize = GetEnvironmentVariableW(Common::Converter::Utf8ToUtf16(sEnvVar).c_str(), nullptr, 0);
        if (wideBufferSize == 0) { return convertPath(sDefaultPath); }
        const std::unique_ptr<wchar_t> utf16value(new wchar_t[wideBufferSize]);
        GetEnvironmentVariableW(Common::Converter::Utf8ToUtf16(sEnvVar).c_str(), utf16value.get(), wideBufferSize);

        const int multiByteSize = WideCharToMultiByte(CP_UTF8, 0, utf16value.get(), -1, nullptr, 0, nullptr, nullptr);
        if (multiByteSize == 0) { return convertPath(sDefaultPath); }
        const std::unique_ptr<char> utf8Value(new char[size_t(multiByteSize)]);
        if (WideCharToMultiByte(CP_UTF8, 0, utf16value.get(), -1, utf8Value.get(), multiByteSize, nullptr, nullptr) == 0) { return convertPath(sDefaultPath); }

        const char* pathPtr = utf8Value.get();
#else
            const char * pathPtr = std::getenv(sEnvVar);
#endif
        const std::string path = (pathPtr ? pathPtr : sDefaultPath);
        return convertPath(path);
    }

    // Returns ENV variable if it is defined, otherwise it extends the ROOT variable if it exists, finally returns a default path
    static std::string pathFromEnvOrExtendedRoot(const char* envVar, const char* rootPostfix, const char* defaultPath)
    {
        if (std::getenv(envVar)) { return pathFromEnv(envVar, defaultPath); }
        if (std::getenv("OV_PATH_ROOT")) {
            // the default case for this one is wrong but it should never happen
            return pathFromEnv("OV_PATH_ROOT", "") + rootPostfix;
        }
#ifdef OV_USE_CMAKE_DEFAULT_PATHS
        return convertPath(defaultPath);
#else
        return guessRootDir() + rootPostfix;
#endif
    }

#if defined TARGET_OS_Windows
    static std::string getLibPrefix() { return "/openvibe-"; }
#else
    static std::string getLibPrefix() { return "/libopenvibe-"; }
#endif

#if defined TARGET_OS_Windows
    static std::string getLibExt() { return ".dll"; }
#elif defined TARGET_OS_Linux
    static std::string getLibExt() { return ".so"; }
#elif defined TARGET_OS_MacOS
    static std::string getLibExt() { return ".dylib"; }
#endif
};
}  // namespace OpenViBE
