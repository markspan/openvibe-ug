///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmFeaturesSelection.hpp
/// \brief Classes of the Box Features Selection Trainer.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/02/2020.
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

#include "ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "algorithm/CMRMR.hpp"
#include <vector>

#define OV_AttributeId_Box_FlagIsUnstable               CIdentifier(0x666FFFFF, 0x666FFFFF)

namespace OpenViBE {
namespace Plugins {
namespace FeaturesSelection {
/// <summary>   The class CBoxAlgorithmFeaturesSelection describes the box Features Selection Trainer. </summary>
class CBoxAlgorithmFeaturesSelection final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_FeaturesSelection)

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxAlgorithmFeaturesSelection> m_stimDecoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmFeaturesSelection> m_stimEncoder;
    std::vector<Toolkit::TFeatureVectorDecoder<CBoxAlgorithmFeaturesSelection>> m_featuresDecoders;
    std::vector<CMatrix*> m_iFeatures;                          // Input Matrix pointer
    CStimulationSet *m_iStim = nullptr, *m_oStim = nullptr;     // Stimulation receiver/sender

    //***** Settings *****
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_Info;       // Log Level
    uint64_t m_stimName          = OVTK_StimulationId_Train;    // Name of stimulation to check for train lunch
    std::string m_filename;
    EFeatureSelection m_method = EFeatureSelection::MRMR;

    size_t m_nbClass = 2;                                       // Number of input classes
    bool m_isTrain   = false;

    // mRMR Settings
    CMRMR m_selector;
    bool m_doDiscretization  = true;                            // Check if we make Discretization
    double m_threshold       = 0.0;                             // Threshold for Discretisation
    size_t m_nFinalFeatures  = size_t(-1);                      // Number of Features in output
    EMRMRMethod m_mRMRMethod = EMRMRMethod::MID;                // mRMR Method
    std::vector<size_t> m_result;                               // mRMR Result

    bool writeConfig();
};


/// <summary> Listener of the box Features Selection Trainer. </summary>
class CBoxAlgorithmFeaturesSelectionListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_FeatureVector);
        box.setInputName(index, ("Class " + std::to_string(index)).c_str());
        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary> Descriptor of the box Features Selection Trainer. </summary>
class CBoxAlgorithmFeaturesSelectionDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Features Selection Trainer"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Apply a Features Selection Algorithm"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Features Selection"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_FeaturesSelection; }
    IPluginObject* create() override { return new CBoxAlgorithmFeaturesSelection; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmFeaturesSelectionListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Train-Start Flag",OV_TypeId_Stimulations);
        prototype.addInput("Class 1",OV_TypeId_FeatureVector);
        prototype.addInput("Class 2",OV_TypeId_FeatureVector);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        prototype.addOutput("Train-Completed Flag",OV_TypeId_Stimulations);

        prototype.addSetting("Log Level", OV_TypeId_LogLevel, "Information");
        prototype.addSetting("Train trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Train");
        prototype.addSetting("Filename to save Feature Selection", OV_TypeId_Filename, "${Player_ScenarioDirectory}/features-selected.xml");
        prototype.addSetting("Method", OVP_TypeId_Features_Selection_Method, toString(EFeatureSelection::MRMR).c_str());
        prototype.addSetting("Number of features to select", OV_TypeId_Integer, "2");
        prototype.addSetting("Discretisation", OV_TypeId_Boolean, "true");
        prototype.addSetting("Threshold", OV_TypeId_Float, "0.0");
        prototype.addSetting("mRMR Method", OVP_TypeId_mRMR_Method, "MID");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_FeaturesSelectionDesc)
};
}  // namespace FeaturesSelection
}  // namespace Plugins
}  // namespace OpenViBE
