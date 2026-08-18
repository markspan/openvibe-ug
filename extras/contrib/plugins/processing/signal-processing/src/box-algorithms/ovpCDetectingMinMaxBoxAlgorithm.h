// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CDetectingMinMaxBoxAlgorithm final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_Box_DetectingMinMaxBoxAlgorithm)

protected:
    Kernel::IAlgorithmProxy* m_decoder         = nullptr;
    Kernel::IAlgorithmProxy* m_encoder         = nullptr;
    Kernel::IAlgorithmProxy* m_detectingMinMax = nullptr;

    uint64_t m_lastStartTime = 0;
    uint64_t m_lastEndTime   = 0;

    bool m_minFlag = false;
    bool m_maxFlag = false;
};

class CDetectingMinMaxBoxAlgorithmDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Min/Max detection"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return "Outputs the minimum or the maximum value inside a time window"; }
    CString getDetailedDescription() const override { return "Either min or max detection can be specified as a box parameter"; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Box_DetectingMinMaxBoxAlgorithm; }
    IPluginObject* create() override { return new CDetectingMinMaxBoxAlgorithm(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input epochs", OV_TypeId_Signal);
        prototype.addOutput("Output epochs", OV_TypeId_StreamedMatrix);
        prototype.addSetting("Min/Max", OVP_TypeId_MinMax, "Max");
        prototype.addSetting("Time window start", OV_TypeId_Float, "300");
        prototype.addSetting("Time window end", OV_TypeId_Float, "500");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_Box_DetectingMinMaxBoxAlgorithmDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
