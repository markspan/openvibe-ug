/**
 * SPDX-License-Identifier: AGPL-3.0-only
 * UG fork: new file, not in upstream OpenViBE. See CharmeleonDriver.h.
 */
#include "CharmeleonDriver.h"

#include <any>
#include <iostream>
#include <utility>

CharmeleonDriver::CharmeleonDriver(std::string logPrefix, std::string deviceName)
    : m_paramMontage("Montage",
                     "Charmeleon montage (a .json file, or the name of one in Charmeleon's library): names the channels, and decides which EEG "
                     "channels are recorded",
                     "", OpenViBE::Parameters::Read, {"Charmeleon montage (*.json)", "Any File (*)"}),
      m_logPrefix(std::move(logPrefix)),
      m_deviceName(std::move(deviceName))
{
    m_paramMontage.connect([this](std::any) { onMontageChanged(); });
}

// ─── Charmeleon ──────────────────────────────────────────────────────────────

std::string CharmeleonDriver::charmeleonMontage() const
{
    const MontageLocation location = locateCharmeleonMontage(m_paramMontage.value());
    return location.source == MontageLocation::Source::NotFound ? m_paramMontage.value() : location.path;
}

void CharmeleonDriver::takeOverCharmeleonMontage(const std::string& path)
{
    const std::string setting = montageSettingAfterSave(m_paramMontage.value(), path);
    // A new value applies itself, through onMontageChanged(); the same one has to be read again explicitly
    if (setting != m_paramMontage.value()) { m_paramMontage.setValue(setting); }
    else { onMontageChanged(); }
}

// ─── The channel list ────────────────────────────────────────────────────────

std::vector<std::string> CharmeleonDriver::channelHints() const
{
    const std::vector<DeviceChannel> channels = deviceChannels();
    const size_t entries                      = m_channelNames.value().size();

    // During an acquisition the list holds the recorded channels only, which all have a name
    std::vector<DeviceChannel> listed = channels;
    if (channels.empty()) {
        if (isAcquiring()) { return {}; }
        for (const MontageChannelKind kind : assumedKinds(entries)) { listed.push_back({kind, {}}); }
    }
    if (listed.empty() || listed.size() != entries) { return {}; }

    std::vector<std::string> hints(listed.size());
    uint32_t nBipolar = 0, nAuxiliary = 0;
    for (size_t i = 0; i < listed.size(); ++i) {
        switch (listed[i].kind) {
            case MontageChannelKind::EEG: break;
            case MontageChannelKind::Bipolar: hints[i] = montageChannelPrefix(listed[i].kind) + std::to_string(++nBipolar); break;
            case MontageChannelKind::Auxiliary: hints[i] = montageChannelPrefix(listed[i].kind) + std::to_string(++nAuxiliary); break;
            default: hints[i] = listed[i].name; break;
        }
    }
    return hints;
}

ovChannelInfo CharmeleonDriver::channelInfo() const
{
    ovChannelInfo info = AbstractDriver::channelInfo();
    const std::string setting = m_paramMontage.value();
    if (trimWhitespace(setting).empty()) { return info; }

    CharmeleonMontage montage;
    std::string error;
    if (!loadCharmeleonMontageSetting(setting, montage, error)) {
        std::cout << m_logPrefix << " The montage is not sent to the clients: " << error << std::endl;
        return info;
    }

    info.montage = montageName(setting);
    info.positions.resize(info.names.size());
    for (size_t i = 0; i < info.names.size(); ++i) {
        for (const auto& electrode : montage.electrodes) {
            if (electrode.onHead && electrode.name == info.names[i]) {
                info.positions[i] = {true, electrode.theta, electrode.phi};
                break;
            }
        }
    }
    return info;
}

void CharmeleonDriver::updateChannelList()
{
    if (!deviceChannels().empty()) {
        fitNamesToDevice();
        return;
    }
    // No amplifier: the montage still names the list, provisionally, and says what it did
    std::vector<std::string> names = m_channelNames.value();
    if (names.size() == size_t(m_channelCount.value()) && applyMontage(names)) { m_channelNames.setValue(names); }
}

