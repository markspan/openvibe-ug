#include "ovasCAcquisitionServer.h"
#include "ovasIAcquisitionServerPlugin.h"

#include <toolkit/ovtk_all.h>

#include <ovp_global_defines.h>

#include <system/ovCTime.h>

#include <fstream>
#include <sstream>

#include <string>
#include <functional>
#include <cctype>
#include <cmath> // std::isnan, std::isfinite
#include <condition_variable>

#include <mutex>

#include <socket/IConnectionClient.h>

// #include <iomanip>

namespace OpenViBE {
namespace AcquisitionServer {

// because std::tolower has multiple signatures,
// it can not be easily used in std::transform
// this workaround is taken from http://www.gcek.net/ref/books/sw/cpp/ticppv2/
template <class T>
static T to_lower(T c) { return std::tolower(c); }

class CDriverContext final : public IDriverContext
{
public:
    CDriverContext(const Kernel::IKernelContext& ctx, CAcquisitionServer& acquisitionServer) : m_kernelCtx(ctx), m_acquisitionServer(acquisitionServer) { }

    Kernel::ILogManager& getLogManager() const override { return m_kernelCtx.getLogManager(); }
    Kernel::IConfigurationManager& getConfigurationManager() const override { return m_kernelCtx.getConfigurationManager(); }

    bool isConnected() const override { return m_acquisitionServer.isConnected(); }
    bool isStarted() const override { return m_acquisitionServer.isStarted(); }
    bool isImpedanceCheckRequested() const override { return m_acquisitionServer.isImpedanceCheckRequested(); }
    bool isChannelSelectionRequested() const { return m_acquisitionServer.isChannelSelectionRequested(); }
    int64_t getDriftSampleCount() const override { return m_acquisitionServer.m_DriftCorrection.getDriftSampleCount(); }

    bool correctDriftSampleCount(const int64_t count) override
    {
        return m_acquisitionServer.m_DriftCorrection.correctDrift(count, m_acquisitionServer.m_nSample, m_acquisitionServer.m_PendingBuffers,
                                                                  m_acquisitionServer.m_PendingStimSet, m_acquisitionServer.m_SwapBuffers);
    }

    int64_t getDriftToleranceSampleCount() const override { return m_acquisitionServer.m_DriftCorrection.getDriftToleranceSampleCount(); }

    int64_t getSuggestedDriftCorrectionSampleCount() const override { return m_acquisitionServer.m_DriftCorrection.getSuggestedDriftCorrectionSampleCount(); }

    void setInnerLatencySampleCount(const int64_t count) override { m_acquisitionServer.m_DriftCorrection.setInnerLatencySampleCount(count); }
    int64_t getInnerLatencySampleCount() const override { return m_acquisitionServer.m_DriftCorrection.getInnerLatencySampleCount(); }

    bool updateImpedance(const size_t index, const double impedance) override { return m_acquisitionServer.updateImpedance(index, impedance); }

    uint64_t getStartTime() const { return m_acquisitionServer.m_startTime; }

protected:
    const Kernel::IKernelContext& m_kernelCtx;
    CAcquisitionServer& m_acquisitionServer;
};

class CConnectionServerHandlerThread
{
public:
    CConnectionServerHandlerThread(CAcquisitionServer& acquisitionServer, Socket::IConnectionServer& connectionServer)
        : m_AcquisitionServer(acquisitionServer), m_ConnectionServer(connectionServer) { }

    void operator()()
    {
        Socket::IConnection* connection;
        do {
            connection = m_ConnectionServer.accept();
            m_AcquisitionServer.acceptNewConnection(connection);
        } while (connection && m_AcquisitionServer.isConnected());
    }

    CAcquisitionServer& m_AcquisitionServer;
    Socket::IConnectionServer& m_ConnectionServer;
};

class CConnectionClientHandlerThread
{
public:
    CConnectionClientHandlerThread(CAcquisitionServer& acquisitionServer, Socket::IConnection& connection)
        : m_AcquisitionServer(acquisitionServer), m_Connection(connection) { }

