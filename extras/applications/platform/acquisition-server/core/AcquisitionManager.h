#pragma once
/**
 * @file AcquisitionManager.h
 * @brief Header file for the AcquisitionManager class.
 *
 * This file declares the `AcquisitionManager` class, which is responsible for managing
 * the acquisition of data from hardware drivers and writers. It provides methods to
 * register drivers and writers, change the current driver or writer, and create
 * instances of `Acquisition` for data acquisition.
 */

#include <memory>
#include <vector>
#include <list>
#include <string>

#include <openvibe/parameters/parameterBool.h>

#include <AbstractDriver.h> ///< For DriverPtr, the owning driver handle handed out by createDriver().

class AbstractDriverBuilder; ///< Forward declaration of the AbstractDriverBuilder class.
class AbstractWriterBuilder; ///< Forward declaration of the AbstractWriterBuilder class.
class AbstractWriter; ///< Forward declaration of the AbstractWriter class.
class Acquisition; ///< Forward declaration of the Acquisition class.

class AcquisitionManagerPrivate; ///< Forward declaration of the AcquisitionManagerPrivate class.

/**
 * @brief Manages the acquisition of data from hardware drivers and writers.
 *
 * The `AcquisitionManager` class is responsible for managing the acquisition process
 * by providing methods to register drivers and writers, change the current driver or
 * writer, and create instances of `Acquisition` for data acquisition.
 */
class AcquisitionManager
{
public:
    AcquisitionManager();
    ~AcquisitionManager();

    static AcquisitionManager& instance()
    {
        static AcquisitionManager instance; ///< Create a static instance of the AcquisitionManager.
        return instance; ///< Return the instance.
    } ///< Returns a singleton instance of the AcquisitionManager.

    ////////////////////////////////////////////////////////////////////////////////
    // Driver and Writer Registration and Retrieval
    ///////////////////////////////////////////////////////////////////////////////
    bool registerDriver(AbstractDriverBuilder const * driverBuilder); ///< Registers a driver builder for the acquisition manager.
    bool registerWriter(AbstractWriterBuilder const * writerBuilder); ///< Registers a writer builder for the acquisition manager.

    std::vector<std::string> getDriverNames() const; ///< Returns a list of registered driver names.
    std::vector<std::string> getAuxilirayDriverNames() const; ///< Returns a list of registered driver names.
    std::vector<std::string> getWriterNames() const; ///< Returns a list of registered writer names.
    

    ///////////////////////////////////////////////////////////////////////////////
    // Acquisition Management
    //////////////////////////////////////////////////////////////////////////////
    Acquisition* createAcquisition(uint64_t id, std::string name = ""); ///< Creates an instance of the Acquisition for data acquisition.
    bool removeAcquisition(uint64_t id, std::string name); ///< Removes the specified Acquisition by name
    std::vector<std::string> getAcquisitionNames() const; ///< Returns a list of registered acquisition names.
    Acquisition* getAcquisition(const std::string& acquisitionName); ///< Returns the Acquisition instance for the specified name.
    std::map<std::string, OpenViBE::Parameters::ParameterBool>& getAcquisitionWritersEnableParam(std::string name); ///< Returns the enabled writers for the specified acquisition.
    bool addAcquisitionDriver(uint64_t id, std::string name, std::string driverName); ///< Sets the driver for the specified acquisition.
    bool addAcquisitionWriter(uint64_t id, std::string name, std::string writerName); ///< Sets the writer for the specified acquisition.
	bool removeAcquisitionDriver(uint64_t id, std::string name, std::string driverName); ///< Removes the driver for the specified acquisition.
	bool removeAcquisitionWriter(uint64_t id, std::string name, std::string writerName); ///< Removes the writer for the specified acquisition.
    bool startAcquisition(uint64_t id, std::string name); ///< Starts the specified acquisition if it is not already running.
    bool stopAcquisition(uint64_t id, std::string name); ///< Stops the specified acquisition if it is currently running.
    bool isAcquisitionRunning(std::string name); ///< Checks if the specified acquisition is currently running.
    bool addClientToAcquisition(uint64_t clientId, std::string name); ///< Adds a client to the specified acquisition.
    bool removeClientFromAcquisition(uint64_t clientId, std::string name); ///< Removes a client from the specified acquisition.
    std::list<uint64_t> getClientsFromAcquisition(std::string name); ///< Retrieves the list of clients associated with the specified acquisition.

    /* @TODO: add these and move some logic to API level
    bool isClientOwner(uint64_t clientId, std::string acquisitionName); ///< Checks if the specified client is the owner of the specified acquisition.
    bool isClientOwner(uint64_t clientId, Acquisition* acquisition); ///< Checks if the specified client is the owner of the specified acquisition.
    */

private:
    DriverPtr createDriver(const std::string& driverName); ///< Creates an instance of the specified driver, owned through its builder.
    AbstractWriter* createWriter(const std::string& writerName); ///< Creates an instance of the specified writer.

private:
    std::unique_ptr<AcquisitionManagerPrivate> m_pImpl; ///< Pointer to the private implementation details of the AcquisitionManager.
};

