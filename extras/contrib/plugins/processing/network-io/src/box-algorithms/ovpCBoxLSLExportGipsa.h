#pragma once

#if defined TARGET_HAS_ThirdPartyLSL

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <vector>

#include "../ovpCInputChannel.h"
#include <lsl_cpp.h>

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {
class CBoxAlgorithmLSLExportGipsa final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmLSLExportGipsa() : m_inputChannel1(0) {}

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_CBoxAlgorithmLSLExportGipsa)

protected:
    int64_t m_decimationFactor = 0;
    uint64_t m_outputSampling  = 0;
    CString m_streamName;
    CString m_streamType;

    SignalProcessing::CInputChannel m_inputChannel1;

    lsl::stream_outlet* m_outlet = nullptr;
    std::vector<std::pair<uint64_t, uint64_t>> m_stims;//identifier,time
};

class CBoxAlgorithmLSLExportGipsaDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "LSL Export (Gipsa)"; }
    CString getAuthorName() const override { return "Anton Andreev"; }
    CString getAuthorCompanyName() const override { return "Gipsa-lab"; }
    CString getShortDescription() const override { return "Streams signal outside OpenVibe using Lab Streaming Layer library"; }

    CString getDetailedDescription() const override
    {
        return "More on how to read the signal in your application: https://code.google.com/p/labstreaminglayer/";
    }

    CString getCategory() const override { return "Acquisition and network IO"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-connect"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_CBoxAlgorithmLSLExportGipsa; }
    IPluginObject* create() override { return new CBoxAlgorithmLSLExportGipsa; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addInput("Input stimulations", OV_TypeId_Stimulations);
        prototype.addSetting("Stream name", OV_TypeId_String, "OpenViBE Stream");
        prototype.addSetting("Stream type", OV_TypeId_String, "EEG");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_CBoxAlgorithmLSLExportGipsaDesc)
};
}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE

#endif
