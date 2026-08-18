// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>

#include <vector>
#include <map>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
namespace WindowingFunctions {
// Used to store information about the signal stream
class CSignalDescription
{
public:
    CSignalDescription() { }

    size_t m_StreamVersion = 1;
    size_t m_Sampling      = 0;
    size_t m_NChannel      = 0;
    size_t m_NSample       = 0;
    std::vector<std::string> m_ChannelName;
    size_t m_CurrentChannel = 0;

    bool m_ReadyToSend = false;
};
}  // namespace WindowingFunctions

/**
* The Window Anlaysis plugin's main class.
*/
class CWindowingFunctions final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CWindowingFunctions() { }

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_WindowingFunctions)

    void setSampleBuffer(const double* buffer) const;

    //start time and end time of the last arrived chunk
    uint64_t m_LastChunkStartTime = 0;
    uint64_t m_LastChunkEndTime   = 0;
    size_t m_SamplesPerBuffer     = 0;
    size_t m_NChannel             = 0;

    // Needed to write on the plugin output
    Toolkit::TSignalDecoder<CWindowingFunctions>* m_Decoder = nullptr;
    Toolkit::TSignalEncoder<CWindowingFunctions>* m_Encoder = nullptr;

    //! Structure containing information about the signal stream
    WindowingFunctions::CSignalDescription* m_SignalDesc = nullptr;

    //! Size of the matrix buffer (output signal)
    size_t m_BufferSize = 0;
    //! Output signal's matrix buffer
    double* m_Buffer = nullptr;

    EWindowMethod m_WindowMethod = EWindowMethod::None;
};

class CWindowingFunctionsDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Windowing (INSERM contrib)"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM"; }
    CString getShortDescription() const override { return "Apply a window to the signal buffer"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Windowing"; }
    CString getVersion() const override { return "0.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_WindowingFunctions; }
    IPluginObject* create() override { return new CWindowingFunctions(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("Window method", OVP_TypeId_WindowMethod, "Hamming");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_WindowingFunctionsDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
