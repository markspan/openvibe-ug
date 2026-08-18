#pragma once

#include "../ovp_defines.h"
#include <toolkit/ovtk_all.h>
#include <array>

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {
class CDecoderAlgorithmTest final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CDecoderAlgorithmTest() { }
    ~CDecoderAlgorithmTest() override { }
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_DecoderAlgorithmTest)

protected:
    std::array<Kernel::IAlgorithmProxy*, 7> m_decoder;
    std::array<Kernel::TParameterHandler<const CMemoryBuffer*>, 7> ip_buffer;
};

class CDecoderAlgorithmTestDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Decoder algorithm test"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Decodes various types of streams and outputs some of the content parameters the log"; }

    CString getDetailedDescription() const override
    {
        return "Note: Warnings are normal as the algorithm polls the decoders for structures they may not contain.";
    }

    CString getCategory() const override { return "Tests/Algorithms"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_DecoderAlgorithmTest; }
    IPluginObject* create() override { return new CDecoderAlgorithmTest(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addInput("Feature vector", OV_TypeId_FeatureVector);
        prototype.addInput("Signal", OV_TypeId_Signal);
        prototype.addInput("Spectrum", OV_TypeId_Spectrum);
        prototype.addInput("Stimulation", OV_TypeId_Stimulations);
        prototype.addInput("Streamed matrix", OV_TypeId_StreamedMatrix);
        prototype.addInput("Channel localisation", OV_TypeId_ChannelLocalisation);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_DecoderAlgorithmTestDesc)
};
}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
