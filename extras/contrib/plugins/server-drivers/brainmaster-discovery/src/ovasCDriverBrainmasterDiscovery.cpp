///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverBrainmasterDiscovery.cpp
/// \copyright Copyright (C) 2012, Yann Renard. All rights reserved.
/// 
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
/// 
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
/// 
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
/// 
///-------------------------------------------------------------------------------------------------

#if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI

#include "ovasCDriverBrainmasterDiscovery.h"
#include "ovasCConfigurationBrainmasterDiscovery.h"
#include "../ovasCConfigurationBuilder.h"

#include <toolkit/ovtk_all.h>

#include "ovas_defines_brainmaster_discovery.h"

namespace OpenViBE {
namespace AcquisitionServer {

extern "C" {
int OvAtlLoginDevice(const char* sSerialNumber, const char* sPassKey);
}

static float decode_24(unsigned char*& rpBuffer)
{
    int res = rpBuffer[0] + rpBuffer[1] * 0x100 + rpBuffer[2] * 0x10000;
    if (res > 0x800000) { res = res - 0x1000000; }
    rpBuffer += 3;
    return res * 0.01163f;
}

static float decode_16(unsigned char*& rpBuffer)
{
    int res = rpBuffer[0] + rpBuffer[1] * 0x100;
    if (res > 0x8000) { res = res - 0x10000; }
    rpBuffer += 2;
    return res * 0.1983f;
}

static float decode_8(unsigned char*& rpBuffer)
{
    int res = rpBuffer[0];
    res -= 0x80;
    rpBuffer += 1;
    return res * 100 / 128.f;
}

static uint32_t next_sync(uint32_t sync)
{
    sync++;
    sync &= 7;
    return sync ? sync : 1;
}

//___________________________________________________________________//
//                                                                   //

CDriverBrainmasterDiscovery::CDriverBrainmasterDiscovery(IDriverContext& ctx)
    : IDriver(ctx)
      , m_preset(Preset_Discovery_24)
{
    m_deviceSerial      = ctx.getConfigurationManager().expand("${AcquisitionServer_Driver_BrainmasterDeviceSerial}").toASCIIString();
    m_devicePasskey     = ctx.getConfigurationManager().expand("${AcquisitionServer_Driver_BrainmasterDevicePasskey}").toASCIIString();
    m_frameDumpFilename = ctx.getConfigurationManager().expand("${AcquisitionServer_Driver_BrainmasterFrameDumpFilename}").toASCIIString();

    m_baudRates[BaudRate_9600]   = 0x30; // 9600
    m_baudRates[BaudRate_115200] = 0x20; // 115200
    m_baudRates[BaudRate_460800] = 0x10; // 460800

    m_baudRateValues[BaudRate_9600]   = 9600; // 9600
    m_baudRateValues[BaudRate_115200] = 115200; // 115200
    m_baudRateValues[BaudRate_460800] = 460800; // 460800

    m_notchsFilters[NotchFilter_Default] = 0; // Off
    m_notchsFilters[NotchFilter_Off]     = 0; // Off
    m_notchsFilters[NotchFilter_50]      = 2; // 50 Hz
    m_notchsFilters[NotchFilter_60]      = 3; // 60 Hz

    m_notchFiltersValues[NotchFilter_Default] = 0; // Off
    m_notchFiltersValues[NotchFilter_Off]     = 0; // Off
    m_notchFiltersValues[NotchFilter_50]      = 50; // 50 Hz
    m_notchFiltersValues[NotchFilter_60]      = 60; // 60 Hz

    m_bitDepths[BitDepth_8]  = 1; // 8 bits
    m_bitDepths[BitDepth_16] = 2; // 16 bits
    m_bitDepths[BitDepth_24] = 3; // 24 bits

    m_bitDepthValues[BitDepth_8]  = 8; // 8 bits
    m_bitDepthValues[BitDepth_16] = 16; // 16 bits
    m_bitDepthValues[BitDepth_24] = 24; // 24 bits

    // 0xcc for 2 EEG, 0xff for 2 EEG + 2 AUX
    m_channelSelectionMasks[24] = 0xffffff; // Type_Discovery
    m_channelSelectionMasks[4]  = 0xf; // Type_Atlantis 2x2
    m_channelSelectionMasks[8]  = 0xff; // Type_Atlantis 4x4

    m_bitDepthDecoders[ChannelType_EEG][BitDepth_8]  = decode_8;  // 8 bits
    m_bitDepthDecoders[ChannelType_EEG][BitDepth_16] = decode_16; // 16 bits
    m_bitDepthDecoders[ChannelType_EEG][BitDepth_24] = decode_24; // 24 bits

    m_bitDepthDecoders[ChannelType_AUX][BitDepth_8]  = decode_8;  // 8 bits
    m_bitDepthDecoders[ChannelType_AUX][BitDepth_16] = decode_16; // 16 bits
    m_bitDepthDecoders[ChannelType_AUX][BitDepth_24] = decode_16; // 24 bits

    m_startModules[Type_Discovery] = ::DiscStartModule;
    m_startModules[Type_Atlantis]  = ::AtlStartModule;

    m_stopModules[Type_Discovery] = ::DiscStopModule;
    m_stopModules[Type_Atlantis]  = ::AtlStopModule;

    m_header.setSamplingFrequency(256);
    m_header.setChannelCount(24);
    m_header.setChannelName(0, "Fp1"/*"FP1"*/);
    m_header.setChannelName(1, "F3");
    m_header.setChannelName(2, "C3");
    m_header.setChannelName(3, "P3");
    m_header.setChannelName(4, "O1");
    m_header.setChannelName(5, "F7");
    m_header.setChannelName(6, "T3");
    m_header.setChannelName(7, "T5");
    m_header.setChannelName(8, "Fz");
    m_header.setChannelName(9, "Fp2"/*"FP2"*/);
    m_header.setChannelName(10, "F4");
    m_header.setChannelName(11, "C4");
    m_header.setChannelName(12, "P4");
    m_header.setChannelName(13, "O2");
    m_header.setChannelName(14, "F8");
    m_header.setChannelName(15, "T4");
    m_header.setChannelName(16, "T6");
    m_header.setChannelName(17, "Cz");
    m_header.setChannelName(18, "Pz");
    m_header.setChannelName(19, "A2");
    m_header.setChannelName(20, "Fpz"/*"FPz"*/);
    m_header.setChannelName(21, "Oz");
    m_header.setChannelName(22, "AUX1");
    m_header.setChannelName(23, "AUX2");

    for (size_t i = 0; i < m_header.getChannelCount(); ++i) { m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro); }
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainmasterDiscovery::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    uint32_t i;

    if (m_driverCtx.isConnected()) { return false; }

    if (m_deviceSerial == "" || m_devicePasskey == "")
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "The device serial or passkey were not configured\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                "You can set their value from the configuration pannel or from the following configuration tokens :\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " - " << CString("AcquisitionServer_Driver_BrainmasterDeviceSerial") << "\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " - " << CString("AcquisitionServer_Driver_BrainmasterDevicePasskey") << "\n";
        return false;
    }

    switch (m_type)
    {
        default:
        case Type_Discovery: m_baudRateReal = (m_baudRate != BaudRate_Default ? m_baudRate : BaudRate_460800);
            m_samplingRateReal = m_samplingRate;
            m_bitDepthReal     = BitDepth_24;
            m_notchFiltersReal = (m_notchFilters != NotchFilter_Default ? m_notchFilters : NotchFilter_Default);
            m_frameSize        = 3;
            m_dataOffset       = 3;
            break;

        case Type_Atlantis: m_baudRateReal = (m_baudRate != BaudRate_Default ? m_baudRate : BaudRate_115200);
            m_samplingRateReal = m_samplingRate;
            m_bitDepthReal     = (m_bitDepth != BitDepth_Default ? m_bitDepth : BitDepth_24);
            m_notchFiltersReal = (m_notchFilters != NotchFilter_Default ? m_notchFilters : NotchFilter_Default);
            m_frameSize        = 1;
            m_dataOffset       = 1;
            break;
    }

