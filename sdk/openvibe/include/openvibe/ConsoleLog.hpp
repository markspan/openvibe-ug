#pragma once
/**
 * @file ConsoleLog.hpp
 * @brief Sends what an application prints to a log file, for an application that has no console window.
 *
 * UG fork: new file, not in upstream OpenViBE, where the acquisition server opens a console window next to its own,
 * and Designer prints to the console of whatever started it. The fork shows no console: what they print goes to
 * <user data>/log/<name>.log instead, next to the kernel's own openvibe-<application>.log.
 */

#include "StringDirectories.hpp"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

namespace OpenViBE {

/**
 * @brief Sends standard output and standard error to <user data>/log/<name>.log, keeping the previous run's file as
 *        <name>.previous.log.
 *
 * Does nothing when the application was started with its output redirected (to a pipe or a file), so that it still
 * goes there, nor outside Windows, where a terminal shows it. Call it first thing in main().
 *
 * @param name Name of the file, without extension, e.g. "openvibe-designer-console".
 * @return Path of the log file, empty when the output was left as it was or the file could not be opened.
 */
inline std::string redirectConsoleToLogFile(const std::string& name)
{
#ifdef _WIN32
    const HANDLE existing = GetStdHandle(STD_OUTPUT_HANDLE);
    if (existing != nullptr && existing != INVALID_HANDLE_VALUE) { return {}; }

    namespace fs = std::filesystem;
    std::error_code error;
    const fs::path folder = fs::u8path(StringDirectories::getLogDir());
    fs::create_directories(folder, error);
    const fs::path file = folder / fs::u8path(name + ".log");
    // The previous run's output is what you look at after a recording went wrong, so it is kept for one more run
    fs::rename(file, folder / fs::u8path(name + ".previous.log"), error);

    // Opened here rather than with freopen(), which locks the file: this way it can be read while the application runs
    const HANDLE handle = CreateFileW(file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) { return {}; }
    const int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_TEXT);
    if (fd < 0) {
        CloseHandle(handle);
        return {};
    }

    // Without a console the streams have no file descriptor to replace yet, so they get one first
    FILE* stream = nullptr;
    if (freopen_s(&stream, "NUL", "w", stdout) != 0 || freopen_s(&stream, "NUL", "w", stderr) != 0) {
        _close(fd);
        return {};
    }
    _dup2(fd, _fileno(stdout));
    _dup2(fd, _fileno(stderr));
    _close(fd);
    // Unbuffered, so that nothing is lost when the application ends abruptly
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    // For code that writes to the Windows handles rather than to the C runtime's streams
    SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stdout))));
    SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stderr))));
    std::ios::sync_with_stdio(true);
    const auto path = file.u8string();  // char8_t from C++20 on, char before
    return std::string(path.begin(), path.end());
#else
    (void)name;
    return {};
#endif
}

}  // namespace OpenViBE
