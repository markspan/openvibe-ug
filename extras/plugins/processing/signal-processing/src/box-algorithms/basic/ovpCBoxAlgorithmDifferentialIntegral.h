#pragma once

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmDifferentialIntegral
 * \author Jozef Legeny (INRIA)
 * \date Thu Oct 27 15:24:05 2011
 * \brief The class CBoxAlgorithmDifferentialIntegral describes the box DifferentialIntegral.
 *
 */
class CBoxAlgorithmDifferentialIntegral final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_DifferentialIntegral)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmDifferentialIntegral> m_decoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmDifferentialIntegral> m_encoder;

private:
    double operation(const double a, const double b) const;
    EDifferentialIntegralOperation m_operation = EDifferentialIntegralOperation::Differential;
    uint64_t m_filterOrder                     = 0;

    /// Holds the differentials/integrals of all orders from the previous step
    double** m_pastData = nullptr;
    double** m_tmpData  = nullptr;

    /// Is true when the filter is stabilized
    bool* m_stabilized = nullptr;
    /// Counts the samples up to the filter order, used to stabilize the filter
    size_t* m_step = nullptr;
};


/**
 * \class CBoxAlgorithmDifferentialIntegralDesc
 * \author Jozef Legeny (INRIA)
 * \date Thu Oct 27 15:24:05 2011
 * \brief Descriptor of the box DifferentialIntegral.
 *
 */
class CBoxAlgorithmDifferentialIntegralDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal Differential/Integral"; }
    CString getAuthorName() const override { return "Jozef Legeny"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Calculates a differential or an integral of a signal"; }
    CString getDetailedDescription() const override { return "Calculates a differential or an integral of a signal."; }
    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_DifferentialIntegral; }
    IPluginObject* create() override { return new CBoxAlgorithmDifferentialIntegral; }

    /*
    virtual IBoxListener* createBoxListener() const               { return new CBoxAlgorithmDifferentialIntegralListener; }
    virtual void releaseBoxListener(IBoxListener* listener) const { delete listener; }
    */
    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input Signal",OV_TypeId_Signal);

        prototype.addOutput("Output Signal",OV_TypeId_Signal);

        prototype.addSetting("Operation", OVP_TypeId_DifferentialIntegralOperation, "Differential");
        prototype.addSetting("Order", OV_TypeId_Integer, "1");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_DifferentialIntegralDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
