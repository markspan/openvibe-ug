#pragma once

#include "../ovp_defines.h"
#include "../ovp_gdf_helpers.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <fstream>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

/**
* The plugin's main class
*
*/
class CGDFFileWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CGDFFileWriter() { }

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_GDFFileWriter)

protected:
    void setChannelCount(size_t n);
    void setChannelName(size_t index, const char* name);
    void setSampleCountPerBuffer(size_t n);
    void setSamplingRate(size_t sampling);
    void setSampleBuffer(const double* buffer);

    void setExperimentInfo();

    void setStimulation(const uint64_t identifier, const uint64_t date);

    void saveMatrixData();
    void saveEvents();
    void padByEvents();

    std::ofstream m_file;
    CString m_filename;

    Toolkit::TSignalDecoder<CGDFFileWriter>* m_signalDecoder           = nullptr;
    Toolkit::TExperimentInfoDecoder<CGDFFileWriter>* m_xpInfoDecoder   = nullptr;
    Toolkit::TStimulationDecoder<CGDFFileWriter>* m_stimulationDecoder = nullptr;

    //GDF structures
    GDF::CFixedGDF1Header m_fixedHeader;
    GDF::CVariableGDF1Header m_variableHeader;

    std::vector<std::vector<double>> m_samples;
    std::vector<int64_t> m_nSamples;


    size_t m_samplesPerChannel = 0;
    size_t m_sampling          = 0;

    std::vector<std::pair<uint64_t, uint64_t>> m_events;

    bool m_error       = false;

    std::vector<double> m_channelScale;
    std::vector<double> m_channelOffset;
};

/**
* Plugin's description
*/
class CGDFFileWriterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "GDF file writer"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "This algorithm records on disk what comes from a specific output"; }
    CString getDetailedDescription() const override { return "This algorithm dumps on disk a stream from a specific output in the standard GDF file format"; }
    CString getCategory() const override { return "File reading and writing/GDF"; }
    CString getVersion() const override { return "0.6"; }
    CString getStockItemName() const override { return "gtk-save"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_GDFFileWriter; }
    IPluginObject* create() override { return new CGDFFileWriter(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box inputs //swap order of the first two
        prototype.addInput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addInput("Signal", OV_TypeId_Signal);
        prototype.addInput("Stimulation", OV_TypeId_Stimulations);

        // Adds box settings
        prototype.addSetting("Filename", OV_TypeId_Filename, "record-[$core{date}-$core{time}].gdf");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_GDFFileWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
