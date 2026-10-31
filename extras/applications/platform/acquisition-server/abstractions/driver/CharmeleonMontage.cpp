/**
 * @file CharmeleonMontage.cpp
 * @brief Implementation of the Charmeleon montage reader.
 *
 * UG fork: new file, not in upstream OpenViBE.
 */

#include <CharmeleonMontage.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <set>
#include <sstream>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace fs = std::filesystem;

namespace {

/// Key of the entry that describes the amplifier, when it is not an electrode
const char* const AMPLIFIER_KEY = "Amplifier";

/// How many electrodes a summary names before it counts the rest
constexpr size_t LISTED_ELECTRODES = 6;

std::string trim(const std::string& s)
{
    const auto begin = std::find_if_not(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c); });
    const auto end   = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return begin < end ? std::string(begin, end) : std::string();
}

std::string toUtf8(const fs::path& path)
{
    const auto utf8 = path.u8string();
    return std::string(utf8.begin(), utf8.end());
}

std::string address(const CharmeleonMontage::Electrode& electrode)
{
    return electrode.kind == MontageChannelKind::EEG ? "EEG " + std::to_string(electrode.number)
                                                     : montageChannelPrefix(electrode.kind) + std::to_string(electrode.number);
}

/// "Left (EEG 73), Right (EEG 75) and 3 more"
std::string listElectrodes(const std::vector<CharmeleonMontage::Electrode>& electrodes)
{
    std::string list;
    for (size_t i = 0; i < electrodes.size() && i < LISTED_ELECTRODES; ++i) {
        if (i > 0) { list += (i + 1 == electrodes.size() ? " and " : ", "); }
        list += electrodes[i].name + " (" + address(electrodes[i]) + ")";
    }
    if (electrodes.size() > LISTED_ELECTRODES) { list += " and " + std::to_string(electrodes.size() - LISTED_ELECTRODES) + " more"; }
    return list;
}

std::string plural(const size_t count, const std::string& singular, const std::string& pluralForm)
{
    return std::to_string(count) + " " + (count == 1 ? singular : pluralForm);
}

/// An integer field that Charmeleon writes as a number, and older montages sometimes as a string; 0 when absent or invalid.
int readInt(const nlohmann::ordered_json& entry, const char* key)
{
    if (!entry.contains(key)) { return 0; }
    const auto& value = entry[key];
    if (value.is_number_integer()) { return value.get<int>(); }
    if (value.is_string()) {
        try { return std::stoi(value.get<std::string>()); }
        catch (const std::exception&) { return 0; }
    }
    return 0;
}

std::string readString(const nlohmann::ordered_json& entry, const char* key)
{
    return entry.contains(key) && entry[key].is_string() ? trim(entry[key].get<std::string>()) : std::string();
}

/// Parses "BIP1", "aux 2", ...; false for anything else.
bool parseChannel(const std::string& text, MontageChannelKind& kind, int& number)
{
    static const std::regex pattern(R"(^\s*(BIP|AUX)\s*0*([1-9][0-9]*)\s*$)", std::regex::icase);
    std::smatch match;
    if (!std::regex_match(text, match, pattern)) { return false; }
    std::string prefix = match[1].str();
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return char(std::toupper(c)); });
    kind = prefix == "BIP" ? MontageChannelKind::Bipolar : MontageChannelKind::Auxiliary;
    try { number = std::stoi(match[2].str()); }
    catch (const std::exception&) { return false; }
    return true;
}

std::string readFile(const fs::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return file ? std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()) : std::string();
}

fs::path keptCopiesFolder()
{
#if defined(_WIN32)
    if (const wchar_t* local = _wgetenv(L"LOCALAPPDATA"); local && *local) { return fs::path(local) / L"OpenViBE" / L"acquisition-server" / L"montages"; }
#else
    if (const char* home = std::getenv("HOME"); home && *home) { return fs::path(home) / ".local" / "share" / "openvibe" / "acquisition-server" / "montages"; }
#endif
    return {};
}

