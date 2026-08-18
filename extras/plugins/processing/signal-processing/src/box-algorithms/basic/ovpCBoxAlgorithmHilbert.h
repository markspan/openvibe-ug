#pragma once

#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmHilbert
 * \author Alison Cellard (Inria)
 * \date Thu Jun  6 13:47:53 2013
 * \brief The class CBoxAlgorithmHilbert describes the box Phase and Envelope.
 *
 */
class CBoxAlgorithmHilbert final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;

    bool process() override;

    // As we do with any class in openvibe, we use the macro below 
    // to associate this box to an unique identifier. 
    // The inheritance information is also made available, 
    // as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Hilbert)

protected:
    // Signal stream decoder
    Toolkit::TSignalDecoder<CBoxAlgorithmHilbert> m_decoder;
    // Signal stream encoder
    Toolkit::TSignalEncoder<CBoxAlgorithmHilbert> m_algo1Encoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmHilbert> m_algo2Encoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmHilbert> m_algo3Encoder;

    Kernel::IAlgorithmProxy* m_hilbertAlgo = nullptr;

    Kernel::TParameterHandler<CMatrix*> ip_signalMatrix;
    Kernel::TParameterHandler<CMatrix*> op_hilbertMatrix;
    Kernel::TParameterHandler<CMatrix*> op_envelopeMatrix;
    Kernel::TParameterHandler<CMatrix*> op_phaseMatrix;
};

/**
 * \class CBoxAlgorithmHilbertDesc
 * \author Alison Cellard (Inria)
 * \date Thu Jun  6 13:47:53 2013
 * \brief Descriptor of the box Phase and Envelope.
 *
 */
class CBoxAlgorithmHilbertDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Hilbert Transform"; }
    CString getAuthorName() const override { return "Alison Cellard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Hilbert transform, Phase and Envelope from discrete-time analytic signal using Hilbert"; }

    CString getDetailedDescription() const override
    {
        return "Return Hilbert transform, phase and envelope of the input signal using analytic signal computation";
    }

    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1"; }
    CString getStockItemName() const override { return "gtk-new"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Hilbert; }
    IPluginObject* create() override { return new CBoxAlgorithmHilbert; }


    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Signal",OV_TypeId_Signal);

        prototype.addOutput("Hilbert Transform", OV_TypeId_Signal);
        prototype.addOutput("Envelope",OV_TypeId_Signal);
        prototype.addOutput("Phase",OV_TypeId_Signal);

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_HilbertDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
