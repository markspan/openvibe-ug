#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Examples {
class CBoxAlgorithmClock final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override { return true; }
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Clock)

protected:
    uint64_t m_clockFrequency    = 0;
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_None;
};

class CBoxAlgorithmClockDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Clock"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Simply prints clock activation times"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Examples/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-info"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Clock; }
    IPluginObject* create() override { return new CBoxAlgorithmClock; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Clock frequency (Hz)", OV_TypeId_Integer, "60");
        prototype.addSetting("Log level to use", OV_TypeId_LogLevel, "Information");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ClockDesc)
};
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
