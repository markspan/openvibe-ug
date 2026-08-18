#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "ovasCDriverGTecGUSBampLegacy.h"
#include "ovasCConfigurationGTecGUSBampLegacy.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <cmath>

#include <windows.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <gUSBamp.h>

#define GTEC_NUM_CHANNELS 16

namespace OpenViBE {
namespace AcquisitionServer {

/*
    This driver always reads 17 channels: 16 + 1
    16 are EEG channels
    1 is the last channel that provides triggers from the parallel port of the GTEC
    Although 17 channels are read only "m_nAcquiredChannel" + 1 (if m_pTriggerInputEnabled==true) are displayed.
    If m_nAcquiredChannel=6 and m_pTriggerInputEnabled=true then the output in OpenVibe is 7 channels. If m_pTriggerInputEnabled=false then 6.
    "m_nAcquiredChannel" is a user modifiable variable with default value 16
*/

CDriverGTecGUSBampLegacy::CDriverGTecGUSBampLegacy(IDriverContext& ctx)
    : IDriver(ctx)
      , m_settings("AcquisitionServer_Driver_GTecGUSBampLegacy", m_driverCtx.getConfigurationManager())
      , m_deviceIdx(uint32_t(-1))
      , m_notchFilterIdx(-1)
      , m_bandPassFilterIdx(-1)
      , m_nAcquiredChannel(GTEC_NUM_CHANNELS)
{
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(GTEC_NUM_CHANNELS);

    m_settings.add("Header", &m_header);
    m_settings.add("DeviceIndex", &m_deviceIdx);
    m_settings.add("CommonGndAndRefBitmap", &m_commonGndAndRefBitmap);
    m_settings.add("NotchFilterIndex", &m_notchFilterIdx);
    m_settings.add("BandPassFilterIndex", &m_bandPassFilterIdx);
    m_settings.add("TriggerInputEnabled", &m_bTriggerInputEnabled);
    m_settings.load();
}

//___________________________________________________________________//
//                                                                   //
bool CDriverGTecGUSBampLegacy::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    if (m_bTriggerInputEnabled)
    {
        m_header.setChannelCount(m_nAcquiredChannel + 1);
        m_header.setChannelName(m_nAcquiredChannel, "CH_Event");
    }
    else { m_header.setChannelCount(m_nAcquiredChannel); }

    // If device has not been selected, try to find a device
    uint32_t i        = 0;
    m_actualDeviceIdx = m_deviceIdx;
    while (i < 11 && m_actualDeviceIdx == uint32_t(-1))
    {
        HANDLE handle = GT_OpenDevice(i);
        if (handle)
        {
            GT_CloseDevice(&handle);
            m_actualDeviceIdx = i;
        }
        i++;
    }

    if (m_actualDeviceIdx == uint32_t(-1)) { return false; }

    m_pEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_pEvent) { return false; }

    //allocate buffers
    m_bufferSize = (GTEC_NUM_CHANNELS + 1) * nSamplePerSentBlock * sizeof(float) + HEADER_SIZE;//+1 channel for trigger
    m_Buffer     = new uint8_t[m_bufferSize];

    if (m_bTriggerInputEnabled) { m_sample = new float[(m_nAcquiredChannel + 1) * nSamplePerSentBlock]; }
    else { m_sample = new float[m_nAcquiredChannel * nSamplePerSentBlock]; }

    if (!m_Buffer || !m_sample)
    {
        delete [] m_Buffer;
        delete [] m_sample;
        CloseHandle(m_pEvent);
        return false;
    }
    memset(m_Buffer, 0, m_bufferSize);
    m_sampleTranspose = reinterpret_cast<float*>(m_Buffer + HEADER_SIZE);
    m_pOverlapped     = new OVERLAPPED;

    if (!m_pOverlapped)
    {
        delete [] m_Buffer;
        delete [] m_sample;
        CloseHandle(m_pEvent);
        return false;
    }

#define m_pOverlapped ((OVERLAPPED*)m_pOverlapped)

    memset(m_pOverlapped, 0, sizeof(OVERLAPPED));
    m_pOverlapped->hEvent = m_pEvent;

    m_Device = GT_OpenDevice(m_actualDeviceIdx);
    if (!m_Device)
    {
        delete m_pOverlapped;
        delete [] m_Buffer;
        delete [] m_sample;
        CloseHandle(m_pEvent);
        return false;
    }

