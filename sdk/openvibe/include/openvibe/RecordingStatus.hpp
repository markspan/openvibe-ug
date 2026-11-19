#pragma once
/**
 * @file RecordingStatus.hpp
 * @brief Whether a running scenario is writing a recording, for Designer to show.
 *
 * UG fork: new file, not in upstream OpenViBE. A box that writes a recording counts itself, while it writes, in a token
 * of its player's runtime configuration: the one it gets from getConfigurationManager(), which the player makes anew
 * for every run (IPlayer::getRuntimeConfigurationManager()). A count, so that several such boxes can run at once.
 * Designer reads it for the scenario it shows, and says "Recording" in its run time overlay (RuntimeOverlay.qml).
 */

#include "kernel/configuration/ovIConfigurationManager.h"

#include <cstdint>
#include <string>

namespace OpenViBE {
namespace RecordingStatus {

/// The token in a run's configuration that counts the boxes writing a recording
inline const char* const TOKEN = "Player_Recording";

/// The number of boxes writing a recording; looked up, not expanded, so that a run without any logs nothing
inline uint64_t count(const Kernel::IConfigurationManager& configuration)
{
    const std::string value = configuration.lookUpConfigurationTokenValue(TOKEN).toASCIIString();
    try { return value.empty() ? 0 : std::stoull(value); }
    catch (...) { return 0; }
}

/// Counts a box in when it starts writing, and out when it stops; a box must call each once
inline void set(Kernel::IConfigurationManager& configuration, const bool recording)
{
    const uint64_t before = count(configuration);
    const uint64_t after  = recording ? before + 1 : (before > 0 ? before - 1 : 0);
    configuration.addOrReplaceConfigurationToken(TOKEN, std::to_string(after).c_str());
}

}  // namespace RecordingStatus
}  // namespace OpenViBE
