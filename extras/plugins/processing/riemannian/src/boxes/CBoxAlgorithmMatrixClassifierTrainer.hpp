///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixClassifierTrainer.hpp
/// \brief Classes for the box Train a Matrix Classifier.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/01/2018.
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

#include <geometry/classifier/IMatrixClassifier.hpp>
#include <geometry/Metrics.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
/// <summary>   The class CBoxAlgorithmMatrixClassifierTrainer describes the box Matrix Classifier Trainer. </summary>
class CBoxAlgorithmMatrixClassifierTrainer final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_MatrixClassifierTrainer)

    static const size_t NON_CLASS_SETTINGS_COUNT = 5; // Train trigger + Filename + Method + Metric + Log Level

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxAlgorithmMatrixClassifierTrainer> m_i0StimulationCodec;
    std::vector<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrixClassifierTrainer>> m_i1MatrixCodec; // Input Signal Codec
    Toolkit::TStimulationEncoder<CBoxAlgorithmMatrixClassifierTrainer> m_o0StimulationCodec;

    //***** Matrices *****
    size_t m_nbClass = 2;                                           // Number of input classes
    std::vector<CMatrix*> m_iMatrix;                                // Input Matrix pointer
    std::vector<std::vector<Eigen::MatrixXd>> m_covs;               // List of Covariance Matrix one class by row

    //***** Stimulations *****
    CStimulationSet *m_iStimulation = nullptr,                      // Stimulation receiver
                    *m_oStimulation = nullptr;                      // Stimulation sender
    uint64_t m_stimulationName      = OVTK_StimulationId_Train;     // Name of stimulation to check for train launch
    std::vector<uint64_t> m_stimulationClassName;                   // Name of stimulation to check for each class
    bool m_isTrain = false;

    //***** Settings *****
    Kernel::ELogLevel m_logLevel          = Kernel::LogLevel_Info;  // Log Level
    Geometry::EMatrixClassifiers m_method = Geometry::EMatrixClassifiers::MDM;
    Geometry::EMetric m_metric            = Geometry::EMetric::Riemann;

    //***** File *****
    CString m_filename;

    bool train();
    bool saveXML(const Geometry::IMatrixClassifier* classifier);
};

/// <summary>   Listener of the box Matrix Classifier Trainer. </summary>
class CBoxAlgorithmMatrixClassifierTrainerListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override;
    bool onInputRemoved(Kernel::IBox& box, const size_t index) override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary>   Descriptor of the box Matrix Classifier Trainer. </summary>
class CBoxAlgorithmMatrixClassifierTrainerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix Classifier Trainer"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Matrix classifier trainer."; }
    CString getDetailedDescription() const override { return "Matrix classifier trainer."; }
    CString getCategory() const override { return "Riemannian Geometry"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_MatrixClassifierTrainer; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrixClassifierTrainer; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmMatrixClassifierTrainerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations",OV_TypeId_Stimulations);
        prototype.addInput("Matrix for class 1",OV_TypeId_StreamedMatrix);
        prototype.addInput("Matrix for class 2",OV_TypeId_StreamedMatrix);

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        prototype.addOutput("Tran-completed Flag",OV_TypeId_Stimulations);

        prototype.addSetting("Train trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Train");
        prototype.addSetting("Filename to save classifier model",OV_TypeId_Filename, "${Player_ScenarioDirectory}/my-classifier.xml");
        prototype.addSetting("Method", TypeId_Matrix_Classifier, toString(Geometry::EMatrixClassifiers::MDM).c_str());
        prototype.addSetting("Metric", TypeId_Metric, toString(Geometry::EMetric::Riemann).c_str());
        prototype.addSetting("Log Level", OV_TypeId_LogLevel, "Information");
        prototype.addSetting("Class 1 label", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_01");
        prototype.addSetting("Class 2 label", OV_TypeId_Stimulation, "OVTK_StimulationId_Label_02");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_MatrixClassifierTrainerDesc)
};
} // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
