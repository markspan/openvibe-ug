// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include "ApplicationPathHandling.h"


// Standard Library
#include <iostream>
#include <algorithm>
#include <cstdlib> // For std::getenv



/**
* @brief Custom string start check (Utility).
*/
bool startsWith(const std::string& fullString, const std::string& prefix)
{
    if (fullString.length() < prefix.length()) return false;
    return fullString.compare(0, prefix.length(), prefix) == 0;
}

/**
* @brief Retrieves the full absolute path of the current executable.
*/
fs::path getExecutablePath()
{
    char buf[4096] = { 0 };
#ifdef _WIN32
    GetModuleFileNameA(NULL, buf, 4096);
#elif defined(__APPLE__)
    uint32_t size = sizeof(buf);
    _NSGetExecutablePath(buf, &size);
#else
    ssize_t len = readlink("/proc/self/exe", buf, 4096);
    if (len != -1) buf[len] = '\0';
#endif

    if (buf[0] == 0) return fs::current_path();
    return fs::absolute(fs::path(buf));
}

/**
* @brief Retrieves the directory containing the executable.
*/
fs::path getExecutableDir()
{
    return getExecutablePath().parent_path();
}

/**
* @brief Resolves the OS-specific user configuration directory (AppData/.config).
*/
void resolveOsDependentPaths(fs::path& userConfigBase, const fs::path& exeDir)
{
#ifdef _WIN32
    const char* appData = std::getenv("LOCALAPPDATA");
    if (!appData) appData = std::getenv("APPDATA");
    userConfigBase = appData ? fs::path(appData) : exeDir;
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    userConfigBase = home ? fs::path(home) / "Library/Application Support" : exeDir;
#else 
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig) userConfigBase = fs::path(xdgConfig);
    else {
        const char* home = std::getenv("HOME");
        userConfigBase = home ? fs::path(home) / ".config" : exeDir;
    }
#endif
}

/**
* @brief Logic to detect if portable mode is active.
*/
bool detectPortableMode(const fs::path& exeDir)
{
    if (fs::exists(exeDir / ".portable") || fs::exists(exeDir / "portable.txt")) return true;

    std::string pathStr = exeDir.string();
#ifdef _WIN32
    if (pathStr.find("Program Files") != std::string::npos) return false;
#elif defined(__APPLE__)
    if (startsWith(pathStr, "/Applications") || startsWith(pathStr, "/System") || startsWith(pathStr, "/Library")) return false;
#else 
    if (startsWith(pathStr, "/bin") || startsWith(pathStr, "/usr") || startsWith(pathStr, "/opt") || startsWith(pathStr, "/var")) return false;
#endif
    return true;
}

/**
* @brief Finds files with a specific extension in a directory.
*/
std::vector<std::string> findFiles(const std::string& dirPath, const std::string& fileExt)
{
    std::vector<std::string> matchingFiles;
    const fs::path rootPath(dirPath);

    if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) return matchingFiles;

    std::string targetExt = fileExt;
    if (!targetExt.empty() && targetExt.front() != '.') targetExt = "." + targetExt;

    try
    {
        for (const auto& entry : fs::directory_iterator(rootPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == targetExt)
            {
                matchingFiles.push_back(entry.path().string());
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
    return matchingFiles;
}

/**
* @brief Removes extension from a list of paths.
*/
std::vector<std::string> removeExt(const std::vector<std::string>& files, const std::string& fileExt)
{
    std::vector<std::string> result;

    result.reserve(files.size());
    std::string targetExt = fileExt;
    if (!targetExt.empty() && targetExt.front() != '.') targetExt = "." + targetExt;

    for (const auto& filePath : files)
    {
        fs::path p(filePath);
        if (p.extension() == targetExt) p.replace_extension("");
        result.push_back(p.string() == "" ? filePath : p.string()); // Handles empty result case if needed, or just p.string()
    }

    return result;
}
