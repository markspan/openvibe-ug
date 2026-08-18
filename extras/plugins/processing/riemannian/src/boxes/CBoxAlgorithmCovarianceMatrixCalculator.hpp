///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCovarianceMatrixCalculator.hpp
/// \brief Classes for the box computing the covariance matrix
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 16/10/2018.
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

# pragma once

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <geometry/Covariance.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
/// <summary>    The class CBoxAlgorithmCovarianceMatrixCalculator describes the box Covariance Matrix Calculator. </summary>
class CBoxAlgorithmCovarianceMatrixCalculator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_CovarianceMatrixCalculator)

protected:
    bool covarianceMatrix() const;

    //***** Codecs *****
    Toolkit::TSignalDecoder<CBoxAlgorithmCovarianceMatrixCalculator> m_i0SignalCodec;           // Input Signal Codec
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmCovarianceMatrixCalculator> m_o0MatrixCodec;   // Output Matrix Codec

    //***** Matrices *****
    CMatrix* m_iMatrix = nullptr;                               // Input Matrix pointer
    CMatrix* m_oMatrix = nullptr;                               // Output Matrix pointer

    //***** Settings *****
    Geometry::EEstimator m_est   = Geometry::EEstimator::COV;   // Covariance Estimator
    bool m_center                = true;                        // Center data
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_Info;       // Log Level
};

/// <summary>    Descriptor of the box Covariance Matrix Calculator. </summary>
class CBoxAlgorithmCovarianceMatrixCalculatorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Covariance Matrix Calculator"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Calculation of the covariance matrix of the input signal."; }

    CString getDetailedDescription() const override
    {
        return
                "Calculation of the covariance matrix of the input signal.\nReturns a covariance matrix of size NxN per each input chunk. Where N is the number of channels.";
    }

    CString getCategory() const override { return "Riemannian Geometry"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_CovarianceMatrixCalculator; }
    IPluginObject* create() override { return new CBoxAlgorithmCovarianceMatrixCalculator; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Signal", OV_TypeId_Signal);
        prototype.addOutput("Output Covariance Matrix", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Estimator", TypeId_Estimator, toString(Geometry::EEstimator::COV).c_str());
        prototype.addSetting("Center Data", OV_TypeId_Boolean, "true");
        prototype.addSetting("Log Level", OV_TypeId_LogLevel, "Information");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_CovarianceMatrixCalculatorDesc)
};
}  // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
