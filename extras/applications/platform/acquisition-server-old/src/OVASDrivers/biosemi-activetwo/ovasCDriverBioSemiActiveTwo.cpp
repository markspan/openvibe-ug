///-------------------------------------------------------------------------------------------------
/// \copyright Copyright (C) 2012, Mensia Technologies SA. All rights reserved.
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
///-------------------------------------------------------------------------------------------------

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#include "ovasCDriverBioSemiActiveTwo.h"
#include "ovasCConfigurationBioSemiActiveTwo.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
// linux, mac
#include <unistd.h>
#endif  // WIN32

#include <toolkit/ovtk_all.h>
#include <system/ovCTime.h>

namespace OpenViBE {
namespace AcquisitionServer {

CDriverBioSemiActiveTwo::CDriverBioSemiActiveTwo(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_BioSemiActiveTwo", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(2048);

    // The amplifier can send up to 256+8 channels
    // as a request from BioSemi, we will make available the maximum channel count
    // User is able to select from 1 to MAX channels. If no data is present on the 
    // corresponding channels, zeros will be sent.
    // The number of channels present in the data flow will still be displayed in 
    // the driver configuration window. Previously selected value will be saved
    // with other settings.
    m_header.setChannelCount(BIOSEMI_ACTIVETWO_MAXCHANNELCOUNT + BIOSEMI_ACTIVETWO_EXCHANNELCOUNT);
    m_useEXChannel = true;

    m_triggers.resize(16, false);

    m_lastWarningTime     = 0;
    m_startTime           = 0;
    m_bCMCurrentlyInRange = true;
    m_bBatteryCurrentlyOk = true;

    m_settings.add("Header", &m_header);
    m_settings.add("UseEXChannel", &m_useEXChannel);
    m_settings.load();
    m_bridge.setUseEXChannels(m_useEXChannel);
}

void CDriverBioSemiActiveTwo::setupInformationWindow()
{
    if (!m_infoWindow)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Information window not allocated\n";
        return;
    }

    std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);

    m_infoWindow->m_builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(m_infoWindow->m_builder, Directories::getDataDir() + "/applications/acquisition-server/interface-BioSemi-ActiveTwo.ui", nullptr))
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "File not found: " << Directories::getDataDir() << "/applications/acquisition-server/interface-BioSemi-ActiveTwo.ui\n";
        return;
    }

    gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(m_infoWindow->m_builder, "label-device-type")), (m_bridge.isDeviceMarkII() ? "- <i>ActiveTwo Mark II</i> -" : "- <i>ActiveTwo Mark I</i> -"));
    m_infoWindow->m_isCMSInRange = m_bridge.isCMSInRange();
    m_infoWindow->m_isBatteryLow = m_bridge.isBatteryLow();
    m_infoWindow->m_isChanged    = true;
    gtk_widget_show_all(GTK_WIDGET(gtk_builder_get_object(m_infoWindow->m_builder, "device-information")));
}

//___________________________________________________________________//
//                                                                   //

gint setup_information_window_callback(void* data)
{
    CDriverBioSemiActiveTwo* pTmp = static_cast<CDriverBioSemiActiveTwo*>(data);

    pTmp->setupInformationWindow();

    return FALSE; // Don't run again
}

