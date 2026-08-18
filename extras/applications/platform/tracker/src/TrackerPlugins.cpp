#include <algorithm> // std::sort

#include "TrackerPlugins.h"
#include "TrackerPluginCountStimulations.h"
#include "TrackerPluginChannelCheck.h"

namespace OpenViBE {
namespace Tracker {

TrackerPlugins::TrackerPlugins(const Kernel::IKernelContext& ctx) : Contexted(ctx)
{
    // Declare all plugins here
    m_pluginCreateCalls.push_back([&ctx]() { return new TrackerPluginCountStimulations(ctx); });
    m_pluginCreateCalls.push_back([&ctx]() { return new TrackerPluginChannelCheck(ctx); });

    // Create example instances of the plugins so the GUI can display a list
    for (auto& fun : m_pluginCreateCalls) {
        auto ptr = fun();
        m_trackerPlugins.push_back(ptr);
    }

    if (m_trackerPlugins.empty()) { return; }

    // get both arrays sorted. No code beauty contest winners here...
    std::vector<size_t> indexes;
    for (size_t i = 0; i < m_trackerPlugins.size(); ++i) { indexes.push_back(i); }

    auto& pluginRef = m_trackerPlugins;
    auto& callRef   = m_pluginCreateCalls;

    std::sort(indexes.begin(), indexes.end(), [&pluginRef](const size_t a, const size_t b) { return (pluginRef[a]->getName()) < (pluginRef[b]->getName()); });

    std::vector<ITrackerPlugin*> sortedPlugins;
    std::transform(indexes.begin(), indexes.end(), std::back_inserter(sortedPlugins), [pluginRef](const size_t i) { return pluginRef[i]; });
    m_trackerPlugins = sortedPlugins;

    std::vector<std::function<ITrackerPlugin*()>> sortedCalls;
    std::transform(indexes.begin(), indexes.end(), std::back_inserter(sortedCalls), [callRef](const size_t i) { return callRef[i]; });
    m_pluginCreateCalls = sortedCalls;
}

TrackerPlugins::~TrackerPlugins()
{
    for (auto f : m_trackerPlugins) { delete f; }
    m_trackerPlugins.clear();
    m_pluginCreateCalls.clear();
}

ITrackerPlugin* TrackerPlugins::getPluginCopy(const size_t index) const
{
    if (index >= m_pluginCreateCalls.size()) { return nullptr; }
    const auto fun = m_pluginCreateCalls[index];
    return fun();
}

}  // namespace Tracker
}  // namespace OpenViBE
