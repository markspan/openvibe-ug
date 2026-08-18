#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <cstdio>
#include <iostream>
#include <fstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmCSVFileWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmCSVFileWriter() { }
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    bool processStreamedMatrix();
    bool processStimulation();

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_CSVFileWriter)


protected:
    bool initializeFile();

    std::ofstream m_fileStream;

    CString m_separator;
    CIdentifier m_typeID                                = CIdentifier::undefined();
    bool m_firstBuffer                                  = false;
    bool (CBoxAlgorithmCSVFileWriter::*m_realProcess)() = nullptr;

    Toolkit::TDecoder<CBoxAlgorithmCSVFileWriter>* m_decoder = nullptr;
    CMatrix m_oMatrix;      // This represents the properties of the input, no data

    uint64_t m_nSample = 0;

    bool m_headerReceived = false;
};

class CBoxAlgorithmCSVFileWriterListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmCSVFileWriterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "CSV File Writer (Deprecated)"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Writes signal in a CSV (text based) file"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/CSV"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_CSVFileWriter; }
    IPluginObject* create() override { return new CBoxAlgorithmCSVFileWriter; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmCSVFileWriterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream", OV_TypeId_Signal);
        prototype.addSetting("Filename", OV_TypeId_Filename, "record-[$core{date}-$core{time}].csv");
        prototype.addSetting("Column separator", OV_TypeId_String, ";");
        prototype.addSetting("Precision", OV_TypeId_Integer, "10");
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Spectrum);
        prototype.addInputSupport(OV_TypeId_Stimulations);
        prototype.addInputSupport(OV_TypeId_FeatureVector);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_CSVFileWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
