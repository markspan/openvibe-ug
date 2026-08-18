#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <Windows.h>

#include "ringbuffer.h"

#include <gtk/gtk.h>
#include <vector>

//threading
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory> // unique_ptr

#include <deque>

namespace OpenViBE
{
    namespace AcquisitionServer
    {
        /**
         * \class CDriverGTecGUSBamp
         * \author Anton Andreev, Gipsa-lab, VIBS team
         * \date 19/07/2012
         * \brief GTEC driver
         *
         * This driver was rewritten to match the code provided by Guger as much as possible. There are several things
         * that all must work together so that higher frequencies are supported and no hardware triggers are lost.
         *
         * This driver supports several buffers so that the more than one GT_GetData can be executed in the beginning (QUEUE_SIZE)
         * and then calls to GT_GetData are queued. This allows data to be processed by OpenVibe while waiting for the next result of
         * a previously issued GT_GetData. The extra thread is added to support this and to allow for async IO. 
         *
         * Hardware triggers on the parallel port are supported.
         *
         * The driver supports several g.tec devices working with the provided async cables. There are several requirements for async
         * acquisition to work properly and these are checked in verifySyncMode().
         */
#ifndef __GDEVICE_H__
#define __GDEVICE_H__
        struct GDevice
        {
            HANDLE handle;
            std::string serial;
        };
#endif

        class CDriverGTecGUSBamp final : public IDriver
        {
        public:

            explicit CDriverGTecGUSBamp(IDriverContext& ctx);

            void release() { delete this; }
            const char* getName() override { return "g.tec gUSBamp Gipsa-lab"; }

            bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
            bool uninitialize() override;

            bool start() override;
            bool stop() override;
            bool loop() override;

            bool isConfigurable() override { return true; }
            bool configure() override;
            const IHeader* getHeader() override { return &m_header; }

            bool CDriverGTecGUSBamp::acquire();

            void configFiltering(HANDLE device);

        protected:

            static const int BUFFER_SIZE_SECONDS = 2;   //the size of the GTEC ring buffer in seconds
            static const int GTEC_NUM_CHANNELS   = 16;  //the number of channels without countig the trigger channel
            static const int QUEUE_SIZE          =
                    8;  //4 default       //the number of GT_GetData calls that will be queued during acquisition to avoid loss of data
            static const int NUMBER_OF_SCANS =
                    32; //the number of scans that should be received simultaneously (depending on the _sampleRate; see C-API documentation for this value!)

            size_t numDevices() const { return m_devices.size(); }

            static const uint32_t N_POINTS = NUMBER_OF_SCANS * (GTEC_NUM_CHANNELS + 1);
            int m_validPoints              = 0;
            static const DWORD BUFFER_SIZE_BYTES;

            SettingsHelper m_settings;

            IDriverCallback* m_callback = nullptr;
            CHeader m_header;

            float* m_sample                = nullptr;
            uint32_t m_nSamplePerSentBlock = 0;

            uint32_t m_globalImpedanceIdx = 0;

            uint8_t m_commonGndAndRefBitmap = 0;

            int m_notchFilterIdx    = -1;
            int m_bandPassFilterIdx = -1;

            bool m_triggerInputEnabled = false;
            bool m_bipolarEnabled      =
                    false;  //electrodes are substracted in sepecific sequence 1-2=1, ... 15-16=15 which results in 8 instead of 16 electrodes - used for EMG
            bool m_calibrationSignalEnabled = false;
            bool m_showDeviceName           = false;    //adds the amplifier serial number to the name of the channel 
            bool m_reconfigurationRequired  = false;    // After some gt calls, we may need reconfig

            uint32_t m_nAcquiredChannel = GTEC_NUM_CHANNELS;    //number of channels specified by the user, never counts the event channels

            uint32_t m_totalHardwareStimulations = 0;   //since start button clicked
            uint32_t m_totalDriverChunksLost     = 0;   //since start button clicked
            uint32_t m_totalDriverTimeouts       = 0;   //since start button clicked
            uint32_t m_totalRingBufferOverruns   = 0;
            uint32_t m_totalDataUnavailable      = 0; 

            //contains buffer per device and then QUEUE_SIZE buffers so that several calls to GT_GetData can be supported
            BYTE*** m_buffers         = nullptr;
            OVERLAPPED** m_overlapped = nullptr;

            bool m_flagIsFirstLoop = true;
            bool m_bufferOverrun   = false;

            //ring buffer provided by Guger
            CRingBuffer<float> m_ringBuffer;

            uint32_t m_currentQueueIdx = 0;

            std::unique_ptr<std::thread> m_threadPtr;
            bool m_isThreadRunning = false;

            std::mutex m_io_mutex;

            float* m_bufferReceivedData = nullptr;
            std::condition_variable m_itemAvailable;

            bool configureDevice(uint32_t deviceNumber);

            bool verifySyncMode();//Checks if devices are configured correctly when acquiring data from multiple devices

            //Selects which device to become the new master, used only when more than 1 device is available
            bool setMasterDevice(const std::string& targetMasterSerial); //0 first device
            void detectDevices();

            uint32_t m_mastersCnt      = 0;
            uint32_t m_slavesCnt       = 0;
            std::string m_masterSerial = "";

            void remapChannelNames();   // Converts channel names while appending the device name and handling event channels
            void restoreChannelNames(); // Restores channel names without the device name

            std::vector<std::string> m_originalChannelNames;    // Channel names without the device name inserted
            std::vector<GDevice> m_devices;                     // List of amplifiers

            std::string CDriverGTecGUSBamp::getSerialByHandler(HANDLE device);

            // Stores information related to each channel available in the recording system
            struct SChannel
            {
                int idx;             // Channel index in openvibe Designer, -1 is unused
                int oldIdx;          // Channel index in the user-settable channel name list
                int gtecDeviceIdx;   // Device index of the gtec amplifier this channel is in
                int gtecChannelIdx;  // Channel index in the device-specific numbering
                bool isEventChannel;                 // Is this the special digital channel?
            };

            // Channel indexes are seen as a sequence [dev1chn1, dev1chn2,...,dev1chnN, dev2chn1, dev2chn2, ..., dev2chnN, ...]
            // The following vector is used to map these 'system indexes' to openvibe channels
            std::vector<SChannel> m_channels;
        };
    }  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI
