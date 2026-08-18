#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmBrainampFileReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_BrainampFileReader)

protected:
    Kernel::IAlgorithmProxy* m_reader                = nullptr;
    Kernel::IAlgorithmProxy* m_experimentInfoEncoder = nullptr;
    Kernel::IAlgorithmProxy* m_signalEncoder         = nullptr;
    Kernel::IAlgorithmProxy* m_stimEncoder           = nullptr;

    bool m_headerSent = false;
};

class CBoxAlgorithmBrainampFileReaderDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "BrainVision Format file reader"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Reads input having the BrainAmp file format"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/BrainVision Format"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-open"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_BrainampFileReader; }
    IPluginObject* create() override { return new CBoxAlgorithmBrainampFileReader; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box outputs
        prototype.addOutput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addOutput("EEG stream", OV_TypeId_Signal);
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);

        // Adds settings
        prototype.addSetting("Filename (header)", OV_TypeId_Filename, "");
        prototype.addSetting("Epoch size (in sec)", OV_TypeId_Float, "0.0625");
        prototype.addSetting("Convert stimuli to OpenViBE labels", OV_TypeId_Boolean, "true");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_BrainampFileReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
