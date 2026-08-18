#pragma once

#include "../defines.hpp"
#include <toolkit/ovtk_all.h>
#include <random>

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {
class CNoiseGenerator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CNoiseGenerator();

    void release() override { delete this; }

    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_NoiseGenerator)

protected:
    Toolkit::TSignalEncoder<CNoiseGenerator> m_encoder;

    bool m_headerSent              = false;
    size_t m_nChannel              = 0;
    size_t m_sampling              = 0;
    size_t m_nGeneratedEpochSample = 0;
    size_t m_nSentSample           = 0;
    size_t m_noiseType             = 0;

    std::default_random_engine m_engine;
    std::normal_distribution<> m_normalDistrib{ 0.0, 1.0 };         // Mean=0, Var=1
    std::uniform_real_distribution<> m_uniformDistrib{ 0.0, 1.0 };  // Min=0, Max=1
};

class CNoiseGeneratorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Noise generator"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Simple random noise generator"; }
    CString getDetailedDescription() const override { return "Generates uniform or Gaussian random data"; }
    CString getCategory() const override { return "Data generation"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return Box_NoiseGenerator; }
    IPluginObject* create() override { return new CNoiseGenerator(); }
    CString getStockItemName() const override { return "gtk-execute"; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Generated signal", OV_TypeId_Signal);

        prototype.addSetting("Channel count", OV_TypeId_Integer, "4");
        prototype.addSetting("Sampling frequency", OV_TypeId_Integer, "512");
        prototype.addSetting("Generated epoch sample count", OV_TypeId_Integer, "32");
        prototype.addSetting("Noise type", TypeId_NoiseType, "Uniform");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_NoiseGeneratorDesc)
};
}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
