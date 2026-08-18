#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "ovpCInputChannel.h"
#include "ovpCOutputChannel.h"

#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmSynchro final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Synchro)

protected:
    //Intern ressources
    bool m_stimulationReceivedStart = false;

    // new
    CInputChannel m_inputChannel;
    COutputChannel m_outputChannel;
};

class CBoxAlgorithmSynchroDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Stream Synchronization"; }
    CString getAuthorName() const override { return "Gelu Ionescu & Matthieu Goyat"; }
    CString getAuthorCompanyName() const override { return "GIPSA-lab"; }
    CString getShortDescription() const override { return "Synchronize two acquisition servers"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-missing-image"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Synchro; }
    IPluginObject* create() override { return new CBoxAlgorithmSynchro; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addInput("Input stimulation", OV_TypeId_Stimulations);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addOutput("Output stimulation", OV_TypeId_Stimulations);
        prototype.addSetting("Synchronisation stimulation", OV_TypeId_Stimulation, "OVTK_StimulationId_ExperimentStart");
        // prototype.addFlag   (Kernel::BoxFlag_CanModifyInput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_SynchroDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
