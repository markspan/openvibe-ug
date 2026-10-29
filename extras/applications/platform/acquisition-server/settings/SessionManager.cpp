// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>

#include "SessionManager.h"

#include <algorithm>
#include <iostream>
#include <list>

#include <openvibe/Services/IService.h>

#include "AbstractWriter.h"
#include "Acquisition.h"
#include "AcquisitionManager.h"
#include "ISettingsProvider.h"

namespace OpenViBE {
namespace AcquisitionServer {

namespace {

const char* SESSION_SECTION  = "Session";
const char* SERVICES_SECTION = "Services";
const char* DRIVER_KEY      = "driver";
const char* DRIVERS_KEY     = "drivers";
const char* WRITERS_KEY     = "writers";
const char* CHUNK_SIZE_KEY  = "chunkSampleCount";
const char* DRIFT_CORRECTION_KEY = "driftCorrection";
const char* DRIFT_TOLERANCE_KEY  = "driftToleranceMs";
const char* NAME_KEY        = "name";
const char* PARAMS_KEY      = "params";
const char* ENABLED_KEY     = "enabled";
const char* CHANNELS_KEY    = "channelNames";

/// True for the parameters that command a service rather than configure it. A service brings its own
/// runtime state up in its constructor, so replaying one of these would repeat a command instead of
/// restoring a setting. They are therefore neither written nor read back.
bool isServiceControlParameter(const std::string& name) { return name == "run"; }

/// Rebuilds detached parameters from a stored json array, skipping the entries it can not read.
std::list<Parameters::ParameterTraits*> parametersFromJson(const nlohmann::json& array)
{
    std::list<Parameters::ParameterTraits*> params;

    if (!array.is_array()) {
        return params;
    }

    for (const auto& item : array) {
        try {
            params.push_back(Parameters::ParameterFactory::create(item));
        }
        catch (const std::exception& e) {
            std::cerr << "Skipping a stored parameter: " << e.what() << std::endl;
        }
    }

    return params;
}

/// Reads the "name"/"params" pairs of a json array into a map, skipping malformed entries.
void readNamedParams(const nlohmann::json& array, std::map<std::string, nlohmann::json>& params)
{
    if (!array.is_array()) {
        return;
    }
    for (const auto& item : array) {
        if (item.contains(NAME_KEY) && item[NAME_KEY].is_string() && item.contains(PARAMS_KEY)) {
            params[item[NAME_KEY].get<std::string>()] = item[PARAMS_KEY];
        }
    }
}

/// Reads the optional "channelNames" of the same array, skipping entries that hold anything else.
void readDriverChannelNames(const nlohmann::json& array, std::map<std::string, std::vector<std::string>>& names)
{
    if (!array.is_array()) {
        return;
    }
    for (const auto& item : array) {
        if (item.contains(NAME_KEY) && item[NAME_KEY].is_string() && item.contains(CHANNELS_KEY)
            && item[CHANNELS_KEY].is_array()) {
            try {
                names[item[NAME_KEY].get<std::string>()] = item[CHANNELS_KEY].get<std::vector<std::string>>();
            }
            catch (const std::exception& e) {
                std::cerr << "Skipping stored channel names: " << e.what() << std::endl;
            }
        }
    }
}


nlohmann::json parametersToJson(const Parameters::Parameters& params)
{
    nlohmann::json array = nlohmann::json::array();
    for (const auto* param : params) {
        if (param) {
            array.push_back(param->serialize());
        }
    }
    return array;
}

void applyStoredParameters(const Parameters::Parameters& live, const nlohmann::json& stored)
{
    std::list<Parameters::ParameterTraits*> storedParams = parametersFromJson(stored);

    for (auto* liveParam : live) {
        if (!liveParam) {
            continue;
        }
        for (const auto* storedParam : storedParams) {
            if (storedParam && storedParam->name() == liveParam->name()) {
                // Avoid unnecessary notification
                if (storedParam->toString() != liveParam->toString()) {
                    liveParam->setValue(storedParam->valueAsAny());
                }
                break;
            }
        }
    }

    for (auto* storedParam : storedParams) {
        delete storedParam;
    }
}

}

SessionManager& SessionManager::instance()
{
    static SessionManager instance;
    return instance;
}

void SessionManager::setProvider(ISettingsProvider* provider, const bool enabled)
{
    m_provider = provider;
    m_enabled  = enabled;
}

void SessionManager::setServices(const std::map<std::string, Services::IService*>& services) { m_services = services; }

Parameters::Parameters SessionManager::serviceSettings(Services::IService* service)
{
    Parameters::Parameters settings;
    if (service) {
        for (auto* param : service->parameters()) {
            if (param && !isServiceControlParameter(param->name())) {
                settings.push_back(param);
            }
        }
    }
    return settings;
}

void SessionManager::restoreServices() const
{
    if (!isEnabled()) {
        return;
    }

    const nlohmann::json stored = m_provider->getSection(SERVICES_SECTION);
    if (!stored.is_array()) {
        return;
    }

    for (const auto& entry : stored) {
        if (!entry.contains(NAME_KEY) || !entry[NAME_KEY].is_string() || !entry.contains(PARAMS_KEY)) {
            continue;
        }
        const auto service = m_services.find(entry[NAME_KEY].get<std::string>());
        if (service != m_services.end()) {
            applyStoredParameters(serviceSettings(service->second), entry[PARAMS_KEY]);
        }
    }
}

nlohmann::json SessionManager::serializeServices() const
{
    nlohmann::json services = nlohmann::json::array();

    // Keep the entries of services that are not loaded in this run, so that unplugging a service
    // does not throw its settings away.
    const nlohmann::json stored = m_provider->getSection(SERVICES_SECTION);
    if (stored.is_array()) {
        for (const auto& service : stored) {
            if (service.contains(NAME_KEY) && service[NAME_KEY].is_string()
                && !m_services.contains(service[NAME_KEY].get<std::string>())) {
                services.push_back(service);
            }
        }
    }

    for (const auto& [name, service] : m_services) {
        if (!service) {
            continue;
        }
        nlohmann::json entry;
        entry[NAME_KEY]   = name;
        entry[PARAMS_KEY] = parametersToJson(serviceSettings(service));
        services.push_back(entry);
    }

    return services;
}

SessionManager::Session SessionManager::readStoredSession()
{
    Session session;
    if (!isEnabled()) {
        return session;
    }

    const nlohmann::json stored = m_provider->getSection(SESSION_SECTION);
    if (!stored.is_object()) {
        return session;   // nothing was saved yet, or somebody replaced the section by hand
    }

    if (stored.contains(DRIVER_KEY) && stored[DRIVER_KEY].is_string()) {
        session.driverName = stored[DRIVER_KEY].get<std::string>();
    }

    if (stored.contains(CHUNK_SIZE_KEY) && stored[CHUNK_SIZE_KEY].is_number_integer()) {
        session.chunkSampleCount = stored[CHUNK_SIZE_KEY].get<int>();
    }

    if (stored.contains(DRIFT_CORRECTION_KEY) && stored[DRIFT_CORRECTION_KEY].is_string()) {
        session.driftCorrectionPolicy = stored[DRIFT_CORRECTION_KEY].get<std::string>();
    }

    if (stored.contains(DRIFT_TOLERANCE_KEY) && stored[DRIFT_TOLERANCE_KEY].is_number_integer()) {
        session.driftToleranceMs = stored[DRIFT_TOLERANCE_KEY].get<int>();
    }

    if (stored.contains(DRIVERS_KEY)) {
        readNamedParams(stored[DRIVERS_KEY], session.driverParams);
        readDriverChannelNames(stored[DRIVERS_KEY], session.driverChannelNames);
    }

    if (stored.contains(WRITERS_KEY) && stored[WRITERS_KEY].is_array()) {
        readNamedParams(stored[WRITERS_KEY], session.writerParams);
        for (const auto& writer : stored[WRITERS_KEY]) {
            if (writer.contains(NAME_KEY) && writer[NAME_KEY].is_string()
                && writer.contains(ENABLED_KEY) && writer[ENABLED_KEY].is_boolean()) {
                session.enabledWriters[writer[NAME_KEY].get<std::string>()] = writer[ENABLED_KEY].get<bool>();
            }
        }
    }

    // What was read is also what this class remembers from now on, so that a driver selected later in
    // the run gets back the settings it had on the previous one.
    m_driverParams       = session.driverParams;
    m_driverChannelNames = session.driverChannelNames;
    m_writerParams       = session.writerParams;

    return session;
}

void SessionManager::restoreAll(const std::string& acquisitionName)
{
    // Applying the stored configuration changes the very things a save reads back, so hold off any
    // save until the whole of it is in place. Single exit, so the flag can not be left set.
    m_restoring = true;

    restoreServices();

    const Session session = readStoredSession();

    auto& manager            = AcquisitionManager::instance();
    Acquisition* acquisition = manager.getAcquisition(acquisitionName);
    if (acquisition) {
        // The driver the acquisition was created with stays in place when the stored one is gone, so
        // that a driver plugin that is no longer installed only costs its own settings.
        if (!session.driverName.empty()) {
            const std::vector<std::string> drivers = manager.getDriverNames();
            if (std::find(drivers.begin(), drivers.end(), session.driverName) == drivers.end()) {
                std::cerr << "Not restoring driver [" << session.driverName << "]: it is not registered anymore" << std::endl;
            }
            else if (!manager.addAcquisitionDriver(LOCAL_CLIENT_ID, acquisitionName, session.driverName)) {
                std::cerr << "Failed to restore driver [" << session.driverName << "]" << std::endl;
            }
        }

        applyDriverState(acquisitionName);

        if (session.chunkSampleCount) {
            acquisition->getChunkSampleCount()->setValue(*session.chunkSampleCount);
        }
        if (session.driftCorrectionPolicy) {
            acquisition->getDriftCorrectionPolicy()->setValue(*session.driftCorrectionPolicy);
        }
        if (session.driftToleranceMs) {
            acquisition->getDriftTolerance()->setValue(*session.driftToleranceMs);
        }

        // Setting the enable flag is what adds the writer to the acquisition, so the settings can
        // only be applied afterwards.
        auto& enableParams = manager.getAcquisitionWritersEnableParam(acquisitionName);
        for (const auto& [name, enabled] : session.enabledWriters) {
            const auto enableParam = enableParams.find(name);
            if (enableParam != enableParams.end()) {
                enableParam->second.setValue(enabled);
            }
        }

        applyWriterState(acquisitionName);
    }
    else {
        std::cerr << "Not restoring the session: acquisition [" << acquisitionName << "] does not exist" << std::endl;
    }

    m_restoring = false;
}

void SessionManager::rememberDriverState(const std::string& acquisitionName)
{
    const Acquisition* acquisition = AcquisitionManager::instance().getAcquisition(acquisitionName);
    if (!acquisition) {
        return;
    }

    const std::string& driverName = acquisition->getDriverName();
    if (driverName.empty()) {
        return;
    }

    m_driverParams[driverName] = parametersToJson(acquisition->getDriverParameters());

    //Save channel names only if any of them was set. This avoids cluttering the settings.
    const std::vector<std::string> channelNames = acquisition->getDriverChannelNames();
    if (std::any_of(channelNames.begin(), channelNames.end(), [](const std::string& n) { return !n.empty(); })) {
        m_driverChannelNames[driverName] = channelNames;
    }
}

void SessionManager::applyDriverState(const std::string& acquisitionName)
{
    Acquisition* acquisition = AcquisitionManager::instance().getAcquisition(acquisitionName);
    if (!acquisition) {
        return;
    }

    const std::string& driverName = acquisition->getDriverName();

    const auto params = m_driverParams.find(driverName);
    if (params != m_driverParams.end()) {
        applyStoredParameters(acquisition->getDriverParameters(), params->second);
    }

    // Only when the montage still matches: a device reconfigured since the names were stored must not
    // be given stale labels. Must run after the parameters, since setting the channel count resizes
    // the names and would wipe them.
    const auto names = m_driverChannelNames.find(driverName);
    if (names != m_driverChannelNames.end() && names->second.size() == acquisition->getDriverChannelNames().size()) {
        acquisition->setDriverChannelNames(names->second);
    }
}

void SessionManager::applyWriterState(const std::string& acquisitionName)
{
    Acquisition* acquisition = AcquisitionManager::instance().getAcquisition(acquisitionName);
    if (!acquisition) {
        return;
    }

    for (auto* writer : acquisition->getWriters()) {
        const auto params = m_writerParams.find(writer->name());
        if (params != m_writerParams.end()) {
            applyStoredParameters(writer->getParameters(), params->second);
        }
    }
}

void SessionManager::saveCurrent(const std::string& acquisitionName)
{
    if (!isEnabled() || m_restoring) {
        return;
    }

    auto& manager            = AcquisitionManager::instance();
    Acquisition* acquisition = manager.getAcquisition(acquisitionName);
    if (!acquisition) {
        return;
    }

    rememberDriverState(acquisitionName);
    for (auto* writer : acquisition->getWriters()) {
        m_writerParams[writer->name()] = parametersToJson(writer->getParameters());
    }

    Session session;
    session.driverName         = acquisition->getDriverName();
    session.driverParams       = m_driverParams;
    session.driverChannelNames = m_driverChannelNames;
    session.writerParams       = m_writerParams;
    session.chunkSampleCount   = acquisition->getChunkSampleCount()->value();
    session.driftCorrectionPolicy = acquisition->getDriftCorrectionPolicy()->value();
    session.driftToleranceMs      = acquisition->getDriftTolerance()->value();

    for (const auto& [name, enableParam] : manager.getAcquisitionWritersEnableParam(acquisitionName)) {
        session.enabledWriters[name] = enableParam.value();
    }

    writeAll(session);
}

void SessionManager::writeAll(const Session& session) const
{
    if (!isEnabled()) {
        return;
    }

    nlohmann::json stored = nlohmann::json::object();

    if (!session.driverName.empty()) {
        stored[DRIVER_KEY] = session.driverName;
    }
    if (session.chunkSampleCount) {
        stored[CHUNK_SIZE_KEY] = *session.chunkSampleCount;
    }
    if (session.driftCorrectionPolicy) {
        stored[DRIFT_CORRECTION_KEY] = *session.driftCorrectionPolicy;
    }
    if (session.driftToleranceMs) {
        stored[DRIFT_TOLERANCE_KEY] = *session.driftToleranceMs;
    }

    // A driver is listed as soon as it has either settings or channel names.
    std::map<std::string, nlohmann::json> driverParams = session.driverParams;
    for (const auto& [name, channelNames] : session.driverChannelNames) {
        driverParams.try_emplace(name, nlohmann::json::array());
    }

    nlohmann::json drivers = nlohmann::json::array();
    for (const auto& [name, params] : driverParams) {
        nlohmann::json driver;
        driver[NAME_KEY]   = name;
        driver[PARAMS_KEY] = params;
        const auto channelNames = session.driverChannelNames.find(name);
        if (channelNames != session.driverChannelNames.end() && !channelNames->second.empty()) {
            driver[CHANNELS_KEY] = channelNames->second;
        }
        drivers.push_back(driver);
    }
    stored[DRIVERS_KEY] = drivers;

    // A writer is listed as soon as it has either an enable flag or settings, so that a writer whose
    // settings were edited while it was disabled does not lose them.
    std::map<std::string, nlohmann::json> writerParams = session.writerParams;
    for (const auto& [name, enabled] : session.enabledWriters) {
        writerParams.try_emplace(name, nlohmann::json::array());
    }

    nlohmann::json writers = nlohmann::json::array();
    for (const auto& [name, params] : writerParams) {
        nlohmann::json writer;
        writer[NAME_KEY]   = name;
        writer[PARAMS_KEY] = params;
        const auto enabled = session.enabledWriters.find(name);
        if (enabled != session.enabledWriters.end()) {
            writer[ENABLED_KEY] = enabled->second;
        }
        writers.push_back(writer);
    }
    stored[WRITERS_KEY] = writers;

    // setSection only replaces the section we own, so the services and the other settings the user
    // put in that file are kept.
    m_provider->setSection(SESSION_SECTION, stored);
    m_provider->setSection(SERVICES_SECTION, serializeServices());
    if (!m_provider->save()) {
        std::cerr << "Could not save the acquisition server session" << std::endl;
    }
}

}
}
