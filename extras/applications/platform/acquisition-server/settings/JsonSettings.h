#pragma once

// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include "ISettingsProvider.h"
#include <filesystem>
#include <nlohmann/json.hpp> // Requires nlohmann/json library
#include <optional>

/**
 * @brief Concrete implementation of ISettingsProvider using JSON format.
 *
 * Handles file I/O in user-specific configuration directories
 * (AppData on Windows, .config on Linux, Application Support on macOS).
 */
class JsonSettings : public ISettingsProvider
{
public:
    /// Name of the file the settings are stored in, inside the directory given to setBasePath().
    static constexpr const char* DEFAULT_FILE_NAME = "settings.json";

    /**
     * @brief Default constructor.
     */
    JsonSettings();

    /**
     * @brief Destructor.
     */
    ~JsonSettings() override = default;

    /**
     * @brief Loads settings from the JSON file.
     *
     * If the file or directory does not exist, it attempts to create them.
     *
     * @return true on success, false on failure.
     */
    bool load() override;


    /**
     * @brief Saves the current in-memory settings to the JSON file.
     *
     * @return true on success, false on failure.
     */
    bool save() const override;

    /**
     * @brief Sets the application name to define the storage folder.
     *
     * @param appName The name of the application subdirectory.
     */
    void setAppName(const std::string& appName) override;

    /**
     * @brief Sets the base path for settings storage.
     *
     * @param settingPath The base path where settings will be stored.
     */
    void setBasePath(const std::string& settingPath) override;

    /**
     * @brief Retrieves a value associated with a key.
     *
     * @tparam T The type of the value to retrieve.
     * @param key The key string in the JSON object.
     * @param defaultValue The value to return if the key is missing.
     * @return The value from JSON or the default value.
     */
    template <typename T>
    T getValue(const std::string& key, const T& defaultValue) const
    {
        if (m_jsonData.contains(key))
        {
            try
            {
                return m_jsonData[key].get<T>();
            }
            catch (...)
            {
                // Type mismatch or conversion error
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief Sets or updates a value for a given key.
     *
     * @tparam T The type of the value to store.
     * @param key The key string.
     * @param value The value to store.
     */
    template <typename T>
    void setValue(const std::string& key, const T& value)
    {
        m_jsonData[key] = value;
    }



    /**
     * @brief Retrieves a whole top level section of the settings.
     *
     * @param name The key of the section.
     * @return The section, or a null json object when the settings hold no such key.
     */
    nlohmann::json getSection(const std::string& name) const override
    {
        return m_jsonData.contains(name) ? m_jsonData[name] : nlohmann::json();
    }

    /**
     * @brief Sets or replaces a whole top level section of the settings.
     *
     * The other sections are left untouched, so a section this application does not know about
     * survives a save.
     *
     * @param name The key of the section.
     * @param section The section contents.
     */
    void setSection(const std::string& name, const nlohmann::json& section) override { m_jsonData[name] = section; }

    /**
     * @brief Indicates whether the application should run in headless mode.
     *
     * @return true if headless mode is enabled, false otherwise.
     */
    bool headlessMode() const override
    {
        return getValue<bool>("headlessMode", false);
    }

private:
    /**
     * @brief Resolves the full path to the settings file based on the OS.
     *
     * @return std::filesystem::path The full path to the json file.
     */
    std::filesystem::path getFilePath() const;

    std::string m_appName;         ///< Name of the application for folder creation.
    std::string m_basePath;        ///< Directory holding the file, empty to derive it from the OS.
    std::string m_filename;        ///< Name of the json file (default: settings.json).
    nlohmann::json m_jsonData;     ///< In-memory representation of the JSON data.

private:

    /**
     * @brief Checks if the directory for the given file path exists, and creates it if not.
     *
     * @param filePath The full file path whose directory is to be checked/created.
     * @return true if the directory exists or was created successfully, false otherwise.
     */
    bool checkAndCreateDirectory(std::filesystem::path& filePath) const;
};
