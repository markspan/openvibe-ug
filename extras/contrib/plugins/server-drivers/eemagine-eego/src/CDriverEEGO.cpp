/**
 * SPDX-FileCopyrightText: Inria / eemagine GmbH
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the eego acquisition driver originally written by Steffen Heimes (eemagine GmbH).
 */

#include "CDriverEEGO.h"

#include <BlockStamps.h>  // UG fork
#include <SteadyTrigger.h>  // UG fork
#include <CharmeleonMontage.h>

// The SDK picks its factory signatures from these; wrapper.cc forces _UNICODE on itself anyway,
// so this only keeps the vendor's "defaulting to..." pragma quiet on our side.
#define _UNICODE
#if defined(_MBCS) // Only unicode should be supported. Better stay away from MBCS methods!
#undef _MBCS
#endif

#include <Windows.h>

#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#include <eemagine/sdk/buffer.h>
#include <eemagine/sdk/channel.h>
#include <eemagine/sdk/stream.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <thread>

namespace es = eemagine::sdk;

namespace {

/**
 * OVTK_StimulationId_Label(i) from sdk/toolkit/include/toolkit/ovtk_defines.h. Inlined because
 * the toolkit include directory is not on a driver plugin's include path.
 */
constexpr uint64_t STIMULATION_LABEL_BASE = 0x00008100;

/**
 * How long read() idles when the amplifier has produced nothing yet. The acquisition server's
 * reader thread is a bare `while (running) read()`, so without this it would spin a core.
 */
constexpr auto IDLE_POLL = std::chrono::milliseconds(1);

int toInt(const std::string& value, const int fallback)
{
    try { return std::stoi(value); }
    catch (const std::exception&) { return fallback; }
}

std::string trim(const std::string& s)
{
    const size_t first = s.find_first_not_of(" \t");
    if (first == std::string::npos) { return ""; }
    return s.substr(first, s.find_last_not_of(" \t") - first + 1);
}

/**
 * \brief Reports a setting the amplifier does not advertise.
 *
 * Not fatal: the amplifier is the real authority and the lists offered as parameters are the
 * generic ones from the SDK documentation, so a model with a narrower set still gets a usable
 * dropdown -- it just warns before the SDK refuses the stream.
 *
 * Matching is approximate because the range lists are volts derived from a millivolt integer.
 */
template <typename T>
void warnIfUnsupported(const std::vector<T>& available, const double value, const char* what)
{
    const auto matches = [value](const T entry) { return std::abs(double(entry) - value) < 1e-6; };

    if (!available.empty() && std::none_of(available.begin(), available.end(), matches))
    {
        std::cout << "[EEGO] Amplifier does not advertise " << what << " " << value << ". Available:";
        for (const auto& entry : available)
        {
            std::cout << " " << entry;
        }
        std::cout << std::endl;
    }
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────

CDriverEEGO::CDriverEEGO()
    : m_paramSamplingRate("SamplingRate", "Sampling rate [Hz]",
                          { "500", "512", "1000", "1024", "2000", "2048", "4000" }, "500"),
      m_paramEEGRange("EEGRange", "Signal range of the referential channels [mV]",
                      { "150", "750", "1000" }, "1000"),
      m_paramBIPRange("BIPRange", "Signal range of the bipolar channels [mV]",
                      { "350", "700", "1500", "4000" }, "1500"),
      m_paramMontage("Montage",
                     "Charmeleon montage (a .json file, or the name of one in Charmeleon's library): names the channels, and decides which EEG "
                     "channels are recorded",
                     "",
                     OpenViBE::Parameters::Read, { "Charmeleon montage (*.json)", "Any File (*)" })
{
    // The rate and the ranges are discrete sets the amplifier rejects anything outside of, so they
    // are dropdowns; the inherited m_samplingRate is derived in init() and stays out of the parameter list.
    // The channel count is read from the amplifier, but kept among the parameters so that the names of the
    // last session's channels can be restored when it is not connected at startup.
    m_channelCount.setDescription("Number of channels (read from the amplifier)");

    m_parameters.push_back(&m_paramSamplingRate);
    m_parameters.push_back(&m_paramEEGRange);
    m_parameters.push_back(&m_paramBIPRange);
    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_paramMontage);

    m_paramMontage.connect([this](std::any) { onMontageChanged(); });
}

CDriverEEGO::~CDriverEEGO() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Channel list
// ─────────────────────────────────────────────────────────────────────────────

OpenViBE::Parameters::Parameters CDriverEEGO::getParameters()
{
    // Never touch the amplifier while it is in use
    if (!m_amplifier)
    {
        if (queryLayout()) { fitNamesToLayout(); }
        else if (m_channelNames.value().size() == size_t(m_channelCount.value()))
        {
            // No amplifier: the montage still names the list, provisionally, and says what it did
            std::vector<std::string> names = m_channelNames.value();
            if (applyMontage(names)) { m_channelNames.setValue(names); }
        }
    }
    return AbstractDriver::getParameters();
}

std::vector<std::string> CDriverEEGO::channelHints() const
{
    // During an acquisition the list holds the recorded channels only, which all have a name
    if (m_layout.empty() || m_channelNames.value().size() != m_layout.size()) { return {}; }

    std::vector<std::string> hints(m_layout.size());
    for (size_t i = 0; i < m_layout.size(); ++i)
    {
        if (m_layout[i].type == es::channel::bipolar) { hints[i] = m_layout[i].name; }
    }
    return hints;
}

void CDriverEEGO::readLayout(es::amplifier& amplifier)
{
    m_layout.clear();
    int nEEG = 0;
    int nBIP = 0;
    for (const auto& channel : amplifier.getChannelList())
    {
        if (channel.getType() == es::channel::reference)
        {
            m_layout.push_back({ channel.getIndex(), int(channel.getType()), "EEG" + std::to_string(++nEEG) });
        }
        else if (channel.getType() == es::channel::bipolar)
        {
            m_layout.push_back({ channel.getIndex(), int(channel.getType()), "BIP" + std::to_string(++nBIP) });
        }
    }
}

bool CDriverEEGO::queryLayout()
{
    // Read once: the session is saved on every setting change, and acquiring the amplifier each time would be slow.
    // init() reads it again from the amplifier actually used.
    if (!m_layout.empty()) { return true; }

    try
    {
        std::unique_ptr<es::amplifier> amplifier(factory().getAmplifier());
        readLayout(*amplifier);
    }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Could not read the amplifier's channels: " << ex.what() << std::endl;
        m_layout.clear();
    }
    return !m_layout.empty();
}