gint information_window_callback(void* infoWindow)
{
    if (!infoWindow)
    {
        // Not initialized yet?
        return TRUE;
    }
    SInformationWindow* window = reinterpret_cast<SInformationWindow*>(infoWindow);
    std::lock_guard<std::mutex> lock(reinterpret_cast<SInformationWindow*>(infoWindow)->m_mutex);
    // If nothing changed, directly return from the callback
    if (!window->m_isChanged) { return TRUE; }
    if (window->m_isAcquisitionEnded)
    {
        // If the acquisition is ended, delete the window
        gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(window->m_builder, "device-information")));
        g_object_unref(window->m_builder);

        // If we get here, we know that the other thread has passed uninitialize() and will no longer access Information Window.
        delete window;
        // The loop should now be stopped
        return FALSE;
    }
    if (window->m_isBatteryLow)
    {
        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(window->m_builder, "label-battery-level")), "- <i>Device battery is low !</i> -");
        gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(window->m_builder, "image-battery-level")), GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON);
    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(window->m_builder, "label-battery-level")), "- <i> Device battery is ok </i> -");
        gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(window->m_builder, "image-battery-level")), GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);
    }

    if (window->m_isCMSInRange)
    {
        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(window->m_builder, "label-CMS-status")), "- <i>CMS/DRL is in range </i> -");
        gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(window->m_builder, "image-CMS-status")), GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);
    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(window->m_builder, "label-CMS-status")), "- <i>CMS/DRL is not in range </i> -");
        gtk_image_set_from_stock(GTK_IMAGE(gtk_builder_get_object(window->m_builder, "image-CMS-status")), GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON);
    }
    gtk_label_set_markup(GTK_LABEL(gtk_builder_get_object(window->m_builder, "label-error-message")), (window->m_sErrorMessage).c_str());

    window->m_isChanged = false;
    return TRUE;
}

bool CDriverBioSemiActiveTwo::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_acquisitionStopped = false;

    if (m_driverCtx.isConnected()) { return false; }

    m_callback = &callback;

    if (!m_bridge.open())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not open the device.\n";
        return false;
    }

    if (!m_bridge.start())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not start the device.\n";
        return false;
    }

    // wait to be sure we get the first packet from which we deduce the actual channel count and other initial configuration.
    System::Time::sleep(500);

    const int byteRead = m_bridge.read();
    if (byteRead < 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "An error occured while reading first data packet from device !\n";
        m_bridge.close();
        return false;
    }

    if (!m_bridge.discard()) // we discard the samples.
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "An error occured while dropping unused samples at initialization time.\n";
        m_bridge.close();
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Bridge initialized with: [SF:" << m_bridge.getSamplingFrequency()
            << "] [CH:" << m_bridge.getChannelCount()
            << "] [MKII:" << m_bridge.isDeviceMarkII()
            << "] [CMInRange:" << m_bridge.isCMSInRange()
            << "] [LowBat:" << m_bridge.isBatteryLow() << "]\n";

    if (m_bridge.isBatteryLow()) { m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Device battery is low !\n"; }

    // the sample buffer is resized to get samples from ALL channels even if the user selected
    // less channels. We will adjust the content when calling setSamples(...)
    // in case the user required more channels than the number available in the stream, we will
    // add 0-padding.
    const size_t nChannelInStream  = m_bridge.getChannelCount();
    const size_t nChannelRequested = m_header.getChannelCount();
    m_samples.clear();
    m_samples.resize(nChannelRequested, 0);
    if (nChannelRequested > nChannelInStream)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning <<
                "The required channel count cannot be reached in current device configuration (data stream contains " << m_bridge.getChannelCount() <<
                " channels). Please check the device speed mode and setup capabilities. Channels with no data will be filled with zeros.\n";
    }

    m_nSample = 0;
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Driver initialized...\n";

    //Check speed mode: speed modes 1, 2 and 3 should not be used for acquisition
    if (m_bridge.getSpeedMode() < 4)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Speed modes 1 to 3 are designed to realize daisy chained montages, they should not be used for acquisition.";
        return false;
    }

    // Rename EX channels after settings were saved 
    if (m_bridge.isUseEXChannels())
    {
        int j = 1;
        for (size_t i = m_header.getChannelCount() - BIOSEMI_ACTIVETWO_EXCHANNELCOUNT; i < m_header.getChannelCount(); ++i, ++j)
        {
            m_header.setChannelName(i, ("EX " + std::to_string(j)).c_str());
            m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Channel name: " << m_header.getChannelName(i) << "\n";
        }
    }

    m_infoWindow = new SInformationWindow();

    // Initialize information window
    // n.b. do all gtk stuff from callbacks to avoid threading issues.
    gdk_threads_add_idle(setup_information_window_callback, this);

    //Launch idle loop: update the information window in a separate glib thread
    gdk_threads_add_idle(information_window_callback, m_infoWindow);

    return true;
}

