#if defined(TARGET_HAS_ThirdPartyLSL)

/*
 *
 * Notes: This code should be kept compatible with changes to LSL Output plugin in OpenViBE Acquisition Server,
 * and LSL Export box in Designer.
 *
 * This driver makes a few assumptions:
 *
 * Signal streams
 *    - are float
 *    - dense, i.e. there are no dropped or extra samples in them
 *    - the driver fills a sample block consequently until either the block has been filled or
 *      time runs out. In case of the latter, the chunk is padded with NaNs.
 * Markers
 *    - are int
 *    - not dense
 *    - markers are retimed wrt the current signal chunk
 *
 * Other notes: Due to network delays, it may be better to disable drift correction or
 * set the drift tolerance to high. This is because
 *  1) Signals are assumed dense, i.e. LSL pull results in samples stamped t,t+1,t+2,...
 *  2) However, Acquisition Server works in real time, so if there is a network delay between
 *     the samples stamped t and t+1, this delay does not indicate a delay in the original signal
 *     and it might not be correct to pad the corresponding signal block during the delay.
 *
 * Todo. It might make sense to improve this driver in the future to take into account the LSL
 * stamps when constructing each signal block. This is currently not done.
 *

 *
 *
 *
 */

#include <limits> // for NaN

#include "ovasCDriverLabStreamingLayer.h"
#include "ovasCConfigurationLabStreamingLayer.h"

#include <system/ovCTime.h>

#include <lsl_cpp.h>

#include <cmath>
#include <algorithm>

namespace OpenViBE {
namespace AcquisitionServer {

// In seconds
static const double LSL_SAMPLING_ESTIMATATION_DURATION = 2.0;
static const double LSL_RESOLVE_TIME_OUT               = 2.0;
static const double LSL_OPEN_TIME_OUT                  = 2.0;
static const double LSL_READ_TIME_OUT                  = 2.0;

static uint32_t getClosestMultipleOf25(const uint32_t value) { return uint32_t(((value + 12) / 25) * 25); }

static uint32_t getClosestPowerOf2(const uint32_t value)
{
    uint32_t bitShiftCount = 0;
    while ((1LL << bitShiftCount) < value) { bitShiftCount++; }

    if (bitShiftCount > 0) {
        const uint32_t greater = uint32_t(1LL << bitShiftCount);
        const uint32_t lesser  = greater >> 1;
        if (greater - value < value - lesser) { return greater; }
        return lesser;
    }

    return uint32_t(1LL << bitShiftCount);
}

static uint32_t getSmartFallbackSamplingRateEstimate(const uint32_t lslSampling, const bool roundToPowerOfTwoAndMultiple25 = true)
{
    if (roundToPowerOfTwoAndMultiple25) {
        const uint32_t closestMultipleOf25     = getClosestMultipleOf25(lslSampling);
        const uint32_t closestPowerOfTwo       = getClosestPowerOf2(lslSampling);
        const uint32_t closestMultipleOf25Diff = uint32_t(std::abs(int(closestMultipleOf25) - int(lslSampling)));
        const uint32_t closestPowerOfTwoDiff   = uint32_t(std::abs(int(closestPowerOfTwo) - int(lslSampling)));
        if (closestMultipleOf25Diff < closestPowerOfTwoDiff) { return closestMultipleOf25; }
        return closestPowerOfTwo;
    }
    return lslSampling;
}

//___________________________________________________________________//
//                                                                   //

CDriverLabStreamingLayer::CDriverLabStreamingLayer(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_LabStreamingLayer", m_driverCtx.getConfigurationManager())
{
    // The following class allows saving and loading driver settings from the acquisition server .conf file
    m_settings.add("Header", &m_header);
    // To save your custom driver settings, register each variable to the SettingsHelper
    m_settings.add("LimitSpeed", &m_limitSpeed);
    m_settings.add("SignalStreamName", &m_sSignalStream);
    m_settings.add("SignalStreamID", &m_sSignalStreamID);
    m_settings.add("MarkerStreamName", &m_sMarkerStream);
    m_settings.add("MarkerStreamID", &m_sMarkerStreamID);
    m_settings.add("FallbackSamplingRate", &m_fallbackSampling);
    m_settings.load();

    /*
    for (uint32_t i = 1; i < 260; ++i)
    {
        ctx.getLogManager() << Kernel::LogLevel_Error << i << " --> " << getSmartFallbackSamplingRateEstimate(i) << "\n";
    }
    */
}

CDriverLabStreamingLayer::~CDriverLabStreamingLayer() {}

const char* CDriverLabStreamingLayer::getName() { return "LabStreamingLayer (LSL)"; }

//___________________________________________________________________//
//                                                                   //

bool CDriverLabStreamingLayer::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    // Find the signal stream
    const std::vector<lsl::stream_info> streams = lsl::resolve_stream("name", m_sSignalStream.toASCIIString(), 1, LSL_RESOLVE_TIME_OUT);
    if (streams.empty()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error resolving signal stream with name [" << m_sSignalStream.toASCIIString() << "]\n";
        return false;
    }
    for (uint32_t i = 0; i < streams.size(); ++i) {
        m_oSignalStream = streams[i];
        if (streams[i].source_id() == std::string(m_sSignalStreamID.toASCIIString())) {
            // This is the best one
            break;
        }
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Finally resolved signal stream to " << m_oSignalStream.name()
                << ", id " << m_oSignalStream.source_id().c_str() << "\n";
    }

    // Find the marker stream
    if (m_sMarkerStream != CString("None")) {
        const std::vector<lsl::stream_info> markerStreams = lsl::resolve_stream("name", m_sMarkerStream.toASCIIString(), 1, LSL_RESOLVE_TIME_OUT);
        if (markerStreams.empty()) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error resolving marker stream with name [" << m_sMarkerStream.toASCIIString() << "]\n";
            return false;
        }
        for (uint32_t i = 0; i < markerStreams.size(); ++i) {
            m_oMarkerStream = markerStreams[i];
            if (markerStreams[i].source_id() == std::string(m_sMarkerStreamID.toASCIIString())) {
                // This is the best one
                break;
            }
        }
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Finally resolved marker stream to " << m_oMarkerStream.name()
                << ", id " << m_oMarkerStream.source_id().c_str() << "\n";
    }
    else {
        // We do not have a marker stream. This is ok.
    }