bool CDriverEEGO::fitNamesToLayout()
{
    if (m_layout.empty()) { return false; }

    // Names stored for another layout (another amplifier) mean nothing here: start over
    std::vector<std::string> names = m_channelNames.value();
    if (names.size() != m_layout.size())
    {
        names.assign(m_layout.size(), "");
        for (size_t i = 0; i < m_layout.size(); ++i)
        {
            if (m_layout[i].type == es::channel::reference) { names[i] = m_layout[i].name; }
        }
    }
    const bool ok = applyMontage(names);

    // Resizes m_channelNames too, through the notifier AbstractDriver installs in its constructor
    m_channelCount.setValue(int(m_layout.size()));
    m_channelNames.setValue(names);
    return ok;
}

std::vector<MontageChannelKind> CDriverEEGO::layoutKinds() const
{
    std::vector<MontageChannelKind> kinds;
    kinds.reserve(m_layout.size());
    for (const auto& channel : m_layout)
    {
        kinds.push_back(channel.type == es::channel::reference ? MontageChannelKind::EEG
                        : channel.type == es::channel::bipolar ? MontageChannelKind::Bipolar
                                                                : MontageChannelKind::Other);
    }
    return kinds;
}

bool CDriverEEGO::applyMontage(std::vector<std::string>& names)
{
    // UG fork: the montage rules live in CharmeleonMontage, shared with the TMSi driver; this only feeds them the
    // amplifier's channel kinds and shows the outcome under the settings
    if (trim(m_paramMontage.value()).empty())
    {
        m_montageNote = {};
        clearNote();
        setChannelLocks({});
        return true;
    }

    MontageApplication application;
    std::string error;
    if (!applyCharmeleonMontageSetting(m_paramMontage.value(), layoutKinds(), names, application, error))
    {
        report(error);
        m_montageNote = {Note::Level::Error, error};
        setNote(m_montageNote.level, m_montageNote.text);
        setChannelLocks({});
        return false;
    }

    std::cout << "[EEGO] " << application.summary << std::endl;
    names         = std::move(application.names);
    m_montageNote = {application.warning ? Note::Level::Warning : Note::Level::Info, application.summary};
    setNote(m_montageNote.level, m_montageNote.text);
    setChannelLocks(std::move(application.locked));
    return true;
}

