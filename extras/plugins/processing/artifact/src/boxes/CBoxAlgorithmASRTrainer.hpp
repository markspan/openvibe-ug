///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmASRProcessor.hpp
/// \brief Classes for the box ASR Processor.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 08/12/2020.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <geometry/artifacts/CASR.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Artifact {

//-------------------------------------------------------------------------------------------------
/// <summary>   The class CBoxAlgorithmASRTrainer describes the box Artifact Subspace Reconstruction (ASR) Trainer. </summary>
class CBoxAlgorithmASRTrainer final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ASR_Trainer)

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxAlgorithmASRTrainer> m_stimulationDecoder; ///< Input Stimulation Decoder
    Toolkit::TSignalDecoder<CBoxAlgorithmASRTrainer> m_signalEncoder;           ///< Input Signal Encoder
    Toolkit::TStimulationEncoder<CBoxAlgorithmASRTrainer> m_stimulationEncoder; ///< Output Stimulation Encoder

    //***** Pointers *****
    CMatrix* m_iMatrix              = nullptr;  ///< Input Matrix pointer
    CStimulationSet *m_iStimulation = nullptr,  ///< Stimulation receiver
                    *m_oStimulation = nullptr;  ///< Stimulation sender

    //***** Settings *****
    std::string m_filename;                                     ///< Filename of ASR Model
    uint64_t m_stimulationName = OVTK_StimulationId_Train;      ///< Name of stimulation to check for train launch
    Geometry::EMetric m_metric = Geometry::EMetric::Euclidian;  ///< Metric for ASR
    double m_ratio             = 1.0;                           ///< Ratio of channel to reconstruct for ASR
    double m_rejection         = 5.0;                           ///< Rejection limit of threshold for ASR

    //***** Misc *****
    std::vector<Eigen::MatrixXd> m_dataset;                     ///< Dataset stack
    bool m_isTrain = false;                                     ///< <c>True</c> if train is already done, <c>False</c> otherwise

    bool train();
};

//-------------------------------------------------------------------------------------------------
/// <summary>   Listener of the box Artifact Subspace Reconstruction (ASR) Trainer. </summary>
class CBoxAlgorithmASRTrainerListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, OV_UndefinedIdentifier)
};

//-------------------------------------------------------------------------------------------------
/// <summary>   Descriptor of the box Artifact Subspace Reconstruction (ASR) Trainer. </summary>
class CBoxAlgorithmASRTrainerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "ASR Trainer"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Artifact Subspace Reconstruction (ASR) Trainer."; }
    CString getDetailedDescription() const override { return "Artifact Subspace Reconstruction (ASR) Trainer."; }
    CString getCategory() const override { return "Artifact"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_ASR_Trainer; }
    IPluginObject* create() override { return new CBoxAlgorithmASRTrainer; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmASRTrainerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations",OV_TypeId_Stimulations);
        prototype.addInput("Input Signal", OV_TypeId_Signal);

        prototype.addOutput("Train-completed Flag",OV_TypeId_Stimulations);

        prototype.addSetting("Filename to save model", OV_TypeId_Filename, "${Player_ScenarioDirectory}/ASR-model.xml");
        prototype.addSetting("Train trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Train");
        prototype.addSetting("Metric", TypeId_Metric, toString(Geometry::EMetric::Euclidian).c_str());
        prototype.addSetting("Channel ratio to reconstruct", OV_TypeId_Float, "1");
        prototype.addSetting("Rejection limit", OV_TypeId_Float, "5");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ASR_Trainer_Desc)
};

} // namespace Artifact
}  // namespace Plugins
}  // namespace OpenViBE
