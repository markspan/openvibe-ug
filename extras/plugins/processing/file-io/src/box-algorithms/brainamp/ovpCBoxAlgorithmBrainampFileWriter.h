#pragma once

#include "../../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <fstream>
#include <map>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmBrainampFileWriter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_BrainampFileWriter)

protected:
    Kernel::IAlgorithmProxy* m_signalDecoder      = nullptr;
    Kernel::IAlgorithmProxy* m_stimulationDecoder = nullptr;

    Kernel::TParameterHandler<const CMemoryBuffer*> ip_signalBuffer;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_stimulationsBuffer;
    Kernel::TParameterHandler<CMatrix*> op_matrix;
    Kernel::TParameterHandler<CStimulationSet*> op_stimSet;
    Kernel::TParameterHandler<uint64_t> op_sampling;

    CString m_filePath;
    CString m_dictionaryFilename;
    bool m_transformStimulations    = false;
    bool m_shouldWriteFullFilenames = false;

private:
    std::ofstream m_headerFStream;
    std::ofstream m_eegFStream;
    std::ofstream m_markerFStream;

    std::map<uint64_t, std::string> m_stimulationToMarkers;

    uint64_t m_markersWritten     = 0;
    bool m_wasMarkerHeaderWritten = false;
};

class CBoxAlgorithmBrainampFileWriterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "BrainVision Format File Writer"; }
    CString getAuthorName() const override { return "Jozef Legeny"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies"; }
    CString getShortDescription() const override { return "Writes its input into a BrainVision format file"; }
    CString getDetailedDescription() const override { return "This box allows to write the input signal under BrainVision file format."; }
    CString getCategory() const override { return "File reading and writing/BrainVision Format"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-save"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_BrainampFileWriter; }
    IPluginObject* create() override { return new CBoxAlgorithmBrainampFileWriter; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box outputs
        prototype.addInput("EEG stream", OV_TypeId_Signal);
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);

        // Adds settings
        prototype.addSetting("Filename (header in vhdr format)", OV_TypeId_Filename, "record-[$core{date}-$core{time}].vhdr");
        prototype.addSetting("Marker to OV Stimulation dictionary", OV_TypeId_Filename, "");
        prototype.addSetting("Convert OpenViBE Stimulations to markers", OV_TypeId_Boolean, "true");
        prototype.addSetting("Use full data and marker file names in header", OV_TypeId_Boolean, "false");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_BrainampFileWriterDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