    m_actualImpedanceIdx  = 0;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_callback            = &callback;

    return true;
}

bool CDriverGTecGUSBampLegacy::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    UCHAR channels[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    // The amplifier is divided in 4 blocks, A to D
    // each one has its own Ref/gnd connections,
    // user can specify whether or not to connect the block to the common ground and reference of the amplifier.
    GND gnd;
    gnd.GND1 = (m_commonGndAndRefBitmap & 1);
    gnd.GND2 = (m_commonGndAndRefBitmap & (1 << 1));
    gnd.GND3 = (m_commonGndAndRefBitmap & (1 << 2));
    gnd.GND4 = (m_commonGndAndRefBitmap & (1 << 3));

    REF ref;
    ref.ref1 = (m_commonGndAndRefBitmap & (1 << 4));
    ref.ref2 = (m_commonGndAndRefBitmap & (1 << 5));
    ref.ref3 = (m_commonGndAndRefBitmap & (1 << 6));
    ref.ref4 = (m_commonGndAndRefBitmap & (1 << 7));

    if (!GT_SetMode(m_Device, M_NORMAL)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetMode\n"; }
    if (!GT_SetBufferSize(m_Device, m_nSamplePerSentBlock))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetBufferSize\n";
    }
    if (!GT_SetChannels(m_Device, channels, sizeof(channels) / sizeof(UCHAR)))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetChannels\n";
    }
    if (!GT_SetSlave(m_Device, FALSE)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetSlave\n"; }
    if (!GT_EnableTriggerLine(m_Device, TRUE))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_EnableTriggerLine - the extra input trigger channel is disabled\n";
    }
    // GT_EnableSC
    // GT_SetBipolar

    for (uint32_t i = 0; i < m_nAcquiredChannel; ++i)
    {
        if (!GT_SetBandPass(m_Device, i + 1, m_bandPassFilterIdx))
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetBandPass for channel " << i << "\n";
        }
        if (!GT_SetNotch(m_Device, i + 1, m_notchFilterIdx))
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetNotch for channel " << i << "\n";
        }
    }

    if (!GT_SetSampleRate(m_Device, m_header.getSamplingFrequency()))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetSampleRate\n";
    }

    if (!GT_SetReference(m_Device, ref)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetReference\n"; }
    if (!GT_SetGround(m_Device, gnd)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetGround\n"; }

    m_lastStimulation           = STIMULATION_0;
    m_totalHardwareStimulations = 0;
    m_totalDriverChunksLost     = 0;

    GT_Start(m_Device);

    return true;
}

bool CDriverGTecGUSBampLegacy::loop()
{
    CStimulationSet stimSet;

    if (!m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted())
    {
        if (GT_GetData(m_Device, m_Buffer, m_bufferSize, m_pOverlapped))
        {
            if (WaitForSingleObject(m_pOverlapped->hEvent, 1000) == WAIT_OBJECT_0)
            {
                DWORD nByte = 0;

                GetOverlappedResult(m_Device, m_pOverlapped, &nByte, FALSE);

                if (nByte == m_bufferSize)
                {
                    for (uint32_t i = 0; i < m_nAcquiredChannel; ++i)
                    {
                        for (uint32_t j = 0; j < m_nSamplePerSentBlock; ++j)
                        {
                            m_sample[i * m_nSamplePerSentBlock + j] = m_sampleTranspose[j * (GTEC_NUM_CHANNELS + 1) + i];
                        }
                    }

                    if (m_bTriggerInputEnabled)
                    {
                        for (uint32_t iSample = 0; iSample < m_nSamplePerSentBlock; ++iSample)
                        {
                            const uint32_t stimCode = uint32_t(
                                m_sampleTranspose[iSample * (GTEC_NUM_CHANNELS + 1) + GTEC_NUM_CHANNELS]);
                            m_sample[m_nAcquiredChannel * m_nSamplePerSentBlock + iSample] = float(stimCode);

                            //this means that the user sends 0 after each stimulatuion and in the beginning
                            if ((stimCode != STIMULATION_0) && (stimCode != m_lastStimulation)
                            )
                            {
                                uint64_t identifier;
                                switch (stimCode)
                                {
                                    case 1: identifier = OVTK_StimulationId_Label_01;
                                        break;
                                    case 2: identifier = OVTK_StimulationId_Label_02;
                                        break;
                                    case 3: identifier = OVTK_StimulationId_Label_03;
                                        break;
                                    case 4: identifier = OVTK_StimulationId_Label_04;
                                        break;
                                    case 5: identifier = OVTK_StimulationId_Label_05;
                                        break;
                                    case 6: identifier = OVTK_StimulationId_Label_06;
                                        break;
                                    case 7: identifier = OVTK_StimulationId_Label_07;
                                        break;
                                    case 8: identifier = OVTK_StimulationId_Label_08;
                                        break;
                                    case 9: identifier = OVTK_StimulationId_Label_09;
                                        break;
                                    case 10: identifier = OVTK_StimulationId_Label_10;
                                        break;
                                    case 11: identifier = OVTK_StimulationId_Label_11;
                                        break;
                                    case 12: identifier = OVTK_StimulationId_Label_12;
                                        break;
                                    case 13: identifier = OVTK_StimulationId_Label_13;
                                        break;
                                    case 14: identifier = OVTK_StimulationId_Label_14;
                                        break;
                                    case 15: identifier = OVTK_StimulationId_Label_15;
                                        break;
                                    default:
                                        identifier = OVTK_StimulationId_Label_FF;
                                        break;
                                }

                                const uint64_t time = CTime(m_header.getSamplingFrequency(), uint64_t(iSample)).time();
                                stimSet.push_back(identifier, time, 0);
                                m_totalHardwareStimulations++;
                            }

                            m_lastStimulation = stimCode;
                        }
                    }

                    m_callback->setSamples(m_sample);
                    m_callback->setStimulationSet(stimSet);
                    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
                }
                else
                {
                    m_totalDriverChunksLost++;
                    //m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "nByte and m_bufferSize differs : " << nByte << "/" << m_bufferSize << "(header size is " << HEADER_SIZE << ")\n";
                    /*m_driverCtx.getLogManager() << Kernel::LogLevel_Warning 
                    << "Returned data is different than expected. Total chunks lost: " << m_totalDriverChunksLost 
                    << ", Total samples lost: " << m_nSamplePerSentBlock * m_totalDriverChunksLost
                    << "\n";*/
                }
            }
            else
            {
                // TIMEOUT
                //m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "timeout 1\n";
            }
        }
        else
        {
            //m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "tError on GT_GetData.\n";
        }
    }
    else
    {
        if (m_driverCtx.isImpedanceCheckRequested())
        {
            double impedance = 0;
            GT_GetImpedance(m_Device, m_actualImpedanceIdx + 1, &impedance);
            if (impedance < 0) { impedance *= -1; }

            m_driverCtx.updateImpedance(m_actualImpedanceIdx, impedance);

            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel " << m_actualImpedanceIdx << " - " 
                    << m_header.getChannelName(m_actualImpedanceIdx) << " : " << impedance << "\n";

            m_actualImpedanceIdx++;
            m_actualImpedanceIdx %= m_header.getChannelCount();

            m_driverCtx.updateImpedance(m_actualImpedanceIdx, -1);
        }
        else { System::Time::sleep(20); }
    }

    return true;
}

bool CDriverGTecGUSBampLegacy::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    //stop device
    GT_Stop(m_Device);
    GT_ResetTransfer(m_Device);

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Total number of hardware stimulations acquired: " << m_totalHardwareStimulations << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Total chunks lost: " << m_totalDriverChunksLost << "\n";

    return true;
}

bool CDriverGTecGUSBampLegacy::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    GT_CloseDevice(&m_Device);
    CloseHandle(m_pEvent);
    delete [] m_Buffer;
    delete [] m_sample;
    delete m_pOverlapped;
    m_Device   = nullptr;
    m_pEvent   = nullptr;
    m_Buffer   = nullptr;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

bool CDriverGTecGUSBampLegacy::configure()
{
    CConfigurationGTecGUSBampLegacy config(Directories::getDataDir() + "/applications/acquisition-server/interface-GTec-GUSBampLegacy.ui",
                                           m_deviceIdx, m_commonGndAndRefBitmap, m_notchFilterIdx, m_bandPassFilterIdx,
                                           m_bTriggerInputEnabled);

    if (!config.configure(m_header)) { return false; }
    this->m_nAcquiredChannel = m_header.getChannelCount();
    m_settings.save();
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
