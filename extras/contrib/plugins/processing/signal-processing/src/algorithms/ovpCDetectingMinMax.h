#pragma once

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CDetectingMinMax final : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_DetectingMinMax)

protected:
    Kernel::TParameterHandler<CMatrix*> ip_signalMatrix;
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<double> ip_timeWindowStart;
    Kernel::TParameterHandler<double> ip_timeWindowEnd;

    Kernel::TParameterHandler<CMatrix*> op_signalMatrix;
};

class CDetectingMinMaxDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Detects Min or Max of input buffer"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Algorithm/Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_DetectingMinMax; }
    IPluginObject* create() override { return new CDetectingMinMax(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_SignalMatrix, "Signal input matrix", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowStart, "Time window start", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_TimeWindowEnd, "Time window end", Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_DetectingMinMax_InputParameterId_Sampling, "Sampling frequency", Kernel::ParameterType_UInteger);
        prototype.addOutputParameter(OVP_Algorithm_DetectingMinMax_OutputParameterId_SignalMatrix, "Signal output matrix", Kernel::ParameterType_Matrix);
        prototype.addInputTrigger(OVP_Algorithm_DetectingMinMax_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMin, "Detects min");
        prototype.addInputTrigger(OVP_Algorithm_DetectingMinMax_InputTriggerId_DetectsMax, "Detects max");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_DetectingMinMaxDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
