#pragma once


/**
  * \brief Acquisition Server plugin that tinkers with the signal for P300 debugging purposes
  * \version 0.1
  * \author Jussi T. Lindgren / Inria
  */

#include "ovasIAcquisitionServerPlugin.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CAcquisitionServer;

namespace Plugins {
class CPluginFiddler final : public IAcquisitionServerPlugin
{
    // Plugin interface
public:
    explicit CPluginFiddler(const Kernel::IKernelContext& ctx);
    ~CPluginFiddler() override {}

    bool startHook(const std::vector<CString>& selectedChannelNames, const size_t sampling, const size_t nChannel, const size_t nSamplePerSentBlock) override;
    void stopHook() override {}
    void loopHook(std::deque<std::vector<float>>& buffers, CStimulationSet& stimSet, const uint64_t start, const uint64_t end,
                  const uint64_t sampleTime) override;

    // Plugin implementation
    uint64_t m_StartSample                = 0;
    uint64_t m_EndSample                  = 0;

    size_t m_Sampling         = 0;
    size_t m_NProcessedSample = 0;
    size_t m_LastBufferSize   = 0;
    size_t m_Counter          = 0;

private:
    OpenViBE::Parameters::ParameterDouble m_fiddlerStrength;
    size_t m_nSamplePerSentBlock = 0;
};
}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
