#pragma once

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmQuadraticForm final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override { return 0; } // the box clock frequency
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& /*msg*/) override { return true; }
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_QuadraticForm)

protected:
    //algorithms for encoding and decoding EBML stream
    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::IAlgorithmProxy* m_decoder = nullptr;

    //input and output buffers
    Kernel::TParameterHandler<const CMemoryBuffer*> m_iEBMLBufferHandle;
    Kernel::TParameterHandler<CMemoryBuffer*> m_oEBMLBufferHandle;

    //the signal matrices (input and output)
    Kernel::TParameterHandler<CMatrix*> m_iMatrixHandle;
    Kernel::TParameterHandler<CMatrix*> m_oMatrixHandle;

    //start and end times
    uint64_t m_startTime = 0;
    uint64_t m_endTime   = 0;

    //The matrix used in the quadratic form: the quadratic operator
    CMatrix m_quadraticOperator;

    //dimensions (number of input channels and number of samples) of the input buffer
    size_t m_nChannels         = 0;
    size_t m_nSamplesPerBuffer = 0;
};

class CBoxAlgorithmQuadraticFormDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Quadratic Form"; }
    CString getAuthorName() const override { return "Fabien Lotte"; }
    CString getAuthorCompanyName() const override { return "IRISA-INSA Rennes"; }
    CString getShortDescription() const override { return "Perform a quadratic matrix operation on the input signals m (result = m^T * A * m)"; }

    CString getDetailedDescription() const override
    {
        return
                "A square matrix A (which can be seen as a spatial filter) is applied to the input signals m (a vector). Then the transpose m^T of the input signals is multiplied to the resulting vector. In other words the output o is such as: o = m^T * A * m.";
    }

    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "0.1"; }
    CString getStockItemName() const override { return "gtk-missing-image"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_QuadraticForm; }
    IPluginObject* create() override { return new CBoxAlgorithmQuadraticForm; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("input signal", OV_TypeId_Signal);
        prototype.addOutput("output signal", OV_TypeId_Signal);
        prototype.addSetting("Matrix values", OV_TypeId_String, "1 0 0 1");
        prototype.addSetting("Number of rows/columns (square matrix)", OV_TypeId_Integer, "2");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_QuadraticFormDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
