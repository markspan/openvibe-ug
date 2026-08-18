#include "BoxPlugins.h"
#include "ovkCBoxProto.h"

namespace OpenViBE {
namespace Tracker {

#define OVP_ClassId_BoxAlgorithm_TemporalFilter             CIdentifier(0xB4F9D042, 0x9D79F2E5)
#define OVP_ClassId_BoxAlgorithm_TemporalFilterDesc         CIdentifier(0x7BF6BA62, 0xAF829A37)
#define OVP_ClassId_BoxAlgorithm_SignalResampling           CIdentifier(0x0E923A5E, 0xDA474058)
#define OVP_ClassId_BoxAlgorithm_SignalResamplingDesc       CIdentifier(0xA675A433, 0xC6690920)
#define OVP_ClassId_BoxAlgorithm_SimpleDSP                  CIdentifier(0x00E26FA1, 0x1DBAB1B2)
#define OVP_ClassId_BoxAlgorithm_SimpleDSPDesc              CIdentifier(0x00C44BFE, 0x76C9269E)
#define OVP_ClassId_BoxAlgorithm_Crop                       CIdentifier(0x7F1A3002, 0x358117BA)
#define OVP_ClassId_BoxAlgorithm_CropDesc                   CIdentifier(0x64D619D7, 0x26CC42C9)
#define OVP_ClassId_BoxAlgorithm_SpatialFilter              CIdentifier(0xDD332C6C, 0x195B4FD4)
#define OVP_ClassId_BoxAlgorithm_SpatialFilterDesc          CIdentifier(0x72A01C92, 0xF8C1FA24)
#define OVP_ClassId_BoxAlgorithm_TimeBasedEpoching          CIdentifier(0x00777FA0, 0x5DC3F560)
#define OVP_ClassId_BoxAlgorithm_TimeBasedEpochingDesc      CIdentifier(0x00ABDABE, 0x41381683)
#define OVP_ClassId_BoxAlgorithm_StimulationFilter          CIdentifier(0x02F96101, 0x5E647CB8)
#define OVP_ClassId_BoxAlgorithm_StimulationFilterDesc      CIdentifier(0x4D2A23FC, 0x28191E18)
#define OVP_ClassId_FastICA                                 CIdentifier(0x00649B6E, 0x6C88CD17)
#define OVP_ClassId_FastICADesc                             CIdentifier(0x00E9436C, 0x41C904CA)
#define OVP_ClassId_BoxAlgorithm_CommonAverageReference     CIdentifier(0x009C0CE3, 0x6BDF71C3)
#define OVP_ClassId_BoxAlgorithm_CommonAverageReferenceDesc CIdentifier(0x0033EAF8, 0x09C65E4E)
#define OVP_ClassId_BoxAlgorithm_ChannelSelector            CIdentifier(0x361722E8, 0x311574E8)
#define OVP_ClassId_BoxAlgorithm_ChannelSelectorDesc        CIdentifier(0x67633C1C, 0x0D610CD8)
#define OVP_ClassId_BoxAlgorithm_ChannelRename              CIdentifier(0x1FE50479, 0x39040F40)
#define OVP_ClassId_BoxAlgorithm_ChannelRenameDesc          CIdentifier(0x20EA1F00, 0x7AED5645)
#define OVP_ClassId_BoxAlgorithm_ReferenceChannel           CIdentifier(0x444721AD, 0x78342CF5)
#define OVP_ClassId_BoxAlgorithm_ReferenceChannelDesc       CIdentifier(0x42856103, 0x45B125AD)
#define OVP_ClassId_BoxAlgorithm_FrequencyBandSelector      CIdentifier(0x140C19C6, 0x4E6E187B)
#define OVP_ClassId_BoxAlgorithm_FrequencyBandSelectorDesc  CIdentifier(0x13462C56, 0x794E3C07)

#include <algorithm>

BoxPlugins::BoxPlugins(const Kernel::IKernelContext& ctx) : Contexted(ctx)
{
    // Register some boxes that can be used as filters
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_TemporalFilter, OVP_ClassId_BoxAlgorithm_TemporalFilterDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_SimpleDSP, OVP_ClassId_BoxAlgorithm_SimpleDSPDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_TimeBasedEpoching, OVP_ClassId_BoxAlgorithm_TimeBasedEpochingDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_Crop, OVP_ClassId_BoxAlgorithm_CropDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_SpatialFilter, OVP_ClassId_BoxAlgorithm_SpatialFilterDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_SignalResampling, OVP_ClassId_BoxAlgorithm_SignalResamplingDesc);
    create(OV_TypeId_Signal, OVP_ClassId_FastICA, OVP_ClassId_FastICADesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_CommonAverageReference, OVP_ClassId_BoxAlgorithm_CommonAverageReferenceDesc);

    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_ChannelSelector, OVP_ClassId_BoxAlgorithm_ChannelSelectorDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_ChannelRename, OVP_ClassId_BoxAlgorithm_ChannelRenameDesc);
    create(OV_TypeId_Signal, OVP_ClassId_BoxAlgorithm_ReferenceChannel, OVP_ClassId_BoxAlgorithm_ReferenceChannelDesc);

    create(OV_TypeId_Spectrum, OVP_ClassId_BoxAlgorithm_FrequencyBandSelector, OVP_ClassId_BoxAlgorithm_FrequencyBandSelectorDesc);

    create(OV_TypeId_Stimulations, OVP_ClassId_BoxAlgorithm_StimulationFilter, OVP_ClassId_BoxAlgorithm_StimulationFilterDesc);

    std::sort(m_boxPlugins.begin(), m_boxPlugins.end(), [](BoxAdapterStream* a, BoxAdapterStream* b)
    {
        return (a->getBox().getName()) < (b->getBox().getName());
    });
}

bool BoxPlugins::create(const CIdentifier& streamType, const CIdentifier& alg, const CIdentifier& desc)
{
    BoxAdapterStream* ptr = new BoxAdapterStream(m_kernelCtx, alg);

    // Initialize the context by polling the descriptor
    const Plugins::IPluginObjectDesc* pod = getKernelContext().getPluginManager().getPluginObjectDesc(desc);
    if (!pod) {
        log() << Kernel::LogLevel_Error << "Unable to load box algorithm " << alg.str() << "\n";
        return false;
    }

    const Plugins::IBoxAlgorithmDesc* pBoxAlgorithmDescriptor = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(pod);
    Kernel::CBoxProto boxProto(m_kernelCtx, ptr->getBox());
    pBoxAlgorithmDescriptor->getBoxPrototype(boxProto);

    // We need to force this so we don't launch the plugin on streams of other types, and not all boxes declare 
    // their capabilities. Here we assume that if the box is created for this stream type, it can support it.
    boxProto.addInputSupport(streamType);

    CIdentifier typeID;
    ptr->getBox().getInputType(0, typeID);
    if (typeID != streamType) { ptr->getBox().setInputType(0, streamType); }
    ptr->getBox().getOutputType(0, typeID);
    if (typeID != streamType) { ptr->getBox().setOutputType(0, streamType); }

    const CString boxName = m_kernelCtx.getTypeManager().getTypeName(streamType) + CString(" : ") + pBoxAlgorithmDescriptor->getName();
    ptr->getBox().setName(boxName);

    m_boxPlugins.push_back(ptr);

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
