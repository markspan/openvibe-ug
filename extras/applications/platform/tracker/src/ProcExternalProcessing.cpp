#include "ProcExternalProcessing.h"

#include <iostream>
#include <thread>
#include <deque>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <algorithm> // std::max

#include "CodecFactory.h"

// thread::
// wait for connection;
// while(connected ||!quit)
// {
//    waitForChunk();
//    pushChunk();
// }
// if(quit) {
//    pushEndStim(); // XML have player controller that will quit
// }
// exit;

// #include "StreamChunk.h"
// #include "StreamSignalChunk.h"
// #include "StreamStimulationChunk.h"

#include "Stream.h"
#include "TypeSignal.h"
#include "TypeStimulation.h"

namespace OpenViBE {
namespace Tracker {

void playerLaunch(const char* xmlFile, const char* args, bool playFast, bool noGUI, uint32_t identifier); // In processor.cpp


ProcExternalProcessing::ProcExternalProcessing(const Kernel::IKernelContext& ctx, const ProcExternalProcessing& other) : Processor(ctx)
{
    uint32_t sendPort, recvPort;
    other.getProcessorPorts(sendPort, recvPort);
    setProcessorPorts(sendPort, recvPort);

    bool noGui, doSend, doReceive;
    other.getProcessorFlags(noGui, doSend, doReceive);
    setProcessorFlags(noGui, doSend, doReceive);

    setArguments(other.getArguments());

    initialize(other.getFilename());
}

bool ProcExternalProcessing::initialize(const std::string& xmlFile)
{
    m_pushStartTime = BufferedClient::CLIENT_NOT_STARTED;
    m_pullStartTime = BufferedClient::CLIENT_NOT_STARTED;

    if (xmlFile.length() == 0) {
        //      log() << Kernel::LogLevel_Trace << "No processor configured\n";
        m_xmlFilename.clear();
        return true;
    }

    //  log() << Kernel::LogLevel_Trace << "Processor: Initializing with " << xmlFile << "\n";

    m_xmlFilename = xmlFile;

    return true;
}

bool ProcExternalProcessing::uninitialize()
{
    //  log() << Kernel::LogLevel_Trace << "Processor: Uninitializing\n";
    stop();

    for (auto decoder : m_decoders) { delete decoder; }
    m_decoders.clear();
    for (auto encoder : m_encoders) { delete encoder; }
    m_encoders.clear();

    return true;
}

CTime ProcExternalProcessing::getCurrentTime() const
{
    const CTime startTime = (m_doSend ? m_pushStartTime : m_pullStartTime);

    if (startTime == BufferedClient::CLIENT_NOT_STARTED) { return CTime::min(); }

    const CTime refTime     = CTime(m_pushClient ? m_pushClient->getTime() : m_pullClient->getTime());
    const CTime currentTime = refTime - startTime;
    //  log() << Kernel::LogLevel_Info << "Its " << CTime(currentTime).toSeconds() << "\n";

    return currentTime;
}

// If source track changes, we need a new codec since they are bound to the streams
// @todo make streamless?
bool ProcExternalProcessing::setNewSource(StreamBundle* source, const bool sendHeader, const bool sendEnd)
{
    m_sendHeader = sendHeader;
    m_sendEnd    = sendEnd;
    m_src        = source;

    for (auto encoder : m_encoders) { delete encoder; }
    m_encoders.clear();
    return true;
}

bool ProcExternalProcessing::setNewTarget(StreamBundle* target)
{
    m_dst = target;

    for (auto decoder : m_decoders) { delete decoder; }
    m_decoders.clear();

    return true;
}


// Sends all chunks up to the current time point t
bool ProcExternalProcessing::push()
{
    if (m_xmlFilename.length() == 0) { return false; }

    if (!m_doSend) {
        // This processor is configured to receive, so we do nothing on send
        return true;
    }

    if (!m_pushClient || m_pushClient->hasQuit()) {
        this->stop();
        return false;
    }

    if (!m_src) { return false; }

    if (m_pushStartTime == BufferedClient::CLIENT_NOT_STARTED) {
        m_pushStartTime = m_pushClient->getStartTime();
        if (m_pushStartTime == BufferedClient::CLIENT_NOT_STARTED) {
            // Not yet synced
            return true;
        }
    }

    bool foundSomething = false;
    m_sentSomething     = false;

    uint64_t chunksSent = 0;

    while (true) {
        size_t streamIndex;
        StreamPtr stream = m_src->getNextStream(streamIndex);

        if (!stream) { break; }

        if (m_encoders.size() <= streamIndex) {
            m_encoders.resize(streamIndex + 1, nullptr);
            m_encoders[streamIndex] = CodecFactory::getEncoder(m_kernelCtx, *stream);
        }

        CTime chunkStartTime = CTime::min(), chunkEndTime = CTime::min();
        if (!stream->peek(chunkStartTime, chunkEndTime)) {
            // if peek fails maybe the stream has ended?
            break;
        }

        // There is something to send, now or later
        foundSomething = true;

        const CTime elapsedTime = getCurrentTime();

        // Send if its time
        if (elapsedTime >= chunkStartTime + m_previousEnd) {
            EncodedChunk chunk;
            EChunkType outputType;
            m_encoders[streamIndex]->setEncodeOffset(m_previousEnd);
            if (m_encoders[streamIndex]->encode(chunk, outputType)) {
                const bool dontPush = (!m_sendHeader && outputType == EChunkType::Header) || (!m_sendEnd && outputType == EChunkType::End);

                if (!dontPush) {
                    chunk.m_StreamIndex = streamIndex;
                    m_pushClient->pushBuffer(chunk);
                    m_pushLastTime = chunk.m_EndTime;

                    /*
                    log() << Kernel::LogLevel_Info << "Enc str " << streamIndex << " at " << CTime(elapsedTime).toSeconds()
                        << " chk [" << CTime(chunk.m_startTime).toSeconds()
                        << "," << CTime(chunk.m_endTime).toSeconds()
                        << "]\n";
                        */
                    m_sentSomething = true;
                    chunksSent++;
                }

                stream->step();
            }
            else {
                //  log() << Kernel::LogLevel_Error << "Error: Failed to encode chunk\n";
                return false;
            }
        }
        else {
            // We are early
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            break;
        }

        //  } while (sentSomething || m_PlayFast); // we loop until we have sent everything up to this moment. 
    }

    if (m_sentSomething) {
        //log() << Kernel::LogLevel_Info << "flush\n";
        m_pushClient->requestFlush();
    }


    // @fixme note that the while loop above may take long to return the control flow for some
    // bad streams


    if (!foundSomething) {
        // Maybe all streams ended?
        // log() << Kernel::LogLevel_Info << "Nothing to send - all streams ended?\n";

        // Update time offset for 'continuous sending' mode
        // n.b. we can always do this, since for noncontinuous mode, new play() will be called, resetting these
        if (m_src->isFinished() && !m_requestNewOffset) {
            const CTime maxEndTime = m_src->getMaxDuration();
            m_previousEnd += maxEndTime; // Incremental for more than 2 tracks
            m_requestNewOffset = true;

            //          log() << Kernel::LogLevel_Info << "New offset at " << CTime(m_PreviousEnd).toSeconds() << "\n";
        }

        // We do this to keep the External Processing box running on the designer side
        m_pushClient->requestFlush();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        return false;
    }

    if (!m_playFast) {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));   
        //  std::this_thread::yield();
    }

