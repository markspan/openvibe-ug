#include "AcquisitionManager.h" ///< Include the header file for the AcquisitionManager class.

#include "Acquisition.h" ///< Include the header file for the Acquisition class.
#include "AbstractDriver.h" ///< Include the header file for the AbstractDriver class.
#include "AbstractDriverBuilder.h" ///< Include the header file for the AbstractDriverBuilder class.
#include "AbstractWriter.h" ///< Include the header file for the AbstractWriter class.
#include "AbstractWriterBuilder.h" ///< Include the header file for the AbstractWriterBuilder class.

#include <map>    ////< Include the header file for the std::map class.
#include <string> ///< Include the header file for the std::string class.
#include <vector> ///< Include the header file for the std::vector class.

#include <iostream> ///< Include the header file for input/output stream operations.

#include <mutex> ///< Include the header file for the std::mutex class.
#include "Api.h"

/**
 * @brief Private implementation class for the AcquisitionManager.
 *
 * This class contains the private data members and methods for the AcquisitionManager.
 * It is used to encapsulate the implementation details and provide a clean interface.
 */
class AcquisitionManagerPrivate
{
public:
    std::map<std::string, AbstractDriverBuilder const *> driverBuilders; ///< Map to store driver builders by name.
    std::map<std::string, AbstractWriterBuilder const *> writerBuilders; ///< Map to store writer builders by name.
    std::map<std::string, AbstractDriverBuilder const *> auxDriverBuilders; ///< Map to store auxiliarie driver builders by name.

    /**
     * @brief Map to store the enabled writers for each acquisition.
     * The key is the acquisition name, and the value is a map of writer names and their enabled status.
     */
    std::map<std::string, std::map<std::string, OpenViBE::Parameters::ParameterBool>> acquisitionEnabledWriters;

    std::map<std::string, Acquisition*> acquisitions; ///< Vector to store acquisitions.
    std::mutex acquisitionsMutex; ///< Mutex to protect access to acquisitions.

    std::string defaultWriter; ///< Default writer name.

    uint32_t numChannels = 0; ///< Number of channels for the acquisition.
    uint32_t sampleRate = 0;  ///< Sample rate for the acquisition.
    uint32_t chunkSize = 0;   ///< Chunk size for the acquisition.
};

/**
 * @brief Constructor for the AcquisitionManager class.
 *
 * This constructor initializes the AcquisitionManager and sets up the necessary
 * data structures for managing drivers, writers, and acquisitions.
 */
AcquisitionManager::AcquisitionManager(): m_pImpl(std::make_unique<AcquisitionManagerPrivate>())
{
}

/**
 * @brief Destructor for the AcquisitionManager class.
 *
 * This destructor cleans up the resources used by the AcquisitionManager.
 * It deletes the current driver, writer, and acquisition if they exist.
 */
AcquisitionManager::~AcquisitionManager()
{
}

/**
 * @brief Registers a driver builder with the acquisition manager.
 *
 * This function checks if the driver builder is valid and not already registered.
 * If valid, it stores the driver builder in the map using its name as the key.
 *
 * @param driverBuilder Pointer to the driver builder to be registered.
 * @return true if the driver builder was successfully registered, false otherwise.
 */
bool AcquisitionManager::registerDriver(AbstractDriverBuilder const* driverBuilder)
{
    bool bRes = false; ///< Initialize the result variable to false.

    if (driverBuilder) {
    	bRes = true;
        if (! driverBuilder->auxiliary()) {
            if (m_pImpl->driverBuilders.find(driverBuilder->getDriverName()) == m_pImpl->driverBuilders.end()) {
                m_pImpl->driverBuilders[driverBuilder->getDriverName()] = driverBuilder; ///< Store the driver builder in the map using its name as the key.
            }
            else {
                std::cout << "Driver already registered: " << driverBuilder->getDriverName() << std::endl; ///< Print a message if the driver is already registered.
				bRes = false; ///< Set the result variable to false if the driver is already registered.
            }
        }
        else {
            if (m_pImpl->auxDriverBuilders.find(driverBuilder->getDriverName()) == m_pImpl->auxDriverBuilders.end()) {
                m_pImpl->auxDriverBuilders[driverBuilder->getDriverName()] = driverBuilder; ///< Store the driver builder in the map using its name as the key.
            }
            else {
                std::cout << "Driver already registered: " << driverBuilder->getDriverName() << std::endl; ///< Print a message if the driver is already registered.
                bRes = false;
            }
        }
    }

    return bRes;
}

/**
 * @brief Registers a writer builder with the acquisition manager.
 *
 * This function checks if the writer builder is valid and not already registered.
 * If valid, it stores the writer builder in the map using its name as the key.
 *
 * @param writerBuilder Pointer to the writer builder to be registered.
 * @return true if the writer builder was successfully registered, false otherwise.
 */
