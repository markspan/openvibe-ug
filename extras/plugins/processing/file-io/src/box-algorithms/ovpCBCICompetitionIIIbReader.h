#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <fstream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBCICompetitionIIIbReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBCICompetitionIIIbReader() { } //TODO

    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool process() override;
    bool processClock(Kernel::CMessageClock& msg) override;

    uint64_t getClockFrequency() override { return m_clockFrequency; }

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_BCICompetitionIIIbReader)

protected:
    void writeSignalInformation();
    void readTriggers();
    void readLabels();
    void readArtifacts();
    void readTrueLabels();

    bool m_errorOccurred = false;   //true if an error has occurred while reading the GDF file

    //The filename and handle
    std::ifstream m_file;
    size_t m_fileSize = 0;

    Toolkit::TSignalEncoder<CBCICompetitionIIIbReader> m_signalEncoder;
    Toolkit::TStimulationEncoder<CBCICompetitionIIIbReader> m_stimEncoder;

    uint64_t m_clockFrequency = 100LL << 32;

    std::vector<uint64_t> m_triggerTimes;
    std::vector<uint64_t> m_endOfTrials;
    std::vector<uint64_t> m_cueDisplayStarts;
    std::vector<uint64_t> m_feedbackStarts;
    std::vector<size_t> m_classLabels;
    std::vector<bool> m_artifacts;
    std::vector<size_t> m_trueLabels;

    size_t m_samplesPerBuffer = 0;
    size_t m_sampling         = 125;
    size_t m_nSentSample      = 0;

    CMatrix* m_buffer = nullptr;
    bool m_endOfFile  = false;

    size_t m_currentTrial = 0;

    bool m_keepTrainingSamples = false;
    bool m_keepTestSamples     = false;
    bool m_keepArtifactSamples = false;

    double m_trialLength     = 0;
    double m_cueDisplayStart = 0;
    double m_feedbackStart   = 0;
};


class CBCICompetitionIIIbReaderDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "BCI competition IIIb reader"; }
    CString getAuthorName() const override { return "Bruno Renier"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Reads ASCII version of BCI competition IIIb datasets."; }

    CString getDetailedDescription() const override
    {
        return "Reads signal samples, stimulations and class labels from the BCI competition IIIb ASCII datasets.";
    }

    CString getCategory() const override { return "File reading and writing/BCI Competition"; }
    CString getVersion() const override { return "0.7"; }
    CString getStockItemName() const override { return "gtk-open"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BCICompetitionIIIbReader; }
    IPluginObject* create() override { return new CBCICompetitionIIIbReader(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box outputs
        prototype.addOutput("Signal", OV_TypeId_Signal);
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);

        // Adds settings
        prototype.addSetting("Signal file", OV_TypeId_Filename, "");
        prototype.addSetting("Triggers file", OV_TypeId_Filename, "");
        prototype.addSetting("Labels file", OV_TypeId_Filename, "");
        prototype.addSetting("Artifact file", OV_TypeId_Filename, "");
        prototype.addSetting("True labels file", OV_TypeId_Filename, "");

        prototype.addSetting("Samples per buffer", OV_TypeId_Integer, "32");

        prototype.addSetting("Offline", OV_TypeId_Boolean, "false");
        prototype.addSetting("Train?", OV_TypeId_Boolean, "true");
        prototype.addSetting("Test?", OV_TypeId_Boolean, "false");
        prototype.addSetting("Keep artifacts?", OV_TypeId_Boolean, "false");

        prototype.addSetting("Trial length", OV_TypeId_Float, "8.0");
        prototype.addSetting("CUE display Start", OV_TypeId_Float, "3.0");
        prototype.addSetting("Feedback start", OV_TypeId_Float, "4.0");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BCICompetitionIIIbReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
