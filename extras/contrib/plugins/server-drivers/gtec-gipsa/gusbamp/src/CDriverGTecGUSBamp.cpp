#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

/*
 *
 * Notes: According to gtec C API V3.12.00, the output from this driver
 * should be microvolts, except when giving out a calibration signal.
 *
 * The auto calibration should be run before actual acquisition to ensure 
 * that all the channels are scaled appropriately.
 *
 * @todo might be better if all event channels were the last channels together
 *
 */

#include "CDriverGTecGUSBamp.hpp"
#include "CConfigurationGTecGUSBamp.hpp"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <cmath>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <limits>

#include <mutex>
#include <thread>
#include <functional>

#include <gUSBamp.h>

namespace OpenViBE {
namespace AcquisitionServer {

#if defined(TARGET_OS_Windows)
#pragma warning(disable: 4800) // disable "forcing value to bool 'true' or 'false' (performance warning)" nag coming from BOOL->bool cast on e.g. VS2010
#endif

const DWORD CDriverGTecGUSBamp::BUFFER_SIZE_BYTES = HEADER_SIZE + N_POINTS * sizeof(float);

/*
    This driver always reads 17 channels: 16 + 1
    16 are EEG channels
    1 is the last channel that provides triggers from the parallel port of the GTEC
    Although 17 channels are read only "m_nAcquiredChannel" + 1 (if m_pTriggerInputEnabled==true) are displayed.
    If m_nAcquiredChannel=6 and m_pTriggerInputEnabled=true then the output in OpenVibe is 7 channels. If m_pTriggerInputEnabled=false then 6.
    "m_nAcquiredChannel" is a user modifiable variable with default value 16
*/

CDriverGTecGUSBamp::CDriverGTecGUSBamp(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_GTecGUSBamp", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(0);

    // Settings to be saved to .conf MUST be registered here
    m_settings.add("Header", &m_header);
    m_settings.add("CommonGndAndRefBitmap", &m_commonGndAndRefBitmap);
    m_settings.add("NotchFilterIndex", &m_notchFilterIdx);
    m_settings.add("BandPassFilterIndex", &m_bandPassFilterIdx);
    m_settings.add("TriggerInputEnabled", &m_triggerInputEnabled);
    m_settings.add("MasterSerial", &m_masterSerial);
    m_settings.add("Bipolar", &m_bipolarEnabled);
    m_settings.add("CalibrationSignal", &m_calibrationSignalEnabled);
    m_settings.add("ShowDeviceName", &m_showDeviceName);

    m_settings.load();

    m_nAcquiredChannel = m_header.getChannelCount();
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGTecGUSBamp::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected() || !m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    detectDevices();

    if (numDevices() == 0) { return false; }

    // See if we can find the selected master serial from the currently available devices
    bool found = false;
    for (uint32_t i = 0; i < numDevices(); ++i)
    {
        if (m_devices[i].serial == m_masterSerial)
        {
            found = true;
            break;
        }
    }
    if (!found) { m_masterSerial = ""; }

    //assign automatically the last device as master if no master has been selected from "Device properties" before that
    if (numDevices() > 1 && m_masterSerial == "") { m_masterSerial = m_devices[numDevices() - 1].serial; }  //serial

    // Already printed by detectDevices() ...
    // m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Number of devices: " << numDevices() << "\n";

    if (numDevices() > 1 && m_nAcquiredChannel != numDevices() * GTEC_NUM_CHANNELS)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "You have selected " << m_nAcquiredChannel << " channels in a "
                << numDevices() * GTEC_NUM_CHANNELS << " channel setup. If this is intentional, please ignore this warning.\n";
    }

    m_globalImpedanceIdx  = 0;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_callback            = &callback;

    //create the temporary data buffers (the device will write data into those)
    m_validPoints        = NUMBER_OF_SCANS * (GTEC_NUM_CHANNELS + 1) * int(numDevices());
    m_buffers            = new BYTE**[numDevices()];
    m_overlapped         = new OVERLAPPED*[numDevices()];
    m_sample             = new float[N_POINTS * numDevices()]; //needed later when data is being acquired
    m_bufferReceivedData = new float[m_validPoints];

    m_ringBuffer.Initialize(BUFFER_SIZE_SECONDS * m_header.getSamplingFrequency() * (GTEC_NUM_CHANNELS + 1) * numDevices());

    for (uint32_t i = 0; i < numDevices(); ++i)
    {
        //Configure each device
        configureDevice(i);
    }

    //Set Master and slaves
    if (numDevices() > 1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Multiamplifier mode enabled. Sync cable should be used between the amps.\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Please configure the sync cable according to the above master/slave configuration.\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "  Alternatively, set your master from \"Driver properties\".\n";
        setMasterDevice(m_masterSerial);
    }

