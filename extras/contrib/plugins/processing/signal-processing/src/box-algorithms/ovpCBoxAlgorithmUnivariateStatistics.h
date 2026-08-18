#pragma once

#include "../ovp_defines.h"
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxUnivariateStatistic final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_UnivariateStatistic)

protected:
    Kernel::IAlgorithmProxy* m_decoder           = nullptr;
    Kernel::IAlgorithmProxy* m_meanEncoder       = nullptr;
    Kernel::IAlgorithmProxy* m_varianceEncoder   = nullptr;
    Kernel::IAlgorithmProxy* m_rangeEncoder      = nullptr;
    Kernel::IAlgorithmProxy* m_medianEncoder     = nullptr;
    Kernel::IAlgorithmProxy* m_iqrEncoder        = nullptr;
    Kernel::IAlgorithmProxy* m_percentileEncoder = nullptr;
    Kernel::IAlgorithmProxy* m_matrixStatistic   = nullptr;

    Kernel::TParameterHandler<double> op_compression;
    Kernel::TParameterHandler<uint64_t> op_sampling;

    Kernel::TParameterHandler<bool> ip_isMeanActive;
    Kernel::TParameterHandler<bool> ip_isVarianceActive;
    Kernel::TParameterHandler<bool> ip_isRangeActive;
    Kernel::TParameterHandler<bool> ip_isMedianActive;
    Kernel::TParameterHandler<bool> ip_isIQRActive;
    Kernel::TParameterHandler<bool> ip_isPercentileActive;
    Kernel::TParameterHandler<uint64_t> ip_parameterValue;

    CIdentifier m_inputTypeID = CIdentifier::undefined();
};

class CBoxUnivariateStatisticDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Univariate Statistics"; }
    CString getAuthorName() const override { return "Matthieu Goyat"; }
    CString getAuthorCompanyName() const override { return "Gipsa-lab"; }
    CString getShortDescription() const override { return "Mean, Variance, Median, etc. on the incoming Signal"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal processing/Statistics"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-missing-image"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_UnivariateStatistic; }
    IPluginObject* create() override { return new CBoxUnivariateStatistic(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Signals", OV_TypeId_Signal);
        prototype.addOutput("Mean", OV_TypeId_Signal);
        prototype.addOutput("Variance", OV_TypeId_Signal);
        prototype.addOutput("Range", OV_TypeId_Signal);
        prototype.addOutput("Median", OV_TypeId_Signal);
        prototype.addOutput("IQR", OV_TypeId_Signal);
        prototype.addOutput("Percentile", OV_TypeId_Signal);
        prototype.addSetting("Mean", OV_TypeId_Boolean, "true");
        prototype.addSetting("Variance", OV_TypeId_Boolean, "true");
        prototype.addSetting("Range", OV_TypeId_Boolean, "true");
        prototype.addSetting("Median", OV_TypeId_Boolean, "true");
        prototype.addSetting("IQR", OV_TypeId_Boolean, "true");
        prototype.addSetting("Percentile", OV_TypeId_Boolean, "true");
        prototype.addSetting("Percentile value", OV_TypeId_Float, "30");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_UnivariateStatisticDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
