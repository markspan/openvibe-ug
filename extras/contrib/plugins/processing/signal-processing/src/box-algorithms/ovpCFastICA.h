// @copyright notice: Possibly due to dependencies, this box used to be GPL before upgrade to AGPL3

#pragma once

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <map>
#include <string>

// TODO create a member function to get rid of this
#ifndef  CString2Boolean
#define CString2Boolean(string) (strcmp(string,"true"))?0:1
#endif

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
* The FastICA plugin's main class.
*/
class CFastICA final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CFastICA() {}

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_FastICA)

protected:
    void computeICA();

    Toolkit::TSignalDecoder<CFastICA> m_decoder;
    Toolkit::TSignalEncoder<CFastICA> m_encoder;

    double* m_fifoBuffer = nullptr;
    CMatrix m_demixer;      // The estimated matrix W

    bool m_trained   = false;
    bool m_fileSaved = false;

    size_t m_buffSize = 0;
    size_t m_nSample  = 0;
    size_t m_nICs     = 0;
    size_t m_duration = 0;
    size_t m_nRepMax  = 0;
    size_t m_nTuneMax = 0;
    CString m_spatialFilterFilename;
    bool m_saveAsFile   = false;
    bool m_setFineTune  = false;
    double m_setMu      = 0;
    double m_epsilon    = 0;
    size_t m_nonLin     = 0;
    size_t m_type       = 0;
    EFastICAMode m_mode = EFastICAMode::ICA;
};

class CFastICADesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Independent Component Analysis (FastICA)"; }
    CString getAuthorName() const override { return "Guillaume Gibert / Jeff B."; }
    CString getAuthorCompanyName() const override { return "INSERM / Independent"; }
    CString getShortDescription() const override { return "Computes fast independent component analysis"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Independent component analysis"; }
    CString getVersion() const override { return "0.2"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_FastICA; }
    IPluginObject* create() override { return new CFastICA(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);

        prototype.addSetting("Number of components to extract", OV_TypeId_Integer, "4");
        prototype.addSetting("Operating mode", OVP_TypeId_FastICA_OperatingMode, "ICA");
        prototype.addSetting("Sample size (seconds) for estimation", OV_TypeId_Integer, "120");
        prototype.addSetting("Decomposition type", OVP_TypeId_FastICA_DecompositionType, "Symmetric");
        prototype.addSetting("Max number of reps for the ICA convergence", OV_TypeId_Integer, "100000");
        prototype.addSetting("Fine tuning", OV_TypeId_Boolean, "true");
        prototype.addSetting("Max number of reps for the fine tuning", OV_TypeId_Integer, "100");
        prototype.addSetting("Used nonlinearity", OVP_TypeId_FastICA_Nonlinearity, "Tanh");
        prototype.addSetting("Internal Mu parameter for FastICA", OV_TypeId_Float, "1.0");
        prototype.addSetting("Internal Epsilon parameter for FastICA", OV_TypeId_Float, "0.0001");
        prototype.addSetting("Spatial filter filename", OV_TypeId_Filename, "");
        prototype.addSetting("Save the spatial filter/demixing matrix", OV_TypeId_Boolean, "false");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_FastICADesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