/// The kept copy of the montage a setting names: its file name plus a hash of the whole setting, so that two
/// montages of the same name in different folders keep a copy each.
fs::path keptCopyPath(const std::string& setting)
{
    const fs::path folder = keptCopiesFolder();
    if (folder.empty()) { return {}; }

    uint32_t hash = 2166136261u;  // FNV-1a
    for (const unsigned char c : setting) { hash = (hash ^ c) * 16777619u; }
    char suffix[16];
    std::snprintf(suffix, sizeof(suffix), "-%08x", unsigned(hash));

    fs::path name = fs::u8path(setting).filename().stem();
    name += suffix;
    name += ".json";
    return folder / name;
}

bool isFile(const fs::path& path)
{
    std::error_code ec;
    return !path.empty() && fs::is_regular_file(path, ec);
}

}  // namespace

std::string montageChannelPrefix(const MontageChannelKind kind)
{
    switch (kind) {
        case MontageChannelKind::Bipolar: return "BIP";
        case MontageChannelKind::Auxiliary: return "AUX";
        default: return "";
    }
}

bool loadCharmeleonMontage(const std::string& path, CharmeleonMontage& montage, std::string& error)
{
    montage = CharmeleonMontage();
    error.clear();

    std::ifstream file(fs::u8path(path));
    if (!file.is_open()) {
        error = "can not open " + path;
        return false;
    }

    // Ordered, so that of two electrodes on the same channel the first one in the file wins
    const nlohmann::ordered_json json = nlohmann::ordered_json::parse(file, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        error = path + " is not a Charmeleon montage (expected a JSON object)";
        return false;
    }

    std::map<std::pair<MontageChannelKind, int>, std::string> used;
    std::map<std::string, std::string> namesUsed;
    for (auto it = json.begin(); it != json.end(); ++it) {
        const auto& entry = it.value();
        if (!entry.is_object()) { continue; }

        if (it.key() == AMPLIFIER_KEY && !entry.contains("LabelText")) {
            montage.hasAmplifier                = true;
            montage.amplifier.type              = readString(entry, "Type");
            montage.amplifier.eegChannels       = readInt(entry, "EEGChannels");
            montage.amplifier.bipolarChannels   = readInt(entry, "BipolarChannels");
            montage.amplifier.auxiliaryChannels = readInt(entry, "AuxiliaryChannels");
            continue;
        }

        // Same rule as Charmeleon itself: an electrode is in use when active and on a channel
        if (entry.contains("IsActive") && entry["IsActive"].is_boolean() && !entry["IsActive"].get<bool>()) { continue; }
        const std::string name = readString(entry, "LabelText");
        if (name.empty()) { continue; }

        CharmeleonMontage::Electrode electrode{name, MontageChannelKind::EEG, readInt(entry, "HardwareChannel")};
        if (const std::string channel = readString(entry, "Channel"); !channel.empty()) {
            if (!parseChannel(channel, electrode.kind, electrode.number)) {
                montage.problems.push_back(name + " is on channel \"" + channel + "\", which is not BIP or AUX and a number, so it is left out.");
                continue;
            }
        }
        if (electrode.number <= 0) { continue; }

        const auto [slot, inserted] = used.emplace(std::make_pair(electrode.kind, electrode.number), name);
        if (!inserted) {
            montage.problems.push_back(name + " is on " + address(electrode) + " as well as " + slot->second + ", so it is left out.");
            continue;
        }
        if (const auto [other, unique] = namesUsed.emplace(name, address(electrode)); !unique) {
            montage.problems.push_back(name + " names both " + other->second + " and " + address(electrode) + ".");
        }
        montage.electrodes.push_back(electrode);
    }

    if (montage.electrodes.empty()) {
        error = path + " has no electrode in use (active and on a channel)";
        return false;
    }
    return true;
}

