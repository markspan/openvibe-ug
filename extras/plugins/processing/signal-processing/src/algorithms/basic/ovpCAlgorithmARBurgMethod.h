# pragma once

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CAlgorithmARBurgMethod final : public Toolkit::TAlgorithm<IAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TAlgorithm<IAlgorithm>, OVP_ClassId_Algorithm_ARBurgMethod)

protected:
    Kernel::TParameterHandler<CMatrix*> ip_pMatrix; // input matrix
    Kernel::TParameterHandler<CMatrix*> op_pMatrix; // output matrix
    Kernel::TParameterHandler<uint64_t> ip_Order;

private:
    Eigen::VectorXd m_errForward;   // Error Forward        
    Eigen::VectorXd m_errBackward;  //Error Backward
    Eigen::VectorXd m_arCoefs;      // AutoRegressive Coefficents

    Eigen::VectorXd m_errForwardPrediction; // Error Forward prediction
    Eigen::VectorXd m_errBackwardPrediction;    //Error Backward prediction

    Eigen::VectorXd m_error;                    // Total error vector

    double m_k     = 0;
    size_t m_order = 0;
};

class CAlgorithmARBurgMethodDesc final : public IAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "AR Burg's Method algorithm"; }
    CString getAuthorName() const override { return "Alison Cellard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Extract AR coefficient using Burg's Method"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Signal Processing"; }
    CString getVersion() const override { return "1.0"; }
    virtual CString getStockItemName() const { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Algorithm_ARBurgMethod; }
    IPluginObject* create() override { return new CAlgorithmARBurgMethod; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        prototype.addInputParameter(OVP_Algorithm_ARBurgMethod_InputParameterId_Matrix, "Vector", Kernel::ParameterType_Matrix);
        prototype.addOutputParameter(OVP_Algorithm_ARBurgMethod_OutputParameterId_Matrix, "Coefficents Vector", Kernel::ParameterType_Matrix);
        prototype.addInputParameter(OVP_Algorithm_ARBurgMethod_InputParameterId_UInteger, "Order", Kernel::ParameterType_UInteger);

        prototype.addInputTrigger(OVP_Algorithm_ARBurgMethod_InputTriggerId_Initialize, "Initialize");
        prototype.addInputTrigger(OVP_Algorithm_ARBurgMethod_InputTriggerId_Process, "Process");
        prototype.addOutputTrigger(OVP_Algorithm_ARBurgMethod_OutputTriggerId_ProcessDone, "Process done");

        return true;
    }

    _IsDerivedFromClass_Final_(IAlgorithmDesc, OVP_ClassId_Algorithm_ARBurgMethodDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
