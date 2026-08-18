#pragma once

// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include <list>
#include <map>
#include <string>

#include <openvibe/parameters/parameters.h>

/**
 * @brief Interface for settings management.
 *
 * Defines the contract for loading and saving application settings
 * regardless of the underlying storage format (JSON, XML, Ini, etc.).
 */
class ISettingsProvider
{
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~ISettingsProvider() = default;

    /**
     * @brief Loads the settings from the storage medium.
     *
     * @return true if loading was successful, false otherwise.
     */
    virtual bool load() = 0;

    /**
     * @brief Saves the current settings to the storage medium.
     *
     * @return true if saving was successful, false otherwise.
     */
    virtual bool save() const = 0;

    /**
     * @brief Sets the name of the application folder.
     *
     * This is used to determine the storage path (e.g., AppData/Local/appName).
     *
     * @param appName The name of the application.
     */
    virtual void setAppName(const std::string& appName) = 0;

    /**
     * @brief Sets the base path for settings storage.
     *
     * @param settingPath The base path where settings will be stored.
     */
    virtual void setBasePath(const std::string& settingPath) = 0;

    /**
     * @brief Retrieves the parameters for a specific service.
     *
     * @return A map of service name to Parameter objects.
     */
    virtual std::map<std::string, std::list<OpenViBE::Parameters::ParameterTraits*>> getServiceParameters() = 0;

    /**
     * @brief Indicates whether the application should run in headless mode.
     *
     * @return true if headless mode is enabled, false otherwise.
     */
    virtual bool headlessMode() const { return false; }
};
