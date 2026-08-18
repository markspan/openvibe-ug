#pragma once

#include "../defines.hpp"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {
class CSinusSignalGenerator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CSinusSignalGenerator() {}
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return CTime(m_nGeneratedEpochSample, m_sampling).time(); } // Intentional parameter swap to get the frequency
    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SinusSignalGenerator)

protected:
    Toolkit::TSignalEncoder<CSinusSignalGenerator> m_encoder;

    bool m_headerSent              = false;
    size_t m_nChannel              = 0;
    size_t m_sampling              = 0;
    size_t m_nGeneratedEpochSample = 0;
    size_t m_nSentSample           = 0;
};

class CSinusSignalGeneratorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Sinus oscillator"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Simple sinus signal generator"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Data generation"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_SinusSignalGenerator; }
    IPluginObject* create() override { return new CSinusSignalGenerator(); }
    CString getStockItemName() const override { return "gtk-execute"; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Generated signal", OV_TypeId_Signal);

        prototype.addSetting("Channel count", OV_TypeId_Integer, "4");
        prototype.addSetting("Sampling frequency", OV_TypeId_Integer, "512");
        prototype.addSetting("Generated epoch sample count", OV_TypeId_Integer, "32");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SinusSignalGeneratorDesc)
};
}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
