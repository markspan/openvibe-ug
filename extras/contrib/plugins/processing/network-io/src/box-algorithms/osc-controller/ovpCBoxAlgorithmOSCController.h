#pragma once

#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "oscpkt.h"
#include "oscpkt_udp.h"

namespace OpenViBE {
namespace Plugins {
namespace NetworkIO {
/**
 * \class CBoxAlgorithmOSCController
 * \author Ozan Caglayan (Galatasaray University)
 * \date Thu May  8 20:57:24 2014
 * \brief The class CBoxAlgorithmOSCController describes the box OSC Controller.
 *
 */
class CBoxAlgorithmOSCController final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    //Here is the different process callbacks possible
    // - On new input received (the most common behaviour for signal processing) :
    bool processInput(const size_t index) override;

    bool process() override;

    // As we do with any class in openvibe, we use the macro below 
    // to associate this box to an unique identifier. 
    // The inheritance information is also made available, 
    // as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_OSCController)

private:
    // Decodes the stream
    Kernel::IAlgorithmProxy* m_decoder = nullptr;

    // UDP Socket (oscpkt_udp.h)
    oscpkt::UdpSocket m_udpSocket;

    // OSC Address to some device
    CString m_oscAddress;
};

/**
 * \class CBoxAlgorithmOSCControllerDesc
 * \author Ozan Caglayan (Galatasaray University)
 * \date Thu May  8 20:57:24 2014
 * \brief Descriptor of the box OSC Controller.
 *
 */
class CBoxAlgorithmOSCControllerDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "OSC Controller"; }
    CString getAuthorName() const override { return "Ozan Caglayan"; }
    CString getAuthorCompanyName() const override { return "Galatasaray University"; }
    // + Stimulation support & some code refactoring in v1.1 by Jussi T. Lindgren / Inria
    CString getShortDescription() const override { return "Sends OSC messages to an OSC controller"; }

    CString getDetailedDescription() const override
    {
        return
                "This box allows OpenViBE to send OSC (Open Sound Control) messages to an OSC server. See http://www.opensoundcontrol.org to learn about the OSC protocol and its use cases.";
    }

    CString getCategory() const override { return "Acquisition and network IO"; }
    CString getVersion() const override { return "1.1"; }
    CString getStockItemName() const override { return "gtk-network"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_OSCController; }
    IPluginObject* create() override { return new CBoxAlgorithmOSCController; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input",OV_TypeId_Signal);

        prototype.addInputSupport(OV_TypeId_Signal);
        prototype.addInputSupport(OV_TypeId_StreamedMatrix);
        prototype.addInputSupport(OV_TypeId_Stimulations);

        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);

        prototype.addSetting("OSC Server IP",OV_TypeId_String, "127.0.0.1");
        prototype.addSetting("OSC Server Port",OV_TypeId_Integer, "9001");
        prototype.addSetting("OSC Address",OV_TypeId_String, "/a/b/c");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_OSCControllerDesc)
};
}  // namespace NetworkIO
}  // namespace Plugins
}  // namespace OpenViBE