bool AcquisitionManager::registerWriter(AbstractWriterBuilder const* writerBuilder)
{
    bool bRes = false; ///< Initialize the result variable to false.
    
    if (writerBuilder)
    {
        if (m_pImpl->writerBuilders.find(writerBuilder->getWriterName()) == m_pImpl->writerBuilders.end())
        {
            m_pImpl->writerBuilders[writerBuilder->getWriterName()] = writerBuilder; ///< Store the writer builder in the map using its name as the key.
            bRes = true;
        }
        else
        {
            std::cout << "Writer already registered: " << writerBuilder->getWriterName() << std::endl; ///< Print a message if the writer is already registered.
        }
    }

    return bRes;
}

/**
 * @brief Returns a list of registered driver names.
 *
 * This function retrieves the names of all registered driver builders and returns them as a vector of strings.
 *
 * @return A vector of strings containing the names of registered drivers.
 */
std::vector<std::string> AcquisitionManager::getDriverNames() const
{
    std::vector<std::string> driverNames; ///< Vector to store the names of registered drivers.

    for (const auto& driver : m_pImpl->driverBuilders)
    {
        driverNames.push_back(driver.first); ///< Add the driver name to the vector.
    }
    
    return driverNames; ///< Return a vector of driver names.
}

/**
 * @brief Returns a list of registered auxiliary driver names.
 *
 * This function retrieves the names of all registered auxiliary driver builders and returns them as a vector of strings.
 *
 * @return A vector of strings containing the names of registered auxiliary drivers.
 */
std::vector<std::string> AcquisitionManager::getAuxilirayDriverNames() const {
	std::vector<std::string> driverNames; ///< Vector to store the names of registered auxiliary drivers.

    for (const auto& driver : m_pImpl->auxDriverBuilders) {
        driverNames.push_back(driver.first); ///< Add the driver name to the vector.
    }

	return driverNames; ///< Return a vector of auxiliary driver names.
}

/**
 * @brief Returns a list of registered writer names.
 *
 * This function retrieves the names of all registered writer builders and returns them as a vector of strings.
 *
 * @return A vector of strings containing the names of registered writers.
 */
std::vector<std::string> AcquisitionManager::getWriterNames() const
{
    std::vector<std::string> writerNames; ///< Vector to store the names of registered writers.
    for (const auto& writer : m_pImpl->writerBuilders)
    {
        writerNames.push_back(writer.first); ///< Add the writer name to the vector.
    }

    return writerNames; ///< Return a vector of writer names.
}

/**
 * @brief Returns a list of registered acquisition names.
 *
 * This function retrieves the names of all registered acquisitions and returns them as a vector of strings.
 *
 * @return A vector of strings containing the names of registered acquisitions.
 */
std::vector<std::string> AcquisitionManager::getAcquisitionNames() const
{
    std::vector<std::string> acquisitionNamesRes;
    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    for (const auto& acquisition : m_pImpl->acquisitions)
    {
        acquisitionNamesRes.push_back(acquisition.first); ///< Add the acquisition name to the vector.
    }
    return acquisitionNamesRes;
}

Acquisition* AcquisitionManager::getAcquisition(const std::string& acquisitionName) {
    Acquisition* acquisitionRes = nullptr;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(acquisitionName);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        acquisitionRes = acquisitionIt->second;
    }

    return acquisitionRes;
}

std::map<std::string, OpenViBE::Parameters::ParameterBool>& AcquisitionManager::getAcquisitionWritersEnableParam(std::string name) {
    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    return m_pImpl->acquisitionEnabledWriters[name];
}

bool AcquisitionManager::addAcquisitionDriver(uint64_t id, std::string name, std::string driverName) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        auto clients = acquisitionIt->second->getClients();
        if (!clients.empty() && *clients.begin() == id) {
            auto driver = createDriver(driverName);
            if (driver) {
                acquisitionIt->second->addDriver(std::move(driver));
                res = true;
            }
        }
    }

    return res;
}

bool AcquisitionManager::addAcquisitionWriter(uint64_t id, std::string name, std::string writerName) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        auto clients = acquisitionIt->second->getClients();
        if (!clients.empty() && *clients.begin() == id) {
            auto writer = createWriter(writerName);
            if (writer) {
                acquisitionIt->second->addWriter(writer);
                res = true;
            }
        }
    }

    return res;
}

/**
 * @brief Starts the specified acquisition if it is not already running.
 * @note Only the client that created the acquisition can start it.
 * 
 * @param id The ID of the client requesting to start the acquisition.
 * @param name The name of the acquisition to be started.
 * @return True if the acquisition was successfully started or already running, false otherwise.
 */
