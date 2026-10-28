/**
 * SPDX-FileCopyrightText: Inria / eemagine GmbH
 * SPDX-License-Identifier: AGPL-3.0-only
 *
 * Ported from the eego acquisition driver originally written by Steffen Heimes (eemagine GmbH).
 */

#include "CDriverEEGO.h"

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
      m_paramEEGMask("EEGMask", "Referential channel mask (0x.., 0b.., octal or decimal)",
                     "0xFFFFFFFFFFFFFFFF"),
      m_paramBIPMask("BIPMask", "Bipolar channel mask (0x.., 0b.., octal or decimal)", "0xFFFFFF"),
      m_paramMontage("Montage", "Charmeleon montage, selects and names the channels instead of the masks", "",
                     OpenViBE::Parameters::Read, { "Charmeleon montage (*.json)", "Any File (*)" })
{
    // The rate and the ranges are discrete sets the amplifier rejects anything outside of, so they
    // are dropdowns; the inherited m_samplingRate / m_channelCount are derived in init() and stay
    // out of the parameter list (same arrangement as CDriverLabStreamingLayer).
    m_parameters.push_back(&m_paramSamplingRate);
    m_parameters.push_back(&m_paramEEGRange);
    m_parameters.push_back(&m_paramBIPRange);
    m_parameters.push_back(&m_paramEEGMask);
    m_parameters.push_back(&m_paramBIPMask);
    m_parameters.push_back(&m_paramMontage);
}

CDriverEEGO::~CDriverEEGO() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Mask parsing
// ─────────────────────────────────────────────────────────────────────────────

bool CDriverEEGO::convertMask(const std::string& str, uint64_t& out)
{
    out = 0;

    // Trim, so a stray space in the settings file is not an error.
    const auto notSpace = [](const unsigned char chr) { return std::isspace(chr) == 0; };
    const auto begin    = std::find_if(str.begin(), str.end(), notSpace);
    const auto end      = std::find_if(str.rbegin(), str.rend(), notSpace).base();
    if (begin >= end) { return false; }
    const std::string input(begin, end);

    const auto startsWith = [&input](const char* prefix)
    {
        const size_t n = strlen(prefix);
        return input.size() > n && _strnicmp(input.c_str(), prefix, n) == 0;
    };

    std::string digits;
    int         base = 10;

    if (startsWith("0x")) { digits = input.substr(2); base = 16; }
    else if (startsWith("0b")) { digits = input.substr(2); base = 2; }
    else if (input.size() > 1 && input[0] == '0') { digits = input.substr(1); base = 8; }
    else { digits = input; }

    // Reject anything the base cannot represent. strtoull would silently stop at the first bad
    // character instead, turning a typo into a different -- and plausible-looking -- mask.
    const auto validDigit = [base](const unsigned char chr)
    {
        switch (base)
        {
            case 2: return chr == '0' || chr == '1';
            case 8: return chr >= '0' && chr < '8';
            case 16: return std::isxdigit(chr) != 0;
            default: return std::isdigit(chr) != 0;
        }
    };

    if (digits.empty() || !std::all_of(digits.begin(), digits.end(), validDigit)) { return false; }

    out = strtoull(digits.c_str(), nullptr, base);
    return true;
}

uint64_t CDriverEEGO::refMask() const
{
    if (!m_montage.empty()) { return m_montageRefMask; }

    uint64_t mask = 0;
    if (!convertMask(m_paramEEGMask.value(), mask))
    {
        std::cout << "[EEGO] Invalid EEG mask '" << m_paramEEGMask.value() << "', using none." << std::endl;
    }
    return mask;
}

uint64_t CDriverEEGO::bipMask() const
{
    if (!m_montage.empty()) { return m_montageBipMask; }

    uint64_t mask = 0;
    if (!convertMask(m_paramBIPMask.value(), mask))
    {
        std::cout << "[EEGO] Invalid BIP mask '" << m_paramBIPMask.value() << "', using none." << std::endl;
    }
    return mask;
}