    // Get the channel names. We open a temporary inlet for this, it will be closed after going out of scope. The actual inlet will be opened in start().
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Polling channel names\n";
    lsl::stream_inlet tmpInlet(m_oSignalStream, 360, 0, false);
    try { tmpInlet.open_stream(LSL_OPEN_TIME_OUT); }
    catch (...) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to open signal stream with name [" << m_oSignalStream.name()
                << "] for polling channel names\n";
        return false;
    }

    lsl::stream_info fullInfo;
    try { fullInfo = tmpInlet.info(LSL_READ_TIME_OUT); }
    catch (...) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Timeout reading full stream info for [" << m_oSignalStream.name()
                << "] for polling channel names\n";
        return false;
    }

    // Now sets channel count and names
    m_header.setChannelCount(m_oSignalStream.channel_count());

    const lsl::xml_element channels = fullInfo.desc().child("channels");
    lsl::xml_element channel        = channels.child("channel");
    for (size_t i = 0; i < m_header.getChannelCount(); ++i) {
        const char* label = channel.child_value("label");

        if (label) { m_header.setChannelName(i, label); }

        channel = channel.next_sibling("channel");
    }

    // Buffer to store a single sample
    m_buffer = new float[m_header.getChannelCount()];
    if (!m_buffer) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Memory allocation problem\n";
        return false;
    }

