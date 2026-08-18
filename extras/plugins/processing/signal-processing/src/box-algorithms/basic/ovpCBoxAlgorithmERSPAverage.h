#pragma once

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmERSPAverage final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_ERSPAverage)

protected:
    bool appendChunk(const CMatrix& chunk, uint64_t startTime, uint64_t endTime);
    bool computeAndSend();

    Toolkit::TSpectrumDecoder<CBoxAlgorithmERSPAverage> m_decoderSpectrum;
    Toolkit::TStimulationDecoder<CBoxAlgorithmERSPAverage> m_decoderStimulations;
    Toolkit::TSpectrumEncoder<CBoxAlgorithmERSPAverage> m_encoder;

    struct STimestamp
    {
        uint64_t start, end;
    };

    std::vector<std::vector<CMatrix*>> m_cachedSpectra;
    std::vector<STimestamp> m_timestamps;

    size_t m_currentChunk = 0;
    size_t m_numTrials    = 0;

    uint64_t m_epochingStim = 0;
    uint64_t m_computeStim  = 0;
};


class CBoxAlgorithmERSPAverageDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "ERSP Average"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }

    CString getShortDescription() const override
    {
        return "Averages a sequence of spectra per trial across multiple trials. The result is a sequence starting from t=0.";
    }

    CString getDetailedDescription() const override
    {
        return
                "Example: Given an input sequence [t,s1,s2,t,s3,s4] for two trials with s* the spectra and t a stimulation denoting the trial start, the box returns 1/2*(s1+s3), 1/2*(s2+s4).";
    }

    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_ERSPAverage; }
    IPluginObject* create() override { return new CBoxAlgorithmERSPAverage; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input spectra", OV_TypeId_Spectrum);
        prototype.addInput("Control stream", OV_TypeId_Stimulations);
        prototype.addOutput("Output spectra", OV_TypeId_Spectrum);

        prototype.addSetting("Trial start marker", OV_TypeId_Stimulation, "OVTK_GDF_Start_Of_Trial", false);
        prototype.addSetting("Computation trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_ExperimentStop", false);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ERSPAverageDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
