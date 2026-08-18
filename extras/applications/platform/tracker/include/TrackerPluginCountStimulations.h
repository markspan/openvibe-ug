#pragma once

#include "ITrackerPlugin.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TrackerPluginCountStimulations 
 * \brief Example of a Tracker plugin that processes given tracks. It counts the occurrences of different stimulations in each stimulation stream.
 * \author J. T. Lindgren
 *
 */
class TrackerPluginCountStimulations final : public ITrackerPlugin
{
public:
    explicit TrackerPluginCountStimulations(const Kernel::IKernelContext& ctx) : ITrackerPlugin(ctx) { }
    bool process(StreamBundle& track) override;
    std::string getName() override { return std::string("Example: Count stimulations"); }
};
}  // namespace Tracker
}  // namespace OpenViBE