    void operator()()
    {
        std::unique_lock<std::mutex> oLock(m_ClientThreadMutex, std::defer_lock);

        while (true) {
            oLock.lock();

            // Wait until something interesting happens...
            m_PendingBufferCondition.wait(oLock, [this]() { return (m_PleaseQuit || !m_Connection.isConnected() || !m_ClientPendingBuffer.empty()); });

            // Exit the loop if we're told to quit or if we've lost the connection
            if (m_PleaseQuit || !m_Connection.isConnected()) {
                oLock.unlock();
                break;
            }

            // At this point, we should have a buffer
            if (m_ClientPendingBuffer.empty()) {
                // n.b. Shouldn't happen, but we don't have an error reporting mechanism in the thread...
                oLock.unlock();
                continue;
            }

            CMemoryBuffer* buffer = m_ClientPendingBuffer.front();
            m_ClientPendingBuffer.pop_front();

            // Don't go into blocking send while holding the lock; ok to unlock as buffer ptr+mem is now owned by this thread
            oLock.unlock();

            const uint64_t size = buffer->getSize();
            m_Connection.sendBufferBlocking(&size, sizeof(size));
            m_Connection.sendBufferBlocking(buffer->getDirectPointer(), buffer->getSize());
            delete buffer;
        }

        oLock.lock();

        // We're done, clean any possible pending buffers
        for (auto it = m_ClientPendingBuffer.begin(); it != m_ClientPendingBuffer.end(); ++it) { delete *it; }
        m_ClientPendingBuffer.clear();

        oLock.unlock();

        // The thread will exit here and can be joined
    }

    void scheduleBuffer(const CMemoryBuffer& buffer)
    {
        {
            std::lock_guard<std::mutex> oLock(m_ClientThreadMutex);
            if (!m_PleaseQuit) {
                CMemoryBuffer* tmp = new CMemoryBuffer(buffer);
                m_ClientPendingBuffer.push_back(tmp);
            }
        }

        // No big harm notifying in any case, though if in 'quit' state, the quit request has already notified
        m_PendingBufferCondition.notify_one();
    }

    CAcquisitionServer& m_AcquisitionServer;
    Socket::IConnection& m_Connection;

    std::deque<CMemoryBuffer*> m_ClientPendingBuffer;

    // Here we use a condition variable to avoid sleeping
    std::mutex m_ClientThreadMutex;
    std::condition_variable m_PendingBufferCondition;

    // Should this thread quit?
    bool m_PleaseQuit = false;
};

static void start_connection_client_handler_thread(CConnectionClientHandlerThread* thread) { (*thread)(); }


//___________________________________________________________________//
//                                                                   //

std::string toString(const ENaNReplacementPolicy policy)
{
    switch (policy) {
        case ENaNReplacementPolicy::Disabled: return "Disabled";
        case ENaNReplacementPolicy::LastCorrectValue: return "LastCorrectValue";
        case ENaNReplacementPolicy::Zero: return "Zero";
        default: return "N/A";
    }
}

CAcquisitionServer::CAcquisitionServer(const Kernel::IKernelContext& ctx)
    : m_DriftCorrection(ctx), m_kernelCtx(ctx)
{
    m_driverCtx = new CDriverContext(ctx, *this);

    m_encoder = &m_kernelCtx.getAlgorithmManager().getAlgorithm(
        m_kernelCtx.getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_MasterAcquisitionEncoder));
    m_encoder->initialize();

    ip_subjectID.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectID));
    ip_subjectAge.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectAge));
    ip_subjectGender.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_SubjectGender));
    ip_matrix.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalMatrix));
    ip_sampling.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_SignalSampling));
    ip_stimSet.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_StimulationSet));
    ip_bufferDuration.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_BufferDuration));
    ip_channelUnits.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_ChannelUnits));
    op_buffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_OutputParameterId_EncodedMemoryBuffer));

    ip_encodeChannelLocalisationData.initialize(
        m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelLocalisationData));
    ip_encodeChannelUnitData.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputParameterId_EncodeChannelUnitData));

    const std::string policy = m_kernelCtx.getConfigurationManager().expand("${AcquisitionServer_NaNReplacementPolicy}").toASCIIString();
    if (policy == "Disabled") { this->setNaNReplacementPolicy(ENaNReplacementPolicy::Disabled); }
    else if (policy == "Zero") { this->setNaNReplacementPolicy(ENaNReplacementPolicy::Zero); }
    else { this->setNaNReplacementPolicy(ENaNReplacementPolicy::LastCorrectValue); }

    this->setOversamplingFactor(m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_OverSamplingFactor}", 1));
    this->setImpedanceCheckRequest(m_kernelCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_CheckImpedance}", false));
    this->setChannelSelectionRequest(m_kernelCtx.getConfigurationManager().expandAsBoolean("${AcquisitionServer_ChannelSelection}", false));

    m_startedDriverSleepDuration = m_kernelCtx.getConfigurationManager().expandAsInteger("${AcquisitionServer_StartedDriverSleepDuration}", 0);
    m_stoppedDriverSleepDuration = m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_StoppedDriverSleepDuration}", 100);
    m_driverTimeoutDuration      = m_kernelCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DriverTimeoutDuration}", 5000);

    for (auto itp = m_Plugins.begin(); itp != m_Plugins.end(); ++itp) { (*itp)->createHook(); }
}

