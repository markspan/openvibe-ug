
#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "matrixDisplayWindow.h"

#include <array>
#include <string>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CBoxAlgorithmMatrixDisplay final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_MatrixDisplay)

protected:
    // we need an algorithm to decode the EBML stream (memory buffer) into a Streamed Matrix

    // for the TARGET
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatrixDisplay> m_decoder;
    Kernel::IAlgorithmProxy* m_iMatrix = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CMatrix*> op_matrix;

    MatrixDisplayWindow *m_view = nullptr;
    Parameters::ParameterColorGradient m_colorGradientParameter;
    std::vector<std::array<int, 4> > m_interpolatedColorGardient;
    CMatrix m_colorGradient;
    size_t m_gradientSteps = 0;
    double m_max           = 0;
    double m_min           = 0;

    bool m_symetricMinMax = false;
    bool m_realTimeMinMax = false;

public:
    bool m_ShowValues = false;
    bool m_ShowColors = false;

    bool ResetColors();
};

class CBoxAlgorithmMatrixDisplayDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matrix Display"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Display a streamed matrix"; }
    CString getDetailedDescription() const override { return "The streamed matrix can be visualized using a table of values and/or a color gradient."; }
    CString getCategory() const override { return "Visualization/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-select-color"; }

    CIdentifier getCreatedClass() const override { return Box_MatrixDisplay; }
    IPluginObject* create() override { return new CBoxAlgorithmMatrixDisplay; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Color gradient", OV_TypeId_ColorGradient, "0:2,36,58; 50:100,100,100; 100:83,17,20");
        prototype.addSetting("Steps", OV_TypeId_Integer, "100");
        prototype.addSetting("Symetric min/max", OV_TypeId_Boolean, "false");
        prototype.addSetting("Real time min/max", OV_TypeId_Boolean, "false");
        prototype.addInput("Matrix", OV_TypeId_StreamedMatrix);
        // prototype.addFlag   (Kernel::BoxFlag_IsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_MatrixDisplayDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