MontageApplication applyCharmeleonMontage(const CharmeleonMontage& montage, const std::vector<MontageChannelKind>& layout, std::vector<std::string> names,
                                          const bool assumed)
{
    MontageApplication result;
    const bool layoutKnown = !layout.empty();
    // Without the amplifier, an assumed layout, or none at all: the result is only a preview
    const bool provisional = assumed || !layoutKnown;
    const std::vector<MontageChannelKind> kinds = layoutKnown ? layout : std::vector<MontageChannelKind>(names.size(), MontageChannelKind::EEG);

    std::set<std::string> montageNames;
    for (const auto& electrode : montage.electrodes) { montageNames.insert(electrode.name); }

    // The channels of each kind, in device order. EEG channels are the montage's to name, all of them.
    // Other channels keep the names the user gave them, except a montage electrode's name: that is left
    // from an older montage, which put electrodes beyond the EEG channels on them
    std::map<MontageChannelKind, std::vector<size_t>> channels;
    result.locked.assign(names.size(), false);
    for (size_t i = 0; i < names.size() && i < kinds.size(); ++i) {
        channels[kinds[i]].push_back(i);
        if (kinds[i] == MontageChannelKind::EEG) {
            names[i].clear();
            result.locked[i] = true;
        }
        else if (montageNames.count(trim(names[i])) != 0) { names[i].clear(); }
    }

    size_t nEEG = 0, nExtra = 0;
    std::vector<CharmeleonMontage::Electrode> missing, pending;
    std::vector<bool> named(names.size(), false);
    for (const auto& electrode : montage.electrodes) {
        if (!layoutKnown && electrode.kind != MontageChannelKind::EEG) {
            pending.push_back(electrode);
            continue;
        }
        const std::vector<size_t>& ofKind = channels[electrode.kind];
        if (size_t(electrode.number) > ofKind.size()) {
            missing.push_back(electrode);
            continue;
        }
        const size_t row = ofKind[size_t(electrode.number) - 1];
        names[row]         = electrode.name;
        result.locked[row] = true;
        named[row]         = true;
        ++(electrode.kind == MontageChannelKind::EEG ? nEEG : nExtra);
    }
    // Without the amplifier the guess is only a preview: what turns out to be another channel must stay editable
    if (provisional) { result.locked = named; }

    std::ostringstream summary;
    summary << plural(nEEG, "EEG electrode", "EEG electrodes");
    if (nExtra > 0) { summary << " and " << plural(nExtra, "other channel", "other channels"); }
    summary << ".";
    if (!layoutKnown) {
        summary << " No amplifier connected, so the names are placed in channel order; Play places them on the amplifier's EEG channels.";
        if (!pending.empty()) { summary << " Named on Play: " << listElectrodes(pending) << "."; }
    }
    else if (assumed) {
        summary << " No amplifier connected, so the names are placed on the channels such an amplifier usually has; Play places them on the "
                   "amplifier's own channels.";
    }
    if (!missing.empty()) {
        summary << " Not recorded, as the " << (provisional ? "list" : "amplifier") << " has no such channel: " << listElectrodes(missing) << ".";
        result.warning = true;
    }
    if (!provisional && montage.hasAmplifier) {
        const auto count = [&](MontageChannelKind kind) { return int(channels[kind].size()); };
        const CharmeleonMontage::Amplifier& amp = montage.amplifier;
        if ((amp.eegChannels > 0 && amp.eegChannels != count(MontageChannelKind::EEG))
            || (amp.bipolarChannels > 0 && amp.bipolarChannels != count(MontageChannelKind::Bipolar))
            || (amp.auxiliaryChannels > 0 && amp.auxiliaryChannels != count(MontageChannelKind::Auxiliary))) {
            summary << " Made for " << (amp.type.empty() ? "an amplifier" : amp.type) << " with " << amp.eegChannels << " EEG, " << amp.bipolarChannels
                    << " BIP and " << amp.auxiliaryChannels << " AUX channels; this one has " << count(MontageChannelKind::EEG) << ", "
                    << count(MontageChannelKind::Bipolar) << " and " << count(MontageChannelKind::Auxiliary) << ".";
            result.warning = true;
        }
    }
    for (const auto& problem : montage.problems) {
        summary << " " << problem;
        result.warning = true;
    }

    result.names   = std::move(names);
    result.summary = summary.str();
    return result;
}

