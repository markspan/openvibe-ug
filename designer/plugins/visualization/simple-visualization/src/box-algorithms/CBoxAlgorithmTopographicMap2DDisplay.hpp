#pragma once

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "topographicMap2DDisplay/CTopographicMapDatabase.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {
class CBoxAlgorithmTopographicMap2DDisplay final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmTopographicMap2DDisplay() = default;

    void release() override { delete this; }

    uint64_t getClockFrequency() override { return uint64_t(1LL) << 37; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_TopographicMap2DDisplay)

protected:
    Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmTopographicMap2DDisplay> m_decoder;

    Kernel::IAlgorithmProxy* m_interpolation                        = nullptr;
    CTopographicMapDatabase* m_database                             = nullptr;
    CSignalDisplayDrawable* m_view                                  = nullptr; //main object used for the display (contains all the GUI code)
    bool m_hasFirstBuffer                                           = false;
};

class CBoxAlgorithmTopographicMap2DDisplayDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "2D topographic map"; }
    CString getAuthorName() const override { return "Vincent Delannoy"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "This box demonstrates how to perform spherical spline interpolation"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Visualization/Topography"; }
    CString getVersion() const override { return "2.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return Box_TopographicMap2DDisplay; }
    IPluginObject* create() override { return new CBoxAlgorithmTopographicMap2DDisplay(); }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Interpolation type", TypeId_SphericalLinearInterpolation, "1");
        prototype.addSetting("Delay (in s)", OV_TypeId_Float, "0");
        prototype.addInput("Signal", OV_TypeId_StreamedMatrix);
        prototype.addInput("Channel localization", OV_TypeId_ChannelLocalisation);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_TopographicMap2DDisplayDesc)
};
}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
