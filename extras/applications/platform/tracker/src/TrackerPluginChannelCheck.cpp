#include "TrackerPluginChannelCheck.h"

#include "Stream.h"
#include "TypeSignal.h"

namespace OpenViBE {
namespace Tracker {

bool TrackerPluginChannelCheck::process(Workspace& wp, ParallelExecutor& /* exec */)
{
    log() << Kernel::LogLevel_Info << "TrackerPluginChannelCheck: Testing if all selected signal streams have the same number of channels ...\n";

    uint32_t nChannels = 0;
    size_t nTested     = 0;

    for (size_t t = 0; t < wp.getNumTracks(); ++t) {
        const auto& track = wp.getTrack(t);

        for (size_t s = 0; s < track->getNumStreams(); ++s) {
            const auto stream = track->getStream(s);

            // This plugin only handles signal streams
            if (stream->getSelected() && stream->getTypeIdentifier() == OV_TypeId_Signal) {
                nTested++;

                auto typedStream = std::static_pointer_cast<Stream<TypeSignal>>(stream);

                const auto& hdr             = typedStream->getHeader();
                const uint32_t thisChannels = hdr.m_Header.getDimensionSize(0);
                if (nTested == 1) { nChannels = thisChannels; }
                else if (nChannels != thisChannels) {
                    log() << Kernel::LogLevel_Error << "Stream " << (s + 1) << " of Track " << t + 1 << " has different number of signal channels ("
                            << thisChannels << ") than previous signal streams (" << nChannels << ")\n";
                    return true;
                }
            }
        }

        // This plugin does not modify the track. If it did, we would need to call the following:
        // track.setDirtyBit(true);
    }

    log() << Kernel::LogLevel_Info << "All signal streams have the same amount of channels (" << nTested << " tested)\n";

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
