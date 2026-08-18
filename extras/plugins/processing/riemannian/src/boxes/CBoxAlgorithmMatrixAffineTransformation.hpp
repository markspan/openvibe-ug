///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixAffineTransformation.hpp
/// \brief Classes for the box Affine Transformation.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 28/08/2019.
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

#include <geometry/classifier/CBias.hpp>

namespace OpenViBE {
namespace Plugins {
namespace Riemannian {
/// <summary>   The class CBoxAlgorithmMatrixAffineTransformation describes the box Matrix Affine Transformation. </summary>
class CBoxAlgorithmMatrixAffineTransformation final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_MatrixAffineTransformation)

protected:
    bool loadXML();
    bool saveXML() const;

    //***** Codecs *****
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrixAffineTransformation> m_iMatrixCodec;    // Input Signal Codec
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatrixAffineTransformation> m_oMatrixCodec;    // Output Signal Codec

    CMatrix *m_iMatrix = nullptr, *m_oMatrix = nullptr;     // Input/Output Matrix pointer

    //***** Settings *****
    std::string m_ifilename, m_ofilename;                   // Input/Output Filename
    bool m_continuous = false;

    //***** Variable *****
    Geometry::CBias m_bias;
    std::vector<Eigen::MatrixXd> m_samples;
};

/// <summary>   Descriptor of the box Matrix Classifier Trainer. </summary>
class CBoxAlgorithmMatrixAffineTransformationDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix Affine Transformation"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Compute and Apply the Bias matrix for Affine Transformation on square matrix."; }

    CString getDetailedDescription() const override
    {
        return
                "Compute and Apply the Reference matrix for Affine Transformation on square matrix (isR * M * isR^(-1) = I).\nYou can load an existing matrix.\nContinuous update is to update Bias at each chunk or at the end.";
    }

    CString getCategory() const override { return "Riemannian Geometry"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_MatrixAffineTransformation; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrixAffineTransformation; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Square Matrix",OV_TypeId_StreamedMatrix);
        prototype.addOutput("Transformed Square Matrix", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Filename to load transformation", OV_TypeId_Filename, "${Player_ScenarioDirectory}/my-transformation-input.xml");
        prototype.addSetting("Filename to save transformation",OV_TypeId_Filename, "${Player_ScenarioDirectory}/my-transformation-output.xml");
        prototype.addSetting("Continuous Update", OV_TypeId_Boolean, "false");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_MatrixAffineTransformationDesc)
};
} // namespace Riemannian
}  // namespace Plugins
}  // namespace OpenViBE