    else if (numDevices() == 1) //a single device must be Master
    {
        if (GT_SetSlave(m_devices[0].handle, false))
        {
            m_masterSerial = m_devices[0].serial;

            m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Configured as MASTER device: " << m_masterSerial << " \n";
            m_mastersCnt++;
        }
        else { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetSlave\n"; }
    }

    // Set channel units
    for (size_t c = 0; c < m_header.getChannelCount(); ++c)
    {
        if (!m_calibrationSignalEnabled) { m_header.setChannelUnits(c, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }
        else
        {
            // For calibration, the outputs are before-scaling raw values from the unit
            // following formula 'normal output = (raw-offset)*factor'. In normal use, 
            // offset and factor are obtained and set by the calibration procedure.
            m_header.setChannelUnits(c, OVTK_UNIT_Unspecified, OVTK_FACTOR_Base);
        }
    }

    // Make the channel map that converts from global system level indexes to gtec and openvibe indexes
    // This mapping is used to handle situations where the user toggles event channels on/off, changes number of channels, etc.
    const uint32_t totalChannels = (GTEC_NUM_CHANNELS + 1) * numDevices();
    m_channels.clear();
    m_channels.resize(totalChannels);
    for (uint32_t i = 0, signalChannels = 0, eventChannels = 0; i < totalChannels; ++i)
    {
        const uint32_t numChannelsPerDevice = (GTEC_NUM_CHANNELS + 1);
        const uint32_t deviceIdx            = i / numChannelsPerDevice;
        const uint32_t channelIdx           = i % numChannelsPerDevice;

        m_channels[i].gtecDeviceIdx  = deviceIdx;
        m_channels[i].gtecChannelIdx = channelIdx;

        if ((i + 1) % (GTEC_NUM_CHANNELS + 1) == 0)
        {
            // This is the digital IO or event channel
            m_channels[i].idx            = int(m_triggerInputEnabled ? (signalChannels + eventChannels) : -1);
            m_channels[i].oldIdx         = -1;
            m_channels[i].isEventChannel = true;
            if (m_triggerInputEnabled) { eventChannels++; }
        }
        else if (signalChannels < m_nAcquiredChannel)
        {
            // This is a normal signal channel
            m_channels[i].idx            = int(signalChannels + eventChannels);
            m_channels[i].oldIdx         = signalChannels;
            m_channels[i].isEventChannel = false;
            signalChannels++;
        }
        else
        {
            // Unused channel
            m_channels[i].idx            = -1;
            m_channels[i].oldIdx         = -1;
            m_channels[i].isEventChannel = false;
        }
        //std::cout << "cm: " << i << " -> " << m_channels[i].idx << ", old=" << m_channels[i].oldIdx << ", event=" << m_channels[i].l_bIsEventChannel << "\n";
    }

    // Take into account user might (or might not) want event channels + add amp name if requested
    remapChannelNames();

    if (m_header.getSamplingFrequency() > 4800)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning <<
                "Sampling rates >4800 may need specific steps to work properly. See gusbamp manuals provided by gtec.\n";
    }

    return true;
}

void CDriverGTecGUSBamp::detectDevices()
{
    m_devices.clear();
    char serial[16];
    int i = 0;
    while (i < 11)
    {
        HANDLE handle = GT_OpenDevice(i);

        if (handle)
        {
            GDevice device;

            device.handle = handle;

            GT_GetSerial(handle, serial, 16);
            //m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Detected device with serial: " << serial << "\n";
            device.serial = serial;

            m_devices.push_back(device);
        }
        i++;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Number of devices: " << m_devices.size() << ". Device order:\n";
    for (size_t j = 0; j < m_devices.size(); ++j)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Device: " << j << " serial: " << m_devices[j].serial << "\n";
    }
}

bool CDriverGTecGUSBamp::configureDevice(const uint32_t deviceNumber)
{
    HANDLE device            = m_devices[deviceNumber].handle;
    const std::string serial = m_devices[deviceNumber].serial;

    UCHAR channel[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

    // The amplifier is divided in 4 blocks, A to D
    // each one has its own Ref/gnd connections,
    // user can specify whether or not to connect the block to the common ground and reference of the amplifier.
    GND ground;
    ground.GND1 = (m_commonGndAndRefBitmap & 1);
    ground.GND2 = (m_commonGndAndRefBitmap & (1 << 1));
    ground.GND3 = (m_commonGndAndRefBitmap & (1 << 2));
    ground.GND4 = (m_commonGndAndRefBitmap & (1 << 3));

    REF reference;
    reference.ref1 = (m_commonGndAndRefBitmap & (1 << 4));
    reference.ref2 = (m_commonGndAndRefBitmap & (1 << 5));
    reference.ref3 = (m_commonGndAndRefBitmap & (1 << 6));
    reference.ref4 = (m_commonGndAndRefBitmap & (1 << 7));

    uint32_t mode = M_NORMAL;
    if (m_calibrationSignalEnabled) { mode = M_CALIBRATE; }
    if (!GT_SetMode(device, mode)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetMode for mode " << mode << "\n"; }
    if (!GT_SetBufferSize(device, NUMBER_OF_SCANS)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetBufferSize\n"; }
    if (!GT_SetChannels(device, channel, sizeof(channel) / sizeof(UCHAR)))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetChannels\n";
    }
    if (!GT_EnableTriggerLine(device, TRUE))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_EnableTriggerLine - the extra input trigger channel is disabled\n";
    }
    // GT_EnableSC

    // GT_SetBipolar
    CHANNEL settings;
    if (this->m_bipolarEnabled)
    {
        //the following configurations produces 8 bipolar: 1,3,5,7,9,11,13,15 and 8 unipolar 2,4,6,8,10,12,14,16 

        settings.Channel1  = 2;
        settings.Channel2  = 0;
        settings.Channel3  = 4;
        settings.Channel4  = 0;
        settings.Channel5  = 6;
        settings.Channel6  = 0;
        settings.Channel7  = 8;
        settings.Channel8  = 0;
        settings.Channel9  = 10;
        settings.Channel10 = 0;
        settings.Channel11 = 12;
        settings.Channel12 = 0;
        settings.Channel13 = 14;
        settings.Channel14 = 0;
        settings.Channel15 = 16;
        settings.Channel16 = 0;
    }
    else
    {
        settings.Channel1  = 0;
        settings.Channel2  = 0;
        settings.Channel3  = 0;
        settings.Channel4  = 0;
        settings.Channel5  = 0;
        settings.Channel6  = 0;
        settings.Channel7  = 0;
        settings.Channel8  = 0;
        settings.Channel9  = 0;
        settings.Channel10 = 0;
        settings.Channel11 = 0;
        settings.Channel12 = 0;
        settings.Channel13 = 0;
        settings.Channel14 = 0;
        settings.Channel15 = 0;
        settings.Channel16 = 0;
    }
    if (!GT_SetBipolar(device, settings))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error on GT_SetBipolar: Couldn't set unipolar derivation for device " << serial;
    }
    else if (this->m_bipolarEnabled) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Bipolar configuration is active.\n"; }


    configFiltering(device);

    if (!GT_SetSampleRate(device, m_header.getSamplingFrequency()))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetSampleRate\n";
    }

