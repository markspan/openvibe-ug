// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include "JsonSettings.h"

#include <cstdlib> // for std::getenv
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#ifdef _WIN32
#include <windows.h> // needed for GetEnvironmentVariable if getenv fails
#endif




JsonSettings::JsonSettings()
    : m_appName("DefaultApp")
    , m_filename(DEFAULT_FILE_NAME)
    , m_jsonData(nlohmann::json::object())
{
}

void JsonSettings::setAppName(const std::string& appName)
{
    m_appName = appName;
}

void JsonSettings::setBasePath(const std::string& settingPath)
{
    m_basePath = settingPath;
}

bool JsonSettings::load()
{
    bool bRes = true;

    std::filesystem::path filePath = getFilePath();

    // Check if file exists
    if (!std::filesystem::exists(filePath))
    {
        bRes = save(); // Create an empty or default file
    }
    else
    {
        // Load existing file
        std::ifstream file(filePath);
        if (file.is_open())
        {
            try
            {
                file >> m_jsonData;
            }
            catch (const nlohmann::json::parse_error& e)
            {
                std::cerr << "JSON Parse Error: " << e.what() << std::endl;
                bRes = false;

                // The settings are about to be rewritten from the defaults, so keep the file that
                // could not be read: it is the user's only chance to recover what it held.
                file.close();
                m_jsonData = nlohmann::json::object();

                std::filesystem::path badPath = filePath;
                badPath += ".bad";
                std::error_code ec;
                std::filesystem::rename(filePath, badPath, ec);
                if (ec) { std::cerr << "Could not keep a copy of the unreadable settings: " << ec.message() << std::endl; }
                else { std::cerr << "The unreadable settings were kept as " << badPath.string() << std::endl; }
            }
        }
    }

    return bRes;
}

bool JsonSettings::save() const
{
    std::filesystem::path filePath = getFilePath();
    if (!checkAndCreateDirectory(filePath))
    {
        return false;
    }

    // Write to a sibling temporary file and rename it over the real one, so that a failure half way
    // through leaves the previous settings untouched instead of a truncated file.
    std::filesystem::path tmpPath = filePath;
    tmpPath += ".tmp";

    {
        std::ofstream file(tmpPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            std::cerr << "Could not open " << tmpPath.string() << " to save the settings" << std::endl;
            return false;
        }

        file << m_jsonData.dump(4);
        file.flush();
        if (!file.good())
        {
            std::cerr << "Could not write the settings to " << tmpPath.string() << std::endl;
            file.close();
            std::error_code discard;
            std::filesystem::remove(tmpPath, discard);
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, filePath, ec);
    if (ec)
    {
        std::cerr << "Could not replace " << filePath.string() << ": " << ec.message() << std::endl;
        std::error_code discard;
        std::filesystem::remove(tmpPath, discard);
        return false;
    }

    return true;
}



std::filesystem::path JsonSettings::getFilePath() const
{
    // When the caller told us where to store the settings, that decision wins: it already accounts
    // for the portable-versus-installed mode the application resolved at startup.
    if (!m_basePath.empty())
    {
        return std::filesystem::path(m_basePath) / m_filename;
    }

    std::filesystem::path basePath;

    // 1. Windows implementation
#ifdef _WIN32
    // Use LOCALAPPDATA for local settings (not roaming)
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (localAppData)
    {
        basePath = std::filesystem::path(localAppData);
    }
    else
    {
        // Fallback if getenv fails (rare on modern Windows)
        basePath = std::filesystem::current_path();
    }

    // 2. Linux implementation
#elif defined(__linux__)
    // Check XDG_CONFIG_HOME first, then fallback to ~/.config
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig)
    {
        basePath = std::filesystem::path(xdgConfig);
    }
    else
    {
        const char* home = std::getenv("HOME");
        if (home)
        {
            basePath = std::filesystem::path(home) / ".config";
        }
        else
        {
            basePath = std::filesystem::current_path();
        }
    }

    // 3. macOS implementation
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home)
    {
        basePath = std::filesystem::path(home) / "Library" / "Application Support";
    }
    else
    {
        basePath = std::filesystem::current_path();
    }

#else
    // Unknown OS, use current directory
    basePath = std::filesystem::current_path();
#endif

    return basePath / m_appName / m_filename;
}



bool JsonSettings::checkAndCreateDirectory(std::filesystem::path& filePath) const
{
    bool bRes = true;

    if (!std::filesystem::exists(filePath.parent_path()))
    {
        try
        {
            std::filesystem::create_directories(filePath.parent_path());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            bRes = false;
        }
    }

    return bRes;
}
