/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <openvibe/parameters/parameters.h>

class ISettingsProvider;

namespace OpenViBE {

namespace Services {
class IService;
}

namespace AcquisitionServer {

/**
 * @brief The SessionManager class persists the acquisition server session from one run to the next.
 *
 * The selected driver & configured settings the chunk size, enabled writers & settings are stored 
 * in the "Session" section of the application settings.
 *
 * Driver settings are kept for every driver the user touched, not only for the current one, because
 * selecting a driver destroys the previous one along with its parameters. Restoring a driver
 * therefore also restores the values it had the last time it was selected, within a run as well as
 * across runs.
 *
*/
class SessionManager
{
public:
    static SessionManager& instance();

    /**
     * @brief Gives the manager the settings to read from and write to.
     *
     * @param provider The settings, which must outlive the manager's use of them. Null disables
     *                 session management, as does a false @p enabled.
     * @param enabled False to leave the settings untouched, for --no-session-management.
     */
    void setProvider(ISettingsProvider* provider, bool enabled);

    /**
     * @brief Gives the manager the services whose parameters it saves and restores.
     *
     * @param services The loaded services, keyed by library name
     */
    void setServices(const std::map<std::string, Services::IService*>& services);

    bool isEnabled() const { return m_enabled && m_provider != nullptr; }

    /**
     * @brief Applies the stored settings to the services given to setServices().
     *
     * Separate from restore() because of when it has to happen: the services are loaded, configured
     * and running during application initialization, long before there is any user interface to
     * restore a session into, so their settings can not wait for it.
     */
    /**
     * @brief Applies everything that was stored: the services, then the acquisition's configuration.
     *
     * Called once during application initialization, after the drivers and writers are loaded and the
     * acquisition exists, and before either front end starts. Doing it here rather than in the user
     * interface is what lets the headless mode start from the stored configuration too.
     *
     * @param acquisitionName The acquisition to configure.
     */
    void restoreAll(const std::string& acquisitionName);

    /**
     * @brief Takes note of the settings the current driver holds, before it is replaced.
     *
     * Selecting a driver destroys the previous one along with its parameters, so what it held has to
     * be kept here to be given back if it is selected again, within a run as well as across runs.
     * Call this before every driver change.
     *
     * @param acquisitionName The acquisition whose driver is about to change.
     */
    void rememberDriverState(const std::string& acquisitionName);

    /**
     * @brief Gives the acquisition's current driver back the settings remembered for it.
     *
     * Call it after every driver change, once the new driver is in place. A driver that was never
     * configured keeps its own defaults. Channel names are only applied when the montage still
     * matches, so a device reconfigured since they were stored is not given stale labels.
     *
     * @param acquisitionName The acquisition whose driver has just changed.
     */
    void applyDriverState(const std::string& acquisitionName);

    /**
     * @brief Gives the acquisition's attached writers back the settings remembered for them.
     *
     * @param acquisitionName The acquisition whose writers are to be configured.
     */
    void applyWriterState(const std::string& acquisitionName);

    /**
     * @brief Builds the session from what the acquisition currently holds, and saves it.
     *
     * Everything needed is read from the AcquisitionManager, so this works with or without a user
     * interface. Never call it from the restore path: it would write back what is being read.
     *
     * @param acquisitionName The acquisition to save the configuration of.
     */
    void saveCurrent(const std::string& acquisitionName);

private:
    SessionManager() = default;

    /**
     * @brief What the acquisition is configured with, as read from or written to the settings.
     *
     * @note The services are deliberately not in here. This is a cache for what gets destroyed and
     * recreated -- a driver on a switch, a writer on a toggle -- so that it can be given back. The
     * services are singletons that live for the whole run, so caching their parameters would only
     * hold a copy that goes stale as soon as anything changes them. They travel straight between the
     * settings and the live objects instead, in both directions: restoreServices() one way,
     * serializeServices() the other.
     */
    struct Session
    {
        std::string driverName;                             ///< selected driver
        std::map<std::string, nlohmann::json> driverParams;  ///< driver name -> serialized parameters
        std::map<std::string, std::vector<std::string>> driverChannelNames;  ///< driver name -> channel names
        std::map<std::string, bool> enabledWriters;          ///< writer name -> whether it is enabled
        std::map<std::string, nlohmann::json> writerParams;  ///< writer name -> serialized parameters
        std::optional<int> chunkSampleCount;                 ///< unset when no chunk size was saved
        std::optional<std::string> driftCorrectionPolicy;    ///< unset when no drift correction setting was saved (UG fork)
        std::optional<int> driftToleranceMs;                 ///< unset when no drift tolerance was saved (UG fork)
    };

    /// Applies the stored settings to the services given to setServices().
    void restoreServices() const;

    /// Reads the session section back. Empty when session management is off. Applies nothing, but
    /// seeds what this class remembers of the drivers and writers.
    Session readStoredSession();

    /// Writes the session section and the services section, then flushes the settings to storage.
    void writeAll(const Session& session) const;

    /// Rewrites the services section from the live services, keeping the ones it does not know.
    nlohmann::json serializeServices() const;

    /// The parameters of a service that are settings, that is all but the ones that command it.
    static Parameters::Parameters serviceSettings(Services::IService* service);

    ISettingsProvider* m_provider = nullptr;
    bool m_enabled                = false;
    /// Set while restoreAll() runs, so that a save triggered by something it changes can not write a
    /// half restored session over the stored one.
    bool m_restoring              = false;
    std::map<std::string, Services::IService*> m_services;

    /// Settings of every driver selected during this run, plus those read back from the previous one.
    std::map<std::string, nlohmann::json> m_driverParams;
    /// Channel names of those same drivers, kept apart because they are not driver parameters.
    std::map<std::string, std::vector<std::string>> m_driverChannelNames;
    /// Settings of the writers, kept for the same reason as the driver ones.
    std::map<std::string, nlohmann::json> m_writerParams;
};

}
}
