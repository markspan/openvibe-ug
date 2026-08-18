#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <iostream>
#include <cstdio>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmCSVFileReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmCSVFileReader() {}
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 128LL << 32; } // the box clock frequency
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    bool processStreamedMatrix();
    bool processStimulation();
    bool processSignal();
    bool processChannelLocalisation();
    bool processFeatureVector();
    bool processSpectrum();
    bool convertVectorDataToMatrix(CMatrix* matrix);

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_CSVFileReader)


protected:
    bool initializeFile();

    FILE* m_file = nullptr;
    std::string m_separator;
    bool m_doNotUseFileTime = false;
    CString m_filename;

    CIdentifier m_typeID       = CIdentifier::undefined();
    size_t m_nCol              = 0;
    size_t m_sampling          = 0;
    size_t m_samplesPerBuffer  = 0;
    size_t m_channelsPerBuffer = 0;

    bool (CBoxAlgorithmCSVFileReader::*m_realProcess)() = nullptr;

    Toolkit::TEncoder<CBoxAlgorithmCSVFileReader>* m_encoder = nullptr;

    bool m_headerSent = false;
    std::vector<std::string> m_lastLineSplits;
    std::vector<std::string> m_headerFiles;
    std::vector<std::vector<std::string>> m_dataMatrices;

    double m_nextTime = 0;

    uint64_t m_startTime = 0;
    uint64_t m_endTime   = 0;

    static const size_t BUFFER_LEN = 16384; // Side-effect: a maximum allowed length for a line of a CSV file
};

class CBoxAlgorithmCSVFileReaderListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onOutputTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        CIdentifier typeID = CIdentifier::undefined();
        box.getOutputType(index, typeID);
        if (typeID == OV_TypeId_Spectrum) {
            box.setSettingName(3, "Unused parameter");
            box.setSettingValue(3, "0");
        }
        else if (typeID == OV_TypeId_ChannelLocalisation) {
            box.setSettingName(3, "Channels number");
            box.setSettingValue(3, "32");
        }
        else if (typeID == OV_TypeId_FeatureVector) {
            box.setSettingName(3, "Unused parameter");
            box.setSettingValue(3, "0");
        }
        else if (typeID == OV_TypeId_StreamedMatrix) {
            box.setSettingName(3, "Samples per buffer");
            box.setSettingValue(3, "32");
        }
        else if (typeID == OV_TypeId_Stimulations) {
            box.setSettingName(3, "Unused parameter");
            box.setSettingValue(3, "0");
        }
        else {
            box.setOutputType(index, OV_TypeId_Signal);
            box.setSettingName(3, "Samples per buffer");
            box.setSettingValue(3, "32");

            OV_ERROR_KRF("Unsupported stream type " << typeID.str(), Kernel::ErrorType::BadOutput);
        }
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmCSVFileReaderDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "CSV File Reader (Deprecated)"; }
    CString getAuthorName() const override { return "Baptiste Payan"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Read signal in a CSV (text based) file"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/CSV"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_CSVFileReader; }
    IPluginObject* create() override { return new CBoxAlgorithmCSVFileReader; }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmCSVFileReaderListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Output stream", OV_TypeId_Signal);
        prototype.addSetting("Filename", OV_TypeId_Filename, "");
        prototype.addSetting("Column separator", OV_TypeId_String, ";");
        prototype.addSetting("Don't use the file time",OV_TypeId_Boolean, "false");
        prototype.addSetting("Samples per buffer", OV_TypeId_Integer, "32");

        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);

        prototype.addOutputSupport(OV_TypeId_StreamedMatrix);
        prototype.addOutputSupport(OV_TypeId_FeatureVector);
        prototype.addOutputSupport(OV_TypeId_ChannelLocalisation);
        prototype.addOutputSupport(OV_TypeId_Signal);
        prototype.addOutputSupport(OV_TypeId_Spectrum);
        prototype.addOutputSupport(OV_TypeId_Stimulations);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_CSVFileReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
