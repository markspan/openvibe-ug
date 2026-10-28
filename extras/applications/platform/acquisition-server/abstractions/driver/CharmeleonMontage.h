#pragma once

/**
 * @file CharmeleonMontage.h
 * @brief Reads Charmeleon montage files, which map amplifier channels to electrode names.
 *
 * A montage is a JSON object of flat entries:
 * @code
 * { "<name>": { "IsActive": true, "LabelText": "Cz", "HardwareChannel": 16, ... }, ... }
 * @endcode
 * HardwareChannel is 1-based and counts the amplifier's channels in device order. Inactive electrodes,
 * and those on channel 0, are not in use.
 */

#include <OVAcquisitionAbstractions_Export.h>

#include <map>
#include <string>
#include <vector>

/**
 * @brief Loads a Charmeleon montage.
 * @param path     Path of the montage file (.json).
 * @param error    Set to a description of the problem when the file can not be used.
 * @param warnings Entries that were skipped, e.g. a second electrode on an already used channel.
 * @return HardwareChannel (1-based) -> electrode name, for the electrodes in use. Empty on error.
 */
OVACQUISITIONABSTRACTIONS_EXPORT std::map<int, std::string> loadCharmeleonMontage(const std::string& path, std::string& error,
                                                                                  std::vector<std::string>& warnings);