    if (mode == M_NORMAL)
    {
        if (!GT_SetReference(device, reference)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetReference\n"; }
        if (!GT_SetGround(device, ground)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetGround\n"; }
    }

    return true;
}

bool CDriverGTecGUSBamp::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    if (m_reconfigurationRequired)
    {
        // Impedance checking or some other GT_ call has changed the device configuration, so we need to reconf
        for (uint32_t i = 0; i < numDevices(); ++i) { configureDevice(i); }
        m_reconfigurationRequired = false;
    }

    m_totalHardwareStimulations = 0;
    m_totalDriverChunksLost     = 0;
    m_totalDriverTimeouts       = 0;
    m_totalRingBufferOverruns   = 0;
    m_totalDataUnavailable      = 0;

    {
        std::lock_guard<std::mutex> lock(m_io_mutex);
        m_ringBuffer.Reset();
    }

    for (uint32_t i = 0; i < numDevices(); ++i)
    {
        HANDLE device = m_devices[i].handle;
        GT_Start(device);
    }

    m_isThreadRunning = true;
    m_flagIsFirstLoop = true;
    m_bufferOverrun   = false;
    m_currentQueueIdx = 0;

    m_threadPtr.reset(new std::thread(std::bind(&CDriverGTecGUSBamp::acquire, this)));

    return true;
}

//This method is called by the AS and it supplies the acquired data to the AS
bool CDriverGTecGUSBamp::loop()
{
    CStimulationSet stimSet;

    if (m_driverCtx.isStarted())
    {
        //bool dataAvailable = false;
        {
            std::unique_lock<std::mutex> lock(m_io_mutex);
            while (m_ringBuffer.GetSize() < m_validPoints) { m_itemAvailable.wait(lock); }
            try
            {
                if (m_bufferOverrun)
                {
                    m_ringBuffer.Reset();
                    m_bufferOverrun = false;
                    m_totalRingBufferOverruns++;
                    return true;
                }

                m_ringBuffer.Read(m_bufferReceivedData, m_validPoints);
            }
            catch (std::exception e) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error reading GTEC ring buffer! Error is:" << e.what() << "\n"; }

            m_itemAvailable.notify_one();
        }

        //Data is aligned as follows: element at position destBuffer[scanIndex * (numberOfChannelsPerDevice * numDevices()) + channelIndex] is 
        //sample of channel channelIndex (zero-based) of the scan with zero-based scanIndex.
        //channelIndex ranges from 0..numDevices()*numChannelsPerDevices where numDevices equals the number of recorded devices 
        //and numChannelsPerDevice the number of channels from each of those devices.
        const uint32_t totalChannels = (GTEC_NUM_CHANNELS + 1) * numDevices();

        for (uint32_t i = 0; i < totalChannels; ++i)
        {
            const int channel = m_channels[i].idx;
            if (channel >= 0)
            {
                for (uint32_t j = 0; j < NUMBER_OF_SCANS; ++j) { m_sample[channel * NUMBER_OF_SCANS + j] = m_bufferReceivedData[j * totalChannels + i]; }
            }
        }
        if (m_triggerInputEnabled)
        {
            //here convert parallel port values to stimulations if you need to
        }

        m_callback->setSamples(m_sample, NUMBER_OF_SCANS);
        m_callback->setStimulationSet(stimSet);
        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    else
    {
        if (m_driverCtx.isImpedanceCheckRequested())
        {
            // The impedance check loops over 'openvibe' channels. Look up the corresponding 'openvibe channel' from the hardware channel list.
            size_t idx = 0;
            for (size_t k = 0; k < m_channels.size(); ++k)
            {
                if (m_channels[k].idx == m_globalImpedanceIdx)
                {
                    idx = k;
                    break;
                }
            }

            const uint32_t deviceIdx  = m_channels[idx].gtecDeviceIdx;
            const uint32_t channelIdx = m_channels[idx].gtecChannelIdx + 1;
            const bool isEventChannel = m_channels[idx].isEventChannel;
            // m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Channel " << m_globalImpedanceIdx + 1 << " -> " << deviceIdx << " : " << channelIdx << "\n";

            if (isEventChannel) { m_driverCtx.updateImpedance(m_globalImpedanceIdx, 0); }
            else
            {
                HANDLE device = m_devices[deviceIdx].handle;

                double impedance = DBL_MAX;
                if (!GT_GetImpedance(device, channelIdx, &impedance))
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Impedance check failed for channel " << m_globalImpedanceIdx + 1
                            << " (amp " << deviceIdx + 1 << ", chn " << channelIdx << ")" << ". The amp may need a reset.\n";
                }
                else
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel " << m_globalImpedanceIdx + 1 << " - " << CString(
                        m_header.getChannelName(m_globalImpedanceIdx)) << " : " << impedance << "\n";
                }

                if (impedance < 0)
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel " << m_globalImpedanceIdx + 1 << " had negative impedance " << impedance
                            << ", set to a high value instead.\n";
                    impedance = 999 * 1000;
                }

                m_driverCtx.updateImpedance(m_globalImpedanceIdx, impedance);
            }

