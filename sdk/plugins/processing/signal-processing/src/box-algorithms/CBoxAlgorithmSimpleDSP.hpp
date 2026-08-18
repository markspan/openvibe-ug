///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSimpleDSP.hpp
/// \brief Classes for the Box Simple DSP.
/// \author Bruno Renier (Inria) / Yann Renard (Inria).
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
#include <toolkit/ovtk_all.h>
#include "../SimpleDSP/CEquationParser.hpp"

#include <vector>
#include <cstdio>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmSimpleDSP final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmSimpleDSP() { }
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SimpleDSP)

protected:
    void evaluate();

    std::vector<Kernel::IAlgorithmProxy*> m_decoders;
    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    std::vector<CMatrix*> m_matrices;

    CEquationParser* m_parser = nullptr;

    uint64_t m_equationType = OP_USERDEF;
    double m_equationParam  = 0;
    double** m_variables    = nullptr;

    bool m_checkDates = false;
};

class CBoxAlgorithmSimpleDSPListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        const std::string name = std::string("Input - ") + char('A' + index);
        CIdentifier typeID     = CIdentifier::undefined();
        box.getOutputType(0, typeID);
        box.setInputType(index, typeID);
        box.setInputName(index, name.c_str());
        return true;
    }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        box.setOutputType(0, typeID);
        for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputType(i, typeID); }
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        box.setOutputType(0, typeID);
        for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputType(i, typeID); }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmSimpleDSPDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Simple DSP"; }
    CString getAuthorName() const override { return "Bruno Renier / Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria / IRISA"; }
    CString getShortDescription() const override { return "Apply mathematical formulaes to matrices."; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_SimpleDSP; }
    IPluginObject* create() override { return new CBoxAlgorithmSimpleDSP(); }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmSimpleDSPListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        //OnInputAdded listener will try to get output type so it has to be created first
        prototype.addOutput("Output", OV_TypeId_Signal);
        prototype.addInput("Input - A", OV_TypeId_Signal);
        prototype.addSetting("Equation", OV_TypeId_String, "x");

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_FeatureVector);
        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);

        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_FeatureVector);
        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SimpleDSPDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
