///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmReferenceChannel.hpp
/// \brief Classes for the Box Reference Channel.
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
class CBoxAlgorithmReferenceChannel final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ReferenceChannel)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmReferenceChannel> m_decoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmReferenceChannel> m_encoder;
    size_t m_referenceChannelIdx = 0;
};

class CBoxAlgorithmReferenceChannelDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Reference Channel"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Subtracts the value of the reference channel from all other channels"; }
    CString getDetailedDescription() const override { return "Reference channel must be specified as a parameter for the box"; }
    CString getCategory() const override { return "Signal processing/Channels"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_ReferenceChannel; }
    IPluginObject* create() override { return new CBoxAlgorithmReferenceChannel; }
    // virtual IBoxListener* createBoxListener() const               { return new CBoxAlgorithmReferenceChannelListener; }
    // virtual void releaseBoxListener(IBoxListener* listener) const { delete listener; }
    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("Channel", OV_TypeId_String, "Ref_Nose");
        prototype.addSetting("Channel Matching Method", TypeId_MatchMethod, "Smart");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ReferenceChannelDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
