// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

#include <CInitApp.h>

// Standard Library
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// Project Headers
#include <openvibe/Services/IService.h>
#include <ISettingsProvider.h>
#include <JsonSettings.h>
#include <AbstractDriverBuilder.h>
#include <AbstractWriterBuilder.h>
#include <system/WindowsUtilities.h>
#include <openvibe/parameters/parameters.h>
#include <ApplicationPathHandling.h>
#include <CNetAcqControlAPI.h>
#include <Api.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Private class for p-impl
//////////////////////////////////////////////////////////////////////////////////////////////////
class CInitAppPrivate
{
public:
    // Status
    bool headlessMode = false;
    bool isPortableMode = false;

    // Configuration & Paths
    std::string appName;
    std::string appDataPath;      
    std::string systemConfigPath; 
    std::string driversPath;      
    std::string writersPath;      
    std::string servicesPath;     

    // Services Data
    std::list<std::string> services;
    std::map<std::string, std::list<OpenViBE::Parameters::ParameterTraits *>> servicesConfiguration;
    std::map<std::string, OpenViBE::Services::IService*> loadedServices;

    // Plugin Handles & Failure Tracking
    std::map<std::string,            OSAL::dlib> servicesLoaded;
    std::map<AbstractDriverBuilder*, OSAL::dlib> driversLoaded;
    std::map<AbstractWriterBuilder*, OSAL::dlib> writersLoaded;

    ~CInitAppPrivate()
    {
        for (auto& [_, svc] : loadedServices) delete svc;
        for (auto& [_, h] : servicesLoaded)   OSAL::freeLibrary(h);
        for (auto& [_, h] : driversLoaded)    OSAL::freeLibrary(h);
        for (auto& [_, h] : writersLoaded)    OSAL::freeLibrary(h);
    }

    std::list<std::string>                 servicesFailed;
    std::list<std::string>                 driversFailed;
    std::list<std::string>                 writersFailed;
};





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// --- Lifecycle --- Constructor / Destructor
//////////////////////////////////////////////////////////////////////////////////////////////////
CInitApp::CInitApp() : d(std::make_unique<CInitAppPrivate>())
{
    // Api::instance(); // protected so cannot be called here.
    resolvePaths();
}

CInitApp::CInitApp(int argc, char const** argv) : d(std::make_unique<CInitAppPrivate>())
{
    //TODO: Process command line arguments if needed
    resolvePaths();
}

CInitApp::~CInitApp()
{
}

/**
 * @brief Resolves application paths based on the execution mode (Portable or Installed).
 *
 * This function determines the executable directory, detects if the application
 * run in portable mode, and sets up the configuration, driver, writer,
 * and service paths accordingly. On Windows, it also registers these paths
 * with the DLL loader to allow dynamic loading of dependencies (using AddDllDirectory).
 */
void CInitApp::resolvePaths()
{
    d->appName = getExecutablePath().stem().string();

    fs::path exeDir = getExecutableDir();
    d->isPortableMode = detectPortableMode(exeDir);

    if (d->isPortableMode)
    {
        std::cout << "[Init] Portable Mode detected." << std::endl;
        d->appDataPath = exeDir.string();
        d->systemConfigPath = exeDir.string();
        d->driversPath = (exeDir / "drivers").string();
        d->writersPath = (exeDir / "writers").string();
        d->servicesPath = (exeDir / "services").string();
    }
    else
    {
        std::cout << "[Init] Installed Mode detected." << std::endl;
        fs::path userConfigBase;
        resolveOsDependentPaths(userConfigBase, exeDir);
        d->appDataPath = (userConfigBase / d->appName).string();

        if (!fs::exists(d->appDataPath)) 
        {
            try { fs::create_directories(d->appDataPath); }
            catch (const std::exception& e) { std::cerr << "[Init] Error creating AppData directory: " << e.what() << std::endl; }
        }

        d->systemConfigPath = exeDir.string();
        d->driversPath = (exeDir / "drivers").string();
        d->writersPath = (exeDir / "writers").string();
        d->servicesPath = (exeDir / "services").string();
    }

#ifdef _WIN32
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
    if (!d->driversPath.empty())  AddDllDirectory(System::WindowsUtilities::convertUtf8ToWide(OSAL::normalizedPath(d->driversPath)).c_str());
    if (!d->writersPath.empty())  AddDllDirectory(System::WindowsUtilities::convertUtf8ToWide(OSAL::normalizedPath(d->writersPath)).c_str());
    if (!d->servicesPath.empty()) AddDllDirectory(System::WindowsUtilities::convertUtf8ToWide(OSAL::normalizedPath(d->servicesPath)).c_str());
#endif
}

