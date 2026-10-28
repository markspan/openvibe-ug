#pragma once

/**
 * @file Charmeleon.h
 * @brief Launching and closing Charmeleon, the external impedance checker for EEGO and TMSi amplifiers.
 *
 * Charmeleon opens the amplifier itself, so it must be gone before an acquisition opens it.
 */

#include <string>

namespace Charmeleon {

/**
 * @brief Finds Charmeleon.exe: the AcquisitionServer_CharmeleonPath token if set, else where its
 *        installer put it.
 * @param configuredPath Value of the configuration token, may be empty.
 * @return Full path of Charmeleon.exe, or an empty string when it is not installed.
 */
std::string findExecutable(const std::string& configuredPath);

/// Starts Charmeleon, or brings it to the front when it already runs. Returns false when it could not be started.
bool launch(const std::string& executable);

/**
 * @brief Closes every running Charmeleon, so that it releases the amplifier.
 *
 * Asks each one to close first, which lets it release the amplifier cleanly, and terminates those
 * still running after @p timeoutMs.
 * @return Number of instances that were running.
 */
int closeAll(unsigned long timeoutMs = 3000);

}  // namespace Charmeleon