bool CDriverEEGO::loadMontage()
{
    m_montage.clear();
    m_montageRefMask = 0;
    m_montageBipMask = 0;

    if (m_paramMontage.value().empty()) { return true; }

    std::string error;
    std::vector<std::string> warnings;
    std::map<int, std::string> montage = loadCharmeleonMontage(m_paramMontage.value(), error, warnings);
    for (const auto& warning : warnings) { std::cout << "[EEGO] Montage: " << warning << std::endl; }
    if (montage.empty())
    {
        std::cout << "[EEGO] Montage: " << error << std::endl;
        return false;
    }

    int nRef = 0;
    int nBip = 0;
    for (const auto& channel : m_amplifier->getChannelList())
    {
        if (channel.getType() == es::channel::reference) { nRef++; }
        else if (channel.getType() == es::channel::bipolar) { nBip++; }
    }

    for (auto it = montage.begin(); it != montage.end();)
    {
        const int hardwareChannel = it->first;
        if (hardwareChannel <= nRef && hardwareChannel <= 64) { m_montageRefMask |= uint64_t(1) << (hardwareChannel - 1); }
        else if (hardwareChannel > nRef && hardwareChannel - nRef <= nBip && hardwareChannel - nRef <= 64)
        {
            m_montageBipMask |= uint64_t(1) << (hardwareChannel - nRef - 1);
        }
        else
        {
            std::cout << "[EEGO] Montage: " << it->second << " is on hardware channel " << hardwareChannel << ", but the amplifier has "
                      << nRef << " EEG + " << nBip << " BIP channels. Ignored." << std::endl;
            it = montage.erase(it);
            continue;
        }
        ++it;
    }

    if (montage.empty())
    {
        std::cout << "[EEGO] Montage: no channel of " << m_paramMontage.value() << " exists on this amplifier." << std::endl;
        return false;
    }

    m_montage = std::move(montage);
    std::cout << "[EEGO] Montage " << m_paramMontage.value() << " selects " << m_montage.size()
              << " channels; the EEG and BIP masks are not used." << std::endl;
    return true;
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

        success = loadMontage() && buildChannelNames();
    }
    catch (const std::exception& ex)
    {
        std::cout << "[EEGO] Failed to initialise: " << ex.what() << std::endl;
    }

    if (!success)
    {
        m_stream.reset();
        m_amplifier.reset();
    }

    return success;
}

bool CDriverEEGO::buildChannelNames()
{
    // The amplifier can enumerate the selected channels without a stream being open, which matters:
    // the SDK allows only one stream at a time and the acquisition server reads channelCount()
    // straight after init(), long before start() opens one.
    const std::vector<es::channel> channels = m_amplifier->getChannelList(refMask(), bipMask());

    if (channels.empty())
    {
        std::cout << "[EEGO] The channel masks select no channel on this amplifier." << std::endl;
        return false;
    }

    // eego channels carry a type and an index but no name, so the names come from the montage or
    // are synthesised.
    std::vector<std::string> names;
    names.reserve(channels.size() + STATUS_CHANNEL_COUNT);

    int nEEG = 0;
    int nBIP = 0;
    if (!m_montage.empty())
    {
        // The masks were built from the montage, and the SDK lists the selected channels in
        // hardware order, which is the montage's key order.
        for (const auto& entry : m_montage) { names.push_back(entry.second); }
        for (const auto& channel : channels)
        {
            if (channel.getType() == es::channel::reference) { nEEG++; }
            else if (channel.getType() == es::channel::bipolar) { nBIP++; }
        }
        if (names.size() != channels.size())
        {
            std::cout << "[EEGO] The montage names " << names.size() << " channels but the amplifier selected "
                      << channels.size() << "." << std::endl;
            return false;
        }
    }
    else
    {
        for (const auto& channel : channels)
        {
            switch (channel.getType())
            {
                case es::channel::reference: names.push_back("EEG" + std::to_string(++nEEG)); break;
                case es::channel::bipolar: names.push_back("BIP" + std::to_string(++nBIP)); break;
                default: names.push_back("CH" + std::to_string(int(names.size()) + 1)); break;
            }
        }
    }

    names.push_back("Trigger");
    names.push_back("SampleCounter");

    m_channelCount.setValue(int(names.size()));
    m_channelNames.setValue(names);

    std::cout << "[EEGO] " << nEEG << " EEG + " << nBIP << " BIP + " << STATUS_CHANNEL_COUNT
              << " status channels at " << m_samplingRate.value() << " Hz." << std::endl;

    return true;
}

