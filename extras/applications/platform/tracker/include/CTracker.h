//
// OpenViBE Tracker
//

#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>

#include "Workspace.h"
#include "BoxPlugins.h"
#include "TrackerPlugins.h"
#include "Contexted.h"

#include "ParallelExecutor.h"

/**
 * \namespace OpenViBE::Tracker
 * \author J.T. Lindgren (Inria)
 * \date 2018-09
 * \brief Main OpenViBE Tracker namespace
 *
 * All the classes defined in the OpenViBE Tracker are currently included in
 * this namespace.
 */
namespace OpenViBE {
namespace Tracker {

/**
 * \class CTracker 
 * \author J. T. Lindgren
 * \brief Tracker is the main class of OpenViBE Tracker that encapsulates the Workspace
 * \details
 * The tracker is in one of the states Stopped, Playing, Paused.
 *
 * - play() will start sending data to a specified processor from the tracks in the workspace.
 * - step() will send out the chunk that is next in time order. Usually called by the GUI idle loop.
 * - stop() will stop sending. Tracker can also stop if the processor quits or if all the data has been sent.
 * 
 * In the stopped state, the Tracker also allows plugins to be applied to the Workspace content.
 *
 * In the future we can extend this class to host more than one workspace at the same time.
 *
 */
class CTracker final : protected Contexted
{
public:
    explicit CTracker(const Kernel::IKernelContext& ctx);
    ~CTracker() override;

    enum class EStates { Stopped = 0, Playing, Paused, Stopping };

    bool initialize();

    bool play(const bool playFast);
    bool stop();

    EStates step();

    // Useful getters
    EStates getCurrentState() const { return m_state; }
    Workspace& getWorkspace() { return m_workspace; }
    const Kernel::IKernelContext& getKernelContext() const override { return m_kernelCtx; }

    // Plugin handling
    const BoxPlugins& getBoxPlugins() const { return m_boxPlugins; }
    const TrackerPlugins& getTrackerPlugins() const { return m_trackerPlugins; }
    bool applyBoxPlugin(size_t index);
    bool applyTrackerPlugin(size_t index);

    uint32_t getNumThreads() const { return m_executor.getNumThreads(); }
    bool setNumThreads(uint32_t numThreads);

    // Configuration handling
    bool saveConfig(const CString& filename) const;
    bool loadConfig(const CString& filename);

protected:
    Workspace m_workspace;
    EStates m_state = EStates::Stopped;
    BoxPlugins m_boxPlugins;
    TrackerPlugins m_trackerPlugins;
    ParallelExecutor m_executor;
};
}  // namespace Tracker
}  // namespace OpenViBE