    // Buffer to store the signal chunk
    m_sample = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    if (!m_sample) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Memory allocation problem\n";
        return false;
    }

    // Now sets sampling rate

    uint32_t sampling = uint32_t(m_oSignalStream.nominal_srate());
    if (sampling == 0) {
        // Check GUI fallback
        if (m_fallbackSampling != 0) {
            sampling = m_fallbackSampling;
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "LSL sampling rate is not defined, falls back to ["
                    << sampling << "] - You can change this in the driver settings\n";
        }
        else {
            // Autodetection was requested, let`s go for a round
            m_driverCtx.getLogManager() << Kernel::LogLevel_Info
                    << "LSL sampling rate is not defined, falls back to [Autodetection] - You can change this in the driver settings\n";

            try {
                tmpInlet.open_stream(LSL_OPEN_TIME_OUT);

                // First drop all available samples
                while (tmpInlet.samples_available() != 0) { tmpInlet.pull_sample(m_buffer, m_header.getChannelCount(), LSL_READ_TIME_OUT); }

                // Now capture incoming samples over a period of 2 secs
                uint32_t nSample              = 0;
                const double startCaptureTime = tmpInlet.pull_sample(m_buffer, m_header.getChannelCount(), LSL_READ_TIME_OUT);
                double currentCaptureTime     = startCaptureTime;
                while (currentCaptureTime != 0.0 && currentCaptureTime - startCaptureTime < LSL_SAMPLING_ESTIMATATION_DURATION) {
                    const double pulledSampleTime = tmpInlet.pull_sample(m_buffer, m_header.getChannelCount(), LSL_READ_TIME_OUT);
                    if (pulledSampleTime != 0) {
                        nSample++;
                        currentCaptureTime = pulledSampleTime;
                    }
                    else {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error
                                << "Timed out while receiving samples from LSL stream, avoiding auto detection.\n";
                        delete[] m_buffer;
                        delete[] m_sample;
                        return false;
                    }
                }

                // Finally set estimated sampling rate
                const double duration           = currentCaptureTime - startCaptureTime;
                const uint32_t lslSampling      = uint32_t(nSample / duration);
                const uint32_t smartLSLSampling = getSmartFallbackSamplingRateEstimate(lslSampling);

                if (smartLSLSampling != lslSampling) {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquired " << nSample << " on a duration of " << duration
                            << " seconds, sampling rate might be approx " << lslSampling << " hz (adjusted to "
                            << smartLSLSampling << " hz to conform with typical EEG sampling rates).\n";
                }
                else {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Acquired " << nSample << " on a duration of " << duration
                            << " seconds, sampling rate might be approx " << smartLSLSampling << " hz.\n";
                }

                sampling = smartLSLSampling;
            }
            catch (...) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to detected actual LSL sampling rate\n";
                delete [] m_buffer;
                delete [] m_sample;
                return false;
            }
        }
    }

    m_header.setSamplingFrequency(sampling);

    if (sampling != m_oSignalStream.nominal_srate()) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Opened an LSL stream with " << m_oSignalStream.channel_count()
                << " channels and a nominal rate of " << m_oSignalStream.nominal_srate() << " hz.\n";
    }
    else {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Opened an LSL stream with " << m_oSignalStream.channel_count()
                << " channels and a nominal rate of " << m_oSignalStream.nominal_srate() << " hz" << " adjusted to " << sampling << " hz.\n";
    }

    // Stores parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    return true;
}

bool CDriverLabStreamingLayer::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_pSignalInlet = new lsl::stream_inlet(m_oSignalStream, 360, 0, false);
    if (!m_pSignalInlet) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error getting signal inlet for [" << m_oSignalStream.name() << "]\n";
        return false;
    }

    try { m_pSignalInlet->open_stream(LSL_OPEN_TIME_OUT); }
    catch (...) {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to open signal stream with name [" << m_oSignalStream.name() << "]\n";
        return false;
    }

    if (m_sMarkerStream != CString("None")) {
        m_pMarkerInlet = new lsl::stream_inlet(m_oMarkerStream, 360, 0, false);
        if (!m_pMarkerInlet) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error getting marker inlet for [" << m_oMarkerStream.name() << "]\n";
            return false;
        }

        try { m_pMarkerInlet->open_stream(LSL_OPEN_TIME_OUT); }
        catch (...) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to open marker stream with name [" << m_oSignalStream.name() << "]\n";
            return false;
        }
    }

    m_startTime = System::Time::zgetTime();
    m_nSample   = 0;

    return true;
}

