/**
* @file Api.cpp
* @brief Implementation of the Api class for managing acquisitions and acquisition contexts.
* This file contains the implementation of the Api class, which provides methods for
* creating and managing acquisition contexts, changing drivers and writers,
* and handling acquisitions.
* 
* @author Florent LERAY (INRIA)
* @date December 2025
* @version 1.0
* @copyright Copyright (c) 2025 INRIA. All rights reserved.
* @license AGPL-3.0
*/

#include "Api.h"

#include <AcquisitionManager.h>
#include <Acquisition.h>
#include <SessionManager.h>

/**
 * @brief Private implementation class for the Api.
 *
 * This class contains the private data members and methods for the Api.
 * It is used to encapsulate the implementation details and provide a clean interface.
 */
class ApiPrivate
{
public:
    ApiPrivate() = default;
    ~ApiPrivate() = default;

    static std::unique_ptr<Api> instance;
    AcquisitionManager& acqMgr = AcquisitionManager::instance();
};

std::unique_ptr<Api> ApiPrivate::instance = nullptr;

/**
 * @brief Constructor for the Api class.
 *
 * This constructor initializes the Api class and sets up the necessary
 * data structures for managing acquisitions and acquisition contexts.
 */
Api::Api(): d(std::make_unique<ApiPrivate>())
{
}

/**
 * @brief Destructor for the Api class.
 *
 * This destructor cleans up any resources used by the Api class.
 */
Api::~Api()
{
}

/**
 * @brief Returns a singleton instance of the Api class.
 *
 * This function checks if an instance of the Api class already exists.
 * If not, it creates a new instance. It then returns a reference to the instance.
 *
 * @return A reference to the singleton Api instance.
 */
Api& Api::instance()
{
    if (ApiPrivate::instance == nullptr)
    {
        ApiPrivate::instance = std::unique_ptr<Api>(new Api());
    }
    return * ApiPrivate::instance;
}


////////////////////////////////////////////////////////////////////////////////
// Get general capabilities
///////////////////////////////////////////////////////////////////////////////

/**
* @brief Retrieves a list of registered driver names.
* * @return A vector containing the names of all registered drivers.
*/
std::vector<std::string> Api::getDriverNames()
{
    return Api::instance().d->acqMgr.getDriverNames();
}

/**
* @brief Retrieves a list of registered writer names.
* * @return A vector containing the names of all registered writers.
*/
std::vector<std::string> Api::getWriterNames()
{
    return Api::instance().d->acqMgr.getWriterNames();
}

///////////////////////////////////////////////////////////////////////////////
// Acquisition Management
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves a list of registered acquisition names.
 * @return A vector containing the names of all registered acquisitions.
 */
std::vector<std::string> Api::getAcquisitionNames()
{
    return Api::instance().d->acqMgr.getAcquisitionNames();
}

bool Api::createAcquisition(uint64_t id, std::string acquisitionName, EOwnership ownership)
{
    return Api::instance().d->acqMgr.createAcquisition(id, acquisitionName, ownership) != nullptr;
}

/**
 * @brief Removes the specified acquisition instance.
 * @param id The ID of the client requesting the removal.
 * @param acquisitionName The name of the acquisition to be removed.
 * @return True if the acquisition was successfully removed, false otherwise.
 */
bool Api::removeAcquisition(uint64_t id, std::string acquisitionName)
{
    return Api::instance().d->acqMgr.removeAcquisition(id, acquisitionName);
}

bool Api::SetAcquisitionDriver(uint64_t id, std::string acquisitionName, std::string driverName)
{
    auto& sessionManager = OpenViBE::AcquisitionServer::SessionManager::instance();

    // Save current driver settings before switching to a different one
    sessionManager.rememberDriverState(acquisitionName);

    if (!Api::instance().d->acqMgr.addAcquisitionDriver(id, acquisitionName, driverName)) {
        return false;
    }

    // Apply new driver settings if any were saved
    sessionManager.applyDriverState(acquisitionName);
    // Save acquisition state
    sessionManager.saveCurrent(acquisitionName);
    return true;
}

bool Api::SetAcquisitionWriter(uint64_t id, std::string acquisitionName, std::string writerName)
{
    bool res = false;
    if (Api::instance().d->acqMgr.addAcquisitionWriter(id, acquisitionName, writerName)) {

        auto& sessionManager = OpenViBE::AcquisitionServer::SessionManager::instance();
        // Apply new writer settings if any were saved
        sessionManager.applyWriterState(acquisitionName);
        // Save acquisition state
        sessionManager.saveCurrent(acquisitionName);
        res = true;
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////
// Acquisition Handling
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief Starts the specified acquisition if it is not already running.
 * @note Only the client that started the acquisition can start it.
 * @param id The ID of the client requesting to start the acquisition.
 * @param acquisitionName The name of the acquisition to be started.
 * @return True if the acquisition was successfully started or already running, false otherwise.
 */
bool Api::start(uint64_t id, std::string acquisitionName)
{
    return Api::instance().d->acqMgr.startAcquisition(id, acquisitionName);
}

/**
 * @brief Stops the specified acquisition if it is currently running.
 * @note Only the client that started the acquisition can stop it.
 * @param id The ID of the client requesting to stop the acquisition.
 * @param acquisitionName The name of the acquisition to be stopped.
 * @return True if the acquisition was successfully stopped or already stopped, false otherwise.
 */
bool Api::stop(uint64_t id, std::string acquisitionName)
{
    return Api::instance().d->acqMgr.stopAcquisition(id, acquisitionName);
}

/**
 * @brief Checks if the specified acquisition is currently running.
 * @param acquisitionName The name of the acquisition to check.
 * @return True if the acquisition is running, false otherwise.
 */
bool Api::isRunning(std::string acquisitionName)
{
    return Api::instance().d->acqMgr.isAcquisitionRunning(acquisitionName);
}

//bool Api::setNbCycles(uint64_t id, std::string acquisitionName, uint32_t cycles)
//{
//    return false;
//}

/**
 * @brief Adds a client to the specified acquisition.
 * @param clientId The ID of the client to be added.
 * @param acquisitionName The name of the acquisition to which the client should be added.
 * @return True if the client was successfully added, false otherwise.
 */
bool Api::addClient(uint64_t clientId, std::string acquisitionName)
{
    return Api::instance().d->acqMgr.addClientToAcquisition(clientId, acquisitionName);
}

/**
 * @brief Removes a client from the specified acquisition.
 * @param clientId The ID of the client to be removed.
 * @param acquisitionName The name of the acquisition from which the client should be removed.
 * @return True if the client was successfully removed, false otherwise.
 */
bool Api::removeClient(uint64_t clientId, std::string acquisitionName)
{
    return Api::instance().d->acqMgr.removeClientFromAcquisition(clientId, acquisitionName);
}

/**
* @brief Retrieves the list of clients associated with the specified acquisition.
* This function looks up the acquisition by its name and retrieves the list of client IDs
* that are currently connected to that acquisition.
* @param acquisitionName The name of the acquisition for which to retrieve the clients.
* @return A list of client IDs associated with the specified acquisition. If the acquisition
* is not found, an empty list is returned.
*/
std::list<uint64_t> Api::getClients(std::string acquisitionName)
{
    return Api::instance().d->acqMgr.getClientsFromAcquisition(acquisitionName);
}
