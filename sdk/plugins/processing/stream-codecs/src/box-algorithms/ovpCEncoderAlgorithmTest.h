#pragma once

#include "../ovp_defines.h"
#include <toolkit/ovtk_all.h>
#include <array>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CEncoderAlgorithmTest : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 1LL << 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_EncoderAlgorithmTest)

protected:
    std::array<Kernel::IAlgorithmProxy*, 7> m_encoders;
    std::array<Kernel::TParameterHandler<CMemoryBuffer*>, 7> op_buffer;

    bool m_hasSentHeader = false;
    uint64_t m_startTime = 0;
    uint64_t m_endTime   = 0;

    CMatrix* m_matrix1         = nullptr;
    CMatrix* m_matrix2         = nullptr;
    CMatrix* m_matrix3         = nullptr;
    CStimulationSet* m_stimSet = nullptr;
};

class CEncoderAlgorithmTestDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Encoder algorithm test"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Setups various streams and outputs them"; }
    CString getDetailedDescription() const override { return "Note: Data sent in the streams does not change over time"; }
    CString getCategory() const override { return "Tests/Algorithms"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_EncoderAlgorithmTest; }
    IPluginObject* create() override { return new CEncoderAlgorithmTest(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addOutput("Feature vector", OV_TypeId_FeatureVector);
        prototype.addOutput("Signal", OV_TypeId_Signal);
        prototype.addOutput("Spectrum", OV_TypeId_Spectrum);
        prototype.addOutput("Stimulation", OV_TypeId_Stimulations);
        prototype.addOutput("Streamed matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Channel localisation", OV_TypeId_ChannelLocalisation);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EncoderAlgorithmTestDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