CAcquisitionServer::~CAcquisitionServer()
{
    if (m_isStarted) {
        m_Driver->stop();
        m_isStarted = false;
    }

    if (m_isInitialized) {
        m_Driver->uninitialize();
        m_isInitialized = false;
    }

    if (m_connectionServer) {
        m_connectionServer->release();
        m_connectionServer = nullptr;
    }

    // n.b. We don't clear the connection list as the teardown order (even on window close)
    // will lead to AcquisitionServerGUI terminating the AcquisitionThread which will
    // in turn call the server's ::stop() that clears the list.

    ip_subjectID.uninitialize();
    ip_subjectAge.uninitialize();
    ip_subjectGender.uninitialize();
    ip_matrix.uninitialize();
    ip_sampling.uninitialize();
    ip_stimSet.uninitialize();
    ip_bufferDuration.uninitialize();
    op_buffer.uninitialize();
    ip_channelUnits.uninitialize();

    ip_encodeChannelLocalisationData.uninitialize();
    ip_encodeChannelUnitData.uninitialize();

    m_encoder->uninitialize();
    m_kernelCtx.getAlgorithmManager().releaseAlgorithm(*m_encoder);
    m_encoder = nullptr;

    delete m_driverCtx;
    m_driverCtx = nullptr;
}

IDriverContext& CAcquisitionServer::getDriverContext() const { return *m_driverCtx; }

//___________________________________________________________________//
//                                                                   //