std::vector<std::string> CDriverEEGO::charmeleonArguments() const
{
    // Charmeleon 2.2 and later; the driver itself uses the first amplifier the SDK lists, and so does Charmeleon
    return {"--amplifier", "eego"};
}

std::string CDriverEEGO::charmeleonMontage() const
{
    const MontageLocation location = locateCharmeleonMontage(m_paramMontage.value());
    return location.source == MontageLocation::Source::NotFound ? m_paramMontage.value() : location.path;
}

void CDriverEEGO::takeOverCharmeleonMontage(const std::string& path)
{
    const std::string setting = montageSettingAfterSave(m_paramMontage.value(), path);
    // A new value applies itself, through onMontageChanged(); the same one has to be read again explicitly
    if (setting != m_paramMontage.value()) { m_paramMontage.setValue(setting); }
    else { onMontageChanged(); }
}

void CDriverEEGO::onMontageChanged()
{
    // During an acquisition the list only holds the recorded channels; init() applies the montage anyway
    if (m_amplifier || (!m_layout.empty() && m_channelNames.value().size() != m_layout.size())) { return; }

    std::vector<std::string> names = m_channelNames.value();
    if (!applyMontage(names)) { return; }
    if (trim(m_paramMontage.value()).empty())
    {
        // Without the amplifier there are no amplifier names to go back to
        if (m_layout.empty()) { return; }
        for (size_t i = 0; i < m_layout.size(); ++i)
        {
            if (m_layout[i].type == es::channel::reference) { names[i] = m_layout[i].name; }
        }
    }
    m_channelNames.setValue(names);
}

void CDriverEEGO::report(const std::string& message)
{
    std::cout << "[EEGO] " << message << std::endl;
    m_lastError = message;
}

