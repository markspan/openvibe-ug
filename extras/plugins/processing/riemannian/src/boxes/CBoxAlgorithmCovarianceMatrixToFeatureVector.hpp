///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCovarianceMatrixToFeatureVector.hpp
/// \brief Classes for the box computing the Feature vector with the covariance matrix.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 17/10/2018.
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

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
/// <summary>   The class CBoxAlgorithmCovarianceMatrixToFeatureVector describes the box Covariance Matrix To Feature Vector. </summary>
class CBoxAlgorithmCovarianceMatrixToFeatureVector final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_CovarianceMatrixToFeatureVector)

protected:
    bool featurization() const;
    bool initRef();

    //***** Codecs *****
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmCovarianceMatrixToFeatureVector> m_i0MatrixCodec;  // Input Matrix Codec
    Toolkit::TFeatureVectorEncoder<CBoxAlgorithmCovarianceMatrixToFeatureVector> m_o0FeatureCodec;  // Output Feature Codec
    //***** Matrices *****
    CMatrix* m_iMatrix = nullptr;                           // Input Matrix pointer
    CMatrix* m_oMatrix = nullptr;                           // Output Matrix pointer
    //***** Settings *****
    bool m_tangentSpace = true;                             // Method to use (only tangent or squeeze now)
    Eigen::MatrixXd m_ref;                                  // Reference matrix for tangent space compute
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_Info;   // Log Level
};

/// <summary>   Descriptor of the box Covariance Matrix To Feature Vector. </summary>
class CBoxAlgorithmCovarianceMatrixToFeatureVectorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Covariance Matrix To Feature Vector"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Transforms a covariance matrix into a feature std::vector"; }

    CString getDetailedDescription() const override
    {
        return
                "Transforms a covariance matrix (size : NxN) into a feature std::vector (size : N(N+1)/2).\nThe Setting Tangent Space define if the transformation into std::vector is done in the tanget space or if it is a squeeze of the upper triangular matrix";
    }

    CString getCategory() const override { return "Riemannian Geometry"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-jump-to"; }

    CIdentifier getCreatedClass() const override { return Box_CovarianceMatrixToFeatureVector; }
    IPluginObject* create() override { return new CBoxAlgorithmCovarianceMatrixToFeatureVector; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Covariance Matrix",OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output Feature Vector",OV_TypeId_FeatureVector);

        prototype.addSetting("Tangent Space", OV_TypeId_Boolean, "true");
        prototype.addSetting("Filename to Reference Matrix (CSV, empty for Identity)", OV_TypeId_Filename, "${Player_ScenarioDirectory}/Mean.csv");
        prototype.addSetting("Log Level", OV_TypeId_LogLevel, "Information");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_CovarianceMatrixToFeatureVectorDesc)
};
} // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
