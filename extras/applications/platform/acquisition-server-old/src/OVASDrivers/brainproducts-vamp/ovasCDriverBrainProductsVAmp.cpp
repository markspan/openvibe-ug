#if defined TARGET_HAS_ThirdPartyUSBFirstAmpAPI

#include "ovasCDriverBrainProductsVAmp.h"
#include "ovasCConfigurationBrainProductsVAmp.h"
#include "ovasCHeaderBrainProductsVAmp.h"

#include <system/ovCTime.h>
#include <windows.h>
#include <FirstAmp.h>

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <vector>

namespace OpenViBE {
namespace AcquisitionServer {

#define OVAS_Driver_VAmp_ImpedanceMask_BaseComponent 127

namespace {
// Low pass FIR filters before downsampling
//
// Computed with python and scipy
// References :
// http://docs.scipy.org/doc/numpy/reference/generated/numpy.ndarray.tofile.html
// http://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.firwin.html
//
// The following filters have roughly 50ms delay as described in "2.1.4 What is the
// delay of a linear-phase FIR?" at http://www.dspguru.com/book/export/html/3 :
// n =   200, Fs =   2000, delay = (n-1)/(2*Fs) = 0.04975
// n =  2000, Fs =  20000, delay = (n-1)/(2*Fs) = 0.049975
//
// In order to correct this delay, filtering should be done as a two steps process with a forward filter
// followed by a backward filter. However, this leads to an n square complexity where a linear complexity is
// sufficient in forward only filtering (using 100kHz input signal, n=10000 taps !)
//
// To avoid such complexity, it is chosen to antedate acquired samples by 50ms cheating the drift correction
// process. Indeed, the acquisition server application monitors the drifting of the acquisition process and
// corrects this drift upon demand. It is up to the driver to require this correction and it can be chosen not
// to fit the 0 drift, but to fit an arbitrary fixed drift instead.
//
// The offset for this correction is stored in the m_nDriftOffsetSample variable

/* ---------------------------------------------------------------------------------------------------------------
from scipy import signal

N = 200
signal.firwin(N, cutoff= 256./(0.5*2000.), window='hamming').tofile("f64_2k_512.bin")
signal.firwin(N, cutoff= 128./(0.5*2000.), window='hamming').tofile("f64_2k_256.bin")
signal.firwin(N, cutoff=  64./(0.5*2000.), window='hamming').tofile("f64_2k_128.bin")

N = 2000
signal.firwin(N, cutoff=2048./(0.5*20000.), window='hamming').tofile("f64_20k_4096.bin")
signal.firwin(N, cutoff=1024./(0.5*20000.), window='hamming').tofile("f64_20k_2048.bin")
signal.firwin(N, cutoff= 512./(0.5*20000.), window='hamming').tofile("f64_20k_1024.bin")
signal.firwin(N, cutoff= 256./(0.5*20000.), window='hamming').tofile("f64_20k_512.bin")
signal.firwin(N, cutoff= 128./(0.5*20000.), window='hamming').tofile("f64_20k_256.bin")
signal.firwin(N, cutoff=  64./(0.5*20000.), window='hamming').tofile("f64_20k_128.bin")

--------------------------------------------------------------------------------------------------------------- */

bool loadFilter(const char* filename, std::vector<double>& filters)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        filters.clear();
        filters.push_back(1);
        return false;
    }
    fseek(file, 0, SEEK_END);
    const size_t len = ftell(file);
    fseek(file, 0, SEEK_SET);
    filters.resize(len / sizeof(double));
    fread(&filters[0], len, 1, file);
    fclose(file);
    return true;
}
}  // namespace


//___________________________________________________________________//
//                                                                   //

