/**
 * @file Charmeleon.cpp
 * @brief Implementation of the Charmeleon launcher.
 *
 * UG fork: new file, not in upstream OpenViBE.
 */

#include "Charmeleon.h"

#include <CharmeleonMontage.h>

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <TlHelp32.h>
#endif

namespace Charmeleon {

namespace {

std::string toUtf8(const std::filesystem::path& path)
{
    const auto utf8 = path.u8string();
    return std::string(utf8.begin(), utf8.end());
}

bool isFile(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

#if defined(_WIN32)

const wchar_t* const EXE_NAME = L"Charmeleon.exe";

std::vector<DWORD> runningInstances()
{
    std::vector<DWORD> pids;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) { return pids; }

    PROCESSENTRY32W entry = {};
    entry.dwSize          = sizeof(entry);
    for (BOOL ok = Process32FirstW(snapshot, &entry); ok; ok = Process32NextW(snapshot, &entry)) {
        if (_wcsicmp(entry.szExeFile, EXE_NAME) == 0) { pids.push_back(entry.th32ProcessID); }
    }
    CloseHandle(snapshot);
    return pids;
}

/// Calls @p action on every visible top-level window of process @p pid.
template <typename Action>
void forEachWindow(const DWORD pid, Action action)
{
    struct Context
    {
        DWORD pid;
        Action* action;
    } context{pid, &action};

    EnumWindows([](HWND hwnd, LPARAM param) -> BOOL {
        auto* ctx       = reinterpret_cast<Context*>(param);
        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);
        if (windowPid == ctx->pid && IsWindowVisible(hwnd)) { (*ctx->action)(hwnd); }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&context));
}

#endif

}  // namespace

std::string findExecutable(const std::string& configuredPath)
{
    if (!configuredPath.empty()) {
        const std::filesystem::path configured = std::filesystem::u8path(configuredPath);
        return isFile(configured) ? toUtf8(configured) : std::string();
    }

#if defined(_WIN32)
    // Where its installer put it; the montage reader looks for Charmeleon's montage library there as well
    const std::filesystem::path installed = std::filesystem::u8path(charmeleonInstallFolder());
    if (!installed.empty() && isFile(installed / EXE_NAME)) { return toUtf8(installed / EXE_NAME); }

    wchar_t programFiles[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"ProgramFiles", programFiles, MAX_PATH) > 0) {
        const std::filesystem::path fallback = std::filesystem::path(programFiles) / L"Charmeleon" / EXE_NAME;
        if (isFile(fallback)) { return toUtf8(fallback); }
    }
#endif
    return {};
}

bool launch(const std::string& executable, const std::string& montage, const std::vector<std::string>& arguments, Launched* launched)
{
#if defined(_WIN32)
    const std::vector<DWORD> running = runningInstances();
    if (!running.empty()) {
        forEachWindow(running.front(), [](HWND hwnd) {
            if (IsIconic(hwnd)) { ShowWindow(hwnd, SW_RESTORE); }
            SetForegroundWindow(hwnd);
        });
        return true;
    }

    const std::filesystem::path exe = std::filesystem::u8path(executable);
    // Charmeleon loads its default layout relative to the working directory, so run it from its own folder
    const std::wstring workingDir = exe.parent_path().wstring();
    std::wstring commandLine      = L"\"" + exe.wstring() + L"\"";
    // Absolute, as Charmeleon runs from another directory
    if (!montage.empty()) {
        std::error_code ec;
        const std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(montage), ec);
        commandLine += L" \"" + (ec ? std::filesystem::u8path(montage) : path).wstring() + L"\"";
    }

    // UG fork: the amplifier the driver uses, so that Charmeleon does not open another one
    for (const auto& argument : arguments) { commandLine += L" \"" + std::filesystem::u8path(argument).wstring() + L"\""; }

    // UG fork: Charmeleon saves the montage when Play closes it, and says which file it ended with, so that the
    // driver can take the edits over. Not ".json", which a Charmeleon before 2.2 would take for the montage.
    std::filesystem::path resultFile;
    if (launched) {
        std::error_code ec;
        const std::filesystem::path temp = std::filesystem::temp_directory_path(ec);
        if (!ec) {
            resultFile = temp / (L"charmeleon-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L".result");
            commandLine += L" --save-on-close --result \"" + resultFile.wstring() + L"\"";
        }
    }

    STARTUPINFOW startupInfo        = {};
    startupInfo.cb                  = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};
    if (!CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, workingDir.c_str(), &startupInfo, &processInfo)) {
        return false;
    }
    CloseHandle(processInfo.hThread);
    if (launched && !resultFile.empty()) {
        launched->process    = processInfo.hProcess;
        launched->resultFile = toUtf8(resultFile);
    }
    else { CloseHandle(processInfo.hProcess); }
    return true;
#else
    (void)executable;
    (void)montage;
    (void)arguments;
    (void)launched;
    return false;
#endif
}

Result takeResult(Launched& launched)
{
    Result result;
#if defined(_WIN32)
    if (launched.process) { CloseHandle(static_cast<HANDLE>(launched.process)); }
#endif
    launched.process = nullptr;
    if (launched.resultFile.empty()) { return result; }

    const std::filesystem::path path = std::filesystem::u8path(launched.resultFile);
    launched.resultFile.clear();
    {
        std::ifstream file(path);
        if (file.is_open()) {
            const nlohmann::json json = nlohmann::json::parse(file, nullptr, false);
            if (json.is_object()) {
                result.valid   = true;
                result.file    = json.contains("File") && json["File"].is_string() ? json["File"].get<std::string>() : std::string();
                result.saved   = json.contains("Saved") && json["Saved"].is_boolean() && json["Saved"].get<bool>();
                result.unsaved = json.contains("Unsaved") && json["Unsaved"].is_boolean() && json["Unsaved"].get<bool>();
            }
        }
    }
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return result;
}

int closeAll(const unsigned long timeoutMs)
{
#if defined(_WIN32)
    const std::vector<DWORD> pids = runningInstances();
    std::vector<std::pair<DWORD, HANDLE>> processes;
    for (const DWORD pid : pids) {
        HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
        if (process) { processes.emplace_back(pid, process); }
    }

    // Closing its windows lets Charmeleon release the amplifier on the way out. Keep at it while
    // waiting: a dialog it shows (e.g. "no amplifier, demo mode") closes first, the main window after.
    constexpr DWORD RETRY_MS = 200;
    for (DWORD waited = 0; waited < timeoutMs; waited += RETRY_MS) {
        bool running = false;
        for (const auto& [pid, process] : processes) {
            if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT) {
                running = true;
                forEachWindow(pid, [](HWND hwnd) { PostMessageW(hwnd, WM_CLOSE, 0, 0); });
            }
        }
        if (!running) { break; }
        Sleep(RETRY_MS);
    }

    for (const auto& [pid, process] : processes) {
        if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT) {
            TerminateProcess(process, 1);
            WaitForSingleObject(process, 1000);
        }
        CloseHandle(process);
    }
    return int(pids.size());
#else
    (void)timeoutMs;
    return 0;
#endif
}

}  // namespace Charmeleon
