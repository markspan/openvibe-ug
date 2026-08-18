#include "ovasCPluginExternalStimulations.h"
#include "ovasParameterHelper.h"

#include <boost/interprocess/ipc/message_queue.hpp>

#include <vector>
#include <ctime>

#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {
namespace Plugins {

CPluginExternalStimulations::CPluginExternalStimulations(const Kernel::IKernelContext& ctx)
    : IAcquisitionServerPlugin(ctx, "AcquisitionServer_Plugin_ExternalStimulations"),
     m_enableExternalStimulations("Enable External Stimulations", "Enable external stimulations", false),
     m_externalStimulationQueueName("External Stimulation Queue Name", "Name of the external stimulations queue", "openvibeExternalStimulations")
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Loading plugin: ExternalStimulations (deprecated)\n";
    m_parameters.push_back(&m_enableExternalStimulations);
    m_parameters.push_back(&m_externalStimulationQueueName);
    
    loadParametersFromConfig(m_kernelCtx.getConfigurationManager(), 
                             {{"EnableExternalStimulations", &m_enableExternalStimulations}, {"ExternalStimulationQueueName", &m_externalStimulationQueueName}},
                             this->m_name);
}

// Hooks
bool CPluginExternalStimulations::startHook(const std::vector<CString>& /*selectedChannelNames*/, const size_t /*sampling*/, const size_t /*nChannel*/, const size_t /*nSamplePerSentBlock*/)
{
    if (m_enableExternalStimulations)
    {
        ftime(&m_CTStartTime);
        m_IsESThreadRunning = true;
        m_ESthreadPtr.reset(new std::thread(std::bind(&CPluginExternalStimulations::readExternalStimulations, this)));
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "External stimulations (deprecated) activated...\n";
    }
    m_ExternalStimulations.clear();

    m_DebugExternalStimulationsSent      = 0;
    m_DebugCurrentReadIPCStimulations    = 0;
    m_DebugStimulationsLost              = 0;
    m_DebugStimulationsReceivedEarlier   = 0;
    m_DebugStimulationsReceivedLate      = 0;
    m_DebugStimulationsReceivedWrongSize = 0;
    m_DebugStimulationsBuffered          = 0;

    return true;
}

void CPluginExternalStimulations::loopHook(std::deque<std::vector<float>>& /* vPendingBuffer */, CStimulationSet& stimulationSet, const uint64_t start, const uint64_t end, const uint64_t /* sampleTime */)
{
    if (m_enableExternalStimulations)
    {
        //m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Checking for external stimulations:" << p << "\n";
        addExternalStimulations(&stimulationSet, m_kernelCtx.getLogManager(), start, end);
    }
}

