#include "ovtkStimulationSet.h"

namespace OpenViBE {
namespace Toolkit {
namespace StimulationSet {

bool shift(CStimulationSet& stimSet, const uint64_t timeShift)
{
    const size_t count = stimSet.size();
    for (size_t i = 0; i < count; ++i) { stimSet.setDate(i, stimSet.getDate(i) + timeShift); }
    return true;
}

bool copy(CStimulationSet& dst, const CStimulationSet& src, const uint64_t timeShift)
{
    dst.copy(src, timeShift);
    return true;
}

bool append(CStimulationSet& dst, const CStimulationSet& src, const uint64_t timeShift)
{
    const size_t count = src.size();
    for (size_t i = 0; i < count; ++i) { dst.push_back(src.getId(i), src.getDate(i) + timeShift, src.getDuration(i)); }
    return true;
}

bool appendRange(CStimulationSet& dst, const CStimulationSet& src, const uint64_t srcStartTime, const uint64_t srcEndTime, const uint64_t timeShift)
{
    const size_t count = src.size();
    for (size_t i = 0; i < count; ++i) {
        const uint64_t date = src.getDate(i);
        if (srcStartTime <= date && date < srcEndTime) { dst.push_back(src.getId(i), src.getDate(i) + timeShift, src.getDuration(i)); }
    }
    return true;
}

bool removeRange(CStimulationSet& stimSet, const uint64_t startTime, const uint64_t endTime)
{
    for (size_t i = 0; i < stimSet.size(); ++i) {
        const uint64_t date = stimSet.getDate(i);
        if (startTime <= date && date < endTime) { stimSet.erase(i--); }
    }
    return true;
}

}  // namespace StimulationSet
}  // namespace Toolkit
}  // namespace OpenViBE