bool CAcquisitionServer::loop()
{
    // m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "loop()\n";

    // Searches for new connection(s)
    if (m_connectionServer) {
        DoubleLock lock(&m_oPendingConnectionProtectionMutex, &m_oPendingConnectionExecutionMutex);

        for (auto it = m_pendingConnections.begin(); it != m_pendingConnections.end(); ++it) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Received new connection...\n";

            Socket::IConnection* connection = it->first;
            if (this->isStarted()) {
                // When a new connection is found and the
                // acq server is started, send the header

                // Computes inner data to skip
                const int64_t toSkip = m_DriftCorrection.isActive()
                                           ? ((int64_t(it->second.connectionTime - m_startTime) * int64_t(m_sampling)) >> 32) - m_nSample + m_PendingBuffers.
                                             size()
                                           : ((int64_t(it->second.connectionTime - m_lastDeliveryTime) * int64_t(m_sampling)) >> 32) + m_PendingBuffers.size();

                const uint64_t theoreticalSampleCountToSkip = (toSkip < 0 ? 0 : uint64_t(toSkip));

                m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Sample count offset at connection : " << theoreticalSampleCountToSkip << "\n";

                if ((m_nSample - m_PendingBuffers.size()) % m_nSamplePerSentBlock != 0) {
                    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Buffer start sample " << m_nSample - m_PendingBuffers.size() <<
                            " doesn't seem to be divisible by " << m_nSamplePerSentBlock << " (case B)\n";
                }

                const uint64_t bufferDuration       = ip_bufferDuration;
                const uint64_t pastBufferCount      = (m_nSample - m_PendingBuffers.size()) / m_nSamplePerSentBlock;
                const uint64_t connBufferTimeOffset = CTime(m_sampling, theoreticalSampleCountToSkip).time();

                connection_info_t info;
                info.connectionTime                   = it->second.connectionTime;
                info.stimulationTimeOffset            = pastBufferCount * bufferDuration + connBufferTimeOffset;
                info.nSampleToSkip                    = theoreticalSampleCountToSkip;
                info.connectionClientHandlerThread    = new CConnectionClientHandlerThread(*this, *connection);
                info.connectionClientHandlerStdThread = new std::thread(std::bind(&start_connection_client_handler_thread, info.connectionClientHandlerThread));
                info.isChannelLocalisationSent        = false;
                info.isChannelUnitsSent               = false;

                m_connections.push_back(std::pair<Socket::IConnection*, connection_info_t>(connection, info));

                m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Creating header\n";

                op_buffer->setSize(0, true);
                m_encoder->process(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputTriggerId_EncodeHeader);

                info.connectionClientHandlerThread->scheduleBuffer(*op_buffer);
            }
            else {
                // When a new connection is found and the
                // acq server is _not_ started, drop the
                // connection

                m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Dropping connection - acquisition is not started\n";
                connection->release();
            }
        }
        m_pendingConnections.clear();
    }

    // Cleans disconnected client(s)
    for (auto it = m_connections.begin(); it != m_connections.end();) {
        Socket::IConnection* connection = it->first;
        if (!connection->isConnected()) {
            if (it->second.connectionClientHandlerStdThread) {
                requestClientThreadQuit(it->second.connectionClientHandlerThread);

                it->second.connectionClientHandlerStdThread->join();
                delete it->second.connectionClientHandlerStdThread;
                delete it->second.connectionClientHandlerThread;
            }
            connection->release();
            it = m_connections.erase(it);
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Closed connection...\n";
        }
        else { ++it; }
    }

    // Handles driver's main loop
    if (m_Driver) {
        bool res;
        if (this->isStarted()) {
            res                           = true;
            bool timeout                  = false;
            m_gotData                     = false;
            const uint32_t timeBeforeCall = System::Time::getTime();
            while (res && !m_gotData && !timeout) {
                // Calls driver's loop
                res = m_Driver->loop();
                // @fixme behavior seems to be bad if the loop() returns false; should fix this!
                if (!m_gotData) {
                    timeout = (System::Time::getTime() > timeBeforeCall + m_driverTimeoutDuration);

                    if (m_startedDriverSleepDuration > 0) {
                        // This may cause jitter due to inaccuracies in sleep duration, but has the
                        // benefit that it frees the CPU core for other tasks
                        System::Time::sleep(size_t(m_startedDriverSleepDuration));
                    }
                    else if (m_startedDriverSleepDuration == 0) {
                        // Generally spins, but gives other threads a chance. Note that there is no guarantee when
                        // the scheduler reschedules this thread.
                        std::this_thread::yield();
                    }
                    else {
                        // < 0 -> NOP, spins, doesn't offer to yield 
                        // n.b. Unless the driver waits for samples (preferably with a hardware event), 
                        // this choice will spin one core fully.
                    }
                }
            }
            if (timeout) {
                m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "After " << m_driverTimeoutDuration <<
                        " milliseconds, did not receive anything from the driver - Timed out\n";
                return false;
            }
            if (m_gotData && m_DriftCorrection.getDriftCorrectionPolicy() == EDriftCorrectionPolicies::Forced) {
                m_DriftCorrection.correctDrift(m_DriftCorrection.getSuggestedDriftCorrectionSampleCount(), m_nSample, m_PendingBuffers, m_PendingStimSet,
                                               m_SwapBuffers);
            }
        }
        else {
            // Calls driver's loop
            res = m_Driver->loop();
            System::Time::sleep(m_stoppedDriverSleepDuration);
        }

        // Calls driver's loop
        if (!res) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "Something bad happened in the loop callback, stopping the acquisition\n";
            return false;
        }
    }

    // Eventually builds up buffer and
    // sends data to connected client(s)
    while (m_PendingBuffers.size() >= m_nSamplePerSentBlock * 2) {
        const int64_t p = m_nSample - m_PendingBuffers.size();
        if (p < 0) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Signed number used for bit operations:" << p << " (case A)\n"; }
        if (p % m_nSamplePerSentBlock != 0) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Buffer start sample " << p << " doesn't seem to be divisible by "
                    << m_nSamplePerSentBlock << " (case A)\n";
        }

        // n.b. here we use arithmetic based on buffer duration so that we are in perfect agreement with
        // Acquisition Client box that sets the chunk starts and ends by steps of buffer duration.
        const uint64_t duration     = ip_bufferDuration;
        const uint64_t startSamples = m_nSample - m_PendingBuffers.size();
        const uint64_t nBuffer      = startSamples / m_nSamplePerSentBlock;
        const uint64_t startTime    = nBuffer * duration;
        const uint64_t endTime      = startTime + duration;
        const uint64_t lastTime     = CTime(m_sampling, m_nSample).time();

        // Pass the stimuli and buffer to all plugins; note that they may modify them
        for (auto itp = m_Plugins.begin(); itp != m_Plugins.end(); ++itp) {
            (*itp)->loopHook(m_PendingBuffers, m_PendingStimSet, startTime, endTime, lastTime);
        }

        // Handle connections
        for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
            // Socket::IConnection* connection=it->first;
            connection_info_t& info = it->second;

            if (info.nSampleToSkip < m_nSamplePerSentBlock) {
                m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Creating buffer for connection " << uint64_t(it->first) << "\n";

                // Signal buffer
                for (size_t j = 0; j < m_nChannel; ++j) {
                    for (size_t i = 0; i < m_nSamplePerSentBlock; ++i) {
                        ip_matrix->getBuffer()[j * m_nSamplePerSentBlock + i] = double(m_PendingBuffers[int(i + info.nSampleToSkip)][j]);
                    }
                }

                // Boundaries of the part of the buffer to be sent to this connection
                const uint64_t connBufferTimeOffset = CTime(m_sampling, info.nSampleToSkip).time();
                const uint64_t connBlockStartTime   = startTime + connBufferTimeOffset;
                const uint64_t connBlockEndTime     = endTime + connBufferTimeOffset;

                //m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "start: " << (start) << "end: " << (end) << "\n";

                // Stimulation buffer
                CStimulationSet& stimSet = *ip_stimSet;
                stimSet.clear();

                // Take the stimuli range valid for the buffer and adjust wrt connection time (stamp at connection = stamp at time 0 for the client)
                for (size_t k = 0; k < m_PendingStimSet.size(); ++k) {
                    const uint64_t date = m_PendingStimSet.getDate(k); // this date is wrt the whole acquisition time in the server
                    if (date >= connBlockStartTime && date <= connBlockEndTime) {
                        // The new date is wrt the specific connection time of the client (i.e. the chunk times on Designer side)
                        const uint64_t newDate = ((date > info.stimulationTimeOffset) ? (date - info.stimulationTimeOffset) : 0);

                        /*
                        std::cout << std::setprecision(10) << CTime(date).toSeconds() << " to " << CTime(newDate).toSeconds()
                                << " for [" << CTime(startTime).toSeconds() << "," << CTime(endTime).toSeconds()  << "]\n";
                        */

                        stimSet.push_back(m_PendingStimSet.getId(k), newDate, m_PendingStimSet.getDuration(k));
                    }
                }

                // Send a chunk of channel units? Note that we'll always send the units header.
                if (!info.isChannelUnitsSent) {
                    // If default values in channel units, don't bother sending unit data chunk
                    ip_encodeChannelUnitData = m_headerCopy->isChannelUnitSet();
                    // std::cout << "Set " <<  ip_channelUnits->getBufferElementCount()  << "\n";
                }

                op_buffer->setSize(0, true);
                m_encoder->process(OVP_GD_Algorithm_MasterAcquisitionEncoder_InputTriggerId_EncodeBuffer);

                if (!info.isChannelUnitsSent) {
                    // Do not send again
                    info.isChannelUnitsSent  = true;
                    ip_encodeChannelUnitData = false;
                }

                info.connectionClientHandlerThread->scheduleBuffer(*op_buffer);
            }
            else {
                // Here sample count to skip >= block size, so we effective drop this chunk from the viewpoint of this connection.
                // n.b. This is the reason why the pending buffer size needs to be 2x, as the skip can be up to 1x bufferSize and 
                // after that we need a full buffer to send. Note that by construction
                // the count to skip can never underflow but remains >= 0 (the other if branch doesnt decrement). 
                info.nSampleToSkip -= m_nSamplePerSentBlock;
            }
        }

        // Clears pending stimulations; Can start from zero as we know we'll never send anything in the future thats
        // before current BufferEndTime.
        m_PendingStimSet.removeRange(0, endTime);

        // Clears pending signal
        m_PendingBuffers.erase(m_PendingBuffers.begin(), m_PendingBuffers.begin() + m_nSamplePerSentBlock);
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CAcquisitionServer::connect(IDriver& driver, IHeader& headerCopy, const uint32_t nSamplingPerSentBlock, const uint32_t port)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "connect\n";


    m_Driver              = &driver;
    m_headerCopy          = &headerCopy;
    m_nSamplePerSentBlock = nSamplingPerSentBlock;

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Connecting to device [" << CString(m_Driver->getName()) << "]...\n";

    // Initializes driver
    if (!m_Driver->initialize(m_nSamplePerSentBlock, *this)) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Connection failed...\n";
        return false;
    }

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Connection succeeded !\n";

    const IHeader& header = *driver.getHeader();
    IHeader::copy(headerCopy, header);

    m_nChannel = headerCopy.getChannelCount();
    m_sampling = headerCopy.getSamplingFrequency() * m_overSamplingFactor;

    m_selectedChannels.clear();
    if (m_isChannelSelectionRequested) {
        for (size_t i = 0, l = 0; i < m_nChannel; ++i) {
            const std::string name = headerCopy.getChannelName(i);
            if (!name.empty()) {
                m_selectedChannels.push_back(i);
                headerCopy.setChannelName(l, name.c_str());

                size_t unit = 0, factor = 0;
                if (headerCopy.isChannelUnitSet()) {
                    headerCopy.getChannelUnits(i, unit, factor); // no need to check, will be defaults on failure
                    headerCopy.setChannelUnits(l, unit, factor);
                }
                l++;
            }
        }
        headerCopy.setChannelCount(m_selectedChannels.size());
        m_nChannel = headerCopy.getChannelCount();
    }
    else { for (size_t i = 0; i < m_nChannel; ++i) { m_selectedChannels.push_back(i); } }

    // These are passed to plugins
    m_selectedChannelNames.clear();
    for (size_t i = 0; i < headerCopy.getChannelCount(); ++i) { m_selectedChannelNames.push_back(CString(headerCopy.getChannelName(i))); }

    if (m_nChannel == 0) {
        std::stringstream ss;
        ss << "Driver claimed to have 0 channel";
        if (isChannelSelectionRequested()) { ss << "(check whether the property `Select only named channel' is set).\n"; }
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << ss.str();
        return false;
    }

    if (m_sampling == 0) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Driver claimed to have a sample frequency of 0.\n";
        return false;
    }

    m_impedances.resize(m_nChannel, OVAS_Impedance_NotAvailable);
    m_SwapBuffers.resize(m_nChannel);

    // A hack to check that the port is not already open; this doesn't guarantee it as another process
    // might grab the port after this test. But it will cover the normal use cases where multiple acquisition servers 
    // are started by the user manually at different times. A proper way to solve this would be to 
    // change ConnectionServer::listen() in the SDK to disallow multiple listens.
    Socket::IConnectionClient* testClient = Socket::createConnectionClient();
    if (testClient->connect("localhost", port, 100))  // 100ms
    {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Server Connection Port " << port <<
                " already in use. Please change the port or close the other application.\n";
        testClient->close();
        delete testClient;
        return false;
    }
    delete testClient;
    testClient = nullptr;

    m_connectionServer = Socket::createConnectionServer();
    if (m_connectionServer->listen(port)) {
        m_isInitialized = true;

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "NaN value correction is set to ";
        switch (m_eNaNReplacementPolicy) {
            default: case ENaNReplacementPolicy::LastCorrectValue: m_kernelCtx.getLogManager() << CString("LastCorrectValue") << "\n";
                break;
            case ENaNReplacementPolicy::Zero: m_kernelCtx.getLogManager() << CString("Zero") << "\n";
                break;
            case ENaNReplacementPolicy::Disabled: m_kernelCtx.getLogManager() << CString("Disabled") << "\n";
                break;
        }

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Oversampling factor set to " << m_overSamplingFactor << "\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Sampling frequency set to " << m_sampling << "Hz\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Started driver sleeping duration is " << m_startedDriverSleepDuration << " milliseconds\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Stopped driver sleeping duration is " << m_stoppedDriverSleepDuration << " milliseconds\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Driver timeout duration set to " << m_driverTimeoutDuration << " milliseconds\n";

        ip_bufferDuration = CTime(m_sampling, m_nSamplePerSentBlock).time();

        ip_subjectID     = headerCopy.getExperimentID();
        ip_subjectAge    = headerCopy.getSubjectAge();
        ip_subjectGender = headerCopy.getSubjectGender();

        ip_sampling = m_sampling;
        ip_matrix->resize(m_nChannel, m_nSamplePerSentBlock);

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Sampling rate     : " << m_sampling << "\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Samples per block : " << m_nSamplePerSentBlock << "\n";
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel count     : " << m_nChannel << "\n";

        for (size_t i = 0; i < m_nChannel; ++i) {
            const std::string name = headerCopy.getChannelName(i);
            if (!name.empty()) { ip_matrix->setDimensionLabel(0, i, name.c_str()); }
            else { ip_matrix->setDimensionLabel(0, i, ("Channel " + std::to_string(i + 1)).c_str()); }
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel name      : " << CString(ip_matrix->getDimensionLabel(0, i)) << "\n";
        }

        // Construct channel units stream header & matrix
        // Note: Channel units, although part of IHeader, will be sent as a matrix chunk during loop() once - if at all - to each client
        if (m_headerCopy->isChannelUnitSet()) {
            ip_channelUnits->resize(m_nChannel, 2);             // Units, Factor
            for (size_t c = 0; c < m_nChannel; ++c) {
                ip_channelUnits->setDimensionLabel(0, c, m_headerCopy->getChannelName(c));
                if (m_headerCopy->isChannelUnitSet()) {
                    size_t unit = 0, factor = 0;
                    m_headerCopy->getChannelUnits(c, unit, factor); // no need to check, will be defaults on failure

                    ip_channelUnits->getBuffer()[c * 2 + 0] = double(unit);
                    ip_channelUnits->getBuffer()[c * 2 + 1] = double(factor);

                    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel Unit      : " << unit << ", factor=" << OVTK_DECODE_FACTOR(factor) <<
                            "\n";
                }
            }
            ip_channelUnits->setDimensionLabel(1, 0, "Unit");
            ip_channelUnits->setDimensionLabel(1, 1, "Factor");
        }
        else {
            // Driver did not set units. Convention: send empty header matrix in this case.
            ip_channelUnits->clean();
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Driver did not set units, sending empty channel units matrix\n";
        }

        // @TODO Channel localisation
        ip_encodeChannelLocalisationData = false; // never at the moment

        // @TODO Gain is ignored
    }
    else {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Could not listen on TCP port (firewall problem ?)\n";
        return false;
    }

    m_connectionServerHandlerStdThread = new std::thread(CConnectionServerHandlerThread(*this, *m_connectionServer));

    return true;
}

