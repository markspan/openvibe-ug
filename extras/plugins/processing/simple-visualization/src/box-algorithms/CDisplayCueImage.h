///-------------------------------------------------------------------------------------------------
/// 
/// \file CDisplayCueImage.hpp
/// \brief Classes for the Box Display cue image.
/// \author Joan Fruitet, Jussi T. Lindgren (Inria Sophia, Inria Rennes).
/// \version 1.2.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "CImageDisplayObject.h"

#include <atomic>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
//#include "../utils.hpp"

namespace TCPTagging {
class IStimulusSender; // fwd declare
}  // namespace TCPTagging

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class CDisplayCueImage final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    ~CDisplayCueImage() noexcept override = default;

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    uint64_t getClockFrequency() override { return (128LL << 32); }             // 128hz
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;
    void Redraw();
    void Resize(size_t width, size_t height);

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_DisplayCueImage)

    void FlushQueue();                  // Sends all accumulated stimuli to the TCP Tagging

    static const size_t NON_CUE_SETTINGS_COUNT = 3;  // fullscreen + scale + clear 
    static const size_t FIXED_SETTING_COUNT = 5; // non cue settings + first pair stim/img

protected:

    CImageDisplayObject* m_imageDisplayObject = nullptr;

    Toolkit::TStimulationDecoder<CDisplayCueImage> m_stimulationDecoder;
    Toolkit::TStimulationEncoder<CDisplayCueImage> m_stimulationEncoder;

    // For the display of the images:
    bool m_imageRequested  = false;        //when true: a new image must be drawn
    int m_requestedImageId = -1;            //ID of the requested image. -1 => clear the screen

    bool m_imageDrawn  = false;            //when true: the new image has been drawn
    int m_drawnImageId = -1;                //ID of the drawn image. -1 => clear the screen

    // Round trip of a request through the GUI. Redraw() hands m_requestedImageId to the view, which
    // raises m_cueDisplayed once the frame carrying it has been swapped; processClock() picks that up
    // on the player thread. The flag is shared rather than a member so a frame swapped after the box
    // is gone cannot write into freed memory.
    bool m_displayPending     = false;  ///< A request is with the GUI and not yet confirmed on screen.
    int  m_displayRequestedId = -1;     ///< Slot that request was for. -1 => clear the screen.
    std::shared_ptr<std::atomic<bool>> m_cueDisplayed = std::make_shared<std::atomic<bool>>(false);

    // Data corresponding to each cue image. Could be refactored to a vector of structs.
    std::vector<uint64_t> m_stimulationsIds;
    std::vector<std::string> m_imageNames;

    /*GdkColor m_backgroundColor = InitGDKColor(0, 0, 0, 0);
    GdkColor m_foregroundColor = InitGDKColor(0, 65535, 65535, 65535);
    */

    //Settings
    size_t m_numberOfCues             = 0;
    uint64_t m_clearScreenStimulation = 0;
    bool m_fullScreen                 = false;
    bool m_scaleImages                = false;

    //Start and end time of the last buffer
    uint64_t m_startTime           = 0;
    uint64_t m_endTime             = 0;
    uint64_t m_lastOutputChunkDate = 0;

    //We save the received stimulations
    CStimulationSet m_pendingStimulationSet;

    // For queuing stimulations to the TCP Tagging
    std::vector<uint64_t> m_stimuliQueue;
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;

private:
    // VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
};

class CDisplayCueImageListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingAdded(Kernel::IBox& box, const size_t index) override
    {
        if (index < CDisplayCueImage::FIXED_SETTING_COUNT) {
            return true;  // Skipping this. Call triggered by prototype
        }
        const size_t previousCues = ((index - 1) - CDisplayCueImage::FIXED_SETTING_COUNT) / 2 + 1;
        const size_t cueNumber    = previousCues + 1;

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << cueNumber;

        std::string value = "${Path_Data}/plugins/simple-visualization/p300-magic-card/" + ss.str() + ".png";

        box.setSettingDefaultValue(index, value.c_str());
        box.setSettingValue(index, value.c_str());

        value = "OVTK_StimulationId_Label_" + ss.str();
        box.addSetting("", OV_TypeId_Stimulation, value.c_str());
        box.setSettingDefaultValue(index + 1, value.c_str());
        box.setSettingValue(index + 1, value.c_str());

        checkSettingNames(box);

        return true;
    }

    bool onSettingRemoved(Kernel::IBox& box, const size_t index) override
    {
        this->getLogManager() << Kernel::LogLevel_Info << "CDisplayCueImageListener::OnSettingRemoved at index" << index << "\n";
        
        if (index < CDisplayCueImage::FIXED_SETTING_COUNT) {
            this->getLogManager() << Kernel::LogLevel_Warning << "CDisplayCueImageListener::OnSettingRemoved at index" << index << ": Skipping this. Call triggered by prototype\n";
            return false;
        }
        // Remove also the associated setting in the other slot
        const size_t indexNumber = (index - CDisplayCueImage::FIXED_SETTING_COUNT);

        if (indexNumber % 2 == 0) {
            // This was the 'cue image' setting, remove 'stimulation setting'
            // when onSettingRemoved is called, index has already been removed, so using it will effectively mean 'remove next setting'.
            box.removeSetting(index);
        }
        else {
            // This was the 'stimulation setting'. Remove the 'cue image' setting.
            box.removeSetting(index - 1);
        }

        checkSettingNames(box);

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())

private:
    // This function is used to make sure the setting names and types are correct
    bool checkSettingNames(Kernel::IBox& box) const
    {
        for (size_t i = CDisplayCueImage::FIXED_SETTING_COUNT; i < box.getSettingCount() - 1; i += 2) {
            const std::string idx = std::to_string(i / 2);
            box.setSettingName(i, ("Cue Image " + idx).c_str());
            box.setSettingType(i, OV_TypeId_Filename);
            box.setSettingName(i + 1, ("Stimulation " + idx).c_str());
            box.setSettingType(i + 1, OV_TypeId_Stimulation);
        }
        return true;
    }
};

/// <summary> Plugin's description. </summary>
class CDisplayCueImageDesc final : public IBoxAlgorithmDesc
{
public:
    CString getName() const override { return "Display cue image"; }
    CString getAuthorName() const override { return "Joan Fruitet, Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria Sophia, Inria Rennes"; }
    CString getShortDescription() const override { return "Display cue images when receiving stimulations"; }

    CString getDetailedDescription() const override
    {
        return "Display cue images when receiving specified stimulations. Forwards the stimulations to the AS using TCP Tagging.";
    }

    CString getCategory() const override { return "Visualization/Presentation"; }
    CString getVersion() const override { return "1.2"; }
    void release() override { }

    CIdentifier getCreatedClass() const override { return Box_DisplayCueImage; }

    CString getStockItemName() const override { return "gtk-fullscreen"; }
    IPluginObject* create() override { return new CDisplayCueImage(); }
    IBoxListener* createBoxListener() const override { return new CDisplayCueImageListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Stimulations (deprecated)", OV_TypeId_Stimulations);
        prototype.addSetting("Display images in full screen", OV_TypeId_Boolean, "false");
        prototype.addSetting("Scale images to fit", OV_TypeId_Boolean, "false");
        prototype.addSetting("Clear screen Stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_VisualStimulationStop");
        prototype.addSetting("Cue Image 1", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/p300-magic-card/01.png");
        prototype.addSetting("Stimulation 1", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_DisplayCueImageDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