std::string charmeleonInstallFolder()
{
#if defined(_WIN32)
    const wchar_t* const key = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Charmeleon_is1";
    for (HKEY root : {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER}) {
        wchar_t value[MAX_PATH] = {};
        DWORD size              = sizeof(value);
        if (RegGetValueW(root, key, L"InstallLocation", RRF_RT_REG_SZ, nullptr, value, &size) == ERROR_SUCCESS && value[0] != 0) {
            return toUtf8(fs::path(value));
        }
    }
#endif
    return {};
}

MontageLocation locateCharmeleonMontage(const std::string& setting)
{
    MontageLocation location;
    if (trim(setting).empty()) { return location; }

    const fs::path given = fs::u8path(setting);
    if (isFile(given)) { return {toUtf8(given), MontageLocation::Source::AsGiven}; }

    // Charmeleon's library ("ca-208.json"), then its default layouts ("1020Layout.json")
    if (const std::string folder = charmeleonInstallFolder(); !folder.empty() && given.has_filename()) {
        for (const wchar_t* sub : {L"montages", L"Resources"}) {
            const fs::path candidate = fs::u8path(folder) / sub / given.filename();
            if (isFile(candidate)) { return {toUtf8(candidate), MontageLocation::Source::CharmeleonFolder}; }
        }
    }

    if (const fs::path kept = keptCopyPath(setting); isFile(kept)) { return {toUtf8(kept), MontageLocation::Source::KeptCopy}; }
    return location;
}

void keepCharmeleonMontageCopy(const std::string& setting, const std::string& path)
{
    const fs::path kept = keptCopyPath(setting);
    if (kept.empty() || kept == fs::u8path(path)) { return; }

    const std::string contents = readFile(fs::u8path(path));
    if (contents.empty() || (isFile(kept) && readFile(kept) == contents)) { return; }

    std::error_code ec;
    fs::create_directories(kept.parent_path(), ec);
    std::ofstream out(kept, std::ios::binary | std::ios::trunc);
    out << contents;
}

std::string montageSettingAfterSave(const std::string& setting, const std::string& savedPath)
{
    if (trim(savedPath).empty()) { return setting; }
    const MontageLocation location = locateCharmeleonMontage(setting);
    std::error_code ec;
    if (location.source != MontageLocation::Source::NotFound && fs::equivalent(fs::u8path(location.path), fs::u8path(savedPath), ec)) {
        return setting;
    }
    return savedPath;
}

bool applyCharmeleonMontageSetting(const std::string& setting, const std::vector<MontageChannelKind>& layout, const std::vector<std::string>& names,
                                   MontageApplication& result, std::string& error, const bool assumed)
{
    const MontageLocation location = locateCharmeleonMontage(setting);
    if (location.source == MontageLocation::Source::NotFound) {
        error = "Montage " + setting + " not found.";
        return false;
    }

    CharmeleonMontage montage;
    if (!loadCharmeleonMontage(location.path, montage, error)) {
        error = "Montage: " + error + ".";
        return false;
    }

    result = applyCharmeleonMontage(montage, layout, names, assumed);
    std::string from;
    switch (location.source) {
        case MontageLocation::Source::CharmeleonFolder: from = " (from " + location.path + ")"; break;
        case MontageLocation::Source::KeptCopy:
            from           = " (the file is gone, so this is the copy kept from its last use)";
            result.warning = true;
            break;
        default: break;
    }
    result.summary = toUtf8(fs::u8path(setting).filename()) + from + ": " + result.summary;

    if (location.source != MontageLocation::Source::KeptCopy) { keepCharmeleonMontageCopy(setting, location.path); }
    return true;
}