bool CAcquisitionServer::start()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonStartPressedCB\n";

    if (isImpedanceCheckRequested()) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Please disable impedance check before pressing Play\n";
        return false;
    }

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Starting the acquisition...\n";

    m_PendingBuffers.clear();
    m_PendingStimSet.clear();

    m_nSample     = 0;
    m_nLastSample = 0;

    // Starts driver
    if (!m_Driver->start()) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Starting failed !\n";
        return false;
    }

    m_startTime        = System::Time::zgetTime();
    m_lastDeliveryTime = m_startTime;

    m_DriftCorrection.start(m_sampling, m_startTime);

    for (auto itp = m_Plugins.begin(); itp != m_Plugins.end(); ++itp) {
        const bool ok = (*itp)->startHook(m_selectedChannelNames, m_sampling, m_nChannel, m_nSamplePerSentBlock);
        if (!ok) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Problem starting a plugin, bailing out for safety.\n";
            return false;
        }
    }

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Now acquiring...\n";
    m_isStarted = true;
    return true;
}

bool CAcquisitionServer::requestClientThreadQuit(CConnectionClientHandlerThread* th)
{
    // Use a scoped lock before toggling a variable owned by the thread
    {
        std::lock_guard<std::mutex> oLock(th->m_ClientThreadMutex);

        // Tell the thread to quit
        th->m_PleaseQuit = true;
    }

    // Wake up the thread in case it happens to be waiting on the cond var
    th->m_PendingBufferCondition.notify_one();

    return true;
}

