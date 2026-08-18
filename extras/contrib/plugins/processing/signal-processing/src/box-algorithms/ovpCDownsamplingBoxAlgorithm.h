// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CDownsamplingBoxAlgorithm final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_Box_DownsamplingBoxAlgorithm)

protected:
    Kernel::IAlgorithmProxy* m_decoder                    = nullptr;
    Kernel::IAlgorithmProxy* m_encoder                    = nullptr;
    Kernel::IAlgorithmProxy* m_computeTemporalFilterCoefs = nullptr;
    Kernel::IAlgorithmProxy* m_applyTemporalFilter        = nullptr;
    Kernel::IAlgorithmProxy* m_downsampling               = nullptr;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_bufferToDecode;
    Kernel::TParameterHandler<CMemoryBuffer*> op_encodedBuffer;

    Kernel::TParameterHandler<CMatrix*> m_iSignal;
    Kernel::TParameterHandler<CMatrix*> m_oSignal;
    CMatrix* m_signalDesc  = nullptr;
    uint64_t m_newSampling = 0;
    Kernel::TParameterHandler<uint64_t> m_samplingRate;

    uint64_t m_lastEndTime     = 0;
    bool m_flagFirstTime       = false;
    bool m_warned              = false;
    size_t m_lastBufferSize    = 0;
    size_t m_currentBufferSize = 0;
    uint64_t m_signalType      = 0;
};

class CDownsamplingBoxAlgorithmDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Downsampling"; }
    CString getAuthorName() const override { return "G. Gibert - E. Maby - P.E. Aguera"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return "Filters and downsamples input buffer."; }

    CString getDetailedDescription() const override
    {
        return
                "First, applies a low-pass (Butterworth or Chebyshev) filter (frequency cut is 1/4, 1/3 or 1/2 of the new sampling rate) to input buffers of signal for anti-aliasing. Then, the input buffers of signal is downsampled.";
    }

    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.01"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Box_DownsamplingBoxAlgorithm; }
    IPluginObject* create() override { return new CDownsamplingBoxAlgorithm(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("New sampling rate (Hz)", OV_TypeId_Integer, "32");
        prototype.addSetting("Frequency cutoff ratio", OVP_TypeId_FrequencyCutOffRatio, "1/4");
        prototype.addSetting("Name of filter", OVP_TypeId_FilterMethod, "Butterworth");
        prototype.addSetting("Filter order", OV_TypeId_Integer, "4");
        prototype.addSetting("Pass band ripple (dB)", OV_TypeId_Float, "0.5");
        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);
        prototype.addFlag(Kernel::BoxFlag_IsDeprecated);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_Box_DownsamplingBoxAlgorithmDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
