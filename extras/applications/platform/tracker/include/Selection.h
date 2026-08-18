//
// OpenViBE Tracker
//
#pragma once

#include <string>
#include <vector>

#include "StreamBundle.h"
#include "Contexted.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class Selection 
 * \brief Some convenience functions for Track/Stream selections
 * \author J. T. Lindgren
 *
 */
class Selection final : protected Contexted
{
public:
    Selection(const Kernel::IKernelContext& ctx, const std::vector<StreamBundle*>& tracks) : Contexted(ctx), m_tracks(tracks) { }

    // Set the selection status of all streams on all tracks to the given state
    bool reset(bool state) const;

    // Is any stream selected on the track?
    bool isTrackSelected(size_t track) const;
    // Nothing selectable?
    bool isEmpty() const;
    // Is something currently selected?
    bool isSomethingSelected() const;
    // Are selections of different tracks compatible?
    bool isSelectionConsistent() const;
    // How many tracks have at least one stream selected?
    size_t countSelectedTracks() const;
    // How many streams does a current track have selected?
    size_t countSelectedStreams(size_t trackIndex) const;

    // Serialize state to ConfigurationManager
    bool save(const char* prefix) const;
    bool load(const char* prefix) const;

protected:
    const std::vector<StreamBundle*>& m_tracks;
};
}  // namespace Tracker
}  // namespace OpenViBE
