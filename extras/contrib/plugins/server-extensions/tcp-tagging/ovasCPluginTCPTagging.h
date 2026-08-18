#pragma once

/**
  * \brief Acquisition Server plugin adding the capability to receive stimulations from external sources
  * via TCP/IP.
  * 
  * The stimulation format is the same as with Shared Memory Tagging. It comprises three blocks of 8 bytes:
  *
  * ----------------------------------------------------------------------
  * |  padding  (8 bytes) |  event id (8 bytes)  |  timestamp (8 bytes)  |
  * ----------------------------------------------------------------------
  *  
  * The padding is only for consistency with Shared Memory Tagging and has no utility.
  * The event id informs about the type of event happening.
  * The timestamp is the posix time (ms since Epoch) at the moment of the event.
  * It the latter is set to 0, the acquisition server issues its own timestamp upon reception of the stimulation.
  *
  * Have a look at contrib/plugins/server-extensions/tcp-tagging/client-example to learn about the protocol
  * to send stimulations from the client. 
  */

#include "ovasIAcquisitionServerPlugin.h"
#include "ovasCTagStream.h"

namespace OpenViBE {
namespace AcquisitionServer {
namespace Plugins {
class CPluginTCPTagging final : public IAcquisitionServerPlugin
{
public:
    explicit CPluginTCPTagging(const Kernel::IKernelContext& ctx);
    ~CPluginTCPTagging() override { }

    // Overrides virtual method startHook inherited from class IAcquisitionServerPlugin.
    bool startHook(const std::vector<CString>& vSelectedChannelNames, const size_t sampling, const size_t nChannel, const size_t nSamplePerSentBlock) override;

    // Overrides virtual method stopHook inherited from class IAcquisitionServerPlugin
    void stopHook() override;

    // Overrides virtual method loopHook inherited from class IAcquisitionServerPlugin.
    void loopHook(std::deque<std::vector<float>>& vPendingBuffer, CStimulationSet& stimulationSet, const uint64_t start, const uint64_t end,
                  const uint64_t sampleTime) override;

private:
    uint64_t m_previousClockTime   = 0;
    uint64_t m_previousSampleTime  = 0;
    uint64_t m_lastTagTime         = 0;
    uint64_t m_lastTagTimeAdjusted = 0;

    std::unique_ptr<CTagStream> m_scopedTagStream;
    OpenViBE::Parameters::ParameterInt m_port;

    bool m_warningPrinted = false;
};
}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
