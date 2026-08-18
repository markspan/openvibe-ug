///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCovarianceMeanCalculator.hpp
/// \brief Classes for the box computing the mean of the covariance matrix.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/11/2018.
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

#include <Eigen/Dense>
#include <geometry/Metrics.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
/// <summary>   The class CBoxAlgorithmCovarianceMeanCalculator describes the box Covariance Mean Calculator. </summary>
class CBoxAlgorithmCovarianceMeanCalculator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_CovarianceMeanCalculator)

protected:
    //***** Codecs *****
    Toolkit::TStimulationDecoder<CBoxAlgorithmCovarianceMeanCalculator> m_i0StimulationCodec;               // Input Stimulation Codec
    std::vector<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmCovarianceMeanCalculator>> m_i1MatrixCodec;    // Input Signal Codec
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmCovarianceMeanCalculator> m_o0MatrixCodec;                 // Output Codec
    //***** Matrices *****
    size_t m_nbClass = 1;                                                   // Number of input classes
    std::vector<CMatrix*> m_iMatrix;                                        // Input Matrix pointer
    CMatrix* m_oMatrix = nullptr;                                           // Output Matrix pointer
    std::vector<Eigen::MatrixXd> m_covs;                                    // List of Covariance Matrix
    Eigen::MatrixXd m_mean;                                                 // Mean
    Geometry::EMetric m_metric = Geometry::EMetric::Euclidian;              // Metric Used

    //***** Settings *****
    CStimulationSet* m_iStimulation = nullptr;                              // Stimulation receiver
    uint64_t m_stimulationName      = OVTK_StimulationId_TrainCompleted;    // Name of stimulation to check
    Kernel::ELogLevel m_logLevel    = Kernel::LogLevel_Info;                // Log Level

    // File
    CString m_filename;
    bool saveCSV();
};


/// <summary>   Listener of the box Covariance Mean Calculator. </summary>
class CBoxAlgorithmCovarianceMeanCalculatorListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override;
    bool onInputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/// <summary>   Descriptor of the box Covariance Mean Calculator. </summary>
class CBoxAlgorithmCovarianceMeanCalculatorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Covariance Mean Calculator"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Calculation of the mean of covariance matrix."; }

    CString getDetailedDescription() const override
    {
        return
                "Calculation of the mean of covariance matrix.\nThe Calculation is done when \"OVTK_StimulationId_TrainCompleted\" is received.\nThe Mean is saved in a CSV File.";
    }

    CString getCategory() const override { return "Riemannian Geometry"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_CovarianceMeanCalculator; }
    IPluginObject* create() override { return new CBoxAlgorithmCovarianceMeanCalculator; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmCovarianceMeanCalculatorListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Stimulation", OV_TypeId_Stimulations);
        prototype.addInput("Input Covariance Matrix 1", OV_TypeId_StreamedMatrix);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addOutput("Output Mean Matrix", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Metric", TypeId_Metric, toString(Geometry::EMetric::Riemann).c_str());
        prototype.addSetting("Filename to save Matrix (CSV, empty to not save)",OV_TypeId_Filename, "${Player_ScenarioDirectory}/Mean.csv");
        prototype.addSetting("Stimulation name that triggers the compute",OV_TypeId_Stimulation, "OVTK_StimulationId_TrainCompleted");
        prototype.addSetting("Log Level", OV_TypeId_LogLevel, "Information");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_CovarianceMeanCalculatorDesc)
};
} // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
