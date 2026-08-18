#pragma once


// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <filesystem>

// Project dependencies
#include <openvibe/parameters/parameters.h>
#include <system/OSAL.h>

// Forward declarations
namespace OpenViBE {
namespace Services {
class IService;
}
}
class CInitAppPrivate;

/**
 * @brief Main initialization class for the application.
 *
 * This class handles the setup of the application environment, including:
 * - Path resolution (Portable vs Installed modes).
 * - Loading configuration settings.
 * - Loading and managing plugins (Drivers, Writers, Services).
 */
class CInitApp
{
public:
    // ==================================================================================
    // Lifecycle
    // ==================================================================================
    CInitApp();
    CInitApp(int argc, char const ** argv);
    ~CInitApp();

    // ==================================================================================
    // Configuration Accessors (Paths & Info)
    // ==================================================================================
    /** @brief Returns the detected application name (based on executable name). */
    std::string getAppName()           const;
    
    /** @brief Returns the full path to the settings JSON file. */
    std::string getSettingsFileName()  const;
    
    /** @brief Returns the path to the general configuration folder. */
    std::string getConfigurationPath() const;
    
    /** @brief Returns the folder path containing Writer plugins. */
    std::string getWriterFolderPath()  const;
    
    /** @brief Returns the folder path containing Driver plugins. */
    std::string getDriverFolderPath()  const;
    
    /** @brief Returns the folder path containing Service plugins. */
    std::string getServiceFolderPath() const;

    // ==================================================================================
    // Loaders (Initialization Sequence)
    // ==================================================================================
    /** * @brief Loads application settings from the provider.
     * @return true if settings were successfully loaded.
     */
    bool loadSettings();

    /** * @brief Loads all services (configured and auto-discovered).
     * @return true if services were successfully loaded.
     */
    bool loadServices();

    /** * @brief Loads all Drivers and Writers plugins.
     * @return true if plugins were successfully loaded.
     */
    bool loadDriversAndWriters();

    // ==================================================================================
    // Runtime Access & Status
    // ==================================================================================
    /** @brief Checks if the application is running in headless mode (no GUI). */
    bool isHeadlessMode() const;

    /** 
     * @brief Retrieves a loaded service instance by name.
     * @param serviceName The name of the service to retrieve.
     * @return Pointer to the IService interface, or nullptr if not found.
     */
    OpenViBE::Services::IService* getServices(const std::string serviceName);

private:

    void resolvePaths();

    // ==================================================================================
    // Internal Plugin Loading Logic (Templates)
    // ==================================================================================
    
    /**
     * @brief Generic method to load plugins from a specific directory.
     * @tparam BuilderT The builder type (AbstractDriverBuilder or AbstractWriterBuilder).
     */
    template <typename BuilderT>
    bool loadPlugins(const std::string& directoryPath, std::map<BuilderT*, OSAL::dlib>& loadedMap, std::list<std::string>& failedList);

    /**
     * @brief Loads a single plugin file and registers it.
     * @tparam BuilderT The builder type.
     */
    template<typename BuilderT>
    bool loadOnePlugin(const std::string& entry, std::list<std::string>& failedList, std::map<BuilderT*, OSAL::dlib>& loadedMap);

    // ==================================================================================
    // Internal Service Logic
    // ==================================================================================
    void loadSingleService(const std::string& serviceName, bool isExtra);
    bool unloadServices();
    void initializeServices();

private:
    std::unique_ptr<CInitAppPrivate> d;
};
