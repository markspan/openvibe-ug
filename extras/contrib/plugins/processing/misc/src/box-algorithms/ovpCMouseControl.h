#pragma once

#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>

#include <string>
#include <map>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace Tools {
class CMouseControl : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CMouseControl() { }

    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_MouseControl)

protected:
    //codec
    Toolkit::TStreamedMatrixDecoder<CMouseControl>* m_decoder = nullptr;

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            ::Display* m_pMainDisplay = nullptr;
            ::Window m_oRootWindow;
#endif
};

class CMouseControlDesc : public IBoxAlgorithmDesc
{
public:
    CString getName() const override { return "Mouse Control"; }
    CString getAuthorName() const override { return "Guillaume Gibert"; }
    CString getAuthorCompanyName() const override { return "INSERM"; }
    CString getShortDescription() const override { return "Mouse Control for Feedback"; }

    CString getDetailedDescription() const override
    {
        return "Experimental box to move the mouse in x direction with respect to the input value. Only implemented on Linux.";
    }

    CString getCategory() const override { return "Tools"; }
    CString getVersion() const override { return "0.1"; }
    void release() override { }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_MouseControl; }
    IPluginObject* create() override { return new CMouseControl(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Amplitude", OV_TypeId_StreamedMatrix);
        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_MouseControlDesc)
};
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
