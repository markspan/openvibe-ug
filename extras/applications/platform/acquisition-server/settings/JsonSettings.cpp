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

#include <openvibe/parameters/parameters.h>


JsonSettings::JsonSettings()
    : m_appName("DefaultApp")
    , m_filename("settings.json")
    , m_jsonData(nlohmann::json::object())
{
}

void JsonSettings::setAppName(const std::string& appName)
{
    m_appName = appName;
}

void JsonSettings::setBasePath(const std::string& settingPath)
{
    //TODO
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
            }
        }
    }

    return bRes;
}

bool JsonSettings::save() const
{
    bool bRes = true;

    std::filesystem::path filePath = getFilePath();
    bRes = checkAndCreateDirectory(filePath);

    std::ofstream file(filePath);
    bRes = file.is_open();

    if (bRes)
    {
        file << m_jsonData.dump(4);
    }

    return bRes;
}

std::map<std::string, std::list<OpenViBE::Parameters::ParameterTraits *>> JsonSettings::getServiceParameters()
{
    std::map<std::string, std::list<OpenViBE::Parameters::ParameterTraits *>> paramsByServicesRes;

    if (m_jsonData.contains("Services") && m_jsonData["Services"].is_array())
    {
        for (const auto& serviceItem : m_jsonData["Services"])
        {
            if (serviceItem.contains("name") && serviceItem["name"].is_string())
            {
                std::string serviceName = serviceItem["name"].get<std::string>();
                if (serviceItem.contains("params") && serviceItem["params"].is_array())
                {
                    for (const auto& paramItem : serviceItem["params"])
                    {
                        try
                        {
                            paramsByServicesRes[serviceName].push_back(OpenViBE::Parameters::ParameterFactory::create(paramItem));
                        }
                        catch (const nlohmann::json::type_error& e)
                        {
                            std::cerr << "Parameter type error: " << e.what() << std::endl; // Skip invalid parameter                            
                        }
                    }
                }
            }
        }
    }

    return paramsByServicesRes;
}

std::filesystem::path JsonSettings::getFilePath() const
{
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