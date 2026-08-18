#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <ebml/CReader.h>
#include <ebml/CReaderHelper.h>

#include <stack>
#include <map>

#include <cstdio>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmGenericStreamReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>, public EBML::IReaderCallback
{
public:
    CBoxAlgorithmGenericStreamReader();
    void release() override { delete this; }
    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_GenericStreamReader)


protected:
    CString m_filename;

    EBML::CReader m_reader;
    EBML::CReaderHelper m_readerHelper;

    CMemoryBuffer m_swap;
    CMemoryBuffer m_pendingChunk;
    uint64_t m_startTime = 0;
    uint64_t m_endTime   = 0;
    size_t m_outputIdx   = 0;
    bool m_pending       = false;
    bool m_hasEBMLHeader = false;

    FILE* m_file = nullptr;
    std::stack<EBML::CIdentifier> m_nodes;
    std::map<size_t, size_t> m_streamIdxToOutputIdxs;
    std::map<size_t, CIdentifier> m_streamIdxToTypeIDs;

private:
    bool initializeFile();
    bool isMasterChild(const EBML::CIdentifier& identifier) override;
    void openChild(const EBML::CIdentifier& identifier) override;
    void processChildData(const void* buffer, const size_t size) override;
    void closeChild() override;
};

class CBoxAlgorithmGenericStreamReaderListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getOutputCount(); ++i) { box.setOutputName(i, ("Output stream " + std::to_string(i + 1)).c_str()); }
        return true;
    }

    bool onDefaultInitialized(Kernel::IBox& box) override
    {
        box.setOutputName(0, "Output Signal");
        box.setOutputType(0, OV_TypeId_Signal);
        box.addOutput("Output Stimulations", OV_TypeId_Stimulations);
        return true;
    }

    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputType(index, OV_TypeId_EBMLStream);
        this->check(box);
        return true;
    }

    bool onOutputRemoved(Kernel::IBox& box, const size_t /*index*/) override
    {
        this->check(box);
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& box, const size_t /*index*/) override
    {
        this->check(box);
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmGenericStreamReaderDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Generic stream reader"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Reads OpenViBE streams saved in the .ov format"; }
    CString getDetailedDescription() const override { return "Generic Stream Writer box can be used to store data in the format read by this box"; }
    CString getCategory() const override { return "File reading and writing/OpenViBE"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_GenericStreamReader; }
    IPluginObject* create() override { return new CBoxAlgorithmGenericStreamReader; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmGenericStreamReaderListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Output stream 1", OV_TypeId_EBMLStream);
        prototype.addSetting("Filename", OV_TypeId_Filename, "");
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_GenericStreamReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