CDriverBrainProductsVAmp::CDriverBrainProductsVAmp(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_BrainProducts-VAmp", m_driverCtx.getConfigurationManager())
{
    // default mode is VAmp 16, aux and trigger depending on the config tokens
    m_header.setAcquisitionMode(VAmp16);

    m_nEEGChannel       = m_header.getEEGChannelCount(VAmp16);
    m_nAuxiliaryChannel = (m_acquireAuxiliaryAsEEG ? m_header.getAuxiliaryChannelCount(VAmp16) : 0);
    m_nTriggerChannel   = (m_acquireTriggerAsEEG ? m_header.getTriggerChannelCount(VAmp16) : 0);
    m_header.setChannelCount(m_nEEGChannel + m_nAuxiliaryChannel + m_nTriggerChannel);

    if (m_acquireAuxiliaryAsEEG)
    {
        m_header.setChannelName(m_nEEGChannel, "Aux 1");
        m_header.setChannelName(m_nEEGChannel + 1, "Aux 2");
    }
    if (m_acquireTriggerAsEEG) { m_header.setChannelName(m_nEEGChannel + m_nAuxiliaryChannel, "Trigger line"); }

    m_header.setSamplingFrequency(512);

    t_faDataModeSettings settings;
    settings.Mode20kHz4Channels.ChannelsPos[0] = 7;
    settings.Mode20kHz4Channels.ChannelsNeg[0] = -1;
    settings.Mode20kHz4Channels.ChannelsPos[1] = 8;
    settings.Mode20kHz4Channels.ChannelsNeg[1] = -1;
    settings.Mode20kHz4Channels.ChannelsPos[2] = 9;
    settings.Mode20kHz4Channels.ChannelsNeg[2] = -1;
    settings.Mode20kHz4Channels.ChannelsPos[3] = 10;
    settings.Mode20kHz4Channels.ChannelsNeg[3] = -1;

    m_header.setFastModeSettings(settings);
    m_header.setDeviceId(FA_ID_INVALID);

    // @note m_header is CHeaderBrainProductsVAmp, whereas the current interface supports only IHeader. Thus, some info may not be loaded/saved.
    m_settings.add("Header", &m_header);
    m_settings.add("AcquireAuxiliaryAsEEG", &m_acquireAuxiliaryAsEEG);
    m_settings.add("AcquireTriggerAsEEG", &m_acquireTriggerAsEEG);
    m_settings.load();
}

CDriverBrainProductsVAmp::~CDriverBrainProductsVAmp() {}

const char* CDriverBrainProductsVAmp::getName() { return "Brain Products V-Amp / First-Amp"; }

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsVAmp::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "INIT called.\n";
    if (m_driverCtx.isConnected())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] VAmp Driver: Driver already initialized.\n";
        return false;
    }

    if (m_acquireAuxiliaryAsEEG) m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] VAmp Driver: will acquire aux as EEG\n";
    else m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] VAmp Driver: will NOT acquire aux as EEG\n";
    if (m_acquireTriggerAsEEG) m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] VAmp Driver: will acquire trigger as EEG\n";
    else m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] VAmp Driver: will NOT acquire trigger as EEG\n";

    m_acquisitionMode   = m_header.getAcquisitionMode();
    m_nEEGChannel       = m_header.getEEGChannelCount(m_acquisitionMode);
    m_nAuxiliaryChannel = (m_acquireAuxiliaryAsEEG ? m_header.getAuxiliaryChannelCount(m_acquisitionMode) : 0);
    m_nTriggerChannel   = (m_acquireTriggerAsEEG ? m_header.getTriggerChannelCount(m_acquisitionMode) : 0);

    m_header.setChannelCount(m_nEEGChannel + m_nAuxiliaryChannel + m_nTriggerChannel);

    if (m_acquireAuxiliaryAsEEG)
    {
        if (strlen(m_header.getChannelName(m_nEEGChannel)) == 0 || strcmp(m_header.getChannelName(m_nEEGChannel), "Trigger line") == 0)
        {
            m_header.setChannelName(m_nEEGChannel, "Aux 1");
        }
        if (strlen(m_header.getChannelName(m_nEEGChannel + 1)) == 0) { m_header.setChannelName(m_nEEGChannel + 1, "Aux 2"); }
    }
    if (m_acquireTriggerAsEEG) { m_header.setChannelName(m_nEEGChannel + m_nAuxiliaryChannel, "Trigger line"); }

    if (m_header.getChannelCount() == 0 || m_header.getSamplingFrequency() == 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] VAmp Driver: Channel count or frequency not set.\n";
        return false;
    }

    // Builds up physical sampling rate
    switch (m_acquisitionMode)
    {
        case VAmp16: m_physicalSamplingHz = 2000;
            break;
        case VAmp8: m_physicalSamplingHz = 2000;
            break;
        case VAmp4Fast: m_physicalSamplingHz = 20000;
            break;
        default: m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Vamp Driver: Unsupported acquisition mode [" << m_acquisitionMode << "].\n";
            return false;
    }

    // Loading low pass filter for decimation
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] Vamp Driver: Setting up the FIR filter for signal decimation (physical rate " <<
            m_physicalSamplingHz << " > driver rate " << m_header.getSamplingFrequency() << ").\n";
    switch (m_physicalSamplingHz)
    {
        case 2000: loadFilter(Directories::getDataDir() + "applications/acquisition-server/filters/f64_2k_512.bin", m_filters);
            break;
        case 20000: loadFilter(Directories::getDataDir() + "applications/acquisition-server/filters/f64_20k_512.bin", m_filters);
            break;
        default: m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] Vamp Driver: Unsupported physical sampling rate [" << m_physicalSamplingHz << "].\n";
            return false;
    }

    // Builds up a buffer to store acquired samples. This buffer will be sent to the acquisition server later.
    m_samples.clear();
    m_samples.resize(m_header.getChannelCount());
    m_resolutions.clear();
    m_resolutions.resize(m_header.getChannelCount());

    // Prepares cache for filtering
    m_sampleCaches.clear();
    for (size_t i = 0; i < m_filters.size(); ++i) { m_sampleCaches.push_back(m_samples); }

    // Setting the inner latency as described in the beginning of the file
    m_nDriftOffsetSample = int64_t(m_header.getSamplingFrequency() * 50) / 1000;
    m_driverCtx.setInnerLatencySampleCount(-m_nDriftOffsetSample);
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Driver inner latency set to 50ms to compensate FIR filtering.\n";

    // Prepares downsampling
    m_counter     = 0;
    m_counterStep = (uint64_t(m_header.getSamplingFrequency()) << 32) / m_physicalSamplingHz;

    // Gets device Id
    int deviceId = m_header.getDeviceId();

    //__________________________________
    // Hardware initialization

    // if no device selected with the properties dialog
    // we take the last device connected
    if (deviceId == FA_ID_INVALID)
    {
        // We try to get the last opened device,
        uint32_t lastOpenedDeviceID = faGetCount(); // Get the last opened Device id.

        if (lastOpenedDeviceID == FA_ID_INVALID) // failed
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] VAmp Driver: faGetCount failed to get last opened device.\n";
            return false;
        }

        deviceId = faGetId(lastOpenedDeviceID - 1);
        m_header.setDeviceId(deviceId);
    }

    if (deviceId != FA_ID_INVALID)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "[INIT] VAmp Driver: Active device ID(" << m_header.getDeviceId() << ").\n";
    }
    else
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] VAmp Driver: No device connected !\n";
        return false;
    }

    // Open the device.
    int openReturn = faOpen(deviceId);
    if (openReturn != FA_ERR_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[INIT] VAmp Driver: faOpen(" << deviceId << ") FAILED(" << openReturn << ").\n";
        return false;
    }

    if (m_acquisitionMode == VAmp4Fast) { faSetDataMode(deviceId, dm20kHz4Channels, &(m_header.getFastModeSettings())); }
    else { faSetDataMode(deviceId, dmNormal, nullptr); }

    if (m_driverCtx.isImpedanceCheckRequested())
    {
        faStart(deviceId);
        faStartImpedance(deviceId);
    }

    if (!m_driverCtx.isImpedanceCheckRequested())
    {
        HBITMAP bitmap = HBITMAP(LoadImage(nullptr, Directories::getDataDir() + "/applications/acquisition-server/vamp-standby.bmp",IMAGE_BITMAP, 0, 0,
                                           LR_LOADFROMFILE));
        if (bitmap == nullptr || faSetBitmap(m_header.getDeviceId(), bitmap) != FA_ERR_OK)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "[LOOP] VAmp Driver: BMP load failed.\n";
        }
    }


    // Gets properties
    t_faProperty properties;
    if (faGetProperty(m_header.getDeviceId(), &properties))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not get properties - Got error \n";
        return false;
    }

    size_t j = 0;
    for (size_t i = 0; i < m_nEEGChannel; i++, j++) m_resolutions[j] = m_header.getChannelGain(i) * properties.ResolutionEeg * 1E6f; // converts to uV
    for (size_t i = 0; i < m_nAuxiliaryChannel; i++, j++) m_resolutions[j] = properties.ResolutionAux * 1E6f; // converts to uV

    for (size_t i = 0; i < m_nEEGChannel; i++, j++) { m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }
    for (size_t i = 0; i < m_nAuxiliaryChannel; i++, j++)
    {
        m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); // converts to uV
    }

    //__________________________________
    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    return true;
}