bool AcquisitionManager::startAcquisition(uint64_t id, std::string name) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        auto clients = acquisitionIt->second->getClients();
        if (!clients.empty() && *clients.begin() == id) {
            if (!acquisitionIt->second->isRunning()) {
                acquisitionIt->second->start();
            }
            res = true;
        }
    }

    return res;
}


/**
 * @brief Stops the specified acquisition if it is currently running.
 * @note Only the client that created the acquisition can stop it.
 * 
 * @param id The ID of the client requesting to stop the acquisition.
 * @param name The name of the acquisition to be stopped.
 * @return True if the acquisition was successfully stopped or already stopped, false otherwise.
 */
bool AcquisitionManager::stopAcquisition(uint64_t id, std::string name) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        auto clients = acquisitionIt->second->getClients();
        if (!clients.empty() && *clients.begin() == id) {
            if (acquisitionIt->second->isRunning()) {
                acquisitionIt->second->stop();
            }
            res = true;
        }
    }

    return res;
}

/**
 * @brief Checks if the specified acquisition is currently running.
 * @param name The name of the acquisition to check.
 * @return True if the acquisition is running, false if it is not running or not found.
 */
bool AcquisitionManager::isAcquisitionRunning(std::string name) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        res = acquisitionIt->second->isRunning();
    }

    return res;
}

/**
 * @brief Adds a client to the specified acquisition.
 * @param clientId The ID of the client to be added.
 * @param name The name of the acquisition to which the client should be added.
 * @return True if the client was successfully added or already present, false if failed or acquisition not found.
 */
bool AcquisitionManager::addClientToAcquisition(uint64_t clientId, std::string name) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        auto clients = acquisitionIt->second->getClients();
        if (clients.empty() || std::find(clients.begin(), clients.end(), clientId) == clients.end()) {
            acquisitionIt->second->addClient(clientId);
        }
        res = true;
    }

    return res;
}

/**
 * @brief Removes a client from the specified acquisition.
 * @param clientId The ID of the client to be removed.
 * @param name The name of the acquisition from which the client should be removed.
 * @return True if the client was successfully removed or not found, false if failed or acquisition not found.
 */
bool AcquisitionManager::removeClientFromAcquisition(uint64_t clientId, std::string name) {
    bool res = false;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        auto clients = acquisitionIt->second->getClients();
        if (!clients.empty() && std::find(clients.begin(), clients.end(), clientId) != clients.end()) {
            acquisitionIt->second->removeClient(clientId);
        }
        res = true;
    }

    return res;
}

/**
 * @brief Retrieves the list of clients associated with the specified acquisition.
 * 
 * @param name The name of the acquisition for which to retrieve the clients.
 * @return A list of client IDs associated with the specified acquisition. If the acquisition is not found, an empty list is returned.
 */
std::list<uint64_t> AcquisitionManager::getClientsFromAcquisition(std::string name) {
    std::list<uint64_t> res;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);
    auto acquisitionIt = m_pImpl->acquisitions.find(name);
    if (acquisitionIt != m_pImpl->acquisitions.end()) {
        res = acquisitionIt->second->getClients();
    }

    return res;
}


/**
 * @brief Creates a new driver instance using the specified driver name.
 *
 * This function checks if the specified driver name is valid and registered.
 * If valid, it creates a new driver instance and returns it.
 *
 * The driver is returned as a DriverPtr carrying its builder, so that a driver created
 * inside a plugin is destroyed inside that same plugin.
 *
 * @param driverName The name of the driver to be created.
 * @return An owning pointer to the newly created driver instance, or an empty one if not found.
 */
DriverPtr AcquisitionManager::createDriver(const std::string& driverName)
{
    AbstractDriverBuilder const* builder = nullptr; ///< Builder matching the requested name, if any.

    if (m_pImpl->driverBuilders.find(driverName) != m_pImpl->driverBuilders.end()) {
        builder = m_pImpl->driverBuilders[driverName]; ///< Look up the builder for the specified driver name.
    }
    else {
        if (m_pImpl->auxDriverBuilders.find(driverName) != m_pImpl->auxDriverBuilders.end()) {
            builder = m_pImpl->auxDriverBuilders[driverName];
        }
        else {
            std::cout << "Driver not found: " << driverName << std::endl; ///< Print a message if the driver is not found.
        }
    }

    if (!builder) { return DriverPtr(); }

    return DriverPtr(builder->createDriver(), DriverDeleter{ builder });
}