            m_globalImpedanceIdx++;
            m_globalImpedanceIdx %= m_header.getChannelCount();

            // Mark next channel as being measured
            if (m_header.getChannelCount() > 1)
            {
                // Do this only if there's more than 1 channel selected as otherwise AS will always show 'Measuring...'
                m_driverCtx.updateImpedance(m_globalImpedanceIdx, -1);
            }
            m_reconfigurationRequired = true;
        }
        else { System::Time::sleep(20); }
    }

    return true;
}

//This function is as close to the original as possible: DoAcquisition in gUSBampSyncDemo.cpp
bool CDriverGTecGUSBamp::acquire()
{
    //we can not make these checks even if they look good
    //if(!m_driverCtx.isConnected()) return false;
    //if(!m_driverCtx.isStarted()) return false;

    if (m_flagIsFirstLoop) //First time do some memory initialization, etc
    {
        //for each device create a number of QUEUE_SIZE data buffers
        for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
        {
            m_buffers[deviceIndex]    = new BYTE*[QUEUE_SIZE];
            m_overlapped[deviceIndex] = new OVERLAPPED[QUEUE_SIZE];

            //for each data buffer allocate a number of BUFFER_SIZE_BYTES bytes
            for (int queueIndex = 0; queueIndex < QUEUE_SIZE; ++queueIndex)
            {
                m_buffers[deviceIndex][queueIndex] = new BYTE[BUFFER_SIZE_BYTES];
                memset(&(m_overlapped[deviceIndex][queueIndex]), 0, sizeof(OVERLAPPED));

                //create a windows event handle that will be signalled when new data from the device has been received for each data buffer
                m_overlapped[deviceIndex][queueIndex].hEvent = CreateEvent(nullptr, false, false, nullptr);

                if (!m_overlapped[deviceIndex][queueIndex].hEvent)
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not create handle!\n";
                    return false;
                }
            }
        }

        for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
        {
            //devices are started in "Start" method, so this part is skipped from the original g.tec code
            HANDLE hDevice = m_devices[deviceIndex].handle;

            //queue-up the first batch of transfer requests
            for (int queueIndex = 0; queueIndex < QUEUE_SIZE; ++queueIndex)
            {
                if (!GT_GetData(hDevice, m_buffers[deviceIndex][queueIndex], BUFFER_SIZE_BYTES, &m_overlapped[deviceIndex][queueIndex]))
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error on GT_GetData in the initialization phase of the loop.\n";
                    return false;
                }
            }
        }

        m_flagIsFirstLoop = false;
        m_currentQueueIdx = 0;
    }

    //__try
    {
        while (m_isThreadRunning == true)
        {
            try
            {
                //bool flagChunkLostDetected    = false;
                //bool flagChunkTimeOutDetected = false;
                DWORD numBytesReceived = 0;

                //acquire data from the amplifier(s)
                for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
                {
                    HANDLE device = m_devices[deviceIndex].handle;

                    //wait for notification from the system telling that new data is available
                    if (WaitForSingleObject(m_overlapped[deviceIndex][m_currentQueueIdx].hEvent, 1000) == WAIT_TIMEOUT)
                    {
                        //std::cout << "Error on data transfer: timeout occurred." << "\n";
                        m_totalDriverTimeouts++;
                        //flagChunkTimeOutDetected = true;
                    }

                    //get number of received bytes...
                    GetOverlappedResult(device, &m_overlapped[deviceIndex][m_currentQueueIdx], &numBytesReceived, false);

                    //...and check if we lost something (number of received bytes must be equal to the previously allocated buffer size)
                    if (numBytesReceived != BUFFER_SIZE_BYTES)
                    {
                        m_totalDriverChunksLost++;
                        //flagChunkLostDetected = true;
                    }
                }

                //this line is commented on purpose
                //if (flagChunkTimeOutDetected==false && flagChunkLostDetected==false)

                //store to ring buffer
                {
                    //to store the received data into the application data buffer at once, lock it
                    {
                        std::lock_guard<std::mutex> lock(m_io_mutex);
                        try
                        {
                            //if we are going to overrun on writing the received data into the buffer, set the appropriate flag; the reading thread will handle the overrun
                            m_bufferOverrun = (m_ringBuffer.GetFreeSize() < int(N_POINTS * numDevices()));

                            //store received data from each device in the correct order (that is scan-wise, where one scan includes all channels of all devices) ignoring the header
                            for (size_t scanIndex = 0; scanIndex < NUMBER_OF_SCANS; ++scanIndex)
                            {
                                for (size_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
                                {
                                    m_ringBuffer.Write(
                                        (float*)(m_buffers[deviceIndex][m_currentQueueIdx] + scanIndex * (GTEC_NUM_CHANNELS + 1) * sizeof(float) + HEADER_SIZE),
                                        (GTEC_NUM_CHANNELS + 1));
                                }
                            }
                        }
                        catch (std::exception& e)
                        {
                            //buffer should be unclocked automatically once the scope is left
                            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error writing to GTEC ring buffer! Error is: " << e.what() << "\n";
                        }
                        //buffer should be unclocked automatically once the scope is left

                        m_itemAvailable.notify_one();
                    }
                }

                //add new GetData call to the queue replacing the currently processed one
                //this gives us time to process data while we wait for a new data chunk from the amplifier
                for (uint32_t deviceIndex = 0; deviceIndex < numDevices(); ++deviceIndex)
                {
                    HANDLE hDevice = m_devices[deviceIndex].handle;
                    if (!GT_GetData(hDevice, m_buffers[deviceIndex][m_currentQueueIdx], BUFFER_SIZE_BYTES,
                                    &m_overlapped[deviceIndex][m_currentQueueIdx]))
                    {
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error on GT_GetData in standard loop processing.\n";

                        return false;
                    }
                }
                //increment circular queueIndex to process the next queue at the next loop repetition (on overrun start at index 0 again)
                m_currentQueueIdx = (m_currentQueueIdx + 1) % QUEUE_SIZE;
            }
            catch (std::exception& e)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                        "General error in the thread function acquiring data from GTEC! Acquisition interrupted. Error is: " << e.what() << "\n";
                m_isThreadRunning = false;
                return false;
            }
        }
    }
    //__finally

    //This code stops the amplifiers in the same thread:
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Stopping devices and cleaning up...\n";

        //clean up allocated resources for each device
        for (uint32_t i = 0; i < numDevices(); ++i)
        {
            HANDLE hDevice = m_devices[i].handle;

            //clean up allocated resources for each queue per device
            for (int j = 0; j < QUEUE_SIZE; ++j)
            {
                WaitForSingleObject(m_overlapped[i][m_currentQueueIdx].hEvent, 1000);
                CloseHandle(m_overlapped[i][m_currentQueueIdx].hEvent);

                //increment queue index
                m_currentQueueIdx = (m_currentQueueIdx + 1) % QUEUE_SIZE;
            }

            //stop device
            m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Sending stop command ...\n";
            if (!GT_Stop(hDevice))
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Stopping device failed! Serial = " << m_devices[numDevices() - 1].serial.c_str() << "\n";
            }

            //reset data transfer
            m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Sending reset transfer command ...\n";
            if (!GT_ResetTransfer(hDevice))
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Reset data transfer failed! Serial = " << m_devices[numDevices() - 1].serial.c_str() << "\n";
            }

            // Sometimes when the amplifier is jammed, freeing the buffer causes heap corruption if its done before stop and reset. So we free the buffers here.
            for (int j = 0; j < QUEUE_SIZE; ++j) { delete [] m_buffers[i][j]; }

            delete [] m_overlapped[i];
            delete [] m_buffers[i];
        }

        m_flagIsFirstLoop = true;
        m_isThreadRunning = false;
    }
    return true;
}

