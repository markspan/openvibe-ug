// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once
#include "../ovp_defines.h"

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmCSPSpatialFilterTrainer final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_CSPSpatialFilterTrainer)

protected:
    Toolkit::TStimulationDecoder<CBoxAlgorithmCSPSpatialFilterTrainer>* m_stimDecoder        = nullptr;
    Toolkit::TSignalDecoder<CBoxAlgorithmCSPSpatialFilterTrainer>* m_signalDecoderCondition1 = nullptr;
    Toolkit::TSignalDecoder<CBoxAlgorithmCSPSpatialFilterTrainer>* m_signalDecoderCondition2 = nullptr;


    Toolkit::TStimulationEncoder<CBoxAlgorithmCSPSpatialFilterTrainer> m_encoder;

    uint64_t m_stimID = 0;
    CString m_spatialFilterConfigFilename;
    size_t m_filterDimension = 0;
    bool m_saveAsBoxConfig   = false;
};

class CBoxAlgorithmCSPSpatialFilterTrainerDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "CSP Spatial Filter Trainer"; }
    CString getAuthorName() const override { return "Dieter Devlaminck"; }
    CString getAuthorCompanyName() const override { return "Ghent University"; }
    CString getShortDescription() const override { return "Computes spatial filter coeffcients according to the Common Spatial Pattern algorithm."; }

    CString getDetailedDescription() const override
    {
        return "The CSP algortihm increases the signal variance for one condition while minimizing the variance for the other condition.";
    }

    CString getCategory() const override { return "Signal processing/Filtering"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_CSPSpatialFilterTrainer; }
    IPluginObject* create() override { return new CBoxAlgorithmCSPSpatialFilterTrainer; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Signal condition 1", OV_TypeId_Signal);
        prototype.addInput("Signal condition 2", OV_TypeId_Signal);
        prototype.addSetting("Train Trigger", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Session");
        prototype.addSetting("Spatial filter configuration", OV_TypeId_Filename, "");
        prototype.addSetting("Filter dimension", OV_TypeId_Integer, "2");
        prototype.addSetting("Save as box config", OV_TypeId_Boolean, "true");

        prototype.addOutput("Train-completed Flag", OV_TypeId_Stimulations);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_CSPSpatialFilterTrainerDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyITPP
