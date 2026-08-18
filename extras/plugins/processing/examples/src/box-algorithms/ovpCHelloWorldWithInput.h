#pragma once

#include "../ovp_defines.h"
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Examples {
class CHelloWorldWithInput final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_HelloWorldWithInput)
};

class CHelloWorldWithInputListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    // The purposes of the following functions is to make the output correspond to the input

    bool onInputNameChanged(Kernel::IBox& box, const size_t index) override
    {
        CString inputName;
        box.getInputName(index, inputName);
        box.setOutputName(index, CString("Copy of '") + inputName + CString("'"));
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override
    {
        // Duplicate input as new output
        box.addOutput("Temporary name", OV_TypeId_EBMLStream);
        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        box.removeOutput(index);
        return true;
    }

    bool onInputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        // Keep input and output types identical
        CIdentifier typeID = CIdentifier::undefined();
        box.getInputType(index, typeID);
        box.setOutputType(index, typeID);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CHelloWorldWithInputDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "HelloWorldWithInput"; }
    CString getAuthorName() const override { return ""; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Duplicates input to output and prints a message to the log for each input block"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Examples/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-copy"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_HelloWorldWithInput; }
    IPluginObject* create() override { return new CHelloWorldWithInput(); }
    IBoxListener* createBoxListener() const override { return new CHelloWorldWithInputListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Message", OV_TypeId_String, "Hello!");        // setting 0

        prototype.addInput("Input 0", OV_TypeId_Signal);
        prototype.addOutput("Copy of 'Input 0'", OV_TypeId_Signal);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_HelloWorldWithInputDesc)
};
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
