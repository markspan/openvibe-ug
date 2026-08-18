#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ebml/CWriter.h>
#include <ebml/CWriterHelper.h>

#include <cstdio>

#include <fstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmGenericStreamWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>, public EBML::IWriterCallback
{
public:
    CBoxAlgorithmGenericStreamWriter();
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    bool generateFileHeader();

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_GenericStreamWriter)

protected:
    bool m_isHeaderGenerate = false;
    CString m_filename;
    EBML::CWriter m_writer;
    EBML::CWriterHelper m_writerHelper;

private:
    void write(const void* buffer, const size_t size) override;

    CMemoryBuffer m_swap;
    std::ofstream m_file;
};

class CBoxAlgorithmGenericStreamWriterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    //it seems the only purpose of the check was to give a name when adding an input
    //without it, the input configuration dialog display random characters in the name field
    //the check is unnecessary when removing/changing inputs and on already named inputs
    bool check(Kernel::IBox& box)
    {
        const size_t i = box.getInputCount() - 1;
        //only check last input (we assume previous inputs have benn named, how could they not?)
        box.setInputName(i, ("Input stream " + std::to_string(i + 1)).c_str());
        //for (i=0; i<box.getInputCount(); ++i) { box.setInputName(i, ("Input stream " + std::to_string(i + 1)).c_str()); }
        return true;
    }

    bool onDefaultInitialized(Kernel::IBox& box) override
    {
        box.setInputName(0, "Input Signal");
        box.setInputType(0, OV_TypeId_Signal);
        box.addInput("Input Stimulations", OV_TypeId_Stimulations);
        return true;
    }

    bool onInputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setInputType(index, OV_TypeId_EBMLStream);
        this->check(box);
        return true;
    }

    bool onInputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        //this->check(box);
        return true;
    }

    bool onInputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        //this->check(box);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmGenericStreamWriterDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Generic stream writer"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Writes any number of streams into an .ov file"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/OpenViBE"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_GenericStreamWriter; }
    IPluginObject* create() override { return new CBoxAlgorithmGenericStreamWriter; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmGenericStreamWriterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream 1", OV_TypeId_EBMLStream);
        prototype.addSetting("Filename", OV_TypeId_Filename, "record-[$core{date}-$core{time}].ov");
        prototype.addSetting("Use compression", OV_TypeId_Boolean, "false");
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_GenericStreamWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