bool CDriverGTecGUSBamp::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    //stop thread
    m_isThreadRunning = false;
    m_threadPtr->join(); //wait until the thread has stopped data acquisition

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Data acquisition completed.\n";

    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Total number of hardware stimulations acquired: " << m_totalHardwareStimulations << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Total chunks lost: " << m_totalDriverChunksLost << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Total internal ring buffer overruns: " << m_totalRingBufferOverruns << "\n";
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "Total times GTEC ring data buffer was empty: " << m_totalDataUnavailable << "\n";

    if (m_totalDriverChunksLost > 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "The driver lost " << m_totalDriverChunksLost << " chunks during the run.\n";
    }
    if (m_totalDriverTimeouts > 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "The driver had " << m_totalDriverTimeouts << " timeout(s) during the run.\n";
    }

    return true;
}

bool CDriverGTecGUSBamp::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    const int totalDevices = numDevices();
    int deviceClosed       = 0;

    for (auto it = m_devices.begin();
         it != m_devices.end();)
    {
        if (!GT_CloseDevice(&it->handle)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unable to close device: " << it->serial.c_str() << "\n"; }
        else { deviceClosed++; }

        it = m_devices.erase(it);
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Total devices closed: " << deviceClosed << " / " << totalDevices << "\n";

    if (!m_devices.empty() || deviceClosed != totalDevices)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Some devices were not closed properly!\n";
    }

    m_devices.clear();

    //clear memory
    delete[] m_bufferReceivedData;
    delete[] m_overlapped;

    if (m_sample != nullptr)
    {
        delete [] m_sample;
        m_sample = nullptr;
    }
    m_callback = nullptr;

    delete[] m_buffers;
    m_buffers = nullptr;

    restoreChannelNames();

    return true;
}

