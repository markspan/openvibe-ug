#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "edf/edflib.h"
#include <queue>
#include <deque>
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
typedef struct SChannel
{
    double min;
    double max;
} channel_info_t;


class CBoxAlgorithmEDFFileWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    //CBoxAlgorithmEDFFileWriter();
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_EDFFileWriter)

protected:
    Toolkit::TExperimentInfoDecoder<CBoxAlgorithmEDFFileWriter> m_experimentInfoDecoder;
    Toolkit::TSignalDecoder<CBoxAlgorithmEDFFileWriter> m_signalDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmEDFFileWriter> m_stimDecoder;

    CString m_filename;
    bool m_isFileOpened       = false;
    int m_fileHandle          = 0;
    size_t m_sampling         = 0;
    size_t m_nChannels        = 0;
    size_t m_nSamplesPerChunk = 0;
    std::queue<double, std::deque<double>> m_buffer;
    //int * m_pTemporyBuffer;
    //int * m_pTemporyBufferToWrite;

    std::vector<channel_info_t> m_channelInfo;
};

class CBoxAlgorithmEDFFileWriterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "EDF File Writer"; }
    CString getAuthorName() const override { return "Aurelien Van Langhenhove"; }
    CString getAuthorCompanyName() const override { return "CICIT Garches"; }
    CString getShortDescription() const override { return "Writes experiment information, signal and stimulations in a EDF file"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/EDF"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-save"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_EDFFileWriter; }
    IPluginObject* create() override { return new CBoxAlgorithmEDFFileWriter; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addInput("Signal", OV_TypeId_Signal);
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Filename", OV_TypeId_Filename, "record-[$core{date}-$core{time}].edf");

        //prototype.addFlag(Kernel::BoxFlag_IsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EDFFileWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