bool CharmeleonDriver::fitNamesToDevice()
{
    const std::vector<DeviceChannel> channels = deviceChannels();
    if (channels.empty()) { return false; }

    // Names kept for another amplifier, or another number of channels, mean nothing here: start over
    std::vector<std::string> names = m_channelNames.value();
    if (names.size() != channels.size()) {
        names.assign(channels.size(), "");
        for (size_t i = 0; i < channels.size(); ++i) {
            if (channels[i].kind == MontageChannelKind::EEG) { names[i] = channels[i].name; }
        }
    }
    const bool ok = applyMontage(names);

    // Resizes m_channelNames too, through the notifier AbstractDriver installs in its constructor
    m_channelCount.setValue(int(channels.size()));
    m_channelNames.setValue(names);
    return ok;
}

std::vector<size_t> CharmeleonDriver::namedChannels(std::vector<std::string>& names)
{
    names.clear();
    m_configuredNames = m_channelNames.value();
    std::vector<size_t> named;
    for (size_t i = 0; i < m_configuredNames.size(); ++i) {
        const std::string name = trimWhitespace(m_configuredNames[i]);
        if (name.empty()) { continue; }
        named.push_back(i);
        names.push_back(name);
    }
    if (named.empty()) {
        m_configuredNames.clear();
        report("No channel has a name, so there is nothing to record. Name the channels, or choose a montage.");
    }
    return named;
}

void CharmeleonDriver::showRecordedChannels(const std::vector<std::string>& names)
{
    m_configuredLocks = channelLocks();
    setChannelLocks({});
    m_channelCount.setValue(int(names.size()));
    m_channelNames.setValue(names);
    setNote(m_montageNote.level, m_montageNote.text);
}

void CharmeleonDriver::restoreChannelList()
{
    if (m_configuredNames.empty()) { return; }
    m_channelCount.setValue(int(m_configuredNames.size()));
    m_channelNames.setValue(m_configuredNames);
    m_configuredNames.clear();
    setChannelLocks(std::move(m_configuredLocks));
    m_configuredLocks.clear();
}

// ─── The montage ─────────────────────────────────────────────────────────────

bool CharmeleonDriver::applyMontage(std::vector<std::string>& names)
{
    if (trimWhitespace(m_paramMontage.value()).empty()) {
        m_montageNote = {};
        clearNote();
        setChannelLocks({});
        return true;
    }

    // Without the amplifier, the channels the driver assumes; Play places the names on the amplifier's own
    std::vector<MontageChannelKind> kinds;
    for (const DeviceChannel& channel : deviceChannels()) { kinds.push_back(channel.kind); }
    bool assumed = false;
    if (kinds.empty()) {
        kinds   = assumedKinds(names.size());
        assumed = !kinds.empty();
    }

    MontageApplication application;
    std::string error;
    if (!applyCharmeleonMontageSetting(m_paramMontage.value(), kinds, names, application, error, assumed)) {
        report(error);
        m_montageNote = {Note::Level::Error, error};
        setNote(m_montageNote.level, m_montageNote.text);
        setChannelLocks({});
        return false;
    }

    std::cout << m_logPrefix << " " << application.summary << std::endl;
    names         = std::move(application.names);
    m_montageNote = {application.warning ? Note::Level::Warning : Note::Level::Info, application.summary};
    setNote(m_montageNote.level, m_montageNote.text);
    setChannelLocks(std::move(application.locked));
    return true;
}

void CharmeleonDriver::onMontageChanged()
{
    // During an acquisition the list only holds the recorded channels; init() applies the montage anyway
    const std::vector<DeviceChannel> channels = deviceChannels();
    if (isAcquiring() || (!channels.empty() && m_channelNames.value().size() != channels.size())) { return; }

    std::vector<std::string> names = m_channelNames.value();
    if (!applyMontage(names)) { return; }
    if (trimWhitespace(m_paramMontage.value()).empty()) {
        // Without the amplifier there are no amplifier names to go back to
        if (channels.empty()) { return; }
        for (size_t i = 0; i < channels.size(); ++i) {
            if (channels[i].kind == MontageChannelKind::EEG) { names[i] = channels[i].name; }
        }
    }
    m_channelNames.setValue(names);
}

// ─── Errors ──────────────────────────────────────────────────────────────────

void CharmeleonDriver::report(const std::string& message) const
{
    std::cout << m_logPrefix << " " << message << std::endl;
    m_lastError = message;
}

bool CharmeleonDriver::failInit(const std::string& message)
{
    if (!message.empty()) { report(message); }
    // Upstream drivers only print why they could not start; the server shows this under the settings
    setNote(Note::Level::Error, "Play failed: " + (m_lastError.empty() ? m_deviceName + " could not be started." : m_lastError));
    return false;
}