bool CDriverLabStreamingLayer::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    const size_t nChannels = m_header.getChannelCount();

    bool timeOut           = false;
    uint32_t timeOutAt     = 0;
    bool blockStartTimeSet = false;
    double blockStartTime  = 0;

    // receive signal from the stream
    for (uint32_t i = 0; i < m_nSamplePerSentBlock; ++i) {
        double captureTime;
        try { captureTime = m_pSignalInlet->pull_sample(m_buffer, nChannels, LSL_READ_TIME_OUT); }
        catch (...) {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to get signal sample from [" << m_oSignalStream.name() << "]\n";
            return false;
        }
        if (captureTime != 0) {
            if (!blockStartTimeSet) {
                blockStartTimeSet = true;
                blockStartTime    = captureTime;
                //m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "First cap time is " << captureTime << "\n";
            }

            // Sample ok, fill
            for (uint32_t j = 0; j < nChannels; ++j) { m_sample[j * m_nSamplePerSentBlock + i] = m_buffer[j]; }
        }
        else {
            // Timeout
            timeOutAt = i;
            timeOut   = true;
            break;
        }
    }

    if (timeOut) {
        // We fill the rest of the buffer with NaNs
        for (uint32_t i = timeOutAt; i < m_nSamplePerSentBlock; ++i) {
            for (uint32_t j = 0; j < nChannels; ++j) { m_sample[j * m_nSamplePerSentBlock + i] = std::numeric_limits<float>::quiet_NaN(); }
        }
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Timeout reading sample from " << timeOutAt << ", filled rest of block with NaN\n";
    }

    if (m_limitSpeed) {
        // If we were faster than what the AS expects, sleep.
        // n.b. This sleep may not be accurate on Windows (it may oversleep)
        const uint64_t timeNow            = System::Time::zgetTime() - m_startTime;
        const uint64_t timeLimitForBuffer = CTime(m_header.getSamplingFrequency(), m_nSample + m_nSamplePerSentBlock).time();

        if (timeNow < timeLimitForBuffer) {
            const uint64_t timeToSleep = timeLimitForBuffer - timeNow;

            //          std::cout << "PostNap " << CTime(timeToSleep).toSeconds()*1000 << "ms at " << m_nSample+m_nSamplePerSentBlock << "\n";

            System::Time::zsleep(timeToSleep);
        }
    }

    m_nSample += m_nSamplePerSentBlock;

    m_callback->setSamples(m_sample);

    // receive and pass markers. Markers are timed wrt the beginning of the signal block.
    CStimulationSet stimulationSet;
    if (m_pMarkerInlet) {
        while (true) {
            int marker;
            double captureTime;
            try {
                captureTime = m_pMarkerInlet->pull_sample(&marker, 1, 0); // timeout is 0 here on purpose, either we have markers or not
            }
            catch (...) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to get marker from [" << m_oMarkerStream.name() << "]\n";
                return false;
            }
            if (captureTime == 0) {
                // no more markers available at the moment
                break;
            }
            // double correction = m_pMarkerInlet->time_correction();
            // double stimTime = captureTime + correction - firstCaptureTime;

            // For openvibe, we need to set the stimulus time relative to the start of the chunk
            static bool warningPrinted = false;
            if (captureTime < blockStartTime && !warningPrinted) {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Received marker from LSL date from before the start of the current chunk."
                        " Adjusting stamp to start of current chunk. Will not warn again.\n";
                warningPrinted = true;
            }
            const double stimTime = (captureTime > blockStartTime ? captureTime - blockStartTime : 0);

            // m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Got a marker " << marker << " at " << captureTime << " -> "
            //  << stimTime << "s."
            //  << "\n";

            stimulationSet.push_back(uint64_t(marker), CTime(stimTime).time(), 0);
            // std::cout << "date " << stimTime << "\n";
        }
    }

    m_callback->setStimulationSet(stimulationSet);

    // LSL is not forcing the sample stream to confirm to the nominal sample rate. Hence, data may be incoming
    // with slower or faster speed than implied by the rate (a little like reading from a file). In some
    // cases it may be meaningful to disable the following drift correction from the AS settings.
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

    return true;
}

bool CDriverLabStreamingLayer::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    if (m_pSignalInlet) {
        m_pSignalInlet->close_stream();

        delete m_pSignalInlet;
        m_pSignalInlet = nullptr;
    }

    if (m_pMarkerInlet) {
        m_pMarkerInlet->close_stream();

        delete m_pMarkerInlet;
        m_pMarkerInlet = nullptr;
    }

    return true;
}

bool CDriverLabStreamingLayer::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_buffer) {
        delete[] m_buffer;
        m_buffer = nullptr;
    }

    if (m_sample) {
        delete[] m_sample;
        m_sample = nullptr;
    }

    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverLabStreamingLayer::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDriverLabStreamingLayer::configure()
{
    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationLabStreamingLayer config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-LabStreamingLayer.ui",
                                           m_header, m_limitSpeed, m_sSignalStream, m_sSignalStreamID, m_sMarkerStream, m_sMarkerStreamID, m_fallbackSampling);

    if (!config.configure(m_header)) { return false; }
    m_settings.save();

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif  // TARGET_HAS_ThirdPartyLSL