bool CDriverGTecGUSBamp::setMasterDevice(const std::string& targetMasterSerial)
{
    int targetDeviceIndex = -1;//points to the one that needs to become master

    //find master device
    for (size_t i = 0; i < numDevices(); ++i)
    {
        if (m_devices[i].serial == targetMasterSerial)
        {
            targetDeviceIndex = i;
            break;
        }
    }

    const uint32_t lastIndex = numDevices() - 1;

    if (numDevices() > 1 && targetDeviceIndex >= 0)
    {
        //swap the handlers and serials, sets the desired one as last and master
        //start swap - put selected device as last
        if (targetDeviceIndex != lastIndex)
        {
            std::swap(m_devices[lastIndex], m_devices[targetDeviceIndex]);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Now the last device is: " << m_devices[lastIndex].serial.c_str() << "\n";
        }
        else { m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Last device was the desired one: " << m_devices[lastIndex].serial.c_str() << "\n"; }

        //set slaves and new master
        m_slavesCnt  = 0;
        m_mastersCnt = 0;

        //configure all devices
        for (uint32_t i = 0; i < numDevices(); ++i)
        {
            bool isSlave = (i != lastIndex);

            if (numDevices() == 1) { isSlave = false; }

            if (!GT_SetSlave(m_devices[i].handle, isSlave))
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Unexpected error while calling GT_SetSlave\n";
            }

            if (isSlave)
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Configured as slave device: " << m_devices[i].serial.c_str() << "\n";
                m_slavesCnt++;
            }
            else
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Configured as MASTER device: " << m_devices[i].serial.c_str() 
                        << " (the master is always last in the acquisition sequence) \n";
                m_mastersCnt++;
                m_masterSerial = m_devices[i].serial;
            }
        }

        verifySyncMode();
    }
    else
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Bad number of devices detected for this operation or invalid master selection!" << "\n";
        return false;
    }

    return true;
}

