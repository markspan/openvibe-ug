#pragma once

#if defined TARGET_HAS_ThirdPartyITPP

#include "../ovp_defines.h"

#include <openvibe/ov_all.h>

#include <toolkit/ovtk_all.h>

#include <itpp/itstat.h>
#include <itpp/itsignal.h>

#define PREC 27
#define MAXEXP 1024
#define MINEXP -1077
#define MAXNUM 1.79769313486231570815E308

typedef struct SComplex
{
    double real;
    double imag;
} cmplex;

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CComputeTemporalFilterCoefficients : virtual public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_ComputeTemporalFilterCoefs)

    // Functions for Butterworth and Chebyshev filters
    void findSPlanePolesAndZeros();
    void convertSPlanePolesAndZerosToZPlane();
    // Functions for Complex arithmetic
    double absComplex(cmplex* z) const;
    void divComplex(cmplex* a, cmplex* b, cmplex* c) const;
    void sqrtComplex(cmplex* z, cmplex* w) const;
    static void addComplex(cmplex* a, cmplex* b, cmplex* c);
    static void mulComplex(cmplex* a, cmplex* b, cmplex* c);
    static void subComplex(cmplex* a, cmplex* b, cmplex* c);

protected:
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<uint64_t> ip_filterMethod;
    Kernel::TParameterHandler<uint64_t> ip_filterType;
    Kernel::TParameterHandler<uint64_t> ip_filterOrder;
    Kernel::TParameterHandler<double> ip_lowCutFrequency;
    Kernel::TParameterHandler<double> ip_highCutFrequency;
    Kernel::TParameterHandler<double> ip_bandPassRipple;

    Kernel::TParameterHandler<CMatrix*> op_matrix;

    size_t m_filterOrder         = 0;
    EFilterMethod m_filterMethod = EFilterMethod::Butterworth;
    EFilterType m_filterType     = EFilterType::BandPass;
    double m_lowPassBandEdge     = 0;
    double m_highPassBandEdge    = 0;
    double m_passBandRipple      = 0;
    size_t m_arraySize           = 0;
    itpp::vec m_coefFilterNum;
    itpp::vec m_coefFilterDen;
    double m_phi    = 0;
    double m_scale  = 0;
    double m_tanAng = 0;
    double m_cosGam = 0;
    double m_cbp    = 0;

    size_t m_sampling = 0;
    size_t m_nyquist  = 0;

    size_t m_nPoles = 0;
    size_t m_nZeros = 0;

    itpp::vec m_zs;
    size_t m_zOrd = 0;

    double m_rho = 0;
    double m_eps = 0;

    size_t m_dimSize = 0;
};

class CComputeTemporalFilterCoefficientsDesc final : virtual public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Compute Filter Coefficients"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM/U821"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Algorithm/Signal processing/Filter"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ComputeTemporalFilterCoefs; }
    IPluginObject* create() override { return new CComputeTemporalFilterCoefficients(); }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_Sampling, "Sampling frequency", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterMethod, "Filter method", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterType, "Filter type", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_FilterOrder, "Filter order", Kernel::ParameterType_UInteger);
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_LowCutFrequency, "Low cut frequency",
                                    Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_HighCutFrequency, "High cut frequency",
                                    Kernel::ParameterType_Float);
        prototype.addInputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_InputParameterId_BandPassRipple, "Band pass ripple", Kernel::ParameterType_Float);
        prototype.addOutputParameter(OVP_Algorithm_ComputeTemporalFilterCoefs_OutputParameterId_Matrix, "Matrix", Kernel::ParameterType_Matrix);
        prototype.addInputTrigger(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_ComputeTemporalFilterCoefs_InputTriggerId_ComputeCoefs, "Compute coefficients");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_ComputeTemporalFilterCoefsDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyITPP
