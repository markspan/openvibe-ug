#ifdef _WIN32

#pragma comment(lib, "comsuppw.lib") // --- Linker Directives ---. Links the COM support library for wide-character BSTR handling.

#include <firewall.h>

// --- Standard Library ---
#include <string>      // For std::wstring to handle Unicode strings.
#include <iostream>    // For console output (std::wcout, std::wcerr) and input (std::wcin).
#include <algorithm>   // For std::transform (used in getProcessNameW).
#include <vector>      // For std::vector (used in argument conversion).

// --- Windows Core and Shell API ---
#include <windows.h>   // Core Windows functions (GetModuleFileNameW, GetLastError, etc.).
#include <shellapi.h>  // For ShellExecuteExW, used to relaunch the app with elevation. Used also for CommandLineToArgvW.

// --- COM and Firewall API ---
#include <netfw.h>     // Main header for Windows Firewall with Advanced Security interfaces (INetFwPolicy2, etc.).
#include <comutil.h>   // For the _bstr_t wrapper, which simplifies handling BSTR strings used by COM.
#include <atlcomcli.h> // For CComPtr, the smart pointer class that simplifies COM object lifetime management.

// --- Constants ---
#define MAX_PATH_LENGTH_W 32768
const std::wstring installRuleArg = L"/installrule";

// ------------------------------------------------------------------------------------------------
// --- UTILITY FUNCTIONS ---
// ------------------------------------------------------------------------------------------------

/**
 * @brief Retrieves the filename of the current executable (process name) using wide characters.
 *
 * This function gets the full path, extracts the filename, and optionally removes
 * the ".exe" extension based on the optional parameter.
 *
 * @param removeExtension Optional boolean. If true (default), the ".exe" extension is removed.
 * @return std::wstring The filename (e.g., "my_app.exe" or "my_app").
 * Returns an empty string (L"") on failure.
 */
static std::wstring getProcessNameW(bool removeExtension = true)
{
    wchar_t buffer[MAX_PATH_LENGTH_W];
    DWORD length = 0;

    // 1. Get the full path name of the executable file
    length = GetModuleFileNameW(
        NULL,                            // hModule: NULL = current process
        buffer,                          // lpFilename: Pointer to buffer
        MAX_PATH_LENGTH_W                // nSize: Size of buffer (in wchar_t units)
    );

    if (length == 0 || length >= MAX_PATH_LENGTH_W)
    {
        return L"";
    }

    std::wstring fullPathName(buffer, length);

    // 2. Extract the filename from the full path
    size_t lastSeparator = fullPathName.find_last_of(L'\\');
    std::wstring fileName = (lastSeparator != std::wstring::npos)
        ? fullPathName.substr(lastSeparator + 1)
        : fullPathName;

    // 3. Conditional extension removal
    if (removeExtension)
    {
        const std::wstring extension = L".exe";

        if (fileName.length() > extension.length())
        {
            size_t extensionPos = fileName.length() - extension.length();

            // Compare the actual extension (case-insensitively)
            std::wstring actualExtension = fileName.substr(extensionPos);
            std::transform(actualExtension.begin(), actualExtension.end(), actualExtension.begin(), ::tolower);

            if (actualExtension == extension)
            {
                // Return the substring before the extension
                return fileName.substr(0, extensionPos);
            }
        }
    }

    return fileName;
}

/**
 * @brief Combines command-line arguments into a single string, suitable for ShellExecuteExW.
 * * Arguments containing spaces are enclosed in quotes.
 * * @param argc The argument count (from wmain).
 * @param argv The argument vector (from wmain).
 * @param skipInstallRule If true, skips the /installrule argument (expected at index 1).
 * @return std::wstring The combined argument string.
 */
std::wstring buildCommandLine(int argc, wchar_t* argv[], bool skipInstallRule)
{
    std::wstring commandLine;
    bool isFirstArg = true;

    // Start from index 1 (after the executable path)
    for (int i = 1; i < argc; ++i)
    {
        // Skip the /installrule argument if required
        if (skipInstallRule && i == 1 && wcscmp(argv[i], installRuleArg.c_str()) == 0)
        {
            continue;
        }

        // Add a space separator if not the first argument being added
        if (!isFirstArg)
        {
            commandLine += L" ";
        }

        // Enclose the argument in quotes if it contains spaces
        std::wstring arg = argv[i];
        if (arg.find(L' ') != std::wstring::npos)
        {
            commandLine += L"\"" + arg + L"\"";
        }
        else
        {
            commandLine += arg;
        }

        isFirstArg = false;
    }

    return commandLine;
}

/**
 * @brief Relaunches the application with administrator privileges or the current user.
 * @param[in] commandLineArgument The argument to pass to the new process.
 * @param[in] asAdmin If true, relaunches with "runas" (UAC prompt). If false, relaunches normally.
 */
void relaunchApp(const std::wstring& commandLineArgument, bool asAdmin)
{
    wchar_t szPath[MAX_PATH_LENGTH_W];
    if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath)))
    {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = asAdmin ? L"runas" : NULL; // Use "runas" for UAC elevation, NULL for normal launch
        sei.lpFile = szPath;
        sei.lpParameters = commandLineArgument.c_str();
        sei.hwnd = NULL;
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (!ShellExecuteExW(&sei))
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_CANCELLED)
            {
                std::wcout << L"User cancelled the elevation request." << std::endl;
            }
            else
            {
                std::wcerr << L"ShellExecuteEx failed with error: " << dwError << std::endl;
            }
        }
    }
}


// ------------------------------------------------------------------------------------------------
// --- FIREWALL FUNCTIONS ---
// ------------------------------------------------------------------------------------------------

/**
 * @brief Checks if a firewall rule with a specific name already exists.
 * @param[in] ruleName The name of the firewall rule to check.
 * @return true if the rule exists, false otherwise.
 */
bool doesFirewallRuleExist(const std::wstring& ruleName)
{
    bool ruleExists = false;
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    if (SUCCEEDED(hr))
    {
        CComPtr<INetFwPolicy2> pFwPolicy2 = nullptr;
        hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pFwPolicy2);

        if (SUCCEEDED(hr))
        {
            CComPtr<INetFwRules> pFwRules = nullptr;
            hr = pFwPolicy2->get_Rules(&pFwRules);

            if (SUCCEEDED(hr))
            {
                CComPtr<INetFwRule> pFwRule = nullptr;
                if (pFwRules->Item(_bstr_t(ruleName.c_str()), &pFwRule) == S_OK)
                {
                    ruleExists = true;
                }
            }
        }
    }
    CoUninitialize();

    return ruleExists;
}

/**
 * @brief Deletes a firewall rule with a specific name if it exists.
 * @param[in] ruleName The name of the firewall rule to delete.
 * @return HRESULT S_OK on success (or if rule was not found), otherwise an error code.
 */
HRESULT deleteFirewallRule(const std::wstring& ruleName)
{
    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    if (SUCCEEDED(hr))
    {
        CComPtr<INetFwPolicy2> pFwPolicy2 = nullptr;
        hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pFwPolicy2);

        if (SUCCEEDED(hr))
        {
            CComPtr<INetFwRules> pFwRules = nullptr;
            hr = pFwPolicy2->get_Rules(&pFwRules);

            if (SUCCEEDED(hr))
            {
                hr = pFwRules->Remove(_bstr_t(ruleName.c_str()));

                if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                {
                    hr = S_OK; // Logical success if rule was already missing
                }
            }
        }        
    }
    CoUninitialize();

    return hr;
}


/**
 * @brief Adds a firewall rule for the current application.
 * * First deletes any existing rule with the same name, then creates the new rule.
 * * @param[in] ruleName The name for the new firewall rule.
 * @return HRESULT S_OK on success, otherwise an error code.
 * @note This function requires administrator privileges to succeed.
 */
HRESULT createFirewallRule(const std::wstring& ruleName)
{
    // Ensure rule is updated by deleting the old one first
    deleteFirewallRule(ruleName);

    HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    if (SUCCEEDED(hr))
    {
        CComPtr<INetFwPolicy2> pFwPolicy2 = nullptr;
        hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pFwPolicy2);

        if (SUCCEEDED(hr))
        {
            CComPtr<INetFwRules> pFwRules = nullptr;
            hr = pFwPolicy2->get_Rules(&pFwRules);

            if (SUCCEEDED(hr))
            {
                CComPtr<INetFwRule> pFwRule = nullptr;
                hr = CoCreateInstance(__uuidof(NetFwRule), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule);

                if (SUCCEEDED(hr))
                {
                    wchar_t modulePath[MAX_PATH_LENGTH_W];
                    if (GetModuleFileNameW(NULL, modulePath, MAX_PATH_LENGTH_W) != 0)
                    {
                        // Populate the rule's properties
                        pFwRule->put_Name(_bstr_t(ruleName.c_str()));
                        pFwRule->put_Description(_bstr_t(L"Allows network access for the application."));
                        pFwRule->put_ApplicationName(_bstr_t(modulePath));
                        pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
                        pFwRule->put_Direction(NET_FW_RULE_DIR_IN);
                        pFwRule->put_Enabled(VARIANT_TRUE);
                        pFwRule->put_Action(NET_FW_ACTION_ALLOW);

                        // Add the rule to the firewall
                        hr = pFwRules->Add(pFwRule);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }
        }
    }
    CoUninitialize();

    return hr;
}

// ------------------------------------------------------------------------------------------------
// --- API IMPLEMENTATION (Surcharges) ---
// ------------------------------------------------------------------------------------------------
int EnsureFirewallRuleW(int argc, wchar_t* argv[])
{
    auto firewallRuleName = getProcessNameW();
    
    if (argc > 1 && wcscmp(argv[1], installRuleArg.c_str()) == 0) // SCENARIO 1: The application is launched elevated to perform the admin task (Instance 2).
    {
        std::wcout << L"Running in elevated mode (Process 2). Messages are NOT displayed on the console." << std::endl;
        system("pause");

        HRESULT hr = createFirewallRule(firewallRuleName);

        if (SUCCEEDED(hr))
        {
            std::wstring originalArgs = buildCommandLine(argc, argv, true);
            relaunchApp(originalArgs, false);

            return 0; // Exit admin elevated instance
        }
        else
        {
            return 1;
        }
    }    
    else // SCENARIO 2: The application is launched normally (Instance 1 or 3).
    {
        if (!doesFirewallRuleExist(firewallRuleName))
        {
            std::wcout << L"Firewall rule is missing. Requesting Administrator approval to add it..." << std::endl;

            std::wstring originalArgs = buildCommandLine(argc, argv, false);
            std::wstring elevatedArgs = installRuleArg;

            if (!originalArgs.empty())
            {
                elevatedArgs += L" " + originalArgs;
            }

            relaunchApp(elevatedArgs, true);
            std::wcout << L"Exiting (Process 1) to allow the elevated process (Process 2) to run." << std::endl;

            return 0; // Exit Instance 1
        }
        else
        {
            return -1; //Rule exist. Application should CONTINUE
        }
    }
}

int EnsureFirewallRule(void)
{
    int argc = 0;
    wchar_t** argv_w = NULL;

    LPWSTR commandLine = GetCommandLineW();

    argv_w = CommandLineToArgvW(commandLine, &argc);
    if (argv_w == NULL) { return 1; /* Error retrieving command line arguments */ }

    int result = EnsureFirewallRuleW(argc, argv_w);
    LocalFree(argv_w);

    return result;
}

int EnsureFirewallRule(int argc, char* argv[])
{
    std::vector<std::wstring> argv_w_strings;
    std::vector<wchar_t*> argv_w(argc); 

    for (int i = 0; i < argc; ++i)
    {
        int requiredSize = MultiByteToWideChar(CP_ACP, 0, argv[i], -1, NULL, 0);
        if (requiredSize > 0)
        {
            std::wstring wstr(requiredSize - 1, L'\0');
            MultiByteToWideChar(CP_ACP, 0, argv[i], -1, &wstr[0], requiredSize);

            argv_w_strings.push_back(std::move(wstr));
        }
    }

    for (int i = 0; i < argv_w_strings.size(); ++i)
    {
        argv_w[i] = &argv_w_strings[i][0];
    }

    return EnsureFirewallRuleW(argv_w.size(), argv_w.data());
}

#endif // _WIN32