    return true;
}

// Sends all chunks up to the current time point t
bool ProcExternalProcessing::pop()
{
    if (m_xmlFilename.length() == 0) { return false; }

    if (!m_doReceive) {
        // This processor is configured to send, so we do nothing on receive
        return true;
    }

    if (!m_pullClient || !m_dst) { return false; }

    if (m_pullClient->hasQuit()) {
        this->stop();
        return false;
    }

    if (m_pullStartTime == BufferedClient::CLIENT_NOT_STARTED) {
        m_pullStartTime = m_pullClient->getStartTime();
        if (m_pullStartTime == BufferedClient::CLIENT_NOT_STARTED) {
            // Not yet synced
            return true;
        }
    }

    EncodedChunk chunk;
    bool gotSomething = false;
    while (m_pullClient->pullBuffer(chunk)) {
        //      log() << Kernel::LogLevel_Info << "Got chunk " << chunk.m_startTime << "," << chunk.m_endTime << "\n";
        StreamPtr targetStream = m_dst->getStream(chunk.m_StreamIndex);
        if (!targetStream) {
            if (!m_dst->createStream(chunk.m_StreamIndex, chunk.m_StreamType)) { continue; }
            targetStream = m_dst->getStream(chunk.m_StreamIndex);
            if (m_decoders.size() <= chunk.m_StreamIndex) { m_decoders.resize(chunk.m_StreamIndex + 1, nullptr); }
            m_decoders[chunk.m_StreamIndex] = CodecFactory::getDecoder(m_kernelCtx, *targetStream);
        }

        // @note Decoder doesn't need to add offsets as these times are coming from the
        // external processor, so they are already offset by the past length.
        if (!chunk.m_Buffer.empty()) {
            m_decoders[chunk.m_StreamIndex]->decode(chunk);

            /*
            log() << Kernel::LogLevel_Info << "Dec str " << chunk.streamIndex << " chk [" << CTime(chunk.m_startTime).toSeconds()
                << "," << CTime(chunk.m_endTime).toSeconds() << "]\n";
                */
        }

        m_pullLastTime = chunk.m_EndTime;

        gotSomething = true;
    }

    if (!gotSomething) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

    if (m_pullClient->hasQuit()) { return false; }

    return true;
}