bool CDriverEEGO::unInit()
{
    m_stream.reset();
    m_amplifier.reset();

    m_pending.clear();
    m_pendingChannels = 0;
    m_pendingSamples  = 0;
    m_pendingOffset   = 0;
    m_stimQueue.clear();

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

bool CDriverEEGO::start()
{
    if (!m_amplifier) { return false; }

    m_pendingSamples   = 0;
    m_pendingOffset    = 0;
    m_lastTriggerValue = ~0u; // every bit high, so the first word cannot look like a rising edge
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
                                                  refMask(), bipMask()));
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

        const auto trigger = std::find_if(channels.begin(), channels.end(), [](const es::channel& chan)
        {
            return chan.getType() == es::channel::trigger;
        });
        if (trigger != channels.end()) { m_triggerChannel = int(trigger - channels.begin()); }
        else { std::cout << "[EEGO] No trigger channel in the stream; no stimulations will be produced." << std::endl; }
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

    if (m_pendingSamples == 0) { return 0; }

    // eemagine::sdk::buffer stores _data[channel + sample * channel_count], i.e. sample-major --
    // the same order the acquisition server wants, so this is a plain copy rather than a transpose.
    const size_t total = m_pendingSamples * m_pendingChannels;
    m_pending.assign(data.data(), data.data() + total);

    return m_pendingSamples;
}

int CDriverEEGO::read(sampleType* data, const uint64_t size)
{
    m_pendingTriggers.clear();
    if (!m_stream) { return 0; }

    if (m_pendingOffset >= m_pendingSamples && fetchPending() == 0)
    {
        // getData() does not block, so pace the acquisition server's reader loop ourselves.
        std::this_thread::sleep_for(IDLE_POLL);
        return 0;
    }

    const size_t channels = std::min(size_t(m_channelCount.value()), m_pendingChannels);
    const size_t count    = std::min(size_t(size), m_pendingSamples - m_pendingOffset);

    for (size_t sample = 0; sample < count; ++sample)
    {
        const double* src = &m_pending[(m_pendingOffset + sample) * m_pendingChannels];
        sampleType*   dst = &data[sample * size_t(m_channelCount.value())];

        for (size_t channel = 0; channel < channels; ++channel)
        {
            dst[channel] = sampleType(src[channel]);
        }
        // start() rejects a stream narrower than announced, so this normally copies nothing; it is
        // here so a surprise short scan cannot leak the previous chunk's values.
        for (size_t channel = channels; channel < size_t(m_channelCount.value()); ++channel)
        {
            dst[channel] = sampleType(0);
        }

        // A trigger is a rising edge: bits that went from 0 to 1 are the code, 1 -> 0 is ignored.
        if (m_triggerChannel >= 0)
        {
            const uint32_t triggers = uint32_t(src[m_triggerChannel]);
            const uint32_t rising   = triggers & ~m_lastTriggerValue;
            m_lastTriggerValue      = triggers;

            // getData() dates it at this sample once the read timestamp is known
            if (rising != 0)
            {
                m_pendingTriggers.emplace_back(STIMULATION_LABEL_BASE | (rising & 0xFF), sample);
            }
        }
    }

    m_pendingOffset += count;

    return int(count);
}

void CDriverEEGO::getData(samplesBuffer* buff)
{
    buff->count             = read(buff->samplesBuff, buff->maxSampleCount);
    const uint64_t readTime = timestamp();
    if (buff->count > 0) { buff->timestampsBuff[buff->count - 1] = readTime; }

    // Date each trigger at its own sample, counting back from the last sample of the block, which is stamped readTime
    const uint64_t periodNs = 1'000'000'000ULL / uint64_t(m_samplingRate.value());
    for (const auto& [id, sample] : m_pendingTriggers)
    {
        const uint64_t samplesBefore = buff->count - 1 - sample;
        m_stimQueue.push_back(ovStimulation{ id, readTime - samplesBefore * periodNs, 0 });
    }
    m_pendingTriggers.clear();

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
        m_stream.reset(m_amplifier->OpenImpedanceStream(refMask()));
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
