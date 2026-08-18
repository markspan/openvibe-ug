// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CTemporalFilterBoxAlgorithm final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_Box_TemporalFilterBoxAlgorithm)

protected:
    Toolkit::TSignalDecoder<CTemporalFilterBoxAlgorithm>* m_decoder = nullptr;
    Toolkit::TSignalEncoder<CTemporalFilterBoxAlgorithm>* m_encoder = nullptr;
    Kernel::IAlgorithmProxy* m_computeTemporalFilterCoefs           = nullptr;
    Kernel::IAlgorithmProxy* m_applyTemporalFilter                  = nullptr;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_bufferToDecode;
    Kernel::TParameterHandler<CMemoryBuffer*> op_encodedBuffer;
    //uint64_t m_lastStartTime = 0;
    uint64_t m_lastEndTime = 0;
};

class CTemporalFilterBoxAlgorithmDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Temporal Filter (INSERM contrib)"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return "Applies temporal filtering on time signal"; }
    CString getDetailedDescription() const override { return "The user can choose among a variety of filter types to process the signal"; }
    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Box_TemporalFilterBoxAlgorithm; }
    IPluginObject* create() override { return new CTemporalFilterBoxAlgorithm(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Filtered signal", OV_TypeId_Signal);
        prototype.addSetting("Filter method", OVP_TypeId_FilterMethod, "Butterworth");
        prototype.addSetting("Filter type", OVP_TypeId_FilterType, "Band Pass");
        prototype.addSetting("Filter order", OV_TypeId_Integer, "4");
        prototype.addSetting("Low cut frequency (Hz)", OV_TypeId_Float, "29");
        prototype.addSetting("High cut frequency (Hz)", OV_TypeId_Float, "40");
        prototype.addSetting("Pass band ripple (dB)", OV_TypeId_Float, "0.5");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_Box_TemporalFilterBoxAlgorithmDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
