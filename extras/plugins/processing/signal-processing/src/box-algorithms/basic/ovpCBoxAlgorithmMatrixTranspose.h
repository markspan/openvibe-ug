#pragma once

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmMatrixTranspose final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_MatrixTranspose)

protected:
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrixTranspose> m_decoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatrixTranspose> m_encoder;
};


class CBoxAlgorithmMatrixTransposeDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix Transpose"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Transposes each matrix of the input stream"; }

    CString getDetailedDescription() const override
    {
        return "Only works for 1 and 2 dimensional matrices. One-dimensional matrixes will be upgraded to two dimensions: [N x 1]";
    }

    CString getCategory() const override { return "Signal processing/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_MatrixTranspose; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrixTranspose; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input matrix", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Output matrix", OV_TypeId_StreamedMatrix);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_MatrixTransposeDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