bool CDriverBrainProductsVAmp::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "START called.\n";
    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted()) { return false; }

    m_firstStart   = true;
    uint32_t error = FA_ERR_OK;
    int deviceId   = m_header.getDeviceId();

    if (m_driverCtx.isImpedanceCheckRequested())
    {
        faStopImpedance(deviceId);
        // stops the impedance mode, but not the acquisition
    }
    else
    {
        // we did not start the acquisition yet, let's do it now.
        error = faStart(deviceId);
    }

    if (error != FA_ERR_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[START] VAmp Driver: faStart FAILED(" << error << "). Closing device.\n";
        faClose(deviceId);
        return false;
    }

    //The bonus...
    HBITMAP bitmap = HBITMAP(LoadImage(nullptr, Directories::getDataDir() + "/applications/acquisition-server/vamp-acquiring.bmp",IMAGE_BITMAP, 0, 0,
                                       LR_LOADFROMFILE));
    if (bitmap == nullptr || faSetBitmap(m_header.getDeviceId(), bitmap) != FA_ERR_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "[START] VAmp Driver: BMP load failed.\n";
    }

    return true;
}

bool CDriverBrainProductsVAmp::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }

    t_faDataModel16 bufferVAmp16; // buffer for the next block in normal mode
    uint32_t lengthVAmp16 = sizeof(t_faDataModel16);

    t_faDataModel8 bufferVAmp8; // buffer for the next block in normal mode
    uint32_t lengthVAmp8 = sizeof(t_faDataModel8);

    t_faDataFormatMode20kHz bufferVamp4Fast; // buffer for fast mode acquisition
    uint32_t lengthVAmp4Fast = sizeof(t_faDataFormatMode20kHz);

    uint32_t status = 0;

    int deviceId = m_header.getDeviceId();

    if (m_driverCtx.isStarted())
    {
        uint32_t nReceivedSamples = 0;
#if DEBUG
        uint32_t nReadError = 0;
        uint32_t nReadSuccess = 0;
        uint32_t nReadZero = 0;
#endif
        if (m_firstStart)
        {
            //empty buffer
            switch (m_acquisitionMode)
            {
                case VAmp16: while (faGetData(deviceId, &bufferVAmp16, lengthVAmp16) > 0);
                    status = bufferVAmp16.Status;
                    break;
                case VAmp8: while (faGetData(deviceId, &bufferVAmp8, lengthVAmp8) > 0);
                    status = bufferVAmp8.Status;
                    break;
                case VAmp4Fast: while (faGetData(deviceId, &bufferVamp4Fast, lengthVAmp4Fast) > 0);
                    status = bufferVamp4Fast.Status;
                    break;
                default: break;
            }
            // Trigger: Digital inputs (bits 0 - 8) + output (bit 9) state + 22 MSB reserved bits
            // The trigger value received is in [0-255]
            status &= 0x000000ff;
            m_lastTrigger = status;
            m_firstStart  = false;
        }

        bool finished = false;
        while (!finished)
        {
            // we need to "getData" with the right output structure according to acquisition mode

            int returnLength;
            signed int* eegArray;
            signed int* auxiliaryArray = nullptr;
            status                     = 0;
            switch (m_acquisitionMode)
            {
                case VAmp16: returnLength = faGetData(deviceId, &bufferVAmp16, lengthVAmp16);
                    eegArray       = bufferVAmp16.Main;
                    auxiliaryArray = bufferVAmp16.Aux;
                    status         = bufferVAmp16.Status;
                    break;

                case VAmp8: returnLength = faGetData(deviceId, &bufferVAmp8, lengthVAmp8);
                    eegArray       = bufferVAmp8.Main;
                    auxiliaryArray = bufferVAmp8.Aux;
                    status         = bufferVAmp8.Status;
                    break;

                case VAmp4Fast: returnLength = faGetData(deviceId, &bufferVamp4Fast, lengthVAmp4Fast);
                    eegArray = bufferVamp4Fast.Main;
                    // auxiliaryArray = bufferVamp4Fast.Aux;
                    status = bufferVamp4Fast.Status;
                    break;

                default: m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "[LOOP] VAmp Driver: unsupported acquisition mode, this should never happen\n";
                    return false;
            }
            // Trigger: Digital inputs (bits 0 - 8) + output (bit 9) state + 22 MSB reserved bits
            // The trigger value received is in [0-255]
            status &= 0x000000ff;

            if (returnLength > 0)
            {
#if DEBUG
                nReadSuccess++;
#endif

                // Stores acquired sample
                for (size_t i = 0; i < m_nEEGChannel; ++i) { m_samples[i] = float((eegArray[i] - eegArray[m_nEEGChannel]) * m_resolutions[i]); }
                for (size_t i = 0; i < m_nAuxiliaryChannel; ++i) { m_samples[m_nEEGChannel + i] = float(auxiliaryArray[i] * m_resolutions[i]); }
                for (size_t i = 0; i < m_nTriggerChannel; ++i) { m_samples[m_nEEGChannel + m_nAuxiliaryChannel + i] = float(status); }

                // Updates cache
                //m_sampleCaches.erase(m_sampleCaches.begin());
                m_sampleCaches.pop_front();
                m_sampleCaches.push_back(m_samples);

                // Every time that a trigger has changed, we send a stimulation.
                if (status != m_lastTrigger)
                {
                    // The date is relative to the last buffer start time (cf the setSamples before setStimulationSet)
                    const uint64_t date = CTime(m_physicalSamplingHz, m_nTotalSample).time();
                    // Code of stimulation = OVTK_StimulationId_LabelStart + value of the trigger bytes.
                    m_stimSet.push_back(OVTK_StimulationId_Label(status), 0, 0);
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "[LOOP] VAmp Driver: Send stimulation: " << status << " at date: " << date << ".\n";
                    m_lastTrigger = status;
                }

                m_counter += m_counterStep;
                if (m_counter >= (1LL << 32))
                {
                    m_counter -= (1LL << 32);

                    // Filters last samples
                    for (size_t i = 0; i < m_samples.size(); ++i)
                    {
                        m_samples[i] = 0;
                        auto it      = m_sampleCaches.begin();
                        for (size_t j = 0; j < m_filters.size(); ++j)
                        {
                            //m_samples[i]+=m_filters[j]*m_sampleCaches[j][i];
                            m_samples[i] += float(m_filters[j] * (*it)[i]);
                            ++it;
                        }
                    }

                    m_callback->setSamples(&m_samples[0], 1);
                    m_callback->setStimulationSet(m_stimSet);
                    m_stimSet.clear();
                }
                nReceivedSamples++;
            }
            else if (returnLength == 0)
            {
                finished = true;
                System::Time::sleep(2);
            }
#if DEBUG
            if(returnLength < 0)
            {
                nReadError++;
                finished = true;
            }
            if(returnLength == 0)
            {
                nReadZero++;
                finished = true;
            }
#endif
        }
#if DEBUG
        m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "[LOOP] VAmp Driver: stats for the current block : Success="<<l_uint32ReadSuccessCount<<" Error="<<l_uint32ReadErrorCount<<" Zero="<<nReadZero<<"\n";
#endif
        //____________________________

        // As described in at the begining of this file, the drift
        // correction process is altered to consider the ~ 50ms delay
        // of the single way filtering process
        // Inner latency was set accordingly.
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    else
    {
        // we drop any data in internal buffer
        switch (m_acquisitionMode)
        {
            case VAmp16: while (faGetData(deviceId, &bufferVAmp16, lengthVAmp16) > 0);
                break;
            case VAmp8: while (faGetData(deviceId, &bufferVAmp8, lengthVAmp8) > 0);
                break;
            case VAmp4Fast: while (faGetData(deviceId, &bufferVamp4Fast, lengthVAmp4Fast) > 0);
                break;
            default: break;
        }

        if (m_driverCtx.isImpedanceCheckRequested())
        {
            // Reads impedances
            std::vector<uint32_t> buffer;
            buffer.resize(20, 0); // all possible channels + ground electrode
            uint32_t error = faGetImpedance(deviceId, &buffer[0], 20 * sizeof(uint32_t));
            if (error != FA_ERR_OK)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[LOOP] Can not read impedances on device id " << deviceId << " - faGetImpedance FAILED(" <<
                        error << ")\n";
            }
            else
            {
                // Updates impedances

                const uint64_t goodImpedanceLimit = m_driverCtx.getConfigurationManager().expandAsUInteger("${AcquisitionServer_DefaultImpedanceLimit}", 5000);
                // as with the default acticap settings (values provided by Brain Products) :
                const uint64_t badImpedanceLimit = 2 * goodImpedanceLimit;

                m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Impedances are [ ";
                for (uint32_t j = 0; j < m_nEEGChannel; ++j)//we do not update the last impedance (ground)
                {
                    m_driverCtx.updateImpedance(j, buffer[j]);
                    m_driverCtx.getLogManager() << buffer[j] << " ";
                }
                m_driverCtx.getLogManager() << "]\n";

                //print impedances on the LCD screen
                HBITMAP bitmapHandler = HBITMAP(LoadImage(nullptr, Directories::getDataDir() + "/applications/acquisition-server/vamp-impedance-mask.bmp",
                                                          IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
                if (bitmapHandler)
                {
                    BITMAP bitmap;
                    GetObject(bitmapHandler, sizeof(bitmap), &bitmap);
                    //uint32_t width        = bitmap.bmWidth;
                    const uint32_t height = bitmap.bmHeight;
                    BYTE* bytePtr         = static_cast<BYTE*>(bitmap.bmBits); // 3 bytes per pixel for RGB values

                    //printf("H x W : %u x %u with %u per scan\n", height, width, bitmap.bmWidthBytes);

                    for (uint32_t y = 0; y < height; ++y) // scan lines
                    {
                        uint32_t x = 0;
                        for (uint32_t byteIdx = 0; byteIdx < uint32_t(bitmap.bmWidthBytes); byteIdx += 3)
                        {
                            x++;
                            unsigned char* b = (bytePtr + bitmap.bmWidthBytes * y + byteIdx);
                            unsigned char* g = (bytePtr + bitmap.bmWidthBytes * y + byteIdx + 1);
                            unsigned char* r = (bytePtr + bitmap.bmWidthBytes * y + byteIdx + 2);
                            // The impedance mask is a BLACK and WHITE bitmap with grey squares for the channels.
                            // For each channel, the grey RGB components are equal to OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - channel index - 1
                            // e.g. with 16 channels and base at 127, the 5th channel square as RGB components to 127 - 5 = 122
                            // Every pixel in the grey range is a channel square and needs repaint according to impedance.
                            if (*r < OVAS_Driver_VAmp_ImpedanceMask_BaseComponent && *r >= OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - m_nEEGChannel
                                && *g < OVAS_Driver_VAmp_ImpedanceMask_BaseComponent && *g >= OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - m_nEEGChannel
                                && *b < OVAS_Driver_VAmp_ImpedanceMask_BaseComponent && *b >= OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - m_nEEGChannel)
                            {
                                const uint32_t idx = OVAS_Driver_VAmp_ImpedanceMask_BaseComponent - *r - 1; //0-based
                                if (buffer[idx] <= goodImpedanceLimit)
                                {
                                    // good impedance: green
                                    *r = 0x00;
                                    *g = 0xff;
                                    *b = 0x00;
                                }
                                else if (buffer[idx] <= badImpedanceLimit)
                                {
                                    // unsufficient impedance: yellow
                                    *r = 0xff;
                                    *g = 0xff;
                                    *b = 0x00;
                                }
                                else
                                {
                                    // bad impedance: red
                                    *r = 0xff;
                                    *g = 0x00;
                                    *b = 0x00;
                                }
                            }
                        }
                    }
                }

                if (bitmapHandler == nullptr || faSetBitmap(m_header.getDeviceId(), bitmapHandler) != FA_ERR_OK)
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "[LOOP] VAmp Driver: BMP load failed.\n";
                }
            }
        }
    }

    return true;
}

bool CDriverBrainProductsVAmp::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "STOP called.\n";
    if (!m_driverCtx.isConnected()) { return false; }

    if (!m_driverCtx.isStarted()) { return false; }

    if (m_driverCtx.isImpedanceCheckRequested()) { faStartImpedance(m_header.getDeviceId()); }

    m_firstStart = false;
    if (!m_driverCtx.isImpedanceCheckRequested())
    {
        HBITMAP bitmap = (HBITMAP)LoadImage(nullptr, Directories::getDataDir() + "/applications/acquisition-server/vamp-standby.bmp",IMAGE_BITMAP, 0, 0,
                                            LR_LOADFROMFILE);
        if (bitmap == nullptr || faSetBitmap(m_header.getDeviceId(), bitmap) != FA_ERR_OK)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "[STOP] VAmp Driver: BMP load failed.\n";
        }
    }

    return true;
}

