#pragma once

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>

// This class could be in its own file
class HilbertTransform
{
public:

    bool transform(const Eigen::VectorXcd& in, Eigen::VectorXcd& out);

private:
    Eigen::VectorXcd m_signalFourier;    // Fourier Transform of the input signal
    Eigen::VectorXcd m_hilbert;          // Vector h used to apply Hilbert transform

    Eigen::FFT<double, Eigen::internal::kissfft_impl<double>> m_fft; // Instance of the fft transform
};

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CAlgorithmHilbertTransform final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_HilbertTransform)


protected:

    Kernel::TParameterHandler<CMatrix*> ip_matrix; //input matrix
    Kernel::TParameterHandler<CMatrix*> op_hilbertMatrix; //output matrix 1 : Hilbert transform of the signal
    Kernel::TParameterHandler<CMatrix*> op_envelopeMatrix; //output matrix 2 : Envelope of the signal
    Kernel::TParameterHandler<CMatrix*> op_phaseMatrix; //output matrix 3 : Phase of the signal

    HilbertTransform m_hilbert; // Instance of the Hilbert transform doing the actual computation
};

class CAlgorithmHilbertTransformDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Hilbert Transform"; }
    CString getAuthorName() const override { return "Alison Cellard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Computes the Hilbert transform of a signal"; }

    CString getDetailedDescription() const override
    {
        return "Give the analytic signal ua(t) = u(t) + iH(u(t)) of the input signal u(t) using Hilbert transform";
    }

    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "0.2"; }
    virtual CString getStockItemName() const { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_HilbertTransform; }
    IPluginObject* create() override { return new CAlgorithmHilbertTransform; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix, "Matrix", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix, "Hilbert Matrix", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix, "Envelope Matrix", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix, "Phase Matrix", Kernel::ParameterType_Matrix);

        prototype.addInputTrigger(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_HilbertTransform_InputTriggerId_Process, "Process");
        prototype.addOutputTrigger(OVP_Algorithm_HilbertTransform_OutputTriggerId_ProcessDone, "Process done");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_HilbertTransformDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
