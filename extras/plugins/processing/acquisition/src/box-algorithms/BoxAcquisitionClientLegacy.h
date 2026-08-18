#pragma once

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <socket/IConnectionClient.h>

namespace OpenViBE {
namespace Plugins {
namespace Acquisition {
class CBoxAlgorithmAcquisitionClientLegacy final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    uint64_t getClockFrequency() override;
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;

	_IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_AcquisitionClientLegacy)

protected:
    Kernel::IAlgorithmProxy* m_decoder = nullptr;

    Kernel::TParameterHandler<CMemoryBuffer*> ip_acquisitionBuffer;
    Kernel::TParameterHandler<uint64_t> op_bufferDuration;
    Kernel::TParameterHandler<CMemoryBuffer*> op_experimentInfoBuffer;
    Kernel::TParameterHandler<CMemoryBuffer*> op_signalBuffer;
    Kernel::TParameterHandler<CMemoryBuffer*> op_stimulationBuffer;
    Kernel::TParameterHandler<CMemoryBuffer*> op_channelLocalisationBuffer;
    Kernel::TParameterHandler<CMemoryBuffer*> op_channelUnitsBuffer;

    Socket::IConnectionClient* m_connectionClient = nullptr;

    uint64_t m_lastStartTime = 0;
    uint64_t m_lastEndTime   = 0;
};

class CBoxAlgorithmAcquisitionClientLegacyDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

	CString getName() const override { return "Acquisition client (Legacy)"; }
	CString getAuthorName() const override { return "Yann Renard"; }
	CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
	CString getShortDescription() const override { return "Legacy network based acquisition client, up to OpenViBE 3.7.0"; }
	CString getDetailedDescription() const override { return "This algorithm waits for EEG data from the network and distributes it into the scenario"; }
	CString getCategory() const override { return "Acquisition and network IO"; }
	CString getVersion() const override { return "1.0"; }

	CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_AcquisitionClientLegacy; }
	IPluginObject* create() override { return new CBoxAlgorithmAcquisitionClientLegacy; }
	CString getStockItemName() const override { return "gtk-connect"; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Experiment information", OV_TypeId_ExperimentInfo);
        prototype.addOutput("Signal stream", OV_TypeId_Signal);
        prototype.addOutput("Stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Channel localisation", OV_TypeId_ChannelLocalisation);
        prototype.addOutput("Channel units", OV_TypeId_ChannelUnits);
        prototype.addSetting("Acquisition server hostname", OV_TypeId_String, "${AcquisitionServer_HostName}");
        prototype.addSetting("Acquisition server port", OV_TypeId_Integer, "1024");
        return true;
    }

	_IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_AcquisitionClientLegacyDesc)
};
}  // namespace Acquisition
}  // namespace Plugins
}  // namespace OpenViBE
