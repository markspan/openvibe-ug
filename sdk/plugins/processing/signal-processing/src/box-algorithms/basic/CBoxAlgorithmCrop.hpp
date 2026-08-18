///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmCrop.hpp
/// \brief Classes for the Box Crop.
/// \author Yann Renard (Inria).
/// \version 1.0.
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

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmCrop final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_Crop)

protected:
    CMatrix* m_matrix                  = nullptr;
    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    double m_minCropValue              = 0;
    double m_maxCropValue              = 0;
    ECropMethod m_cropMethod           = ECropMethod::MinMax;
};

class CBoxAlgorithmCropListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        box.setOutputType(index, typeID);
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        box.setInputType(index, typeID);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmCropDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Crop"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria/IRISA"; }
    CString getShortDescription() const override { return "Truncates signal values to a specified range"; }
    CString getDetailedDescription() const override { return "Minimum or maximum or both limits can be specified"; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_Crop; }
    IPluginObject* create() override { return new CBoxAlgorithmCrop; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmCropListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output matrix", OV_TypeId_StreamedMatrix);
        prototype.addSetting("Crop method", TypeId_CropMethod, "Min/Max");
        prototype.addSetting("Min crop value", OV_TypeId_Float, "-1");
        prototype.addSetting("Max crop value", OV_TypeId_Float, "1");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_FeatureVector);

        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_FeatureVector);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_CropDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