std::string CInitApp::getAppName() const
{
    return d->appName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// --- Configuration Accessors ---
//////////////////////////////////////////////////////////////////////////////////////////////////
std::string CInitApp::getSettingsFileName() const
{
    return (fs::path(d->appDataPath) / "AcquisitionServerAppSettings.json").string();
}

std::string CInitApp::getConfigurationPath() const
{
    return (fs::path(d->appDataPath) / "config").string();
}

std::string CInitApp::getWriterFolderPath() const
{
    return d->writersPath;
}

std::string CInitApp::getDriverFolderPath() const
{
    return d->driversPath;
}

std::string CInitApp::getServiceFolderPath() const
{
    return d->servicesPath;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
// --- Loaders ---
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
* @brief Loads application settings using a JSON settings provider.
* This function creates a JsonSettings instance, loads the settings,
* and retrieves service parameters and headless mode configuration.
* @return true if settings are loaded successfully, false otherwise.
*/
bool CInitApp::loadSettings()
{
    bool bRes = false;

    ISettingsProvider* setting = new JsonSettings();
    setting->setAppName(d->appName);
    setting->setBasePath(d->servicesPath);

    bRes = setting->load();

    d->servicesConfiguration = setting->getServiceParameters();
    d->headlessMode = setting->headlessMode();
    
    delete setting;
    
    return bRes;
}

/**
* @brief Loads services based on configuration and available files.
* This function first loads explicitly configured services,
* then loads any additional services found in the service directory.
* Finally, it initializes the loaded services with their parameters.
* @return true if all services are loaded successfully, false otherwise.
*/
bool CInitApp::loadServices()
{
    bool bRes = true;

    auto availablesServicesFiles = removeExt(findFiles(getServiceFolderPath(), DLL_EXTENSION), DLL_EXTENSION);


    for (const auto& serviceName : d->services)    // 1. Load explicit services from configuration
    {
        loadSingleService(serviceName, false);
    }
    
    for (const auto& servicePath : availablesServicesFiles)    // 2. Load implicit services from directory
    {
        if (!d->servicesLoaded.contains(servicePath))
        {
            loadSingleService(fs::path(servicePath).filename().string(), true);
        }
    }

    initializeServices();

    CNetAcqControlAPI::getInstance();

    return bRes;
}

/**
* @brief Loads drivers and writers by invoking their respective loading functions.
* This function combines the results of loading drivers and writers.
* @return true if both drivers and writers are loaded successfully, false otherwise.
*/
bool CInitApp::loadDriversAndWriters()
{
    bool bRes = true;

    bRes &= loadPlugins<AbstractDriverBuilder>(getDriverFolderPath(), d->driversLoaded, d->driversFailed);
    bRes &= loadPlugins<AbstractWriterBuilder>(getWriterFolderPath(), d->writersLoaded, d->writersFailed);

    return bRes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// --- Runtime Access ---
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
* @brief Return the headles mode boolean.
* @return true if the application must run in headless mode, false otherwise.
*/
bool CInitApp::isHeadlessMode() const
{
    return d->headlessMode;
}

OpenViBE::Services::IService* CInitApp::getServices(const std::string serviceName)
{
    OpenViBE::Services::IService* serviceRes = nullptr;

    if (d->loadedServices.contains(serviceName))
    {
        serviceRes = d->loadedServices[serviceName];
    }

    return serviceRes;
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// --- Internal Plugin Logic for Drivers and Writers (Template Implementations) --- 
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Loads plugins of a specific type from a directory.
 *
 * Iterates through the specified directory and attempts to load each file
 * with the defined DLL extension as a plugin. It delegates the loading
 * of individual files to the loadOnePlugin method.
 *
 * @tparam BuilderT The builder type (AbstractDriverBuilder or AbstractWriterBuilder).
 * @param directoryPath The path to the directory containing the DLLs.
 * @param loadedMap Reference to the map where loaded builders and their handles are stored.
 * @param failedList Reference to the list where failed plugin names are stored.
 * @return true if at least one pluging is correctly loaded, or if no plugin are present into the specified folder. False in other cases.
 */
template <typename BuilderT>
bool CInitApp::loadPlugins(const std::string& directoryPath, std::map<BuilderT*, OSAL::dlib>& loadedMap, std::list<std::string>& failedList)
{
    int pluginCount = 0;
    int pluginLoadedCount = 0;

    if (fs::exists(directoryPath) && fs::is_directory(directoryPath))
    {
        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == DLL_EXTENSION)
            {
                pluginCount++;
                if (loadOnePlugin<BuilderT>(entry.path().string(), failedList, loadedMap))
                {
                    pluginLoadedCount++;
                }
            }
        }
    }
    else
    {
        std::cerr << "Directory skipped (not found): " << directoryPath << std::endl;
    }

    std::cout << "Loaded " << pluginLoadedCount << "/" << pluginCount << " plugins from " << directoryPath << std::endl;

    return (pluginCount > 0 && pluginLoadedCount > 0) || pluginCount == 0;
}

/**
 * @brief Loads a single plugin file.
 *
 * This function loads the dynamic library, retrieves the factory instance,
 * creates the builder, and registers it with the AcquisitionManager using
 * C++20 'if constexpr' to handle type-specific registration.
 *
 * @tparam BuilderT The builder type (AbstractDriverBuilder or AbstractWriterBuilder).
 * @param entry The filesystem entry corresponding to the plugin file.
 * @param failedList Reference to the list where failed plugin names are stored.
 * @param loadedMap Reference to the map where loaded builders are stored.
 * @return True if the plugin was successfully loaded and registered, false otherwise.
 */
template<typename BuilderT>
bool CInitApp::loadOnePlugin(const std::string& entry, std::list<std::string>& failedList, std::map<BuilderT*, OSAL::dlib>& loadedMap)
{
    bool bRes = false;

    std::string fileName = fs::path(entry).stem().string();
    OSAL::dlib handle = OSAL::loadLibrary(entry);
    if (handle)
    {
        auto* instanceGetter = reinterpret_cast<BuilderT*(*)()>(OSAL::getProcAddr(handle, "getInstance"));
        if (instanceGetter)
        {
            auto* builder = instanceGetter();
            if (builder)
            {
                if constexpr (std::is_same_v<BuilderT, AbstractDriverBuilder>)
                    bRes = AcquisitionManager::instance().registerDriver(builder);
                else if constexpr (std::is_same_v<BuilderT, AbstractWriterBuilder>)
                    bRes = AcquisitionManager::instance().registerWriter(builder);

                if (bRes)
                {
                    loadedMap[builder] = handle;
                    std::cout << "Loaded " << fileName << " as: " << builder->getName() << std::endl;
                }
                else
                {
                    OSAL::freeLibrary(handle);
                    failedList.push_back(fileName);
                    std::cerr << "Registration failed (Unknown Builder Type or Duplicate) for: " << fileName << std::endl;
                }
            }
            else
            {
                OSAL::freeLibrary(handle);
                failedList.push_back(fileName);
                std::cerr << "Failed to get builder instance for: " << fileName << std::endl;
            }
        }
        else
        {
            OSAL::freeLibrary(handle);
            failedList.push_back(fileName);
            std::cerr << "Failed to find 'getInstance' in: " << fileName << std::endl;
        }
    }
    else
    {
        failedList.push_back(fileName);
        std::cerr << "Failed to load library: " << fileName << std::endl;
    }

    return bRes;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
// --- Internal Service Logic ---
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
* @brief Loads a single service by its name.
* This function attempts to load the service's dynamic library,
* retrieve the service instance, and store it in the loaded services map.
* @param serviceName The name of the service to load.
* @param isExtra Indicates if the service is an extra (not explicitly configured) service.
*/
void CInitApp::loadSingleService(const std::string& serviceName, bool isExtra)
{
    std::string fullPath = getServiceFolderPath() + "\\" + serviceName + DLL_EXTENSION;

    OSAL::dlib handle = OSAL::loadLibrary(fullPath);
    if (handle)
    {
        auto* getInstance = reinterpret_cast<OpenViBE::Services::IService*(*)()>(OSAL::getProcAddr(handle, "getServiceInstance"));
        if (getInstance)
        {
            OpenViBE::Services::IService* service = getInstance();
            if (service)
            {
                d->loadedServices[serviceName] = service;
                d->servicesLoaded[serviceName] = handle;
                std::cout << "Loaded " << (isExtra ? "extra " : "") << "service: " << serviceName << std::endl;
            }
            else
            {
                OSAL::freeLibrary(handle);
                d->servicesFailed.push_back(serviceName);
                std::cerr << "Failed to get service instance for: " << serviceName << std::endl;
            }
        }
        else
        {
            OSAL::freeLibrary(handle);
            if (!isExtra)
            {
                d->servicesFailed.push_back(serviceName);
                std::cerr << "Failed to find getServiceInstance in service: " << serviceName << std::endl;
            }
        }
    }
    else
    {
        d->servicesFailed.push_back(serviceName);
        std::cerr << "Failed to load service: " << serviceName << std::endl;
    }
}

/**
* @brief Unloads all loaded services by freeing their resources.
* This function iterates through the loaded services, deletes their instances,
* and frees the associated dynamic libraries.
* @return true if all services are unloaded successfully, false otherwise.
*/
bool CInitApp::unloadServices()
{
    bool bRes = true;

    for (auto& service : d->services)
    {
        if (d->loadedServices.contains(service))
        {
            delete d->loadedServices[service];
            d->loadedServices.erase(service);
        }
        if (d->servicesLoaded.contains(service))
        {
            OSAL::freeLibrary(d->servicesLoaded[service]);
            d->servicesLoaded.erase(service);
        }
    }
    
    return bRes;
}

/**
* @brief Initializes loaded services by updating their parameters from the configuration.
* This function iterates through the loaded services and updates their parameters.
* If a service has stored parameters in the configuration, it updates the service's parameters accordingly.
*/
void CInitApp::initializeServices()
{
    for (auto& service : d->loadedServices)
    {
        if (d->servicesConfiguration.find(service.first) != d->servicesConfiguration.end())
        {
            for (auto& serviceParam : service.second->parameters())
            {
                for (auto& storedParam : d->servicesConfiguration[service.first])
                {
                    if (storedParam->name() == serviceParam->name())
                    {
                        serviceParam->setValue(storedParam->valueAsAny());
                        break;
                    }
                }
            }
        }
    }
}