//Checks if devices are configured correctly when acquiring data from multiple devices
bool CDriverGTecGUSBamp::verifySyncMode()
{
    //g.tec check list
    if (numDevices() > 1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "More than 1 device detected, performing some basic sync checks.\n";

        //Test that only one device is master
        if ((m_mastersCnt + m_slavesCnt != numDevices()) || (m_mastersCnt != 1))
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Problem with number of slaves/masters compared to total number of devices!\n";
            return false;
        }

        //Test that the master device is the last in the sequence
        const std::string serialLastDevice = m_devices[numDevices() - 1].serial;

        if (serialLastDevice != m_masterSerial)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Master device is not the last one! serial last device =" << serialLastDevice 
                    << " and master=" << m_masterSerial << " .\n";
            return false;
        }
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverGTecGUSBamp::configure()
{
    detectDevices();

    std::vector<std::string> serials;
    for (size_t i = 0; i < numDevices(); ++i) { serials.push_back(m_devices[i].serial); }

    CConfigurationGTecGUSBamp config(Directories::getDataDir() + "/applications/acquisition-server/interface-GTec-GUSBamp.ui", //m_deviceIdx,
                                     m_commonGndAndRefBitmap, m_notchFilterIdx, m_bandPassFilterIdx, m_triggerInputEnabled, serials, m_masterSerial,
                                     m_bipolarEnabled, m_calibrationSignalEnabled, m_showDeviceName);


    // The number of channels is initially the maximum with the trigger channels not counted
    if (m_header.getChannelCount() == 0)
    {
        const uint32_t nTotalChannel = numDevices() * GTEC_NUM_CHANNELS;
        m_header.setChannelCount(nTotalChannel);
    }

    if (!config.configure(m_header)) { return false; }

    //get new value from header
    this->m_nAcquiredChannel = m_header.getChannelCount();

    m_settings.save();

    //start reconfigure based on the new input:

    if (numDevices() > 1) { setMasterDevice(m_masterSerial); }

    //end reconfigure based on the new input

    return true;
}

