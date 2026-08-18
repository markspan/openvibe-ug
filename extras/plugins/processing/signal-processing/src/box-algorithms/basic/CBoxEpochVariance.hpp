///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxEpochVariance.hpp
/// \brief Classes of the box Epoch variance.
/// \author Dieter Devlaminck & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 11/11/2021.
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
#include "CMatrixVariance.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxEpochVariance final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_EpochVariance)

protected:
    Toolkit::TGenericDecoder<CBoxEpochVariance> m_decoder;
    Toolkit::TGenericEncoder<CBoxEpochVariance> m_encoderAverage, m_encoderVariance, m_encoderConfidence;
    CMatrix* m_iMatrix           = nullptr;
    CMatrix* m_oMatrixAverage    = nullptr;
    CMatrix* m_oMatrixVariance   = nullptr;
    CMatrix* m_oMatrixConfidence = nullptr;

    CMatrixVariance m_variance;
};

class CBoxEpochVarianceListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        for (size_t i = 0; i < box.getOutputCount(); ++i) { box.setOutputType(i, typeID); }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxEpochVarianceDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Epoch variance"; }
    CString getAuthorName() const override { return "Dieter Devlaminck & Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Computes variance of each sample over several epochs"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_EpochVariance; }
    IPluginObject* create() override { return new CBoxEpochVariance(); }

    IBoxListener* createBoxListener() const override { return new CBoxEpochVarianceListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input epochs", OV_TypeId_StreamedMatrix);

        prototype.addOutput("Averaged epochs", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Variance of epochs", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Confidence bounds", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Averaging type", OVP_TypeId_EpochAverageMethod, "Moving epoch average");
        prototype.addSetting("Epoch count", OV_TypeId_Integer, "4");
        prototype.addSetting("Significance level", OV_TypeId_Float, "0.01");

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_FeatureVector);
        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_EpochVarianceDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
