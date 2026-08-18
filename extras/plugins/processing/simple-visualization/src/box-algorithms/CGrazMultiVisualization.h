///-------------------------------------------------------------------------------------------------
/// 
/// \file CGrazMultiVisualization.hpp
/// \brief Classes for the Generalized Graz Visualization box.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 28/05/2019.
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
//#include <visualization-toolkit/ovviz_all.h>


#include <string>
#include <vector>

namespace TCPTagging {
class IStimulusSender; // fwd declare
}  // namespace TCPTagging

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
/// <summary>   The class CGrazMultiVisualization describes the box Multimodal Graz Visuallisation. </summary>
/// <seealso cref="Toolkit::TBoxAlgorithm{IBoxAlgorithm}" />
class CGrazMultiVisualization final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    void FlushQueue();                  // Sends all accumulated stimuli to the TCP Tagging

    static const size_t NON_MODALITY_SETTINGS_COUNT = 6;

protected:
    /// <summary>   Enumeration of States. </summary>
    enum class EStates { Idle, Cross, Black, Instruction, Feedback };


    /// <summary>   Initialize GTK Images. </summary>
    /// <param name="paths">    The pathsof the image file.</param>
    /// <returns>   True if it succeeds, false if it fails. </returns>
    bool initImages(const std::vector<std::string>& paths);

    /// <summary>   Update the box state according to the stimulation received. </summary>
    /// <param name="stimulation">The stimulation received.</param>
    void setStimulation(uint64_t stimulation);

    /// <summary>   Update the amplitudesaccording to the new amplitude received in buffer. </summary>
    /// <param name="buffer">   The buffer with amplitudes.</param>
    void setMatrixBuffer(const double* buffer);

    /// <summary> Draw the reference. </summary>
    /// 
    ///     ┌──────────────────────────────┐ \n
    ///     │  │        │       │       │  │ \n
    ///     │  │        │       │       │  │ \n
    ///     │  │        │       │       │  │ \n
    ///     │  │        │       │       │  │ \n
    ///     │ ─┴─      ─┴─     ─┴─     ─┴─ │ \n
    ///     │ Im01    Im02    Im03    Im04 │ \n
    ///     └──────────────────────────────┘
    void drawReference() const;

    /// <summary> Draw Cross on Screen.</summary>
    void drawCross() const;

    /// <summary> Draw the actual modality in center.</summary>
    void drawModality() const;

    /// <summary> Draw the Feedback bar.</summary>
    /// 
    ///     ┌──────────────────────────────┐ \n
    ///     │  │        │       │       │  │ \n
    ///     │  │        │       │       │  │ \n
    ///     │  │        │      ┌┴┐      │  │ \n
    ///     │  │        │      │ │      │  │ \n
    ///     │ ─┴─      ─┴─     ┴─┴     ─┴─ │ \n
    ///     │ Im01    Im02    Im03    Im04 │ \n
    ///     └──────────────────────────────┘
    void drawBar() const;

    /// <summary> Draw the accuracy on top left.</summary>
    void drawAccuracy() const;

    /// <summary> Update the confusion matrix draw in the corner.</summary>
    void updateConfusionMatrix() const;

    /// <summary> Get the mean of last predictions.</summary>
    /// <param name="all">  All previous predictions is used if true, only <see cref="m_nbPredictionsMin" /> if false. </param>
    void aggregatePredictions(bool all);

    /// <summary>   Return some infos about the class.</summary>
    /// <returns>   A string with infos.</returns>
    std::string infos() const;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_GeneralizedGrazVisualization)

    //********** Variables **********
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CGrazMultiVisualization> m_stimDecoder;
    Toolkit::TStreamedMatrixDecoder<CGrazMultiVisualization> m_classifDecoder;
    Toolkit::TStreamedMatrixEncoder<CGrazMultiVisualization> m_barSizeEncoder;
    Toolkit::TStreamedMatrixEncoder<CGrazMultiVisualization> m_confusionEncoder;
    CStimulationSet* m_iStim = nullptr; // Input StimulationSet Pointer
    CMatrix* m_iMatrix       = nullptr; // Input Matrix pointer
    CMatrix* m_oBarSize      = nullptr; // Outpout bar size matrix pointer (in percent)
    CMatrix* m_oConfusion    = nullptr; // Outpout confusion Matrix pointer

    //***** Interface *****
    class CGrazMultiVisuWindow *m_window = nullptr;
    std::vector<double> m_barScales;                                // 0 to 1 for each bar (depends of the feedback if only positive feedback or not)
    EStates m_state   = EStates::Idle;                              // Actual state
    int m_modality    = -1;                                         // Actual Modality
    int m_vote        = -1;                                         // Computed Modality

    //***** Settings *****
    bool m_showInstruction = true,
         m_delayFeedback   = false,
         m_showAccuracy    = false;

    size_t m_feedbackMode     = 0,
           m_nbPredictionsMin = 5,
           m_nbModality       = 2,
           m_currModality     = 0;

    std::vector<uint64_t>
    m_stimlist; // List of stimulations (OVTK_GDF_End_Of_Trial, OVTK_GDF_End_Of_Session, OVTK_GDF_Cross_On_Screen, OVTK_GDF_Feedback_Continuous)

    //***** TCP Tagging *****
    std::vector<uint64_t> m_stimuliQueue;
    TCPTagging::IStimulusSender* m_stimulusSender = nullptr;

    //***** Other *****
    std::vector<std::vector<double>> m_amplitudes;  // All amplitudes the current trial
    bool m_twoValueInput = false;
    friend class CGrazMultiVisuWindow;
};

/// <summary>   Listener of the box  Multimodal Graz Visuallisation. </summary>
/// <seealso cref="Toolkit::TBoxListener{IBoxListener}" />
class CGrazMultiVisualizationListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    /// <summary> Action when we add a setting (we copy the previous setting value. </summary>
    /// <param name="box">  The box to listen.</param>
    /// <param name="idx">  The idx of the new setting.</param>
    /// <returns>   True if it succeeds, false if it fails. </returns>
    bool onSettingAdded(Kernel::IBox& box, const size_t idx) override
    {
        CString value;
        if(idx > CGrazMultiVisualization::NON_MODALITY_SETTINGS_COUNT + 1) {
            //copy the value of the previous modality
            box.getSettingDefaultValue(idx - 2, value);
            box.setSettingDefaultValue(idx, value);
            box.setSettingValue(idx, value);

            box.getSettingDefaultValue(idx - 1, value);
            box.addSetting("", OV_TypeId_Filename, value);
            box.setSettingDefaultValue(idx + 1, value);
            box.setSettingValue(idx + 1, value);
            checkSettingNames(box);
        }

        return true;
    }

    /// <summary> Action when when remove a setting (we remove the associated setting. </summary>
    /// <param name="box">  The box to listen.</param>
    /// <param name="idx">  The idx of the deleted setting.</param>
    /// <returns>   True if it succeeds, false if it fails. </returns>
    bool onSettingRemoved(Kernel::IBox& box, const size_t idx) override
    {
        // Remove also the associated setting in the other slot
        const size_t modalityNumber = (idx - CGrazMultiVisualization::NON_MODALITY_SETTINGS_COUNT);
        if (modalityNumber % 2 == 0) { box.removeSetting(idx); }
        else { box.removeSetting(idx - 1); }
        checkSettingNames(box);
        return true;
    }

protected:
    /// <summary>   This function is used to make sure the setting names and types are correct (if you remove one middle setting).</summary>
    /// <param name="box">  The box to listen.</param>
    /// <returns>   True if it succeeds, false if it fails. </returns>
    bool checkSettingNames(Kernel::IBox& box) const
    {
        size_t idx = 1;
        for (size_t i = CGrazMultiVisualization::NON_MODALITY_SETTINGS_COUNT; i < box.getSettingCount() - 1; i += 2) {
            const std::string tmp = std::to_string(idx);
            box.setSettingName(i, ("Stimulation modality " + tmp).c_str());
            box.setSettingType(i, OV_TypeId_Stimulation);
            box.setSettingName(i + 1, ("Image modality " + tmp).c_str());
            box.setSettingType(i + 1, OV_TypeId_Filename);
            idx++;
        }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};


/// <summary>   Descriptor of the box  Multimodal Graz Visuallisation. </summary>
/// <seealso cref="IBoxAlgorithmDesc" />
class CGrazMultiVisualizationDesc final : public IBoxAlgorithmDesc
{
public:
    CString getName() const override { return "Multimodal Graz visualization"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }

    CString getShortDescription() const override { return "Generalization of visualization plugin for the Graz experiment"; }

    CString getDetailedDescription() const override { return "Generalization of Visualization/Feedback plugin for the Graz experiment"; }

    CString getCategory() const override { return "Visualization/Presentation"; }
    CString getVersion() const override { return "1.0"; }
    void release() override { }

    CIdentifier getCreatedClass() const override { return Box_GeneralizedGrazVisualization; }
    CString getStockItemName() const override { return "gtk-fullscreen"; }
    IPluginObject* create() override { return new CGrazMultiVisualization(); }

    IBoxListener* createBoxListener() const override { return new CGrazMultiVisualizationListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool hasFunctionality(const EPluginFunctionality identifier) const override { return identifier == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Amplitude", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Show instruction", OV_TypeId_Boolean, "true");
        prototype.addSetting("Feedback display mode", TypeId_FeedbackMode, "Positive Only");
        prototype.addSetting("Delay feedback", OV_TypeId_Boolean, "false");
        prototype.addSetting("Show accuracy", OV_TypeId_Boolean, "false");
        prototype.addSetting("Predictions to integrate", OV_TypeId_Integer, "5");
        prototype.addSetting("Image no instruction", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/graz/none.png");
        prototype.addSetting("Stimulation modality 1", OV_TypeId_Stimulation, "OVTK_GDF_Left");
        prototype.addSetting("Image modality 1", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/graz/left.png");
        prototype.addSetting("Stimulation modality 2", OV_TypeId_Stimulation, "OVTK_GDF_Right");
        prototype.addSetting("Image modality 2", OV_TypeId_Filename, "${Path_Data}/plugins/simple-visualization/graz/right.png");
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);

        prototype.addOutput("Displayed Bar Size", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Confusion Matrix", OV_TypeId_StreamedMatrix);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_GeneralizedGrazVisualizationDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
