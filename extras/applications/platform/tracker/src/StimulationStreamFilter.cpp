//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 

#include "Stream.h"
#include "StimulationStreamFilter.h"

#include "TypeStimulation.h"

namespace OpenViBE {
namespace Tracker {

// @note At some point I thought to make a derived class of stimulation stream that simply wouldn't pass through
// the filtered stimulations on calling the getChunk(), but this resulted in some issues. Since modifying
// the original stream would not have been appropriate, I'd have had to choose one of the following, 
// 1) make getChunk() in general copy all data instead of returning pointers 2) drop const qualifier 
// from the getChunk() to keep track of allocated memory internally in the derived class or 3) make getChunk() 
// return a smart pointer. I wasn't very happy about these options, so instead we just make a modded copy of the stream.
std::shared_ptr<StreamBase> filterStimulationStreamEndPoints(const std::shared_ptr<const StreamBase>& src, const Kernel::IKernelContext& ctx)
{
    const std::vector<uint64_t> stims = { OVTK_StimulationId_ExperimentStop, OVTK_StimulationId_EndOfFile, OVTK_GDF_End_Of_Session };

    auto result = filterStimulationStream(src, ctx, stims);
    return result;
}

std::shared_ptr<StreamBase> filterStimulationStream(const std::shared_ptr<const StreamBase>& src, const Kernel::IKernelContext& ctx,
                                                    const std::vector<uint64_t>& stimsToFilter)
{
    if (src->getTypeIdentifier() != OV_TypeId_Stimulations) { return nullptr; }

    const auto typedSrc = std::static_pointer_cast<const Stream<TypeStimulation>>(src);
    auto target         = std::make_shared<Stream<TypeStimulation>>(ctx);

    target->clear();

    target->getHeader().m_StartTime = typedSrc->getHeader().m_StartTime;
    target->getHeader().m_EndTime   = typedSrc->getHeader().m_EndTime;

    for (size_t chk = 0; chk < typedSrc->getChunkCount(); ++chk) {
        const auto chunk = typedSrc->getChunk(chk);
        auto newChunk    = new TypeStimulation::Buffer;

        for (size_t i = 0; i < chunk->m_buffer.size(); ++i) {
            const uint64_t id = chunk->m_buffer.getId(i);
            if (std::none_of(stimsToFilter.begin(), stimsToFilter.end(), [id](const uint64_t val) { return val == id; })) {
                const uint64_t timestamp = chunk->m_buffer.getDate(i);
                const uint64_t duration  = chunk->m_buffer.getDuration(i);
                newChunk->m_buffer.push_back(id, timestamp, duration);
            }
        }

        newChunk->m_StartTime = chunk->m_StartTime;
        newChunk->m_EndTime   = chunk->m_EndTime;

        target->push(newChunk);
    }

    target->getEnd().m_StartTime = typedSrc->getEnd().m_StartTime;
    target->getEnd().m_EndTime   = typedSrc->getEnd().m_EndTime;

    return target;
}
}  // namespace Tracker
}  // namespace OpenViBE
