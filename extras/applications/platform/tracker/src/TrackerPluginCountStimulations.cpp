#include <memory> // shared_ptr

#include "TrackerPluginCountStimulations.h"

#include "Stream.h"
#include "TypeStimulation.h"

namespace OpenViBE {
namespace Tracker {

bool TrackerPluginCountStimulations::process(StreamBundle& track)
{
    log() << Kernel::LogLevel_Info << "TrackerPluginCountStimulation: Counting stimulations in the selected streams\n";

    bool processedSomething = false;

    for (size_t s = 0; s < track.getNumStreams(); ++s) {
        const auto stream = track.getStream(s);

        // This plugin only handles stimulation streams
        if (stream->getSelected() && stream->getTypeIdentifier() == OV_TypeId_Stimulations) {
            processedSomething = true;

            std::map<uint64_t, uint64_t> histogram;
            double numStims = 0;

            const auto typedStream = std::static_pointer_cast<const Stream<TypeStimulation>>(stream);

            // Count occurrences
            for (size_t i = 0; i < typedStream->getChunkCount(); ++i) {
                const auto chunk = typedStream->getChunk(i);
                for (size_t stimIdx = 0; stimIdx < chunk->m_buffer.size(); ++stimIdx) {
                    auto id = chunk->m_buffer.getId(stimIdx);

                    auto it = histogram.find(id);
                    if (it != histogram.end()) { it->second++; }
                    else { histogram[id] = 1; }
                    numStims++;
                }
            }

            // Print
            log() << Kernel::LogLevel_Info << "Stimulation counts for stream " << (s + 1) << " ...\n";
            for (auto& it : histogram) {
                const CString name = m_kernelCtx.getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, it.first);

                log() << Kernel::LogLevel_Info << (name.length() > 0 ? name : "Unregistered") << " (" << it.first << ") " << " : " << it.second
                        << " (" << (it.second / numStims) * 100 << "%)" << "\n";
            }
        }
    }

    // This plugin does not modify the track. If it did, we would need to call the following:
    // track.setDirtyBit(true);

    if (!processedSomething) { log() << Kernel::LogLevel_Info << "No selected streams compatible with stimulation counting plugin\n"; }

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
