#pragma once

/**
* @file Api.h
* @brief Implementation of the Api class for managing acquisitions and acquisition contexts.
* This file contains the implementation of the Api class, which provides methods for
* creating and managing acquisition contexts, changing drivers and writers,
* and handling acquisitions.
* @details The Api class serves as a high-level interface for managing acquisitions and acquisition contexts.
* 
* @author Florent LERAY and Thomas Prampart (INRIA)
* @date December 2025
* @version 1.0
* @copyright Copyright (c) 2025 INRIA. All rights reserved.
* @license AGPL-3.0
*/

#include <list>
#include <memory>
#include <string>
#include <vector>

class Acquisition;
class ApiPrivate;

/**
 * @class Api
 * @brief The Api class provides methods to manage acquisitions and acquisition contexts.
 * @details This class allows you to create, release, and retrieve acquisition contexts,
 * change drivers and writers for specific contexts, and manage acquisitions.
 * 
 * @note The Api class is implemented as a singleton, ensuring that only one instance exists throughout the application.
 * To facilitate access all methods are static and call signleton internaly.
 */
class Api
{
public:
    ~Api();

    ////////////////////////////////////////////////////////////////////////////////
    // Get general capabilities
    ///////////////////////////////////////////////////////////////////////////////
    /**
     * @defgroup general capabilities
     * @brief Functions to get general capabilities of the acquisition manager.
     * @details These functions allow you to retrieve the names of registered drivers and writers.
     *
     * @{
     */
    static std::vector<std::string> getDriverNames(); ///< Returns a list of registered driver names.
    static std::vector<std::string> getWriterNames(); ///< Returns a list of registered writer names.
    /** @} */

    ///////////////////////////////////////////////////////////////////////////////
    // Acquisition Management
    //////////////////////////////////////////////////////////////////////////////
    /**
    * @defgroup Acquisition Management
    * @brief Functions to manage acquisitions.
    * Acquisitions are responsible for data acquisition and can be created, removed, and retrieved using these functions.
    * @details These functions allow you to create, remove, and retrieve acquisitions.
    * 
    * @note An acquisition is identified by a unique acquisition name and an client/remote ID.
    * @{
    */
    static std::vector<std::string> getAcquisitionNames(); ///< Returns a list of registered acquisition names.
    static bool createAcquisition(uint64_t id, std::string contextName = ""); ///< Creates an instance of the Acquisition for data acquisition.
    static bool removeAcquisition(uint64_t id, std::string acquisitionName); ///< Removes the specified Acquisition instance.
    //static Acquisition* getAcquisition(uint64_t id, std::string acquisitionName); ///< Returns the current Acquisition being used by the acquisition manager.
    static bool SetAcquisitionDriver(uint64_t id, std::string acquisitionName, std::string driverName); ///< Sets the driver for the specified acquisition.
    static bool SetAcquisitionWriter(uint64_t id, std::string acquisitionName, std::string writerName); ///< Sets the writer for the specified acquisition.

    /** @} */

    ///////////////////////////////////////////////////////////////////////////////
    // Acquisition Handling
    //////////////////////////////////////////////////////////////////////////////
    /**
    * @defgroup Acquisition Handling
    * @brief Functions to handle acquisition operations.
    * Acquisition operations include starting, stopping, checking the status, setting the number of cycles,
    * and managing clients for acquisitions.
    * @details These functions allow you to start, stop, check the status,
    * and manage clients for acquisitions.
    * 
    * @note An acquisition is identified by a unique acquisition name and an client/remote ID.
    * @{
    */
    static bool start(uint64_t id, std::string acquisitionName);
    static bool stop(uint64_t id, std::string acquisitionName);

    static bool isRunning(std::string acquisitionName);

    //static bool setNbCycles(uint64_t id, std::string acquisitionName, uint32_t cycles);

    static bool addClient(uint64_t clientId, std::string acquisitionName);
    static bool removeClient(uint64_t clientId, std::string acquisitionName);
    static std::list<uint64_t> getClients(std::string acquisitionName);
    /** @} */

protected:
    static Api& instance();

private:
    Api();

private:
    std::unique_ptr< ApiPrivate > d;
};
