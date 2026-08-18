///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixClassifierProcessor.hpp
/// \brief Classes for the box Process a Matrix Classifier.
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

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
/// <summary>   The class CBoxAlgorithmMatrixClassifierProcessor describes the box Matrix Classifier Processor. </summary>
class CBoxAlgorithmMatrixClassifierProcessor final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_MatrixClassifierProcessor)

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxAlgorithmMatrixClassifierProcessor> m_i0StimulationCodec;
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrixClassifierProcessor> m_i1MatrixCodec;
    Toolkit::TStimulationEncoder<CBoxAlgorithmMatrixClassifierProcessor> m_o0StimulationCodec;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatrixClassifierProcessor> m_o1MatrixCodec, m_o2MatrixCodec;

    //***** Matrices *****
    CMatrix *m_i1Matrix = nullptr, *m_o1Matrix = nullptr, *m_o2Matrix = nullptr;    // Matrix Pointer
    Eigen::MatrixXd m_distance, m_probability;                              // Eigen Matrix

    //***** Stimulations *****
    CStimulationSet *m_i0Stimulation = nullptr,                             // Stimulation receiver
                    *m_o0Stimulation = nullptr;                             // Stimulation sender
    std::vector<uint64_t> m_stimulationClassName;                           // Name of stimulation to check for each class

    Geometry::IMatrixClassifier* m_classifier = nullptr;                    // Classifier
    size_t m_lastLabelReceived                = std::numeric_limits<size_t>::max(); // Last label received for Supervised Adaptation
    Geometry::EAdaptations m_adaptation       = Geometry::EAdaptations::None;   // Adaptation Method

    //***** Setting *****
    CString m_ifilename, m_ofilename;
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_Info;                   // Log Level

    bool classify(uint64_t tEnd);
    bool loadXML();
    bool saveXML();
};

/// <summary>   Descriptor of the box Matrix Classifier Processor. </summary>
class CBoxAlgorithmMatrixClassifierProcessorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix Classifier Processor"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Matrix classifier Processor."; }
    CString getDetailedDescription() const override { return "Matrix classifier Processor."; }
    CString getCategory() const override { return "Riemannian Geometry"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_MatrixClassifierProcessor; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrixClassifierProcessor; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Expected Label", OV_TypeId_Stimulations);
        prototype.addInput("Input Matrix",OV_TypeId_StreamedMatrix);

        prototype.addOutput("Label",OV_TypeId_Stimulations);
        prototype.addOutput("Distance",OV_TypeId_StreamedMatrix);
        prototype.addOutput("Probability",OV_TypeId_StreamedMatrix);

        prototype.addSetting("Filename to load classifier model", OV_TypeId_Filename, "${Player_ScenarioDirectory}/input-classifier.xml");
        prototype.addSetting("Filename to save classifier model",OV_TypeId_Filename, "${Player_ScenarioDirectory}/output-classifier.xml");
        prototype.addSetting("Adaptation", TypeId_Classifier_Adaptation, toString(Geometry::EAdaptations::None).c_str());
        prototype.addSetting("Log Level", OV_TypeId_LogLevel, "Information");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_MatrixClassifierProcessorDesc)
};
} // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
