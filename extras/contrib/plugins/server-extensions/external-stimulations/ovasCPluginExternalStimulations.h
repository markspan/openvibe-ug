#pragma once

/**
  * \brief Acquisition Server plugin adding the capability to receive stimulations from external sources
  *
  * \author Anton Andreev
  * \author Jozef Legeny
  *
  * \note This plugin is deprecated. The users are recommended to use the TCP Tagging plugin instead. (11.05.2016)
  *
  */

#include <thread>
#include <mutex>
#include <condition_variable>

#include <sys/timeb.h>

#include "ovasIAcquisitionServerPlugin.h"

namespace OpenViBE {
namespace AcquisitionServer {
class CAcquisitionServer;

namespace Plugins {
class CPluginExternalStimulations final : public IAcquisitionServerPlugin
{
    // Plugin interface
public:
    explicit CPluginExternalStimulations(const Kernel::IKernelContext& ctx);
    ~CPluginExternalStimulations() override {}

    bool startHook(const std::vector<CString>& selectedChannelNames, const size_t sampling, const size_t nChannel, const size_t nSamplePerSentBlock) override;
    void stopHook() override;
    void loopHook(std::deque<std::vector<float>>& vPendingBuffer, CStimulationSet& stimulationSet, const uint64_t start, const uint64_t end,
                  const uint64_t sampleTime) override;
    void acceptNewConnectionHook() override { m_ExternalStimulations.clear(); }


    // Plugin implementation


    struct SExternalStimulation
    {
        uint64_t timestamp;
        uint64_t identifier;
    };

    void addExternalStimulations(CStimulationSet* ss, Kernel::ILogManager& logm, const uint64_t start, const uint64_t end);
    void readExternalStimulations();

    //void acquireExternalStimulationsVRPN(CStimulationSet* ss, Kernel::ILogManager& logm, uint64_t start, uint64_t end);

    struct timeb m_CTStartTime; //time when the acquisition process started in local computer time

    std::vector<SExternalStimulation> m_ExternalStimulations;

    bool setExternalStimulationsEnabled(bool active);
    bool isExternalStimulationsEnabled() const { return m_enableExternalStimulations; }

    // Debugging of external stimulations
    int m_DebugStimulationsLost              = 0;
    int m_DebugExternalStimulationsSent      = 0;
    int m_DebugCurrentReadIPCStimulations    = 0;
    int m_DebugStimulationsReceivedEarlier   = 0;
    int m_DebugStimulationsReceivedLate      = 0;
    int m_DebugStimulationsReceivedWrongSize = 0;
    int m_DebugStimulationsBuffered          = 0;

    //added for acquiring external stimulations
    std::unique_ptr<std::thread> m_ESthreadPtr;
    bool m_IsESThreadRunning = false;
    std::mutex m_es_mutex;
    std::condition_variable m_esAvailable;

protected:
    OpenViBE::Parameters::ParameterBool m_enableExternalStimulations;
    OpenViBE::Parameters::ParameterString m_externalStimulationQueueName;
};
}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
