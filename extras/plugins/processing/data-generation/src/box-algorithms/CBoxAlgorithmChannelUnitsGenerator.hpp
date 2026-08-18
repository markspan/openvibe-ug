#pragma once

#include "../defines.hpp"
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {
class CChannelUnitsGenerator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 1LL << 32; }
    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& /*msg*/) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, Box_ChannelUnitsGenerator)

protected:
    bool m_headerSent = false;
    size_t m_nChannel = 0;
    size_t m_unit     = 0;
    size_t m_factor   = 0;

    Toolkit::TChannelUnitsEncoder<CChannelUnitsGenerator> m_encoder;
};

class CChannelUnitsGeneratorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Channel units generator"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Generates channel units"; }

    CString getDetailedDescription() const override { return "This box can generate a channel unit stream if specific measurement units are needed. The box is mainly provided for completeness."; }

    CString getCategory() const override { return "Data generation"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_ChannelUnitsGenerator; }
    IPluginObject* create() override { return new CChannelUnitsGenerator(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Number of channels", OV_TypeId_Integer, "4");
        prototype.addSetting("Unit", OV_TypeId_MeasurementUnit, "V");
        prototype.addSetting("Factor", OV_TypeId_Factor, "1e-06");

        prototype.addOutput("Channel units", OV_TypeId_ChannelUnits);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ChannelUnitsGeneratorDesc)
};
}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
