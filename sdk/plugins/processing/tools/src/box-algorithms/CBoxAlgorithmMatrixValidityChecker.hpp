///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmMatrixValidityChecker.hpp
/// \brief Classes for the Box Matrix validity checker.
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

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Tools {
class CBoxAlgorithmMatrixValidityChecker final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_MatrixValidityChecker)

protected:
    std::vector<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrixValidityChecker>> m_decoders;
    std::vector<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatrixValidityChecker>> m_encoders;
    Kernel::ELogLevel m_logLevel   = Kernel::ELogLevel::LogLevel_None;
    uint64_t m_validityCheckerType = 0;

    std::vector<size_t> m_nTotalInterpolatedSample;
    std::vector<size_t> m_nTotalInterpolatedChunk;
    std::vector<std::vector<double>> m_lastValidSamples;
};

class CBoxAlgorithmMatrixValidityCheckerListener final : public Toolkit::TBoxListener<IBoxListener>
{
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputName(i, ("Stream " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_StreamedMatrix);
        }
        for (size_t i = 0; i < box.getOutputCount(); ++i) {
            box.setOutputName(i, ("Output stream " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_StreamedMatrix);
        }

        return true;
    }

public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_StreamedMatrix);
        if (box.getSettingCount() > 1) { box.addOutput("", OV_TypeId_StreamedMatrix); }
        this->check(box);
        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeOutput(index);
        this->check(box);
        return true;
    }

    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputType(index, OV_TypeId_StreamedMatrix);
        box.addInput("", OV_TypeId_StreamedMatrix);
        this->check(box);
        return true;
    }

    bool onOutputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeInput(index);
        this->check(box);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmMatrixValidityCheckerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix validity checker"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Checks if a matrix contains \"not a number\" or \"infinity\" elements"; }

    CString getDetailedDescription() const override
    {
        return
                "This box is for debugging purposes and allows an author to check the validity of a streamed matrix and derived stream. This box can log a message, stop the player or interpolate data.";
    }

    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_MatrixValidityChecker; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrixValidityChecker; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmMatrixValidityCheckerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stream 1", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output stream 1", OV_TypeId_StreamedMatrix);
        prototype.addSetting("Log level", OV_TypeId_LogLevel, "Warning");
        prototype.addSetting("Action to do", TypeId_ValidityCheckerType, "Log warning");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_MatrixValidityCheckerDesc)
};
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
