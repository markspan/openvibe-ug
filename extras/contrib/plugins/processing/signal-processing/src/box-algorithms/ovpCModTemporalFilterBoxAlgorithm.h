// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CModTemporalFilterBoxAlgorithm final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_Box_ModTemporalFilterBoxAlgorithm)

protected:
    //update the settings value from the UI
    //return true if any setting has changed
    bool updateSettings();

    //compute the filter coeff
    //return false if failed
    bool compute();

    Kernel::IAlgorithmProxy* m_decoder                       = nullptr;
    Kernel::IAlgorithmProxy* m_encoder                       = nullptr;
    Kernel::IAlgorithmProxy* m_computeModTemporalFilterCoefs = nullptr;
    Kernel::IAlgorithmProxy* m_applyModTemporalFilter        = nullptr;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_bufferToDecode;
    Kernel::TParameterHandler<CMemoryBuffer*> op_encodedBuffer;
    uint64_t m_lastEndTime = 0;

    //setting last value to avoid recompute if they haven't changed
    CString m_filterMethod;
    CString m_filterType;
    CString m_filterOrder;
    CString m_lowBand;
    CString m_highBand;
    CString m_passBandRiple;

    bool m_hasBeenInit = false;
};

class CModTemporalFilterBoxAlgorithmDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Modifiable Temporal filter"; }
    CString getAuthorName() const override { return "Guillaume Gibert / lmahe"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821 INRIA"; }
    CString getShortDescription() const override { return "Applies temporal filtering on time signal, modifiable parameters"; }

    CString getDetailedDescription() const override
    {
        return "The user can choose among a variety of filter types to process the signal and change the settings online";
    }

    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Box_ModTemporalFilterBoxAlgorithm; }
    IPluginObject* create() override { return new CModTemporalFilterBoxAlgorithm(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Filtered signal", OV_TypeId_Signal);
        prototype.addSetting("Filter method", OVP_TypeId_FilterMethod, "Butterworth", true);
        prototype.addSetting("Filter type", OVP_TypeId_FilterType, "Band Pass", true);
        prototype.addSetting("Filter order", OV_TypeId_Integer, "4", true);
        prototype.addSetting("Low cut frequency (Hz)", OV_TypeId_Float, "29", true);
        prototype.addSetting("High cut frequency (Hz)", OV_TypeId_Float, "40", true);
        prototype.addSetting("Pass band ripple (dB)", OV_TypeId_Float, "0.5", true);

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_Box_ModTemporalFilterBoxAlgorithmDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
