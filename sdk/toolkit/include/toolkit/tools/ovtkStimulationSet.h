#pragma once

#include "../ovtk_base.h"

namespace OpenViBE {
namespace Toolkit {
namespace StimulationSet {
//@todo All is useless now. Avoid to used it, intended to be removed.  
/// \deprecated Use the CStimulationSet class shift() method instead.
OV_Deprecated("Use the CStimulationSet class shift() method instead.")
OVTK_API bool shift(CStimulationSet& stimSet, uint64_t timeShift);
/// \deprecated Use the CStimulationSet class copy() method instead.
OV_Deprecated("Use the CStimulationSet class copy() method instead.")
OVTK_API bool copy(CStimulationSet& dst, const CStimulationSet& src, uint64_t timeShift = 0);
/// \deprecated Use the CStimulationSet class append() method instead.
OV_Deprecated("Use the CStimulationSet class append() method instead.")
OVTK_API bool append(CStimulationSet& dst, const CStimulationSet& src, uint64_t timeShift = 0);
/// \deprecated Use the CStimulationSet class appendRange() method instead.
OV_Deprecated("Use the CStimulationSet class appendRange() method instead.")
OVTK_API bool appendRange(CStimulationSet& dst, const CStimulationSet& src, uint64_t srcStartTime, uint64_t srcEndTime, uint64_t timeShift = 0);
/// \deprecated Use the CStimulationSet class removeRange() method instead.
OV_Deprecated("Use the CStimulationSet class removeRange() method instead.")
OVTK_API bool removeRange(CStimulationSet& stimSet, uint64_t startTime, uint64_t endTime);
}  // namespace StimulationSet
}  // namespace Toolkit
}  // namespace OpenViBE