bool CDriverBrainProductsVAmp::uninitialize()
{
    if (!m_driverCtx.isConnected()) { return false; }

    if (m_driverCtx.isStarted()) { return false; }

    uint32_t error = faStop(m_header.getDeviceId());
    if (error != FA_ERR_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[UINIT] VAmp Driver: faStop FAILED(" << error << ").\n";
        faClose(m_header.getDeviceId());
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Uninitialize called. Closing the device.\n";


    // for a black bitmap use :
    //HDC hDC = CreateCompatibleDC(NULL);
    //HBITMAP bitmap = CreateCompatibleBitmap(l_hDC, 320, 240);
    // Default bitmap : BP image (provided by N. Soldati)
    HBITMAP bitmap = HBITMAP(LoadImage(nullptr, Directories::getDataDir() + "/applications/acquisition-server/vamp-default.bmp",IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
    if (faSetBitmap(m_header.getDeviceId(), bitmap) != FA_ERR_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "[UINIT] VAmp Driver: BMP load failed.\n";
    }


    error = faClose(m_header.getDeviceId());
    if (error != FA_ERR_OK)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "[UINIT] VAmp Driver: faClose FAILED(" << error << ").\n";
        return false;
    }

    m_callback = nullptr;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainProductsVAmp::configure()
{
    // the specific header is passed into the specific configuration
    CConfigurationBrainProductsVAmp config(m_driverCtx, Directories::getDataDir() + "/applications/acquisition-server/interface-BrainProducts-VAmp.ui",
                                           &m_header, m_acquireAuxiliaryAsEEG, m_acquireTriggerAsEEG);

    if (!config.configure(*(m_header.getBasicHeader()))) { return false; }  // the basic configure will use the basic header 

    m_settings.save();

    if (m_acquisitionMode == VAmp4Fast)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Pair names :\n";
        for (uint32_t i = 0; i < m_header.getPairCount(); ++i)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "  Pair " << i << " > " << m_header.getPairName(i) << "\n";
        }
    }

    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyUSBFirstAmpAPI
