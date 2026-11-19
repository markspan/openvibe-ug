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
#include <QWinEventNotifier>

#include <Windows.h>
#include <TlHelp32.h>
#endif

namespace Charmeleon {

namespace {

#if defined(_WIN32)

const wchar_t* const EXE_NAME = L"Charmeleon.exe";

/**
 * @brief @p argument as one argument on a Windows command line, quoted when it has to be.
 *
 * By the rules CommandLineToArgvW and the C runtime read a command line with: inside quotes, backslashes are literal
 * except before a quote, so the backslashes before a quote and at the end are doubled, and a quote is escaped. A plain
 * pair of quotes around "C:\montages\" would otherwise end in an escaped quote.
 */
std::wstring quoteArgument(const std::wstring& argument)
{
    if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) { return argument; }
    std::wstring quoted = L"\"";
    for (auto it = argument.begin();; ++it) {
        size_t backslashes = 0;
        while (it != argument.end() && *it == L'\\') {
            ++it;
            ++backslashes;
        }
        if (it == argument.end()) {
            quoted.append(2 * backslashes, L'\\');
            break;
        }
        quoted.append(*it == L'"' ? 2 * backslashes + 1 : backslashes, L'\\');
        quoted.push_back(*it);
    }
    quoted.push_back(L'"');
    return quoted;
}

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
        return isRegularFile(configured) ? pathToUtf8(configured) : std::string();
    }

#if defined(_WIN32)
    // Where its installer put it; the montage reader looks for Charmeleon's montage library there as well
    const std::filesystem::path installed = std::filesystem::u8path(charmeleonInstallFolder());
    if (!installed.empty() && isRegularFile(installed / EXE_NAME)) { return pathToUtf8(installed / EXE_NAME); }

    wchar_t programFiles[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"ProgramFiles", programFiles, MAX_PATH) > 0) {
        const std::filesystem::path fallback = std::filesystem::path(programFiles) / L"Charmeleon" / EXE_NAME;
        if (isRegularFile(fallback)) { return pathToUtf8(fallback); }
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
    std::wstring commandLine      = quoteArgument(exe.wstring());
    // Absolute, as Charmeleon runs from another directory
    if (!montage.empty()) {
        std::error_code ec;
        const std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(montage), ec);
        commandLine += L" " + quoteArgument((ec ? std::filesystem::u8path(montage) : path).wstring());
    }

    // The amplifier the driver uses, so that Charmeleon does not open another one
    for (const auto& argument : arguments) { commandLine += L" " + quoteArgument(std::filesystem::u8path(argument).wstring()); }

    // Charmeleon saves the montage when Play closes it, and says which file it ended with, so that the
    // driver can take the edits over. Not ".json", which a Charmeleon before 2.2 would take for the montage.
    std::filesystem::path resultFile;
    if (launched) {
        std::error_code ec;
        const std::filesystem::path temp = std::filesystem::temp_directory_path(ec);
        if (!ec) {
            resultFile = temp / (L"charmeleon-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L".result");
            commandLine += L" --save-on-close --result " + quoteArgument(resultFile.wstring());
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
        launched->resultFile = pathToUtf8(resultFile);
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

Watcher::Watcher(const Launched& launched, QObject* parent) : QObject(parent)
{
#if defined(_WIN32)
    if (!launched.process) { return; }
    m_notifier = new QWinEventNotifier(static_cast<HANDLE>(launched.process), this);
    connect(m_notifier, &QWinEventNotifier::activated, this, &Watcher::ended);
#else
    (void)launched;
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
    // Each round waits on the processes themselves, so that it ends as soon as they have all gone,
    // rather than sleeping its full length
    constexpr DWORD RETRY_MS = 200;
    std::vector<HANDLE> handles;
    for (const auto& entry : processes) { handles.push_back(entry.second); }
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    while (!handles.empty() && GetTickCount64() < deadline) {
        for (const auto& [pid, process] : processes) {
            if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT) { forEachWindow(pid, [](HWND hwnd) { PostMessageW(hwnd, WM_CLOSE, 0, 0); }); }
        }
        // At most MAXIMUM_WAIT_OBJECTS (64) handles at once; more Charmeleons than that are not waited for here
        const DWORD count = DWORD((std::min)(handles.size(), size_t(MAXIMUM_WAIT_OBJECTS)));
        if (WaitForMultipleObjects(count, handles.data(), TRUE, RETRY_MS) != WAIT_TIMEOUT) { break; }
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