/**
 * @brief Creates a new writer instance using the specified writer name.
 *
 * This function checks if the specified writer name is valid and registered.
 * If valid, it creates a new writer instance and returns it.
 *
 * @param writerName The name of the writer to be created.
 * @return A pointer to the newly created writer instance, or nullptr if not found.
 */
AbstractWriter* AcquisitionManager::createWriter(const std::string& writerName)
{
    AbstractWriter* writerRes = nullptr; ///< Initialize the writer pointer to nullptr.

    if (m_pImpl->writerBuilders.find(writerName) != m_pImpl->writerBuilders.end())
    {
        writerRes = m_pImpl->writerBuilders[writerName]->createWriter(); ///< Create a new writer instance using the specified writer name.
    }
    else
    {
        std::cout << "Writer not found: " << writerName << std::endl; ///< Print a message if the writer is not found.
    }

    return writerRes;
}

/**
 * @brief Creates a new instance of the Acquisition for data acquisition.
 *
 * This function creates a new Acquisition instance and initializes it with
 * the current driver and writer. It also sets the number of channels, sample rate,
 * and chunk size for the acquisition.
 * 
 * @param id The client/remote ID of the acquisition context.
 * @param contextName The name of the acquisition context to be used.
 *
 * @return A pointer to the newly created Acquisition instance.
 */
Acquisition* AcquisitionManager::createAcquisition(uint64_t id, std::string name)
{
    Acquisition* acquisition = nullptr;

    std::lock_guard<std::mutex> lock(m_pImpl->acquisitionsMutex);

    if (m_pImpl->acquisitions.find(name) == m_pImpl->acquisitions.end()) {

        acquisition = new Acquisition(); ///< Create a new instance of the Acquisition.
            
        if (acquisition) {
            acquisition->addDriver(createDriver(m_pImpl->driverBuilders.begin()->first));
            // All auxiliary drivers run alongside the primary driver by default;
            // there is currently no UI/API path to select them individually.
            for (const auto& [auxDriverName, auxDriverBuilder] : m_pImpl->auxDriverBuilders) {
                acquisition->addDriver(createDriver(auxDriverName));
            }
            // All writers are added and can be enabled/disabled in UI (or via their params)
            for (auto it = m_pImpl->writerBuilders.begin(); it != m_pImpl->writerBuilders.end(); ++it) {
                auto writerName = it->first;
                /**
                 * Default writer is Net Writer
                 * It is added by default, while other writers are added through UI interaction
                 */
                if (writerName == "Net Writer") {
                    acquisition->addWriter(createWriter(writerName));
                } else {
                    m_pImpl->acquisitionEnabledWriters[name][writerName] = OpenViBE::Parameters::ParameterBool("Enable " + writerName, "", false);
					m_pImpl->acquisitionEnabledWriters[name][writerName].connect([this, name, writerName, acquisition](std::any enabled) {
						//TODO Remove this lambda and use SRP (Single Responsibility Principle) of SOLID Oncle BOB's principle.
                        if (std::any_cast<bool>(enabled)) {
                            acquisition->addWriter(createWriter(writerName));
                        } else {
                            for (auto& w : acquisition->getWriters()) {
                                if (w->name() == writerName) {
                                    acquisition->removeWriter(w);
                                    break;
                                }
                            }
                        }
                    });
                }
            }

            acquisition->addClient(id);
            m_pImpl->acquisitions[name] = acquisition;
        } else {
            std::cout << "Failed to create Acquisition instance." << std::endl; ///< Print a message if the acquisition creation fails.
        }
    } else {
        std::cout << "Acquisition already exists: " << name << std::endl; ///< Print a message if the acquisition already exists.
    }

    return acquisition;
}

/**
 * @brief Removes the specified Acquisition instance.
 *
 * This function stops the acquisition if it is running and deletes the instance.
 *
 * @param acquisition Pointer to the Acquisition instance to be removed.
 * @return true if the acquisition was successfully removed, false otherwise.
 */
bool AcquisitionManager::removeAcquisition(uint64_t id, std::string name)
{
    bool bRes = false; ///< Initialize the result variable to false.

    if (m_pImpl->acquisitions.find(name) != m_pImpl->acquisitions.end())
    {
        Acquisition* acquisition = m_pImpl->acquisitions[name];
        if (acquisition) {
            if (acquisition->isRunning())
            {
                acquisition->stop(); ///< Stop the acquisition if it is running.
            }
            delete acquisition; ///< Delete the acquisition instance.
        }
        m_pImpl->acquisitions.erase(name);
        bRes = true; ///< Set true to indicate success.
    }
    else
    {
        std::cout << "Failed to remove Acquisition instance." << std::endl; ///< Print a message if the removal fails.
    }

    return bRes; ///< Return the result.
}
