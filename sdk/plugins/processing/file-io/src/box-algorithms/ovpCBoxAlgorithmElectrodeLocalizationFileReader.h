#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
class CBoxAlgorithmElectrodeLocalisationFileReader final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReader)

protected:
    Kernel::IAlgorithmProxy* m_pOVMatrixFileReader                                                = nullptr;
    Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmElectrodeLocalisationFileReader>* m_encoder = nullptr;

    CString m_filename;
    bool m_headerSent = false;
    bool m_bufferSent = false;
};

class CBoxAlgorithmElectrodeLocalisationFileReaderDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Electrode localisation file reader"; }
    CString getAuthorName() const override { return "Vincent Delannoy"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Loads files containing the normalized coordinates of an electrode set"; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "File reading and writing/OpenViBE"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReader; }
    IPluginObject* create() override { return new CBoxAlgorithmElectrodeLocalisationFileReader; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box outputs
        prototype.addOutput("Channel localisation", OV_TypeId_ChannelLocalisation);

        // Adds settings
        prototype.addSetting("Filename", OV_TypeId_Filename, "");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ElectrodeLocalisationFileReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