bool CDriverBioSemiActiveTwo::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    m_startTime       = System::Time::getTime();
    m_lastWarningTime = 0;

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Acquisition started...\n";
    return true;
}

bool CDriverBioSemiActiveTwo::loop()
{
    if (m_acquisitionStopped || !m_driverCtx.isConnected()) { return true; }

    if (m_driverCtx.isStarted())
    {
        //size_t nChannelInStream  = m_bridge.getChannelCount();
        const size_t nChannelRequested = m_header.getChannelCount();

        //uint32_t max = (nChannelRequested > nChannelInStream) ? nChannelRequested : nChannelInStream;
        const uint32_t max = nChannelRequested;
        const int byteRead = m_bridge.read();
        if (byteRead > 0)
        {
            for (uint32_t i = 0; i < m_bridge.getAvailableSampleCount(); ++i)
            {
                // we consume one sample per channel, values are given in uV
                if (!m_bridge.consumeOneSamplePerChannel(&m_samples[0], max))
                {
                    std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                    m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Something bad happened while consuming samples from device.\n";
                    m_infoWindow->m_isChanged     = true;
                    m_infoWindow->m_sErrorMessage = "<span color=\"darkred\">Something bad happened while consuming samples from device.</span>\n";
                    if (m_bridge.getLastError() == BioSemiError_SyncLost)
                    {
                        m_infoWindow->m_sErrorMessage += "\t <span color=\"darkred\"> Synchronization lost during acquisition. Fiber optic may be loose or damaged.</span>";
                        m_driverCtx.getLogManager() << "\t >Synchronization lost during acquisition. Fiber optic may be loose or damaged.\n";
                    }
                    if (m_bridge.getLastError() == BioSemiError_BufferOverflow)
                    {
                        m_infoWindow->m_sErrorMessage += "\t <span color=\"darkred\"> Buffer overflow. Please check that you have enough CPU and memory available to run the acquisition server at full speed before retrying.</span>";
                        m_driverCtx.getLogManager() << "\t > Buffer overflow. Please check that you have enough CPU and memory available to run the acquisition server at full speed before retrying.\n";
                    }
                    m_acquisitionStopped = true;
                    return true;
                }

                // this call uses the header's channel count, so it will naturally take the first samples (not necessarily all channels).
                m_callback->setSamples(&m_samples[0], 1);

                // triggers:
                // we simply send OVTK_StimulationId_Label_X where X is the trigger index between 1 and 16
                // We don't handle rising and falling edges.
                for (uint32_t j = 0; j < m_triggers.size(); ++j)
                {
                    if (m_bridge.getTrigger(j) != m_triggers[j])
                    {
                        m_triggers[j]       = m_bridge.getTrigger(j);
                        const uint64_t date = (1LL << 32) / m_bridge.getSamplingFrequency(); // date is relative to the buffer start. I only have one sample in the buffer so it's fairly simple
                        m_stimSet.push_back(OVTK_StimulationId_Label(j+1), date, 0);
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Trigger " << j + 1 << "/16 has switched to " << m_triggers[j] << "\n";
                    }
                }

                // "CMS/DRL in range" warning, once every 2 seconds max
                /*
                From: Coen (BioSemi):

                The current flow via the DRL is constantly monitored by the safety circuitry inside the AD-box. The current flow is limited to 50 uA (IEC 601 safety limit).
                If the current runs into the limit, the CMS/DRL circuit cannot keep the Common Mode value within its normal working range, and the blue LED turns off.
        
                The safety circuitry reacts on this error situation by shutting the power supply to ALL active electrodes off. Consequently, no meaningful signals can be measured so long as the blue LED is off. 
                The circuit operation described above implies that any electrode can be the cause of a CM out of range error. 
                Examples of errors are broken wires, bad connector contacts (pollution of connector with gel), defect IC inside the electrode, 
                bare electrode wire contacting the subject (damaged cable isolation) etc.. For example, if one of the active electrode wires is broken,
                the electrode input circuit is not anymore biased correctly, and the input resistance may fall below its specified value of 10^12 Ohm.
                The resultant extra input current is detected by the CMS/DRL circuit, and the blue LED goes off. 
                Save operation of the system is ensured because the power supply to the active electrodes is only restored if ALL electrodes connected to the subject work correctly.
                In other words, both cap en EX electrodes can in principle cause CM out of range errors. 
                */

                bool warningDisplayed = false;
                if (!m_bridge.isCMSInRange())
                {
                    // we print a warning message once every 2secs maximum
                    if (System::Time::getTime() > m_lastWarningTime + 2000)
                    {
                        warningDisplayed = true;
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(" << ((System::Time::getTime() - m_startTime) / 1000) << "') CMS/DRL is not in range. For safety purpose, any active electrode connected has been shut down and signals should not be used.\n";
                        m_driverCtx.getLogManager() << "\t  The corresponding sample range has been tagged with OVTK_StimulationId_SegmentStart and OVTK_StimulationId_SegmentStop.\n";
                        m_driverCtx.getLogManager() << "\t  Possible causes include broken wires, damaged cable isolation, bad connector contacts, defect IC inside the electrode.\n";
                        {
                            std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                            m_infoWindow->m_isCMSInRange = false;
                            m_infoWindow->m_isChanged    = true;
                        }
                    }

                    // the possibly incorrect sample range is tagged using OVTK_StimulationId_SegmentStart and OVTK_StimulationId_SegmentStop
                    // CM is now not in range
                    if (m_bCMCurrentlyInRange)
                    {
                        const uint64_t date = (1LL << 32) / m_bridge.getSamplingFrequency();
                        m_stimSet.push_back(OVTK_StimulationId_SegmentStart, date, 0);
                    }
                    m_bCMCurrentlyInRange = false;
                    m_bCMSBackInRange     = false;
                }
                else
                {
                    // CMS/DRL is now back in range
                    if (!m_bCMCurrentlyInRange)
                    {
                        m_lastCmsBackInRange = System::Time::getTime();
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "isCMSInRange \n";
                        const uint64_t date = (1LL << 32) / m_bridge.getSamplingFrequency();
                        m_stimSet.push_back(OVTK_StimulationId_SegmentStop, date, 0);
                        m_bCMCurrentlyInRange = true;
                    }
                        // CMS is considered "back in range" if it stayed in range for more than 500ms 
                        // -> avoids changing the gtk markup too often (which would not be readable for the user nor efficient)
                    else if (!m_bCMSBackInRange && (System::Time::getTime() > m_lastCmsBackInRange + 100))
                    {
                        m_bCMSBackInRange = true;
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Back in range \n";
                        {
                            std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                            m_infoWindow->m_isCMSInRange = true;
                            m_infoWindow->m_isChanged    = true;
                        }
                    }
                }

                if (m_bridge.isBatteryLow())
                {
                    // we print a warning message once every 2secs maximum
                    if (System::Time::getTime() > m_lastWarningTime + 2000)
                    {
                        warningDisplayed = true;
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(" << ((System::Time::getTime() - m_startTime) / 1000) << "') Device battery is low !\n";
                        {
                            std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                            m_infoWindow->m_isBatteryLow = true;
                            m_infoWindow->m_isChanged    = true;
                        }
                    }
                    m_bBatteryCurrentlyOk = false;
                    m_bBatteryBackOk      = false;
                }
                else
                {
                    if (!m_bBatteryCurrentlyOk)
                    {
                        m_lastBatteryLow = System::Time::getTime();
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "(" << ((System::Time::getTime() - m_startTime) / 1000) << "') Device battery seems ok.\n";
                        m_bBatteryCurrentlyOk = true;
                    }
                        // CMS is considered "back in range" if it stayed in range for more than 500ms 
                        // -> avoids changing the gtk markup too often (which would not be readable for the user nor efficient)
                    else if (!m_bBatteryBackOk && (System::Time::getTime() > m_lastBatteryLow + 100))
                    {
                        m_bBatteryBackOk = true;
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Back in range \n";
                        {
                            std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                            m_infoWindow->m_isBatteryLow = false;
                            m_infoWindow->m_isChanged    = true;
                        }
                    }
                }

                if (warningDisplayed) { m_lastWarningTime = System::Time::getTime(); }

                m_callback->setStimulationSet(m_stimSet);
                m_stimSet.clear();
            }
        }
        if (byteRead < 0)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "An error occured while reading data from device !\n";
            m_acquisitionStopped = true;
            {
                std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                m_infoWindow->m_sErrorMessage = "<span color=\"darkred\">An error occured while reading data from device !</span>";
                m_infoWindow->m_isChanged     = true;
            }
            return true;
        }

        m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    }
    else
    {
        // acquisition is not started, but device is.
        int byteRead = m_bridge.read();
        while (byteRead > 0)
        {
            if (!m_bridge.discard()) // we discard the samples.
            {
                m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "An error occured while dropping samples.\n";
                m_acquisitionStopped = true;
                {
                    std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                    m_infoWindow->m_sErrorMessage = "<span color=\"darkred\">An error occured while dropping samples.</span>";
                    m_infoWindow->m_isChanged     = true;
                }
                return true;
            }
            byteRead = m_bridge.read();
        }
        if (byteRead < 0)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "An error occured while reading data from device (drop)!\n";
            m_acquisitionStopped = true;
            {
                std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);
                m_infoWindow->m_sErrorMessage = "<span color=\"darkred\">An error occured while reading data from device (drop)!</span>";
                m_infoWindow->m_isChanged     = true;
            }
            return true;
        }
    }

    return true;
}

bool CDriverBioSemiActiveTwo::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Acquisition stopped...\n";

    return true;
}

bool CDriverBioSemiActiveTwo::uninitialize()
{
    {
        std::lock_guard<std::mutex> lock(m_infoWindow->m_mutex);

        m_infoWindow->m_isChanged          = true;
        m_infoWindow->m_isAcquisitionEnded = true;
        m_infoWindow                       = nullptr; // The callback will free the memory
    }

    // Rename EX channels as "" so that the names are not saved as settings
    if (m_bridge.isUseEXChannels())
    {
        for (size_t i = m_header.getChannelCount() - m_bridge.getEXChannelCount(); i < m_header.getChannelCount(); ++i)
        {
            m_header.setChannelName(i, "");
            m_header.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Micro);
        }
    }

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }
    if (!m_bridge.close())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not close the device.\n";
        return false;
    }
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Driver uninitialized...\n";
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverBioSemiActiveTwo::configure()
{
    CConfigurationBioSemiActiveTwo config(Directories::getDataDir() + "/applications/acquisition-server/interface-BioSemi-ActiveTwo.ui", m_useEXChannel);

    if (config.configure(m_header))
    {
        m_bridge.setUseEXChannels(m_useEXChannel);
        if (m_bridge.isUseEXChannels()) { m_header.setChannelCount(m_header.getChannelCount() + BIOSEMI_ACTIVETWO_EXCHANNELCOUNT); }

        m_settings.save();

        return true;
    }
    return false;
}
/*
size_t CDriverBioSemiActiveTwo::getChannelCount()
{
    if(m_useEXChannel)
    {
        return m_header.getChannelCount() + m_bridge.getEXChannelCount(); 
    }
    else { return m_header.getChannelCount(); }
}
*/

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
