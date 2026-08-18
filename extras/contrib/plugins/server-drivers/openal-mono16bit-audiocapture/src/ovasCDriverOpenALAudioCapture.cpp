#if defined TARGET_HAS_ThirdPartyOpenAL
#include "ovasCDriverOpenALAudioCapture.h"
#include "ovasCConfigurationOpenALAudioCapture.h"

#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverOpenALAudioCapture::CDriverOpenALAudioCapture(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_OpenALAudioCapture", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(8192);
    m_header.setChannelCount(1);
    Device        = nullptr;
    Context       = nullptr;
    CaptureDevice = nullptr;

    m_settings.add("Header", &m_header);
    m_settings.load();
}

CDriverOpenALAudioCapture::~CDriverOpenALAudioCapture() {}

const char* CDriverOpenALAudioCapture::getName() { return "OpenAL audio capture"; }

//___________________________________________________________________//
//                                                                   //

bool CDriverOpenALAudioCapture::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::initialize\n";

    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    m_sample = new float[nSamplePerSentBlock];
    if (!m_sample)
    {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }

    Samples = new ALshort[nSamplePerSentBlock];
    if (!Samples)
    {
        delete [] Samples;
        Samples = nullptr;
        return false;
    }

    // Open default audio device
    Device = alcOpenDevice(nullptr);
    if (!Device)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Default audio device opening failed.\n";
        return false;
    }

    // Create an audio context
    Context = alcCreateContext(Device, nullptr);
    if (!Context)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Audio context creation failed.\n";
        return false;
    }

    // Activate context
    if (!alcMakeContextCurrent(Context))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Audio context activation failed.\n";
        return false;
    }

    // Verify if audio capture is supported by the computer
    if (alcIsExtensionPresent(Device, "ALC_EXT_CAPTURE") == AL_FALSE)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Default audio device does not support audio capture.\n";
        return false;
    }

    // Open capture device
    CaptureDevice = alcCaptureOpenDevice(nullptr, (ALCsizei)m_header.getSamplingFrequency(), AL_FORMAT_MONO16, (ALCsizei)m_header.getSamplingFrequency());
    if (!CaptureDevice)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Default capture device opening failed.\n";
        return false;
    }

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    return true;
}

bool CDriverOpenALAudioCapture::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // request hardware to start
    // sending data
    // ...
    alcCaptureStart(CaptureDevice);

    return true;
}

bool CDriverOpenALAudioCapture::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::loop\n";

    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    // Activate context
    if (!alcMakeContextCurrent(Context)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Audio context activation failed.\n"; }

    ALCint SamplesAvailable;

    do
    {
        alcGetIntegerv(CaptureDevice, ALC_CAPTURE_SAMPLES, 1, &SamplesAvailable);
        if ((uint32_t)SamplesAvailable >= m_nSamplePerSentBlock)
        {
            alcCaptureSamples(CaptureDevice, &Samples[0], (ALCsizei)m_nSamplePerSentBlock);
            for (uint32_t i = 0; i < m_nSamplePerSentBlock; ++i) { m_sample[i] = (float)Samples[i]; }
            m_callback->setSamples(m_sample);
            m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
        }
    } while ((uint32_t)SamplesAvailable >= m_nSamplePerSentBlock);

    return true;
}

bool CDriverOpenALAudioCapture::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::start\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    alcCaptureStop(CaptureDevice);

    return true;
}

bool CDriverOpenALAudioCapture::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::start\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // Close capture device
    alcCaptureCloseDevice(CaptureDevice);

    // Context desactivation
    alcMakeContextCurrent(nullptr);

    // Context destruction
    alcDestroyContext(Context);

    // Close device
    alcCloseDevice(Device);

    delete [] m_sample;
    m_sample = nullptr;

    delete [] Samples;
    Samples = nullptr;

    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverOpenALAudioCapture::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::isConfigurable\n";

    return true;
}

bool CDriverOpenALAudioCapture::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverOpenALAudioCapture::start\n";

    // Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
    CConfigurationOpenALAudioCapture config(m_driverCtx,
                                            Directories::getDataDir() + "/applications/acquisition-server/interface-OpenALAudioCapture.ui");

    if (!config.configure(m_header)) { return false; }

    m_settings.save();

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif //TARGET_HAS_ThirdPartyOpenAL
