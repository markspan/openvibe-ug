#pragma once

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>

#include <toolkit/ovtk_all.h>

#include <itpp/itstat.h>
#include <itpp/itsignal.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CApplyTemporalFilter final : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_ApplyTemporalFilter)

protected:
    Kernel::TParameterHandler<CMatrix*> ip_signalMatrix;
    Kernel::TParameterHandler<CMatrix*> ip_filterCoefsMatrix;
    Kernel::TParameterHandler<CMatrix*> op_signalMatrix;

    itpp::vec m_coefFilterDen;
    itpp::vec m_coefFilterNum;
    std::vector<itpp::vec> m_currentStates;

    bool m_flagInitialize = false;
};

class CApplyTemporalFilterDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Temporal Filter (INSERM contrib)"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Algorithm/Signal processing/Filter"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ApplyTemporalFilter; }
    IPluginObject* create() override { return new CApplyTemporalFilter(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_ApplyTemporalFilter_InputParameterId_SignalMatrix, "Signal matrix", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(
            OVP_Algorithm_ApplyTemporalFilter_InputParameterId_FilterCoefsMatrix, "Filter coefficients matrix", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(
            OVP_Algorithm_ApplyTemporalFilter_OutputParameterId_FilteredSignalMatrix, "Filtered signal matrix", Kernel::ParameterType_Matrix);
        prototype.addInputTrigger(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilter, "Apply filter");
        prototype.addInputTrigger(OVP_Algorithm_ApplyTemporalFilter_InputTriggerId_ApplyFilterWithHistoric, "Apply filter with historic");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_ApplyTemporalFilterDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyITPP
