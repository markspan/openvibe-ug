#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CAlgoUnivariateStatistic final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_AlgoUnivariateStatistic)

protected:
    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<CMatrix*> op_MeanMatrix;
    Kernel::TParameterHandler<CMatrix*> op_VarianceMatrix;
    Kernel::TParameterHandler<CMatrix*> op_RangeMatrix;
    Kernel::TParameterHandler<CMatrix*> op_MedianMatrix;
    Kernel::TParameterHandler<CMatrix*> op_IQRMatrix;
    Kernel::TParameterHandler<CMatrix*> op_PercentileMatrix;

    Kernel::TParameterHandler<bool> ip_isMeanActive;
    Kernel::TParameterHandler<bool> ip_isVarianceActive;
    Kernel::TParameterHandler<bool> ip_isRangeActive;
    Kernel::TParameterHandler<bool> ip_isMedianActive;
    Kernel::TParameterHandler<bool> ip_isIQRActive;
    Kernel::TParameterHandler<bool> ip_isPercentileActive;
    Kernel::TParameterHandler<uint64_t> ip_percentileValue;

    Kernel::TParameterHandler<double> op_compression;

    bool m_isSumActive      = false;
    bool m_isSqaresumActive = false;
    bool m_isSortActive     = false;
    CMatrix m_sumMatrix;
    CMatrix m_sumMatrix2;
    CMatrix m_sortMatrix;

    uint64_t m_percentileValue = 0;

    bool setMatrixDimension(CMatrix* matrix, CMatrix* ref);
};

class CAlgoUnivariateStatisticDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal Statistic"; }
    CString getAuthorName() const override { return "Matthieu Goyat"; }
    CString getAuthorCompanyName() const override { return "Gipsa-lab"; }
    CString getShortDescription() const override { return "Calculate Mean, Variance, Median, etc. on the incoming buffer"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Statistics"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_AlgoUnivariateStatistic; }
    IPluginObject* create() override { return new CAlgoUnivariateStatistic(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_Matrix, "Matrix input", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_MeanActive, "active mean", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_VarActive, "active variance", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_RangeActive, "active range", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_MedActive, "active median", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_IQRActive, "active IQR", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentActive, "active Percentile", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(OVP_Algorithm_UnivariateStatistic_InputParameterId_PercentValue, "Percentile Value", Kernel::ParameterType_Integer);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Mean, "Mean output", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Var, "Variance output", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Range, "Range output", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Med, "Median output", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_IQR, "Inter-Quantile-Range output", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Percent, "Percentile output", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_UnivariateStatistic_OutputParameterId_Compression, "compression ratio", Kernel::ParameterType_Float);

        prototype.addInputTrigger(OVP_Algorithm_UnivariateStatistic_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_UnivariateStatistic_InputTriggerId_Process, "Process");
        prototype.addOutputTrigger(OVP_Algorithm_UnivariateStatistic_OutputTriggerId_ProcessDone, "Done");
        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_AlgoUnivariateStatisticDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
