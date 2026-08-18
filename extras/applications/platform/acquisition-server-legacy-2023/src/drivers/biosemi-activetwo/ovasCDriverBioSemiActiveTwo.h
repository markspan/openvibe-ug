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

#pragma once

#ifdef TARGET_HAS_ThirdPartyBioSemiAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include "mCBridgeBioSemiActiveTwo.h"

#include "gtk/gtk.h"

#include <vector>
#include <thread>
#include <mutex>

namespace OpenViBE {
namespace AcquisitionServer {
/**
* SInformationWindow: encapsulate the GtkBuilder used to build the window
* and the information that we want to display in it.
* The use of a structure allows to use a GLib idle loop.
*/
typedef struct SInformationWindow
{
    SInformationWindow() {}
    // Builder of the window
    GtkBuilder* m_builder = nullptr;
    // When set to true, the idle loop is stopped and the window is destroyed
    bool m_isAcquisitionEnded = false;
    // Set to true when at least one of the acquisition window member changed
    bool m_isChanged            = false;
    bool m_isCMSInRange         = false;
    bool m_isBatteryLow         = false;
    std::string m_sErrorMessage = "";

    // The window is changed in a idle loop in function of m_isCMSInRange, 
    // m_isBatteryLow and m_sErrorMessage that change in the driver loop
    // A mutex is necessary to secure the access to the data
    std::mutex m_mutex;
} SInformationWindow;

/**
 * \class CDriverBioSemiActiveTwo
 * \author Mensia Technologies
 */
class CDriverBioSemiActiveTwo final : public IDriver
{
public:

    explicit CDriverBioSemiActiveTwo(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "BioSemi Active Two (MkI and MkII)"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }
    //size_t getChannelCount();

    // Called from gtk callback
    void setupInformationWindow();

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;

    std::vector<uint32_t> m_impedances;
    std::vector<float> m_samples;

    CStimulationSet m_stimSet;

    CBridgeBioSemiActiveTwo m_bridge;

    std::vector<bool> m_triggers;
    uint64_t m_nSample = 0;

    bool m_bCMCurrentlyInRange = false;
    bool m_bBatteryCurrentlyOk = false;
    uint32_t m_lastWarningTime = 0;
    // Used to determine for how long CMS was in range
    uint32_t m_lastCmsBackInRange = 0;
    uint32_t m_lastBatteryLow     = 0;
    // True if CMS is in range for more than 100ms
    bool m_bCMSBackInRange = true;
    bool m_bBatteryBackOk  = true;
    uint32_t m_startTime   = 0;
    bool m_useEXChannel    = false;
    // Set to true 
    bool m_acquisitionStopped = false;

    SInformationWindow* m_infoWindow = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE


#endif // TARGET_HAS_ThirdPartyBioSemiAPI
