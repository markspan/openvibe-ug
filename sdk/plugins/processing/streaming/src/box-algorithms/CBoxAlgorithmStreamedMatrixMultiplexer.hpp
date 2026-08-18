///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStreamedMatrixMultiplexer.hpp
/// \brief Classes for the Box Signal Merger.
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
namespace Streaming {
class CBoxAlgorithmStreamedMatrixMultiplexer final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_StreamedMatrixMultiplexer)

protected:
    uint64_t m_lastStartTime = 0;
    uint64_t m_lastEndTime   = 0;
    bool m_headerSent        = false;
};

class CBoxAlgorithmStreamedMatrixMultiplexerListener final : public Toolkit::TBoxListener<IBoxListener>
{
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputName(i, ("Input stream " + std::to_string(i + 1)).c_str()); }
        return true;
    }

public:
    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(0, typeID);
        box.setInputType(index, typeID);
        return this->check(box);
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(0, typeID);

        while (box.getInputCount() < 2) { box.addInput("", typeID); }

        return this->check(box);
    }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);

        if (this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_StreamedMatrix)) {
            for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputType(i, typeID); }

            box.setOutputType(0, typeID);
        }
        else {
            box.getOutputType(0, typeID);
            box.setInputType(index, typeID);
        }

        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(0, typeID);

        if (this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_StreamedMatrix)) {
            for (size_t i = 0; i < box.getInputCount(); ++i) { box.setInputType(i, typeID); }
        }
        else {
            box.getInputType(0, typeID);
            box.setOutputType(0, typeID);
        }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmStreamedMatrixMultiplexerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Streamed matrix multiplexer"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Multiplexes streamed matrix buffers in a new stream"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Streaming"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_StreamedMatrixMultiplexer; }
    IPluginObject* create() override { return new CBoxAlgorithmStreamedMatrixMultiplexer; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmStreamedMatrixMultiplexerListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream 1", OV_TypeId_StreamedMatrix);
        prototype.addInput("Input stream 2", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Multiplexed streamed matrix", OV_TypeId_StreamedMatrix);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_ChannelLocalisation);
        prototype.addInputSupport(OV_TypeId_FeatureVector);
        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_Spectrum);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_StreamedMatrixMultiplexerDesc)
};
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
