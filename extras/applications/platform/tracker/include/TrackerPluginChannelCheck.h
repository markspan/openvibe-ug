#pragma once

#include "ITrackerPlugin.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TrackerPluginChannelCheck 
 * \brief Example of a Tracker plugin processing given Workspaces. It checks if all selected signal streams in all tracks have the same number of channels.
 * \author J. T. Lindgren
 *
 */
class TrackerPluginChannelCheck final : public ITrackerPlugin
{
public:
    explicit TrackerPluginChannelCheck(const Kernel::IKernelContext& ctx) : ITrackerPlugin(ctx) { }
    bool process(Workspace& wp, ParallelExecutor& exec) override;
    bool hasCapability(const ECapabilities capability) override { return (capability == ECapabilities::Workspace); }
    std::string getName() override { return std::string("Example: Channel check"); }
};
}  // namespace Tracker
}  // namespace OpenViBE
