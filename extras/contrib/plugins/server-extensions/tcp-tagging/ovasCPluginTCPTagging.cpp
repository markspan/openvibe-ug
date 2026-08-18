#include "ovasCPluginTCPTagging.h"
#include "ovasParameterHelper.h"

#include <system/ovCTime.h>

// #define TCPTAGGING_DEBUG
#if defined(TCPTAGGING_DEBUG)
#include <iomanip>
#endif

namespace OpenViBE {
namespace AcquisitionServer {
namespace Plugins {

CPluginTCPTagging::CPluginTCPTagging(const Kernel::IKernelContext& ctx)
    : IAcquisitionServerPlugin(ctx, "AcquisitionServer_Plugin_TCPTagging"),
      m_port("TCP Tagging Port", "TCP Tagging Port" , 15361, 0 , 65535)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Loading plugin: TCP Tagging\n";

    m_parameters.push_back(&m_port);
    loadParametersFromConfig(m_kernelCtx.getConfigurationManager(),
                            {{"TCP_Tagging_Port", &m_port}},
                             this->m_name);
}

bool CPluginTCPTagging::startHook(const std::vector<CString>& /*vSelectedChannelNames*/, const size_t /*sampling*/, const size_t /*nChannel*/,
                                  const size_t /*nSamplePerSentBlock*/)
{
    // initialize tag stream
    // this may throw exceptions, e.g. when the port is already in use.
    try { m_scopedTagStream.reset(new CTagStream(m_port)); }
    catch (std::exception& e)
    {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Could not create tag stream for TCP Tagging [" << e.what()
                << "]. Make sure the port " << m_port << " is not already reserved by another Acquisition Server.\n";
        return false;
    }

    // Initialize time counters.
    m_previousClockTime   = System::Time::zgetTimeRaw(false);
    m_previousSampleTime  = 0;
    m_lastTagTime         = 0;
    m_lastTagTimeAdjusted = 0;
    m_warningPrinted      = false;

    return true;
}

void CPluginTCPTagging::stopHook() { m_scopedTagStream.reset(); }

// n.b. With this version of tcp tagging, all the timestamps are in fixed point
void CPluginTCPTagging::loopHook(std::deque<std::vector<float>>& /*vPendingBuffer*/,
                                 CStimulationSet& stimulationSet, uint64_t /*start*/, uint64_t /*end*/, const uint64_t sampleTime)
{
    const uint64_t clockTime = System::Time::zgetTimeRaw(false);

    Tag tag;

    // n.b. the last chunk received but not yet sent is between timestamps [previousClockTime,clockTime]. Note 
    // that more stims may be arriving in the loop meaning that they are newer than 'clockTime'. This is not an issue,
    // they will be scheduled with future samples.

    // Collect tags from the stream until exhaustion.
    while (m_scopedTagStream.get() && m_scopedTagStream->pop(tag))
    {
        const uint64_t tagTime = tag.timestamp;

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "New Tag received (" << tag.flags << ", " << tag.identifier << ", "
                << CTime(tagTime).toSeconds() << "s) at "
                << CTime(clockTime).toSeconds() << "s\n";

        uint64_t tagDelay = 0;
        // Check that the timestamp fits the current chunk. Unfortunately we cannot send stimulations to the past.
        if (tagTime < m_previousClockTime)
        {
            // This condition is relatively easy to achieve with high sampling rate & small block size with frequent stims (like in P300)
            tag.timestamp = m_previousClockTime;
            tagDelay      = tag.timestamp - tagTime;
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "A tag "
                    << tag.identifier << " is stamped before the current chunk start; it will be late"
                    << " (delay " << CTime(tagDelay).toSeconds() * 1000.0 << "ms)\n";
        }
        if (tagTime < m_lastTagTime)
        {
            tag.timestamp = m_lastTagTime;
            tagDelay      = tag.timestamp - tagTime;
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "A tag "
                    << tag.identifier << " is stamped before the previous tag; will delay"
                    << " (delay " << CTime(tagDelay).toSeconds() * 1000.0 << "ms)\n";
        }
        m_lastTagTime = tagTime;

        // This simple and intuitive implementation has issues if the device lags:
        // if previoussampletime does not advance evenly we get problems with tag ordering; this may also be related to the frequency this function is called
        // const uint64_t tagOffsetClock = tag.timestamp - m_previousClockTime;  // How far in time the marker is from the last call to this function
        // uint64_t adjustedTagTime = m_previousSampleTime + tagOffsetClock;

        // This version estimates how far a sample is between [t1,t2] where the stamps t1,t2 are realtime of previous and current call, 
        // and uses this fraction on [previousSampleTime,currentSampleTime] to get an adjustment in terms of sample time. It is
        // equivalent to interpolating between the two. To avoid implementing unsigned fixed point division, we go for doubles. 
        // This should give 10e-15 decimal precision which should be more than enough for EEG.

        const double elapsedClockTime  = CTime(clockTime - m_previousClockTime).toSeconds();        // n.b. here we assume the clocks will not run backwards
        const double elapsedSampleTime = CTime(sampleTime - m_previousSampleTime).toSeconds();
        const double tagOffsetClock    = CTime(tag.timestamp - m_previousClockTime).toSeconds();    // How far in time the marker is from the last call to this

        const double scaling            = elapsedSampleTime / elapsedClockTime;
        const double interpolatedOffset = (elapsedClockTime > 0 ? (tagOffsetClock * scaling) : 0);
        const uint64_t offsetSampleTime = CTime(interpolatedOffset).time();

        uint64_t adjustedTagTime = m_previousSampleTime +
                                   offsetSampleTime;    // Time since the beginning of the current buffer (as approx by time of the last sample of the prev. run)

        if (adjustedTagTime < m_lastTagTimeAdjusted)
        {
            tagDelay = m_lastTagTimeAdjusted - adjustedTagTime;
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "A tag "
                    << tag.identifier << " was adjusted before the previous tag; will delay"
                    << " (delay " << CTime(tagDelay).toSeconds() * 1000.0 << "ms)"
                    << " oc " << tagOffsetClock * 1000.0 << "ms\n";
            adjustedTagTime = m_lastTagTimeAdjusted;
        }
        m_lastTagTimeAdjusted = adjustedTagTime;

#if defined(TCPTAGGING_DEBUG)
        // If the amp and the AS computer are both behaving similarly, the scaling term should be very 
        // close to 1 and the diff term should be nearly zero. In that case the approach behaves
        // like the simple, commented out solution above.
        std::cout << "Set tag " << tag.identifier 
            << " at " << std::setprecision(6) << CTime(adjustedTagTime).toSeconds()
            << " (pst = " << CTime(m_previousSampleTime).toSeconds() << "s,"
            << " pct = " << CTime(m_previousClockTime).toSeconds() << "s,"
            << " otag = " << CTime(tagTime).toSeconds()  << "s,"
            << " ntag = " << CTime(tag.timestamp).toSeconds() << "s,"
            << " s = " << scaling << ","
            << " off = " << tagOffsetClock << "s,"
            << " offI = " << interpolatedOffset << "s,"
            << " diff = " << (interpolatedOffset - tagOffsetClock)*1000.0 << "ms,"
            << " del = " << CTime(tagDelay).toSeconds()*1000.0 << "ms"
            << ")\n";
#endif

        if (tagDelay > 0)
        {
            // Indicates that the next tag after this one may not be correctly placed in time.
            // The duration encodes our estimate how much the tag was delayed. This has
            // the benefit that this knowledge can be inserted into file recordings and is not lost like logs potentially.
            stimulationSet.push_back(OVTK_GDF_Incorrect, adjustedTagTime, tagDelay);
        }

        // Insert tag into the stimulation set.
        stimulationSet.push_back(tag.identifier, adjustedTagTime, 0);
    }

    // Update time counters. Basically these counters allow to map the time a stamp was received to the time related to the sample buffers,
    // as we know this function is called right after receiving samples from a device.
    m_previousClockTime  = clockTime;
    m_previousSampleTime = sampleTime;
}

}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