bool CAcquisitionServer::stop()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "buttonStopPressedCB\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Stopping the acquisition.\n";

    m_DriftCorrection.printStats();
    m_DriftCorrection.stop();

    // Stops driver
    m_Driver->stop();

    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it->first->isConnected()) { it->first->close(); }
        if (it->second.connectionClientHandlerStdThread) {
            requestClientThreadQuit(it->second.connectionClientHandlerThread);

            it->second.connectionClientHandlerStdThread->join();
            delete it->second.connectionClientHandlerStdThread;
            delete it->second.connectionClientHandlerThread;
        }
        it->first->release();
    }
    m_connections.clear();

    for (auto itp = m_Plugins.begin(); itp != m_Plugins.end(); ++itp) { (*itp)->stopHook(); }


    m_isStarted = false;

    m_PendingBuffers.clear();

    return true;
}

bool CAcquisitionServer::disconnect()
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Disconnecting.\n";

    if (m_isInitialized) { m_Driver->uninitialize(); }

    m_impedances.clear();

    if (m_connectionServer) {
        m_connectionServer->close();
        m_connectionServer->release();
        m_connectionServer = nullptr;
    }

    m_isInitialized = false;

    // Thread joining must be done after
    // switching m_isInitialized to false
    if (m_connectionServerHandlerStdThread) {
        m_connectionServerHandlerStdThread->join();
        delete m_connectionServerHandlerStdThread;
        m_connectionServerHandlerStdThread = nullptr;
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

void CAcquisitionServer::setSamples(const float* samples)
{
    if (samples == nullptr) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Null data detected\n"; }
    this->setSamples(samples, m_nSamplePerSentBlock);
}

void CAcquisitionServer::setSamples(const float* samples, const size_t count)
{
    if (m_isStarted) {
        for (size_t i = 0; i < count; ++i) {
            if (!m_replacementInProgress) {
                // otherwise NaN are propagating
                m_overSamplingSwapBuffers = m_SwapBuffers;
            }
            for (size_t k = 0; k < m_overSamplingFactor; ++k) {
                const float alpha = float(k + 1) / m_overSamplingFactor;

                bool hadNaN = false;

                for (size_t j = 0; j < m_nChannel; ++j) {
                    const size_t channel = m_selectedChannels[j];

                    if (std::isnan(samples[channel * count + i]) || !std::isfinite(samples[channel * count + i])) { // NaN or infinite values
                        hadNaN = true;

                        switch (m_eNaNReplacementPolicy) {
                            case ENaNReplacementPolicy::Disabled: m_SwapBuffers[j] = std::numeric_limits<float>::quiet_NaN();
                                break;
                            case ENaNReplacementPolicy::Zero: m_SwapBuffers[j] = 0;
                                break;
                            case ENaNReplacementPolicy::LastCorrectValue:
                                // we simply don't update the value
                                break;
                            default: break;
                        }
                    }
                    else { m_SwapBuffers[j] = alpha * samples[channel * count + i] + (1 - alpha) * m_overSamplingSwapBuffers[j]; }
                }

                const uint64_t currentIdx = m_nSample + i * m_overSamplingFactor + k;       // j is not included here as all channels have the equal sample time

                if (hadNaN) {
                    // When a NaN is encountered at time t1 on any channel, OVTK_StimulationId_Artifact stimulus is sent. When a first good sample is encountered 
                    // after the last bad sample t2, OVTK_StimulationId_NoArtifact stimulus is sent, i.e. specifying a range of bad data : [t1,t2]. The stimuli are global 
                    // and not specific to channels.

                    if (!m_replacementInProgress) {
                        const uint64_t incorrectBlockStarts = CTime(m_sampling, currentIdx).time();
                        m_PendingStimSet.push_back(OVTK_StimulationId_Artifact, incorrectBlockStarts, 0);
                        m_replacementInProgress = true;
                    }
                }
                else {
                    if (m_replacementInProgress) {
                        // @note -1 is used here because the incorrect-correct range is inclusive, [a,b]. So when sample is good at b+1, we set the end point at b.
                        const uint64_t incorrectBlockStops = CTime(m_sampling, currentIdx - 1).time();

                        m_PendingStimSet.push_back(OVTK_StimulationId_NoArtifact, incorrectBlockStops, 0);
                        m_replacementInProgress = false;
                    }
                }

                m_PendingBuffers.push_back(m_SwapBuffers);
            }
        }

        m_nLastSample = m_nSample;
        m_nSample += count * m_overSamplingFactor;

        m_DriftCorrection.estimateDrift(count * m_overSamplingFactor);

        m_lastDeliveryTime = System::Time::zgetTime();
        m_gotData          = true;
    }
    else { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "The acquisition is not started\n"; }
}

void CAcquisitionServer::setStimulationSet(const CStimulationSet& stimSet)
{
    if (m_isStarted) {
        const uint64_t time = CTime(m_sampling, m_nLastSample).time();
        m_PendingStimSet.append(stimSet, time);
    }
    else { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "The acquisition is not started\n"; }
}


bool CAcquisitionServer::updateImpedance(const size_t index, const double impedance)
{
    for (size_t i = 0; i < m_selectedChannels.size(); ++i) {
        if (index == m_selectedChannels[i]) {
            m_impedances[i] = impedance;
            return true;
        }
    }
    return false;
}

// ____________________________________________________________________________
//

bool CAcquisitionServer::setOversamplingFactor(const uint64_t oversamplingFactor)
{
    m_overSamplingFactor = oversamplingFactor;
    if (m_overSamplingFactor < 1) { m_overSamplingFactor = 1; }
    if (m_overSamplingFactor > 16) { m_overSamplingFactor = 16; }
    return true;
}

// ____________________________________________________________________________
//

bool CAcquisitionServer::acceptNewConnection(Socket::IConnection* connection)
{
    if (!connection) { return false; }

    const uint64_t time = System::Time::zgetTime();

    DoubleLock lock(&m_oPendingConnectionProtectionMutex, &m_oPendingConnectionExecutionMutex);

    connection_info_t info;
    info.connectionTime                   = time;
    info.stimulationTimeOffset            = 0; // not used
    info.nSampleToSkip                    = 0; // not used
    info.connectionClientHandlerThread    = nullptr; // not used
    info.connectionClientHandlerStdThread = nullptr; // not used
    info.isChannelLocalisationSent        = false;
    info.isChannelUnitsSent               = false;

    m_pendingConnections.push_back(std::pair<Socket::IConnection*, connection_info_t>(connection, info));

    for (auto itp = m_Plugins.begin(); itp != m_Plugins.end(); ++itp) { (*itp)->acceptNewConnectionHook(); }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
