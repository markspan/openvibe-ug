//
// OpenViBE Tracker
//
// @author J.T. Lindgren
// 
#pragma once

#include <memory>
#include "StreamBase.h"

namespace OpenViBE {
namespace Tracker {
// Returns a copy of a stream with stimulations suggesting end of stream dropped
std::shared_ptr<StreamBase> filterStimulationStreamEndPoints(const std::shared_ptr<const StreamBase>& src, const Kernel::IKernelContext& ctx);

// Returns a copy of a stream with given stimulations dropped
std::shared_ptr<StreamBase> filterStimulationStream(const std::shared_ptr<const StreamBase>& src, const Kernel::IKernelContext& ctx,
                                                    const std::vector<uint64_t>& stimsToFilter);
}  // namespace Tracker
}  // namespace OpenViBE
