#pragma once

#include "ovas_base.h"

#include "../ovasCSettingsHelper.h"

#include "boost/variant.hpp"
#include <deque>

/**
  * \brief Interface for acquisition server plugins
  *
  * Contains an interface to the acquisition server plugins. Any plugin must inherit from this class in order to be able to register with the acquisition server.
  */

namespace OpenViBE {
namespace AcquisitionServer {
class CAcquisitionServer;

class IAcquisitionServerPlugin
{
public:
    // Interface of the plugin. To develop a new plugin override any of the Hook functions in your implementation.

    /// Hook called at the end of the AcquisitionServer constructor
    virtual void createHook() {}

    /// Hook called at the end of the start() function of AcquisitionServer. At this point the device has been connected to,
    /// and signal properties should already be correct.
    /// This function should return false if start failed.
    virtual bool startHook(const std::vector<CString>& /*selectedChannelNames*/, const size_t /*sampling*/, const size_t /*nChannel*/,
                           const size_t /*nSamplePerSentBlock*/) { return true; }

    /// Hook called at the end of the stop() function of AcquisitionServer
    virtual void stopHook() {}


    /** \brief Hook called in the loop() function of AcquisitionServer
      *
      * This hook is called before sending the stimulations or signal to the connected clients.
      * It gets a reference to the current signal buffer and the stimulation set with its start and end dates.
      *
      * Note that the given input buffer may have more samples than what should be processed 
      * per iteration. All operations on vPendingBuffer done in the hook should only consider
      * the first sampleCountSentPerBlock samples. The later samples should be left as-is
      * and will be provided on the next call.
      */
    virtual void loopHook(std::deque<std::vector<float>>& /*pendingBuffer*/, CStimulationSet& /*stimulationSet*/, const uint64_t /*start*/,
                          const uint64_t /*end*/, const uint64_t /*sampleTime*/) {}

    /// Hook called at the end of the acceptNewConnection() function of AcquisitionServer
    virtual void acceptNewConnectionHook() {}

    IAcquisitionServerPlugin(const Kernel::IKernelContext& ctx, const CString& name) : m_kernelCtx(ctx), m_settings(name, ctx.getConfigurationManager()) {}

    virtual ~IAcquisitionServerPlugin() {}

    const SettingsHelper& getSettingsHelper() const { return m_settings; }
    SettingsHelper& getSettingsHelper() { return m_settings; }

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    SettingsHelper m_settings;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