void CPluginExternalStimulations::stopHook()
{
    if (m_enableExternalStimulations)
    {
        m_IsESThreadRunning = false;
        if (m_ESthreadPtr) { m_ESthreadPtr->join(); }
        else { m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Warning: External Stims plugin stopHook() tried to join a NULL thread\n"; }
    }

    //software tagging diagnosting
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Total external ones received through IPC: " << m_DebugCurrentReadIPCStimulations << "\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Sent to Designer: " << m_DebugExternalStimulationsSent << "\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Lost because of invalid timestamp: " << m_DebugStimulationsLost << "\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Stimulations that came earlier: " << m_DebugStimulationsReceivedEarlier << "\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Stimulations that came later: " << m_DebugStimulationsReceivedLate << "\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Stimulations that had wrong size: " << m_DebugStimulationsReceivedWrongSize << "\n";
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "  Buffered: " << m_DebugStimulationsBuffered << "\n";
    //end software tagging diagnosting
}

// Plugin specific methods

void CPluginExternalStimulations::readExternalStimulations()
{
    using namespace boost::interprocess;

    //std::cout << "Creating External Stimulations thread" << std::endl;
    //std::cout << "Queue Name : " << m_externalStimulationQueueName << std::endl;
    //char mq_name[255];
    //std::strcpy(mq_name, m_externalStimulationQueueName.toASCIIString());
    const int chunkLength = 3;
    const int pauseTime   = 5;

    uint32_t priority;
    size_t recvdSize;

    uint64_t chunk[chunkLength];

    while (m_IsESThreadRunning)
    {
        bool success;
        try
        {
            //Open a message queue.
            message_queue mq(open_only  //only open
                             , m_externalStimulationQueueName.value().c_str()    //name
                             //,mq_name    //name
            );

            success = mq.try_receive(&chunk, sizeof(chunk), recvdSize, priority);
        }
        catch (interprocess_exception& /* ex */)
        {
            //m_IsESThreadRunning = false;
            //m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Problem with message queue in external stimulations:" << ex.what() << "\n";
            System::Time::sleep(pauseTime);
            continue;
        }

        if (!success)
        {
            System::Time::sleep(pauseTime);
            continue;
        }

        m_DebugCurrentReadIPCStimulations++;

        if (recvdSize != sizeof(chunk))
        {
            //m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Problem with type of received data when reqding external stimulation!\n";
            m_DebugStimulationsReceivedWrongSize++;
        }
        else
        {
            //m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "received\n";

            SExternalStimulation stim;

            stim.identifier             = chunk[1];
            const uint64_t receivedTime = chunk[2];

            //1. calculate time
            const uint64_t ctStartTimeMs = (m_CTStartTime.time * 1000 + m_CTStartTime.millitm);

            const int64_t timeTest = receivedTime - ctStartTimeMs;

            if (timeTest < 0)
            {
                m_DebugStimulationsLost++;
                //m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning <<  "AS: external stimulation time is invalid, probably stimulation is before reference point, total invalid so far: " << m_FlashesLost << "\n";
                System::Time::sleep(pauseTime);
                continue; //we skip this stimulation
            }
            //2. Convert to OpenVibe time
            const uint64_t ctEventTime = receivedTime - ctStartTimeMs;

            const double time = double(ctEventTime) / double(1000);

            const uint64_t ovTime = CTime(time).time();
            stim.timestamp        = ovTime;

            //3. Store, the main thread will process it
            {
                //lock
                std::lock_guard<std::mutex> lock(m_es_mutex);

                m_ExternalStimulations.push_back(stim);
                m_DebugStimulationsBuffered++;
                m_esAvailable.notify_one();
                //unlock
            }

            System::Time::sleep(pauseTime);
        }
    }
}

void CPluginExternalStimulations::addExternalStimulations(CStimulationSet* ss, Kernel::ILogManager& /*logm*/, const uint64_t start, const uint64_t /*end*/)
{
    const uint64_t durationMs = 40;
    {
        //lock
        std::lock_guard<std::mutex> lock(m_es_mutex);

        for (auto i = m_ExternalStimulations.begin(); i != m_ExternalStimulations.end(); ++i)
        {
            // if time is current or any time in the future - send it (AS will buffer it)
            if (i->timestamp >= start)
            {
                //flashes_in_this_time_chunk++;
                //logm << Kernel::LogLevel_Error << "Stimulation added." << "\n";
                ss->push_back(i->identifier, i->timestamp, durationMs);
            }
            else
            {
                //the stimulation is coming too late - after the current block being processed
                //we correct the timestamp to the current block and we send it
                m_DebugStimulationsReceivedLate++;
                ss->push_back(i->identifier, start, durationMs);
            }
            m_DebugExternalStimulationsSent++;
        }

        // Since we processed all stimulations, we can clear the queue
        m_ExternalStimulations.clear();

        m_esAvailable.notify_one();
        //unlock
    }
}

bool CPluginExternalStimulations::setExternalStimulationsEnabled(const bool active)
{
    m_enableExternalStimulations = active;
    return true;
}

}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
