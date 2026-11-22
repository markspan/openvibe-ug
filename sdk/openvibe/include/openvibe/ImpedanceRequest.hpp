#pragma once
/**
 * @file ImpedanceRequest.hpp
 * @brief Whether a scenario wants the amplifier's impedances measured when it starts.
 *
 * UG fork: new file, not in upstream OpenViBE. See UG.md, 2.63. A box that records the impedances (the BrainVision Format file
 * writer (UG)) says so when it initialises, in a token of its player's runtime configuration: the one it gets from
 * getConfigurationManager(), which the player makes anew for every run, as RecordingStatus.hpp does. The Acquisition client reads
 * it once every box has initialised, at its first process(), and then asks the acquisition server to measure the impedances before
 * it asks for the stream.
 */

#include "kernel/configuration/ovIConfigurationManager.h"

#include <string>

namespace OpenViBE {
namespace ImpedanceRequest {

/// The token in a run's configuration that a box sets when it wants the impedances
inline const char* const TOKEN = "Player_WantsImpedances";

/// Says that this run wants the impedances; for a box's initialize()
inline void ask(Kernel::IConfigurationManager& configuration) { configuration.addOrReplaceConfigurationToken(TOKEN, "true"); }

/// Whether a box of this run asked for the impedances; looked up, not expanded, so that a run without one logs nothing
inline bool wanted(const Kernel::IConfigurationManager& configuration)
{
    return std::string(configuration.lookUpConfigurationTokenValue(TOKEN).toASCIIString()) == "true";
}

}  // namespace ImpedanceRequest
}  // namespace OpenViBE
