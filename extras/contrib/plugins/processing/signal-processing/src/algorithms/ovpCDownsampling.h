#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CDownsampling final : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_Downsampling)

protected:
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<uint64_t> ip_newSampling;
    Kernel::TParameterHandler<CMatrix*> ip_signalMatrix;
    Kernel::TParameterHandler<CMatrix*> op_signalMatrix;
    double* m_lastValueOrigSignal = nullptr;
    double m_lastTimeOrigSignal   = 0;
    double m_lastTimeNewSignal    = 0;
    bool m_first                  = false;
};

class CDownsamplingDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Downsampling"; }
    CString getAuthorName() const override { return "G. Gibert - E. Maby - P.E. Aguera"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return "Downsamples input signal."; }
    CString getDetailedDescription() const override { return "Downsamples input signal to the new sampling rate chosen by user."; }
    CString getCategory() const override { return "Signal processing Gpl/Basic"; }
    CString getVersion() const override { return "1.1"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_Downsampling; }
    IPluginObject* create() override { return new CDownsampling(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_Downsampling_InputParameterId_Sampling, "Sampling frequency", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_Downsampling_InputParameterId_NewSampling, "New sampling frequency", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_Downsampling_InputParameterId_SignalMatrix, "Signal matrix", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_Downsampling_OutputParameterId_SignalMatrix, "Signal matrix", Kernel::ParameterType_Matrix);
        prototype.addInputTrigger(OVP_Algorithm_Downsampling_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_Downsampling_InputTriggerId_Resample, "Resample");
        prototype.addInputTrigger(OVP_Algorithm_Downsampling_InputTriggerId_ResampleWithHistoric, "Resample with historic");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_DownsamplingDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
