//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include "Selection.h"

#include <sstream>
#include <iomanip>

namespace OpenViBE {
namespace Tracker {

bool Selection::reset(const bool state) const
{
    for (auto& t : m_tracks) { for (auto& s : t->getAllStreams()) { s->setSelected(state); } }
    return true;
}

bool Selection::isEmpty() const
{
    for (const auto& t : m_tracks) { if (t->getNumStreams() > 0) { return false; } }
    return true;
}

bool Selection::isSomethingSelected() const
{
    for (const auto& t : m_tracks) { for (const auto& str : t->getAllStreams()) { if (str->getSelected()) { return true; } } }
    return false;
}


bool Selection::isTrackSelected(const size_t track) const
{
    if (track >= m_tracks.size()) { return false; }
    for (const auto& s : m_tracks[track]->getAllStreams()) { if (s->getSelected()) { return true; } }
    return false;
}

bool Selection::isSelectionConsistent() const
{
    if (isEmpty() || !isSomethingSelected()) { return true; }

    std::vector<CIdentifier> selectedTypesPrevious;

    // Check that selection for each track is identical to selection of previous track
    // (empty selections per track are ignored)
    for (auto& t : m_tracks) {
        std::vector<CIdentifier> selectedTypes;
        for (const auto& s : t->getAllStreams()) { if (s->getSelected()) { selectedTypes.push_back(s->getTypeIdentifier()); } }
        if (!selectedTypesPrevious.empty() && !selectedTypes.empty() && selectedTypesPrevious != selectedTypes) { return false; }
        if (!selectedTypes.empty()) { selectedTypesPrevious = selectedTypes; }
    }

    return true;
}

size_t Selection::countSelectedTracks() const
{
    size_t selectedTracks = 0;

    for (const auto& t : m_tracks) {
        for (const auto& s : t->getAllStreams()) {
            if (s->getSelected()) {
                selectedTracks++;
                break;
            }
        }
    }

    return selectedTracks;
}

size_t Selection::countSelectedStreams(const size_t trackIndex) const
{
    if (trackIndex >= m_tracks.size()) { return 0; }

    size_t numSelected = 0;
    for (const auto& str : m_tracks[trackIndex]->getAllStreams()) { if (str && str->getSelected()) { numSelected++; } }
    return numSelected;
}

bool Selection::save(const char* prefix) const
{
    auto& mgr = this->getKernelContext().getConfigurationManager();

    // @note with really huge datasets the .ovw file handling can get slow. should be profiled.
    for (size_t t = 0; t < m_tracks.size(); ++t) {
        std::stringstream token;
        token << prefix << "Track_" << std::setw(3) << std::setfill('0') << (t + 1) << "_Selected";
        std::stringstream value;

        for (size_t s = 0; s < m_tracks[t]->getNumStreams(); ++s) { if (m_tracks[t]->getStream(s)->getSelected()) { value << (s + 1) << " "; } }

        mgr.addOrReplaceConfigurationToken(token.str().c_str(), value.str().c_str());
    }

    return true;
}

// Relies  on the current track/stream set being compatible with the tokens. no error checking
bool Selection::load(const char* prefix) const
{
    auto& mgr = this->getKernelContext().getConfigurationManager();

    for (size_t t = 0; t < m_tracks.size(); ++t) {
        std::stringstream token;
        token << prefix << "Track_" << std::setw(3) << std::setfill('0') << (t + 1) << "_Selected";

        std::stringstream value(mgr.lookUpConfigurationTokenValue(token.str().c_str()).toASCIIString());

        // Mark everything as unselected by default
        for (size_t s = 0; s < m_tracks[t]->getNumStreams(); ++s) { m_tracks[t]->getStream(s)->setSelected(false); }

        // Load sparse selection from file
        std::string selected;
        while (std::getline(value, selected, ' ')) {
            const uint32_t selectedIdx = atoi(selected.c_str()) - 1;
            if (selectedIdx < m_tracks[t]->getNumStreams()) { m_tracks[t]->getStream(selectedIdx)->setSelected(true); }
        }
    }

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
