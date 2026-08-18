///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmTimeSignalGenerator.hpp
/// \brief Classes for the Box Time signal.
/// \author Yann Renard (Inria).
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

#include "../defines.hpp"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {
class CBoxAlgorithmTimeSignalGenerator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmTimeSignalGenerator() {}
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 128LL << 32; }

    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_TimeSignalGenerator)

protected:
    Toolkit::TSignalEncoder<CBoxAlgorithmTimeSignalGenerator> m_encoder;

    bool m_headerSent              = false;
    size_t m_sampling              = 0;
    size_t m_nGeneratedEpochSample = 0;
    size_t m_nSentSample           = 0;
};

class CBoxAlgorithmTimeSignalGeneratorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Time signal"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Simple time signal generator (for use with DSP)"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Data generation"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Box_TimeSignalGenerator; }
    IPluginObject* create() override { return new CBoxAlgorithmTimeSignalGenerator(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Generated signal", OV_TypeId_Signal);

        prototype.addSetting("Sampling frequency", OV_TypeId_Integer, "512");
        prototype.addSetting("Generated epoch sample count", OV_TypeId_Integer, "32");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_TimeSignalGeneratorDesc)
};
}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
