#pragma once

#include "ovpCBCI2000ReaderHelper.h"
#include "../../ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#define OVP_ClassId_BoxAlgorithm_BCI2000Reader OpenViBE::CIdentifier(0xFF78DAF4, 0xC41544B8)
#define OVP_ClassId_BoxAlgorithm_BCI2000ReaderDesc OpenViBE::CIdentifier(0xFF53D107, 0xC31144B8)

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
/**
 * \class CBoxAlgorithmBCI2000Reader
 * \author Olivier Rochel (INRIA)
 * \date Tue Jun 21 11:11:04 2011
 * \brief The class CBoxAlgorithmBCI2000Reader describes the box BCI2000 Reader.
 *
 */
class CBoxAlgorithmBCI2000Reader final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    uint64_t getClockFrequency() override;

    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_BCI2000Reader)

protected:
    bool m_headerSent = false;

    Toolkit::TSignalEncoder<CBoxAlgorithmBCI2000Reader> m_signalEncoder;
    Toolkit::TSignalEncoder<CBoxAlgorithmBCI2000Reader> m_stateEncoder;

    // These 2 were from the time the matrices were built, not given by the encoders.
    // They could be removed, but for now make the code a bit easier to read - that's
    // why they're still there.
    CMatrix* m_oSignalMatrix = nullptr;
    CMatrix* m_oStateMatrix  = nullptr;

    size_t m_rate             = 0;
    size_t m_nChannel         = 0;
    size_t m_nSamplePerBuffer = 0;
    std::vector<double> m_buffer;       // temporary buffer as we'll have to transpose data for signal_out
    std::vector<uint32_t> m_states;     // state variables, to be converted too;
    uint64_t m_samplesSent                  = 0;
    BCI2000::CBCI2000ReaderHelper* m_helper = nullptr;
    // helpers
    void sendHeader();
};

/**
 * \class CBoxAlgorithmBCI2000ReaderDesc
 * \author Olivier Rochel (INRIA)
 * \date Tue Jun 21 11:11:04 2011
 * \brief Descriptor of the box BCI2000 Reader.
 *
 */
class CBoxAlgorithmBCI2000ReaderDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "BCI2000 File Reader"; }
    CString getAuthorName() const override { return "Olivier Rochel"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Reads BCI2000 .dat files."; }
    CString getDetailedDescription() const override { return "The box reads EEG/States signals from a BCI2000 file (.dat)"; }
    CString getCategory() const override { return "File reading and writing/BCI2000"; }
    CString getVersion() const override { return "1.3"; }
    CString getStockItemName() const override { return "gtk-open"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_BCI2000Reader; }
    IPluginObject* create() override { return new CBoxAlgorithmBCI2000Reader; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addOutput("Signal",OV_TypeId_Signal);
        prototype.addOutput("State",OV_TypeId_Signal);
        prototype.addSetting("File name",OV_TypeId_Filename, "");
        prototype.addSetting("Samples per buffer",OV_TypeId_Integer, "16");
        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable); // meuh non !

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_BCI2000ReaderDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