bool CDriverEEGO::failInit(const std::string& message)
{
    if (!message.empty()) { report(message); }
    // UG fork: upstream drivers only print why they could not start; the server shows this under the settings
    setNote(Note::Level::Error, "Play failed: " + (m_lastError.empty() ? std::string("the eego amplifier could not be started.") : m_lastError));
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// SDK factory
// ─────────────────────────────────────────────────────────────────────────────

std::string CDriverEEGO::sdkLibraryPath()
{
    wchar_t exePath[MAX_PATH] = {};
    const DWORD length        = GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    if (length > 0 && length < MAX_PATH)
    {
        const std::filesystem::path candidate =
            std::filesystem::path(exePath).parent_path() / L"eego-SDK.dll";

        std::error_code ec;
        if (std::filesystem::exists(candidate, ec))
        {
            // The SDK treats this std::string as UTF-8, so do not go through path::string(),
            // which yields the active code page.
            const auto utf8 = candidate.u8string();
            return std::string(utf8.begin(), utf8.end());
        }
    }

    // Fall back to the bare name and let the Windows loader search: its order already covers the
    // executable's own directory and the drivers directory CInitApp registers, plus PATH -- which
    // is what makes this work from the build tree, where the DLL only lives in the pixi env.
    return "eego-SDK.dll";
}

es::factory& CDriverEEGO::factory()
{
    if (!m_factory)
    {
        const std::string path = sdkLibraryPath();
        std::cout << "[EEGO] Loading SDK from: " << path << std::endl;
        m_factory = std::make_unique<es::factory>(path);

        const auto version = m_factory->getVersion();
        std::cout << "[EEGO] SDK version: " << version.major << "." << version.minor << "."
                  << version.micro << "." << version.build << std::endl;
    }

    return *m_factory;
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverEEGO::init()
{
    bool success = false;
    m_lastError.clear();

    try
    {
        m_amplifier.reset(factory().getAmplifier());

        std::cout << "[EEGO] Amplifier " << m_amplifier->getSerialNumber()
                  << " (type " << m_amplifier->getType()
                  << ", firmware " << m_amplifier->getFirmwareVersion() << ")" << std::endl;

        const int rate     = toInt(m_paramSamplingRate.value(), 500);
        const int eegRange = toInt(m_paramEEGRange.value(), 1000);
        const int bipRange = toInt(m_paramBIPRange.value(), 1500);

        warnIfUnsupported(m_amplifier->getSamplingRatesAvailable(), rate, "sampling rate");
        warnIfUnsupported(m_amplifier->getReferenceRangesAvailable(), eegRange / 1000.0, "EEG range [V]");
        warnIfUnsupported(m_amplifier->getBipolarRangesAvailable(), bipRange / 1000.0, "BIP range [V]");

        m_samplingRate.setValue(rate);
        m_effectiveBipRange = bipRange / 1000.0;

        success = selectChannels();
    }
    catch (const std::exception& ex)
    {
        report(std::string("No eego amplifier could be opened: ") + ex.what());
    }

    if (!success)
    {
        m_stream.reset();
        m_amplifier.reset();
        return failInit();
    }

    setNote(m_montageNote.level, m_montageNote.text);
    return true;
}

bool CDriverEEGO::selectChannels()
{
    // The amplifier can enumerate its channels without a stream being open, which matters: the SDK
    // allows only one stream at a time and the acquisition server reads channelCount() straight
    // after init(), long before start() opens one.
    readLayout(*m_amplifier);

    // Record the channels that have a name, under that name
    if (!fitNamesToLayout()) { return false; }
    m_configuredNames = m_channelNames.value();
    m_selectedChannels.clear();
    std::vector<std::string> names;
    for (size_t i = 0; i < m_layout.size(); ++i)
    {
        const std::string name = trim(m_configuredNames[i]);
        if (name.empty()) { continue; }
        m_selectedChannels.emplace_back(m_layout[i].index, m_layout[i].type);
        names.push_back(name);
    }

    if (m_selectedChannels.empty())
    {
        report("No channel has a name, so there is nothing to record. Name the channels, or choose a montage.");
        m_configuredNames.clear();
        return false;
    }

    names.push_back("Trigger");
    names.push_back("SampleCounter");

    // For the duration of the acquisition the channel count and names describe the recorded channels only; unInit() restores the full list
    m_configuredLocks = channelLocks();
    setChannelLocks({});
    m_channelCount.setValue(int(names.size()));
    m_channelNames.setValue(names);

    std::cout << "[EEGO] Recording " << m_selectedChannels.size() << " of the " << m_layout.size() << " channels + " << STATUS_CHANNEL_COUNT
              << " status channels at " << m_samplingRate.value() << " Hz." << std::endl;

    return true;
}

std::string CDriverEEGO::streamLayoutMismatch(const std::vector<es::channel>& channels) const
{
    const auto typeName = [](const int type)
    {
        switch (type)
        {
            case es::channel::reference: return std::string("referential channel");
            case es::channel::bipolar: return std::string("bipolar channel");
            case es::channel::trigger: return std::string("trigger channel");
            case es::channel::sample_counter: return std::string("sample counter");
            default: return "channel of type " + std::to_string(type);
        }
    };

    const size_t nSelected = m_selectedChannels.size();
    if (channels.size() != nSelected + STATUS_CHANNEL_COUNT)
    {
        return std::to_string(channels.size()) + " channels where " + std::to_string(nSelected + STATUS_CHANNEL_COUNT) + " were asked for";
    }
    bool sameNumbers = true;
    for (size_t i = 0; i < channels.size(); ++i)
    {
        const int expected = i < nSelected ? m_selectedChannels[i].second : i == nSelected ? int(es::channel::trigger) : int(es::channel::sample_counter);
        if (int(channels[i].getType()) != expected)
        {
            return "its channel " + std::to_string(i + 1) + " is a " + typeName(int(channels[i].getType())) + ", where a " + typeName(expected)
                   + " was asked for";
        }
        if (i < nSelected && channels[i].getIndex() != m_selectedChannels[i].first) { sameNumbers = false; }
    }
    // The SDK may number a stream's channels its own way; then only the kinds could be checked
    if (!sameNumbers) { std::cout << "[EEGO] The stream numbers its channels differently from the amplifier; their order was checked by kind only." << std::endl; }
    return {};
}

/// The selected channels as the SDK lists them, optionally only the referential ones.
static std::vector<es::channel> toChannelList(const std::vector<std::pair<unsigned int, int>>& selected, const bool referentialOnly)
{
    std::vector<es::channel> channels;
    for (const auto& [index, type] : selected)
    {
        if (!referentialOnly || type == es::channel::reference) { channels.emplace_back(index, es::channel::channel_type(type)); }
    }
    return channels;
}

bool CDriverEEGO::unInit()
{
    m_bufferReported = false;
    m_stream.reset();
    m_amplifier.reset();

    m_pending.clear();
    m_pendingChannels = 0;
    m_pendingSamples  = 0;
    m_pendingOffset   = 0;
    m_stimQueue.clear();

    if (!m_configuredNames.empty())
    {
        m_channelCount.setValue(int(m_configuredNames.size()));
        m_channelNames.setValue(m_configuredNames);
        m_configuredNames.clear();
        setChannelLocks(std::move(m_configuredLocks));
        m_configuredLocks.clear();
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverEEGO::start()
{
    if (!m_amplifier) { return false; }

    m_pendingSamples   = 0;
    m_pendingOffset    = 0;
    m_trigger.resetUnknown(); // UG fork: the first word is where the lines are, not a rising edge
    m_triggerValues.clear();  // UG fork
    m_lastSampleTime   = 0;
    m_blockFetchTime   = 0;  // UG fork
    m_readFirst        = 0;  // UG fork
    m_triggerChannel   = -1;
    m_stimQueue.clear();

    const int rate     = toInt(m_paramSamplingRate.value(), 500);
    const int eegRange = toInt(m_paramEEGRange.value(), 1000);

    try
    {
        // EE-511 amplifiers constrain the bipolar range to 2.5x the EEG range. Finding out requires
        // a stream to have been opened, and the SDK then needs the amplifier released and
        // re-acquired in two steps before it will hand out another one.
        m_stream.reset();
        m_stream.reset(m_amplifier->OpenEegStream(rate));

        const std::string type = m_amplifier->getType();
        if (std::regex_search(type, std::regex("EE511")))
        {
            const double constrained = 2.5 * eegRange / 1000.0;
            if (m_effectiveBipRange != constrained)
            {
                m_effectiveBipRange = constrained;
                std::cout << "[EEGO] Amplifier type EE-511: bipolar range is constrained to 2.5x the"
                          << " EEG range. Forcing it to " << constrained * 1000 << " mV." << std::endl;
            }
        }

        m_stream.reset();
        m_amplifier.reset();
        m_amplifier.reset(factory().getAmplifier());
    }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Did not manage to verify the amplifier type: " << ex.what() << std::endl;
        return false;
    }

    try
    {
        m_stream.reset();
        m_stream.reset(m_amplifier->OpenEegStream(rate, eegRange / 1000.0, m_effectiveBipRange,
                                                  toChannelList(m_selectedChannels, false)));
        if (!m_stream)
        {
            std::cout << "[EEGO] The amplifier returned a null stream." << std::endl;
            return false;
        }

        const std::vector<es::channel> channels = m_stream->getChannelList();
        if (int(channels.size()) != m_channelCount.value())
        {
            std::cout << "[EEGO] The stream carries " << channels.size() << " channels but "
                      << m_channelCount.value() << " were announced at init." << std::endl;
            m_stream.reset();
            return false;
        }

        // UG fork: the channels were named in the order of m_selectedChannels, then Trigger and SampleCounter. The SDK does not
        // promise to keep that order, so check it: a stream in another order would record each name over another channel's data.
        const std::string mismatch = streamLayoutMismatch(channels);
        if (!mismatch.empty())
        {
            report("The eego stream does not list its channels as they were named: " + mismatch + ". Nothing is recorded, so "
                   "that no channel is recorded under another's name.");
            setNote(Note::Level::Error, "Play failed: " + m_lastError);
            m_stream.reset();
            return false;
        }

        // UG fork: the SDK gives volt; the EEG and bipolar channels go out in microvolt, as the TMSi driver's do and as the
        // BrainVision writer declares them. Upstream sent volt, which BrainVision files then read as a million times too small.
        m_columnScale.assign(channels.size(), 1.0);
        for (size_t i = 0; i < channels.size(); ++i)
        {
            const auto type = channels[i].getType();
            if (type == es::channel::reference || type == es::channel::bipolar) { m_columnScale[i] = 1e6; }
        }

        const auto trigger = std::find_if(channels.begin(), channels.end(), [](const es::channel& chan)
        {
            return chan.getType() == es::channel::trigger;
        });
        if (trigger != channels.end()) { m_triggerChannel = int(trigger - channels.begin()); }
        else { std::cout << "[EEGO] No trigger channel in the stream; no stimulations will be produced." << std::endl; }

        // UG fork: the sample counter shows every sample the amplifier lost on the way
        const auto counter = std::find_if(channels.begin(), channels.end(), [](const es::channel& chan)
        {
            return chan.getType() == es::channel::sample_counter;
        });
        m_counterChannel = counter != channels.end() ? int(counter - channels.begin()) : -1;
        if (m_counterChannel < 0) { std::cout << "[EEGO] No sample counter in the stream; lost samples can not be counted." << std::endl; }
        m_haveCounter    = false;
        m_counterStep    = 0;
        m_lostSamples    = 0;
        m_peakPending    = 0;
        m_lossCounted    = m_counterChannel >= 0;
        m_bufferRate     = uint32_t(rate);
        m_lastFetchNs    = std::chrono::steady_clock::now().time_since_epoch().count();
        m_bufferReported = true;
    }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Could not open the EEG stream: " << ex.what() << std::endl;
        m_stream.reset();
        return false;
    }

    return true;
}

bool CDriverEEGO::stop()
{
    m_stream.reset();
    return true;
}

void CDriverEEGO::setChunkSize(const uint32_t chunkSize)
{
    m_chunkSize = chunkSize;
}

// ─────────────────────────────────────────────────────────────────────────────

size_t CDriverEEGO::fetchPending()
{
    es::buffer data;
    try { data = m_stream->getData(); }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Error fetching data: " << ex.what() << std::endl;
        return 0;
    }

    m_pendingSamples  = data.getSampleCount();
    m_pendingChannels = data.getChannelCount();
    m_pendingOffset   = 0;
    m_blockFetchTime  = timestamp();  // UG fork: the block's last sample is the newest the SDK had now, see getData()

    // UG fork: what waited in the SDK's buffer, see takeDeviceBuffer()
    m_lastFetchNs = std::chrono::steady_clock::now().time_since_epoch().count();
    if (m_pendingSamples > m_peakPending) { m_peakPending = m_pendingSamples; }

    if (m_pendingSamples == 0) { return 0; }

    // eemagine::sdk::buffer stores _data[channel + sample * channel_count], i.e. sample-major --
    // the same order the acquisition server wants, so this is a plain copy rather than a transpose.
    const size_t total = m_pendingSamples * m_pendingChannels;
    m_pending.assign(data.data(), data.data() + total);

    countLostSamples();
    return m_pendingSamples;
}

void CDriverEEGO::countLostSamples()
{
    if (m_counterChannel < 0 || size_t(m_counterChannel) >= m_pendingChannels) { return; }

    for (size_t sample = 0; sample < m_pendingSamples; ++sample)
    {
        const double counter = m_pending[sample * m_pendingChannels + size_t(m_counterChannel)];
        if (m_haveCounter)
        {
            const double step = counter - m_lastCounter;
            // The first step tells how the counter counts; a step back is the counter wrapping around
            if (m_counterStep <= 0) { m_counterStep = step > 0 ? step : 0; }
            else if (step > 1.5 * m_counterStep)
            {
                const auto missing = uint64_t(std::llround(step / m_counterStep)) - 1;
                m_lostSamples += missing;
                std::cout << "[EEGO] The sample counter skipped " << missing << " samples: they were lost." << std::endl;
            }
        }
        m_lastCounter = counter;
        m_haveCounter = true;
    }
}

AbstractDriver::DeviceBuffer CDriverEEGO::takeDeviceBuffer()
{
    DeviceBuffer result;
    const uint32_t rate = m_bufferRate;
    if (!m_bufferReported || rate == 0) { return result; }

    result.reported    = true;
    result.peakSeconds = double(m_peakPending.exchange(0)) / rate;
    // While the reader is held up nothing is fetched, but the backlog grows all the same
    const int64_t sinceFetch = std::chrono::steady_clock::now().time_since_epoch().count() - m_lastFetchNs;
    result.peakSeconds       = (std::max)(result.peakSeconds, std::chrono::duration<double>(std::chrono::steady_clock::duration(sinceFetch)).count());
    result.lossCounted       = m_lossCounted;
    result.lostSamples       = m_lostSamples;
    return result;
}

int CDriverEEGO::read(sampleType* data, const uint64_t size)
{
    m_triggerValues.clear();  // UG fork
    if (!m_stream) { return 0; }

    if (m_pendingOffset >= m_pendingSamples && fetchPending() == 0)
    {
        // getData() does not block, so pace the acquisition server's reader loop ourselves.
        std::this_thread::sleep_for(IDLE_POLL);
        return 0;
    }

    const size_t channels = std::min(size_t(m_channelCount.value()), m_pendingChannels);
    const size_t count    = std::min(size_t(size), m_pendingSamples - m_pendingOffset);
    m_readFirst           = m_pendingOffset;  // UG fork: where in the block this read starts, see getData()

    for (size_t sample = 0; sample < count; ++sample)
    {
        const double* src = &m_pending[(m_pendingOffset + sample) * m_pendingChannels];
        sampleType*   dst = &data[sample * size_t(m_channelCount.value())];

        for (size_t channel = 0; channel < channels; ++channel)
        {
            // UG fork: volt to microvolt for the EEG and bipolar channels, see start()
            dst[channel] = sampleType(src[channel] * (channel < m_columnScale.size() ? m_columnScale[channel] : 1.0));
        }
        // start() rejects a stream narrower than announced, so this normally copies nothing; it is
        // here so a surprise short scan cannot leak the previous chunk's values.
        for (size_t channel = channels; channel < size_t(m_channelCount.value()); ++channel)
        {
            dst[channel] = sampleType(0);
        }

        // getData() turns the trigger word into triggers once it has the stamps
        if (m_triggerChannel >= 0) { m_triggerValues.push_back(uint32_t(src[m_triggerChannel])); }  // UG fork
    }

    m_pendingOffset += count;

    return int(count);
}

void CDriverEEGO::getData(samplesBuffer* buff)
{
    buff->count = read(buff->samplesBuff, buff->maxSampleCount);

    // Stamp every sample from when its SDK block was fetched and its place in that block, and keep the stamps strictly
    // increasing across blocks. The client places a stimulation on the sample whose stamp matches its date, so dating each
    // trigger with its own sample's stamp puts it on exactly that sample.
    // UG fork: before, this counted back from the end of this read, so a block bigger than one read came out too late, by up
    // to the block's length, with its later samples squeezed 1 ns apart. See UG.md, 2.24.
    const uint64_t periodNs = 1'000'000'000ULL / uint64_t(m_samplingRate.value());
    stampFromBlock(m_blockFetchTime, m_pendingSamples, m_readFirst, buff->count, periodNs, m_lastSampleTime, buff->timestampsBuff);

    // A trigger is a rising edge: the bits that went from 0 to 1 are the code, 1 -> 0 is ignored. The word counts once it has
    // held for two samples, and the trigger is dated at the first sample where it changed. A change can settle in a later read
    // than it started in: its date then is a sample sent before, and the client still places it on that sample.
    // UG fork: before, every change counted, so a word caught while the trigger lines switched gave a trigger for the lines
    // already on, and another for the rest. See UG.md, 2.25.
    for (size_t s = 0; s < m_triggerValues.size(); ++s)
    {
        const auto change = m_trigger.feed(m_triggerValues[s], buff->timestampsBuff[s]);
        const uint32_t rising = change ? change->value & ~change->previous : 0;
        if (rising != 0)
        {
            m_stimQueue.push_back(ovStimulation{ STIMULATION_LABEL_BASE | (rising & 0xFF), change->onset, 0 });
        }
    }
    m_triggerValues.clear();

    buff->stimCount = readStimulations(buff->stimulationsBuff, buff->maxStimCount);
}

int CDriverEEGO::readStimulations(ovStimulation* buff, const uint32_t maxCount)
{
    uint32_t count = 0;
    while (count < maxCount && !m_stimQueue.empty())
    {
        buff[count++] = m_stimQueue.front();
        m_stimQueue.pop_front();
    }
    return int(count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Impedance
// ─────────────────────────────────────────────────────────────────────────────

bool CDriverEEGO::startImpedanceCheck()
{
    if (!m_amplifier) { return false; }

    try
    {
        m_stream.reset(); // at most one stream may be alive at a time
        m_stream.reset(m_amplifier->OpenImpedanceStream(toChannelList(m_selectedChannels, true)));
    }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Could not open the impedance stream: " << ex.what() << std::endl;
        m_stream.reset();
        return false;
    }

    return m_stream != nullptr;
}

int CDriverEEGO::readImpedances(double* values, const size_t count)
{
    if (!m_stream) { return 0; }

    es::buffer data;
    try { data = m_stream->getData(); }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Error fetching impedances: " << ex.what() << std::endl;
        return 0;
    }

    if (data.getSampleCount() == 0) { return 0; }

    // The impedance stream only carries the referential channels, so it is always shorter than
    // channelCount() (which also counts the bipolar and status channels). Values are in ohm.
    const size_t measured = std::min(count, size_t(data.getChannelCount()));
    for (size_t channel = 0; channel < measured; ++channel)
    {
        values[channel] = data.getSample(uint32_t(channel), 0);
    }
    for (size_t channel = measured; channel < count; ++channel)
    {
        values[channel] = 0.0;
    }

    // Report only what was measured: the caller lists one row per returned value and names it from
    // the driver's channel names, so padding up to count would show fake 0 ohm bipolar rows.
    return int(measured);
}

bool CDriverEEGO::stopImpedanceCheck()
{
    // Only the stream goes: Acquisition::start() leaves impedance mode by calling this and then
    // starts streaming without re-running init(), so the amplifier has to survive.
    m_stream.reset();
    return true;
}

REGISTER_DRIVER(CDriverEEGO, "EEGO", false);
