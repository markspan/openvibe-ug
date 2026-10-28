/**
 * @file CharmeleonMontage.cpp
 * @brief Implementation of the Charmeleon montage reader.
 */

#include <CharmeleonMontage.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

std::map<int, std::string> loadCharmeleonMontage(const std::string& path, std::string& error, std::vector<std::string>& warnings)
{
    std::map<int, std::string> montage;
    error.clear();
    warnings.clear();

    std::ifstream file(std::filesystem::u8path(path));
    if (!file.is_open()) {
        error = "can not open " + path;
        return montage;
    }

    // Ordered, so that of two electrodes on the same channel the first one in the file wins
    const nlohmann::ordered_json json = nlohmann::ordered_json::parse(file, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        error = path + " is not a Charmeleon montage (expected a JSON object)";
        return montage;
    }

    for (const auto& entry : json) {
        if (!entry.is_object() || !entry.contains("LabelText") || !entry.contains("HardwareChannel")) { continue; }

        // Same rule as Charmeleon itself: an electrode is in use when active and on a channel above 0
        if (entry.contains("IsActive") && entry["IsActive"].is_boolean() && !entry["IsActive"].get<bool>()) { continue; }

        const auto& label   = entry["LabelText"];
        const auto& channel = entry["HardwareChannel"];
        if (!label.is_string() || label.get<std::string>().empty()) { continue; }

        int index = 0;
        if (channel.is_number_integer()) { index = channel.get<int>(); }
        else if (channel.is_string()) {
            try { index = std::stoi(channel.get<std::string>()); }
            catch (const std::exception&) { continue; }
        }
        if (index <= 0) { continue; }

        const auto [it, inserted] = montage.emplace(index, label.get<std::string>());
        if (!inserted) {
            warnings.push_back(label.get<std::string>() + " is on hardware channel " + std::to_string(index) + ", already used by " + it->second
                               + ". Ignored.");
        }
    }

    if (montage.empty()) { error = path + " has no active electrode with a hardware channel"; }
    return montage;
}
