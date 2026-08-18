///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSpatialFilter.hpp
/// \brief Classes for the Box Spatial Filter.
/// \author Yann Renard (Inria) / Jussi T. Lindgren (Inria).
/// \version 1.1.
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
class CBoxAlgorithmSpatialFilter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SpatialFilter)

protected:
    Toolkit::TDecoder<CBoxAlgorithmSpatialFilter>* m_decoder = nullptr;
    Toolkit::TEncoder<CBoxAlgorithmSpatialFilter>* m_encoder = nullptr;

    CMatrix m_filterBank;

private:
    // Loads the m_vCoefficient vector (representing a matrix) from the given string. c1 and c2 are separator characters between floats.
    size_t loadCoefs(const CString& coefs, char c1, char c2, size_t nRows, size_t nCols);
};

class CBoxAlgorithmSpatialFilterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(0, typeID);
        box.setOutputType(0, typeID);
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(0, typeID);
        box.setInputType(0, typeID);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmSpatialFilterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spatial Filter"; }
    CString getAuthorName() const override { return "Yann Renard, Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Maps M inputs to N outputs by multiplying the each input vector with a matrix"; }

    CString getDetailedDescription() const override
    {
        return CString(
            "The applied coefficient matrix must be specified as a box parameter. The filter processes each sample independently of the past samples.");
    }

    CString getCategory() const override { return "Signal processing/Filtering"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Box_SpatialFilter; }
    IPluginObject* create() override { return new CBoxAlgorithmSpatialFilter; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmSpatialFilterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Signal", OV_TypeId_Signal);
        prototype.addOutput("Output Signal", OV_TypeId_Signal);
        prototype.addSetting("Spatial Filter Coefficients", OV_TypeId_String, "1;0;0;0;0;1;0;0;0;0;1;0;0;0;0;1");
        prototype.addSetting("Number of Output Channels", OV_TypeId_Integer, "4");
        prototype.addSetting("Number of Input Channels", OV_TypeId_Integer, "4");
        prototype.addSetting("Filter matrix file", OV_TypeId_Filename, "");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_Signal);

        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_Signal);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SpatialFilterDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
