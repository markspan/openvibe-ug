#pragma once

#include "StreamBundle.h"

#include <iostream>
#include <thread>

#include "Processor.h"
#include "ProcExternalProcessingHelper.h" // Push and Pull clients

#include "Encoder.h"
#include "Decoder.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class ProcExternalProcessing 
 * \brief A processor implemented by passing data to/from External Processing Boxes inserted into Designer scenarios
 * \author J. T. Lindgren
 *
 */
class ProcExternalProcessing final : public Processor
{
public:
    explicit ProcExternalProcessing(const Kernel::IKernelContext& ctx) : Processor(ctx) {}
    ProcExternalProcessing(const Kernel::IKernelContext& ctx, const ProcExternalProcessing& other);

    bool initialize(const std::string& xmlFile) override;
    bool uninitialize() override;

    // Set targets for push and pull
    bool setNewSource(StreamBundle* source, bool sendHeader, bool sendEnd) override;
    bool setNewTarget(StreamBundle* target) override;

    bool play(const bool playFast, const std::function<bool(CTime)>& quitCB) override { return play(playFast, quitCB, nullptr); }
    // @param nextTrackFun Callback to request more data (used for catenate mode)
    bool play(const bool playFast, const std::function<bool(CTime)>& quitCB, const std::function<bool()>& nextTrackFun);

    // bool play(const bool playFast) override;
    bool stop() override;

    bool setProcessorPorts(const uint32_t sendPort, const uint32_t recvPort) override
    {
        m_sendPort = sendPort;
        m_recvPort = recvPort;
        return true;
    }

    bool getProcessorPorts(uint32_t& sendPort, uint32_t& recvPort) const override
    {
        sendPort = m_sendPort;
        recvPort = m_recvPort;
        return true;
    }

    bool setProcessorFlags(const bool noGUI, const bool doSend, const bool doReceive)
    {
        m_noGUI     = noGUI;
        m_doSend    = doSend;
        m_doReceive = doReceive;
        return true;
    }

    bool getProcessorFlags(bool& noGUI, bool& doSend, bool& doReceive) const
    {
        noGUI     = m_noGUI;
        doSend    = m_doSend;
        doReceive = m_doReceive;
        return true;
    }

    bool canPull() const { return m_doReceive; }
    bool canPush() const { return m_doSend; }

    CTime getCurrentTime() const override;

    bool isSynced() const
    {
        return (m_doSend ? (m_pushStartTime != BufferedClient::CLIENT_NOT_STARTED) : true)
               && (m_doReceive ? (m_pullStartTime != BufferedClient::CLIENT_NOT_STARTED) : true);
    }

    bool isRunning() const override { return m_isRunning; }

    // Serialize state to configuration manager
    bool save() override;
    bool load() override;

protected:
    //bool connectClient(Communication::MessagingClient& client, uint32_t port);

    // Send data to External Processing box
    bool push();
    // Receive data from External Processing box
    bool pop();

    // n.b. for each external processing box, we need to have a separate thread since we don't know for
    // sure in which order OpenViBE will schedule their execution.
    PushClient* m_pushClient        = nullptr;
    std::thread* m_pushClientThread = nullptr;
    CTime m_pushStartTime           = BufferedClient::CLIENT_NOT_STARTED;

    PullClient* m_pullClient        = nullptr;
    std::thread* m_pullClientThread = nullptr;
    CTime m_pullStartTime           = BufferedClient::CLIENT_NOT_STARTED;

    std::thread* m_playerThread = nullptr;

    bool m_sentSomething = false;
    bool m_isRunning     = false;

    uint64_t m_chunksSent = 0;
    CTime m_pushLastTime  = CTime::min();
    CTime m_pullLastTime  = CTime::min();

    // if either port is zero, then the communication from the scenario is expected to flow to one direction only
    uint32_t m_sendPort = 50011;
    uint32_t m_recvPort = 50012;

    bool m_doSend    = true;
    bool m_doReceive = true;

    // Used in continuous playing mode to get the start offset for the next stream
    CTime m_previousEnd     = CTime::min();
    bool m_requestNewOffset = false;

    bool m_sendHeader = true;
    bool m_sendEnd    = true;

    bool m_noGUI = true;

    StreamBundle* m_src = nullptr;
    StreamBundle* m_dst = nullptr;

    std::vector<EncoderBase*> m_encoders;
    std::vector<DecoderBase*> m_decoders;
};
}  // namespace Tracker
}  // namespace OpenViBE