void CDriverGTecGUSBamp::configFiltering(HANDLE device)
{
    int nrOfFilters;
    const float mySamplingRate = float(m_header.getSamplingFrequency());

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << getSerialByHandler(device) << ": Notch filter index " << m_notchFilterIdx 
            << ", bandpass filter index " << m_bandPassFilterIdx << "\n";

    //Set BandPass

    // get the number of available filters
    bool status = GT_GetNumberOfFilter(&nrOfFilters);
    if (status == false)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << getSerialByHandler(device) 
                << ": Could not get the number of dsp filters!Filtering is disabled.\n";
        return;
    }

    // create array of FILT structures to store the filter settings
    FILT* filters = new FILT[nrOfFilters];

    // fill array with filter settings
    status = GT_GetFilterSpec(filters);
    if (status == false)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << getSerialByHandler(device) 
                << ": Could not get the list of dsp filters! Filtering is disabled.\n";
        delete[] filters;
        return;
    }

    for (int i = 1; i <= GTEC_NUM_CHANNELS; ++i)  //channels must be [1..16]
    {
        status = GT_SetBandPass(device, i, m_bandPassFilterIdx);
        if (status == false)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << getSerialByHandler(device) << ": Could not set band pass filter on channel " << i << "\n";
            delete[] filters;
            return;
        }
    }

    if (m_bandPassFilterIdx == -1) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << getSerialByHandler(device) << ": No BandPass filter applied.\n"; }
    else
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << getSerialByHandler(device) << ": Bandpass filter applied: between " 
                    << filters[m_bandPassFilterIdx].fu << " and " << filters[m_bandPassFilterIdx].fo << ", order = " 
                    << filters[m_bandPassFilterIdx].order << ", type = " << ((filters[m_bandPassFilterIdx].type == 1) ? "butterworth" : "chebyshev") 
                    << ", frequency = " << mySamplingRate << "\n";
    }
    delete[] filters;

    //Set Notch

    // get the number of available filters
    status = GT_GetNumberOfNotch(&nrOfFilters);
    if (status == false)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << getSerialByHandler(device) 
                << ": Could not get the number of notch filters! Filtering is disabled.\n";
        return;
    }

    // create array of FILT structures to store the filter settings
    filters = new FILT[nrOfFilters];

    // fill array with filter settings
    status = GT_GetNotchSpec(filters);
    if (status == false)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << getSerialByHandler(device) 
                << ": Could not get the list of notch filters! Filtering is disabled.\n";
        delete[] filters;
        return;
    }

    for (int i = 1; i <= GTEC_NUM_CHANNELS; ++i)  //channels must be [1..16]
    {
        status = GT_SetNotch(device, i, m_notchFilterIdx);
        if (status == 0)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << getSerialByHandler(device) << ": Could not set notch filter on channel " << i << "\n";
            delete[] filters;
            return;
        }
    }

    if (m_notchFilterIdx == -1) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << getSerialByHandler(device) << ": No Notch filter applied.\n"; }
    else
    {
        const uint32_t frequency = uint32_t((filters[m_notchFilterIdx].fo + filters[m_notchFilterIdx].fu) / 2);
        std::string country("Unknown countries");
        if (frequency == 50) { country = "Europe"; }
        else if (frequency == 60) { country = "USA"; }

        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << getSerialByHandler(device) << ": Notch filter applied: " << frequency 
                << " Hz (usually used e.g. in " << country << ")\n";
    }

    delete[] filters;
}

//Adds the amplifier ID to the channel name
void CDriverGTecGUSBamp::remapChannelNames()
{
    // Store original channel names
    m_originalChannelNames.clear();
    for (size_t i = 0; i < m_nAcquiredChannel; ++i) { m_originalChannelNames.push_back(m_header.getChannelName(i)); }

    // Add the trigger channels
    if (m_triggerInputEnabled) { m_header.setChannelCount(m_nAcquiredChannel + numDevices()); }
    else { m_header.setChannelCount(m_nAcquiredChannel); }

    for (size_t i = 0; i < m_channels.size(); ++i)
    {
        if (m_channels[i].idx >= 0)
        {
            const uint32_t maxChannelsPerAmp = (GTEC_NUM_CHANNELS + 1);
            const uint32_t idx               = i / maxChannelsPerAmp;

            std::string channelName;
            if (m_channels[i].isEventChannel) { channelName = "CH_Event" + std::to_string(idx); }
            else { channelName = m_originalChannelNames[m_channels[i].oldIdx]; }

            if (channelName.length() == 0)
            {
                if (!m_showDeviceName) { channelName = "Channel "; }

                channelName += std::to_string(m_channels[i].oldIdx + 1);
            }

            if (m_showDeviceName) { channelName += "_" + m_devices[idx].serial; }

            //std::cout << m_channels[i].idx << " to'" << channelName << "'" << std::endl;

            m_header.setChannelName(m_channels[i].idx, channelName.c_str());
        }
    }
}

void CDriverGTecGUSBamp::restoreChannelNames()
{
    // Restore channel count without event channels
    m_header.setChannelCount(m_nAcquiredChannel);

    // Reset the original channel names
    for (uint32_t i = 0; i < m_nAcquiredChannel; ++i) { m_header.setChannelName(i, m_originalChannelNames[i].c_str()); }
}

std::string CDriverGTecGUSBamp::getSerialByHandler(const HANDLE device)
{
    if (m_devices.empty())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not find serial: no devices!\n";
        return "ERROOR";
    }

    for (size_t i = 0; i < m_devices.size(); ++i) { if (device == m_devices[i].handle) { return m_devices[i].serial; } }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not find serial for this device!\n";
    return "ERROR UNKNOWN SERIAL";
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& var)
{
    for (size_t i = 0; i < var.size(); ++i) { out << var[i] << " "; }
    return out;
}

inline std::istream& operator>>(std::istream& in, std::vector<std::string>& var)
{
    var.clear();
    std::string tmp;
    while (in >> tmp) { var.push_back(tmp); }
    return in;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<GDevice>& var)
{
    for (size_t i = 0; i < var.size(); ++i) { out << var[i].serial << " "; }
    return out;
}

inline std::istream& operator>>(std::istream& in, std::vector<GDevice>& var)
{
    //std::cout << "Error not implemented operator >>!";
    return in;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