    switch (m_bitDepthReal)
    {
        default:
        case BitDepth_24: for (i = 0; i < m_header.getChannelCount(); ++i)
            {
                switch (m_channelTypes[i])
                {
                    default:
                    case ChannelType_EEG: m_frameSize += 3;
                        break;

                    case ChannelType_AUX: m_frameSize += 2;
                        break;
                }
            }
            break;

        case BitDepth_16: m_frameSize += m_header.getChannelCount() * 2;
            break;

        case BitDepth_8: m_frameSize += m_header.getChannelCount();
            break;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Computed frame size of " << m_frameSize << " bytes\n";

    m_samples.resize(m_header.getChannelCount());
    m_buffers.resize(m_frameSize);

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    // Sets/Autodetects port
    m_portReal = m_port;
    if (m_portReal == 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Auto detecting COM port...\n";

        m_portReal = autoDetectPort();
        if (m_portReal == uint32_t(-1))
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Port is not set and could not be auto detected, please configure the driver\n";
            return false;
        }

        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Found COM port " << m_portReal << " !\n";
    }

    // Opens device handle at default speed
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Opening COM port " << m_portReal << " at 9600 bauds\n";
    if (!::AtlOpenPort(m_portReal, 9600, nullptr))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open port " << m_portReal << "\n";
        return false;
    }

    // Writes baud rate
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Setting baud rate to " << m_baudRateValues[m_baudRateReal] << " bauds (code " << m_baudRates[
        m_baudRateReal] << ")\n";
    if (!::AtlSetBaudRate(m_baudRates[m_baudRateReal]))
    {
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not set baud rate\n";
        return false;
    }

    // Closes port (which was at default speed)
    ::AtlClosePort(m_portReal);

    // Opens device handle at targeted speed
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Re-opening COM port " << m_portReal << " at " << m_baudRateValues[m_baudRateReal] <<
            " bauds (code " << m_baudRates[m_baudRateReal] << ")\n";
    if (!::AtlOpenPort(m_portReal, m_baudRateValues[m_baudRateReal], nullptr))
        // if(!::AtlOpenPort(m_portReal, 9600, nullptr))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not open port " << m_portReal << "\n";
        return false;
    }

    // Logs in the device
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Logging in the device...\n";
    const int code = OvAtlLoginDevice(m_deviceSerial.c_str(), m_devicePasskey.c_str());
    if (!code)
    {
        ::AtlSetBaudRate(m_baudRates[BaudRate_9600]);
        ::AtlClosePort(m_portReal);

        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not log in device (error code was " << uint32_t(code) << ")\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Please check the device serial and passkey configuration\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                "You can set their value from the configuration pannel or from the following configuration tokens :\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " - " << CString("AcquisitionServer_Driver_BrainmasterDeviceSerial") << " (was set to [" <<
                m_deviceSerial.c_str() << "])\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << " - " << CString("AcquisitionServer_Driver_BrainmasterDevicePasskey") << " (was set to [" <<
                m_devicePasskey.c_str() << "])\n";

        if (!checkDeviceSerial(m_deviceSerial))
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                    "By the way, serial seems to be malformed, it is supposed to be XXXXX with each X be a digit\n";
        if (!checkDevicePasskey(m_devicePasskey))
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error <<
                    "By the way, passkey seems to be malformed, it is supposed to be XXXX-XXXX-XXXX with each X be a digit or a letter\n";

        return false;
    }

    // Reads firmware version
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Successfully opened device on port " << m_portReal << " | Firware nr : " <<
            uint32_t(AtlQueryFirmware(0)) << "\n";

    // Writes notch filters
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Setting notch filters to " << m_notchFiltersValues[m_notchFiltersReal] << " (code " <<
            m_notchsFilters
            [m_notchFiltersReal] << ")...\n";
    if (!::AtlSetNotchFilters(m_notchsFilters[m_notchFiltersReal]))
    {
        ::AtlSetBaudRate(m_baudRates[BaudRate_9600]);
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not set notch filters\n";
        return false;
    }

    // Writes bit depth
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Setting bit depth to " << m_bitDepthValues[m_bitDepthReal] << " bits (code " << m_bitDepths[
        m_bitDepthReal] << ")...\n";
    if (!::AtlSetBytesPerSample(m_bitDepths[m_bitDepthReal]))
    {
        ::AtlSetBaudRate(m_baudRates[BaudRate_9600]);
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not set bytes per sample\n";
        return false;
    }

    // Selects all the channels
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Selecting all channels with channel mask " << m_channelSelectionMasks[m_header.getChannelCount()]
            <<
            "...\n";
    if (!::AtlSelectChannels(m_channelSelectionMasks[m_header.getChannelCount()]))
    {
        ::AtlSetBaudRate(m_baudRates[BaudRate_9600]);
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not select all the channels\n";
        return false;
    }

    // Clears special data
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Clearing specials...\n";
    if (!::AtlClearSpecials())
    {
        ::AtlSetBaudRate(m_baudRates[BaudRate_9600]);
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not clear special data selection\n";
        return false;
    }

    // Poking special code for Atlantis 4x4
    if (m_header.getChannelCount() == 8)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Poking special code for Atlantis 4x4...\n";
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << uint32_t(AtlPoke(0xc006, 0x00)) <<
                "\n"; // Sets internal sampling rate set to 512 Hz - This hack solves the EEG chan 3 & 4 flat lines issue
        //      m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << uint32_t(AtlPoke(0xb607, 0xff)) << "\n"; // ATC_CHANOUTMASK
        //      m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << uint32_t(AtlPoke(0xb608, 0x20)) << "\n"; // ATC_ADCMODE
        //      m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << uint32_t(AtlPoke(0xb7e9, 0x03)) << "\n"; // ??? Bit mode ?
    }

    // serialn_p ::AtlQuerySerialNumber(int auth)

    // Actually starts the acquisition
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Starting module !\n";
    if (!m_startModules[m_type]())
    {
        ::AtlSetBaudRate(m_baudRates[BaudRate_9600]);
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not start module\n";
        return false;
    }

    m_syncByte = 1;

    // Preparing optional frame dump
    if (m_frameDumpFilename == "")
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Won't dump frames\n";
        m_frameDumpFlag = false;
    }
    else
    {
        FILE* file = fopen(m_frameDumpFilename.c_str(), "wb");
        if (file)
        {
            m_frameDumpFlag = true;
            fclose(file);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Will dump frames in [" << m_frameDumpFilename << "]\n";
        }
        else
        {
            m_frameDumpFlag = false;
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Should have dumped frames in [" << m_frameDumpFilename <<
                    "] but file can't be opened for writing !\n";
        }
    }


    return true;
}

bool CDriverBrainmasterDiscovery::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    return true;
}

bool CDriverBrainmasterDiscovery::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }
    bool gotData = false;

    for (uint32_t i = 0; i < m_nSamplePerSentBlock; ++i)
    {
        // Updates Sync bits
        m_syncByte = next_sync(m_syncByte);

        // Searches for Sync bits
        this->read(&m_buffers[0], 1);
        if (((m_buffers[0] >> 5) & 7) != m_syncByte)
        {
            // Needs resync
            uint32_t sync = this->sync();
            if (sync != uint32_t(-1)) { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Resynced by " << sync << " byte(s) !\n"; }
            else { m_driverCtx.getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not resync, did not find the sync bytes !\n"; }

            m_wasLastFrameCorrect = false;
        }
        else
        {
            // Sends samples if last frame was correct
            if (m_wasLastFrameCorrect)
            {
                m_callback->setSamples(&m_samples[0], 1);
                gotData = true;
            }

            // Assumed synced stream - Reads leading bytes
            this->read(&m_buffers[1], m_buffers.size() - 1);

#if 1
            // For debug purpose
            if (m_frameDumpFlag)
            {
                FILE* file = fopen(m_frameDumpFilename.c_str(), "ab");
                if (file)
                {
                    fprintf(file, "Frame : ");
                    for (uint32_t i = 0; i < m_buffers.size(); ++i) { fprintf(file, "0x%02x ", m_buffers[i]); }
                    fprintf(file, "\n");
                    fclose(file);
                }
                else
                {
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Unexpected: Could not open dump file [" << m_frameDumpFilename <<
                            "] for writing\n";
                    m_frameDumpFlag = false;
                }
            }
#endif

            // Converts new buffer to samples
            unsigned char* buffer = &m_buffers[m_dataOffset];

            for (uint32_t j = 0; j < m_samples.size(); ++j) { m_samples[j] = m_bitDepthDecoders[m_channelTypes[j]][m_bitDepthReal](buffer); }

            m_wasLastFrameCorrect = true;
        }
    }

    if (gotData) { m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount()); }

    return true;
}

bool CDriverBrainmasterDiscovery::read(unsigned char* frame, uint32_t size)
{
    const uint32_t count = size;
    uint32_t idx         = 0;

    while (idx != count)
    {
        int iCode = ::AtlReadData(frame + idx, count - idx);
        if (iCode >= 0) { idx += iCode; }
    }

    return true;
}

uint32_t CDriverBrainmasterDiscovery::sync()
{
    std::vector<unsigned char> buffer;
    buffer.resize(m_buffers.size() * 4);

    bool finished  = false;
    uint32_t nSync = 0;
    uint32_t i     = 0;

    while (!finished && i < 16)
    {
        i++;
        this->read(&buffer[0], buffer.size());
        for (uint32_t i = 0; i < m_buffers.size() && !finished; ++i)
        {
            uint32_t syncCandidate[4];
            syncCandidate[0] = buffer[i + 0 * m_buffers.size()] >> 5;
            syncCandidate[1] = buffer[i + 1 * m_buffers.size()] >> 5;
            syncCandidate[2] = buffer[i + 2 * m_buffers.size()] >> 5;
            syncCandidate[3] = buffer[i + 3 * m_buffers.size()] >> 5;
            if (next_sync(syncCandidate[0]) == syncCandidate[1]
                && next_sync(syncCandidate[1]) == syncCandidate[2]
                && next_sync(syncCandidate[2]) == syncCandidate[3])
            {
                this->read(&buffer[0], i);
                m_syncByte = syncCandidate[3];
                nSync      = i;
                finished   = true;
            }
        }
    }

#if 1
    // For debug purpose
    if (m_frameDumpFlag)
    {
        FILE* file = fopen(m_frameDumpFilename.c_str(), "ab");
        if (file)
        {
            fprintf(file, "Syncing...\n");
            for (uint32_t i = 0; i < buffer.size(); ++i)
            {
                fprintf(file, "0x%02x ", buffer[i]);
                if (((i + 1) % m_buffers.size()) == 0) { fprintf(file, "\n"); }
            }

            fclose(file);
        }
        else
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Unexpected: Could not open dump file [" << m_frameDumpFilename <<
                    "] for writing\n";
            m_frameDumpFlag = false;
        }
    }
#endif

    return finished ? nSync : uint32_t(-1);
}

bool CDriverBrainmasterDiscovery::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    return true;
}

bool CDriverBrainmasterDiscovery::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // Stops acquisition
    if (!m_stopModules[m_type]())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could stop module\n";
        return false;
    }

    // Writes baud rate back to 9600
    if (!::AtlSetBaudRate(m_baudRates[BaudRate_9600]))
    {
        ::AtlClosePort(m_portReal);
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not set baud rate\n";
        return false;
    }

    // Closes device handle
    if (!::AtlClosePort(m_portReal))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not close port " << m_portReal << "\n";
        return false;
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBrainmasterDiscovery::configure()
{
    CConfigurationBrainmasterDiscovery config(Directories::getDataDir() + "/applications/acquisition-server/interface-Brainmaster-Discovery.ui",
                                              m_port, m_preset, m_type, m_baudRate, m_samplingRate, m_bitDepth,
                                              m_notchFilters, m_channelTypes, m_deviceSerial, m_devicePasskey);
    if (!config.configure(m_header)) { return false; }
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