bool ProcExternalProcessing::play(const bool playFast, const std::function<bool(CTime)>& quitCB, const std::function<bool()>& nextTrackFun)
{
    m_pushLastTime = CTime::min();
    m_pullLastTime = CTime::min();
    m_isRunning    = false;

    if (m_xmlFilename.length() == 0) {
        log() << Kernel::LogLevel_Error << "Error: No processor initialized\n";
        return false;
    }

    m_chunksSent = 0;
    //  m_PreviousChunkEnd = 0;
    m_playFast         = playFast;  // @todo To work neatly it'd be better to be able to pass in the chunk times to the designer side
    m_previousEnd      = 0;
    m_requestNewOffset = false;

    for (auto decoder : m_decoders) { delete decoder; }
    m_decoders.clear();
    for (auto encoder : m_encoders) { delete encoder; }
    m_encoders.clear();

    //  log() << Kernel::LogLevel_Info << "Reset offset to " << CTime(m_PreviousEnd).toSeconds() << "\n";

    const CString expandedName = m_kernelCtx.getConfigurationManager().expand(m_xmlFilename.c_str());

    std::stringstream ss;
    ss << "--define Tracker_Port_Send " << m_sendPort << " ";
    ss << "--define Tracker_Port_Receive " << m_recvPort << " ";

    const std::string allArgs = m_arguments + " " + ss.str();

    m_playerThread = new std::thread(std::bind(&playerLaunch, expandedName, allArgs.c_str(), m_playFast, m_noGUI, m_sendPort));

    // @fixme this ad-hoc sleep is pretty terrible, but it seems that if player launch is slow enough and
    // we launch the client threads immediately below, in Windows it results in deadlocks and errors, suggesting that
    // the concurrency control logic is not really correct in the communication with the external processing box.
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    if (m_doSend) {
        m_pushClient       = new PushClient(m_sendPort);
        m_pushClientThread = new std::thread(&PushClient::start, m_pushClient);
    }
    if (m_doReceive) {
        m_pullClient       = new PullClient(m_recvPort);
        m_pullClientThread = new std::thread(&PullClient::start, m_pullClient);
    }

    //  log() << Kernel::LogLevel_Debug << "External processing thread(s) launched, waiting sync\n";

    for (size_t retries = 0; retries < 20; ++retries) {
        if (m_doSend) { m_pushStartTime = m_pushClient->getStartTime(); }
        if (m_doReceive) { m_pullStartTime = m_pullClient->getStartTime(); }
        if (isSynced()) {
            //          log() << Kernel::LogLevel_Debug << "Clients synced after " << retries << " retries\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if (!isSynced()) {
        //      log() << Kernel::LogLevel_Error << "Error syncing client(s) after 10 secs of retries\n";
        if (m_pushClient) { m_pushClient->requestQuit(); }
        if (m_pullClient) { m_pullClient->requestQuit(); }
        return false;
    }

    m_isRunning = true;
    while (m_isRunning) {
        m_isRunning &= push();
        m_isRunning &= pop();

        if (!m_isRunning && nextTrackFun) { m_isRunning = nextTrackFun(); }
        // @fixme the quit callback might be a bit expensive, shouldn't hammer it
        if (quitCB && quitCB((m_doSend ? m_pushLastTime : m_pullLastTime))) { m_isRunning = false; }
    }

    stop();

    return true;
}

bool ProcExternalProcessing::stop()
{
    m_isRunning = false;

    if (m_pushClient) {
        //      log() << Kernel::LogLevel_Info << "Stopping external processing push client\n";

        m_pushClient->requestQuit();
        if (m_pushClientThread) { m_pushClientThread->join(); }
        delete m_pushClientThread;
        m_pushClientThread = nullptr;
        delete m_pushClient;
        m_pushClient = nullptr;
    }

    if (m_pullClient) {
        //      log() << Kernel::LogLevel_Info << "Stopping external processing pull client\n";

        m_pullClient->requestQuit();
        if (m_pullClientThread) { m_pullClientThread->join(); }
        delete m_pullClientThread;
        m_pullClientThread = nullptr;
        delete m_pullClient;
        m_pullClient = nullptr;
    }

    // tear down the player object
    if (m_playerThread) {
        //      log() << Kernel::LogLevel_Trace << "Joining player thread\n";
        m_playerThread->join();
        delete m_playerThread;
        m_playerThread = nullptr;
    }

    m_pushStartTime = BufferedClient::CLIENT_NOT_STARTED;
    m_pullStartTime = BufferedClient::CLIENT_NOT_STARTED;

    return true;
}

bool ProcExternalProcessing::save()
{
    auto& mgr = m_kernelCtx.getConfigurationManager();

    std::stringstream sPort;
    sPort << m_sendPort;
    // std::stringstream rPort; rPort << m_RecvPort;

    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor", m_xmlFilename.c_str());
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor_FirstPort", sPort.str().c_str());
    // mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor_RecvPort", rPort.str().c_str());
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor_NoGUI", (m_noGUI ? "true" : "false"));
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor_DoSend", (m_doSend ? "true" : "false"));
    mgr.addOrReplaceConfigurationToken("Tracker_Workspace_Processor_DoReceive", (m_doReceive ? "true" : "false"));

    return true;
}

bool ProcExternalProcessing::load()
{
    auto& mgr = m_kernelCtx.getConfigurationManager();

    if (mgr.lookUpConfigurationTokenIdentifier("Tracker_Workspace_Processor") != CIdentifier::undefined()) {
        initialize(mgr.lookUpConfigurationTokenValue("Tracker_Workspace_Processor").toASCIIString());
    }

    if (mgr.lookUpConfigurationTokenIdentifier("Tracker_Workspace_Processor_FirstPort") != CIdentifier::undefined()) {
        const uint32_t port = uint32_t(mgr.expandAsUInteger("${Tracker_Workspace_Processor_FirstPort}"));
        setProcessorPorts(port, port + 1);
    }

    m_noGUI     = mgr.expandAsBoolean("${Tracker_Workspace_Processor_NoGUI}", m_noGUI);
    m_doSend    = mgr.expandAsBoolean("${Tracker_Workspace_Processor_DoSend}", m_doSend);
    m_doReceive = mgr.expandAsBoolean("${Tracker_Workspace_Processor_DoReceive}", m_doReceive);

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
