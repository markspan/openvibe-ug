// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>

#include <map>
#include <string>

#ifndef  CString2Boolean
#define CString2Boolean(string) (strcmp(string,"true"))?0:1
#endif

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
* The Spectral Anlaysis plugin's main class.
*/
class CSpectralAnalysis final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CSpectralAnalysis() { }

    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_SpectralAnalysis)

private:
    //start time and end time of the last arrived chunk
    uint64_t m_lastChunkStartTime = 0;
    uint64_t m_lastChunkEndTime   = 0;

    //codecs
    Toolkit::TSignalDecoder<CSpectralAnalysis> m_decoder;
    Toolkit::TSpectrumEncoder<CSpectralAnalysis> m_encoders[4];

    ///number of channels
    size_t m_nChannel       = 0;
    size_t m_sampling       = 0;
    size_t m_nFrequencyBand = 0;
    size_t m_nSample        = 0;

    size_t m_halfFFTSize = 1;           // m_nSample / 2 + 1;

    bool m_amplitudeSpectrum = false;
    bool m_phaseSpectrum     = false;
    bool m_realPartSpectrum  = false;
    bool m_imagPartSpectrum  = false;
};

class CSpectralAnalysisDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Spectral Analysis (FFT)(INSERM contrib)"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM"; }
    CString getShortDescription() const override { return "Compute spectral analysis using Fast Fourier Transform"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Spectral Analysis"; }
    CString getVersion() const override { return "0.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_SpectralAnalysis; }
    IPluginObject* create() override { return new CSpectralAnalysis(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);

        prototype.addOutput("Amplitude", OV_TypeId_Spectrum);
        prototype.addOutput("Phase", OV_TypeId_Spectrum);
        prototype.addOutput("Real Part", OV_TypeId_Spectrum);
        prototype.addOutput("Imag Part", OV_TypeId_Spectrum);

        prototype.addSetting("Spectral components", OVP_TypeId_SpectralComponent, "Amplitude");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_SpectralAnalysisDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
