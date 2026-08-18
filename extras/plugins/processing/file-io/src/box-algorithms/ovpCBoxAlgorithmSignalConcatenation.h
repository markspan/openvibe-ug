#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <list>
// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OVP_ClassId_BoxAlgorithm_SignalConcatenation OpenViBE::CIdentifier(0x372F3A9D, 0x49E20CD2)
#define OVP_ClassId_BoxAlgorithm_SignalConcatenationDesc OpenViBE::CIdentifier(0x372F3A9D, 0x49E20CD2)

namespace OpenViBE {
namespace Plugins {
namespace FileIO {
/**
 * \class CBoxAlgorithmSignalConcatenation
 * \author Laurent Bonnet (INRIA)
 * \date Tue Jun 28 09:52:48 2011
 * \brief The class CBoxAlgorithmSignalConcatenation describes the box Signal Concatenation.
 *
 */
class CBoxAlgorithmSignalConcatenation final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    uint64_t getClockFrequency() override { return 8LL << 32; }

    bool processInput(const size_t index) override;


    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_SignalConcatenation)

protected:
    bool concate();
    bool m_finished      = false;
    bool m_resynchroDone = false;
    uint64_t m_timeOut   = 0;

    bool m_headerSent              = false;
    uint32_t m_headerReceivedCount = 0;
    uint32_t m_endReceivedCount    = 0;
    bool m_stimHeaderSent          = false;
    bool m_endSent                 = false;
    bool m_statsPrinted            = false;

    std::vector<uint64_t> m_eofStimulations;
    std::vector<bool> m_eofReached;

    struct SChunk
    {
        CMemoryBuffer* m_Buffer;
        uint64_t m_StartTime;
        uint64_t m_EndTime;
    };

    struct SStimulationChunk
    {
        CStimulationSet* m_StimulationSet;
        uint64_t m_StartTime;
        uint64_t m_EndTime;
    };

    uint64_t m_stimChunkLength = 0;

    // File end times
    std::vector<uint64_t> m_fileEndTimes;

    // The signal buffers, one per file
    std::vector<std::vector<SChunk>> m_signalChunkBuffers;

    std::vector<std::vector<SStimulationChunk>> m_stimulationChunkBuffers;

    // The stimulations are stored in one stimulation set per file. The chunk are reconstructed.
    std::vector<CStimulationSet*> m_stimulationSets;

    //The decoders, (1 signal/1 stim) per file
    std::vector<Toolkit::TStimulationDecoder<CBoxAlgorithmSignalConcatenation>*> m_stimulationDecoders;
    std::vector<Toolkit::TSignalDecoder<CBoxAlgorithmSignalConcatenation>*> m_signalDecoders;

    // the encoders : signal, stim and trigger encoder.
    Toolkit::TSignalEncoder<CBoxAlgorithmSignalConcatenation> m_signalEncoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmSignalConcatenation> m_stimulationEncoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmSignalConcatenation> m_triggerEncoder;

    uint64_t m_triggerDate        = 0;
    uint64_t m_lastChunkStartTime = 0;
    uint64_t m_lastChunkEndTime   = 0;

    struct SConcatenationState
    {
        SConcatenationState() : m_CurrentFileIdx(0), m_CurrentChunkIdx(0), m_CurrentStimulationIdx(0) { }
        uint32_t m_CurrentFileIdx;
        uint32_t m_CurrentChunkIdx;
        uint32_t m_CurrentStimulationIdx;
    };

    SConcatenationState m_state;
};


// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
// Please uncomment below the callbacks you want to use.
class CBoxAlgorithmSignalConcatenationListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool check(Kernel::IBox& box) const
    {
        for (uint32_t i = 0; i < box.getInputCount() >> 1; ++i) {
            box.setInputName(i * 2, ("Input signal " + std::to_string(i + 1)).c_str());
            box.setInputType(i * 2, OV_TypeId_Signal);

            box.setInputName(i * 2 + 1, ("Input stimulations " + std::to_string(i + 1)).c_str());
            box.setInputType(i * 2 + 1, OV_TypeId_Stimulations);

            box.setSettingName(i + 1, ("End-of-file stimulation for input " + std::to_string(i + 1)).c_str());
        }

        return true;
    }

    bool onInputRemoved(Kernel::IBox& box, const size_t index) override
    {
        if (index & 1) { box.removeInput(index - 1); }  // odd index
        else { box.removeInput(index); }                // even index
        box.removeSetting(index >> 1);
        return this->check(box);
    }

    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override
    {
        box.addInput("", OV_TypeId_Stimulations);
        box.addSetting("",OV_TypeId_Stimulation, "OVTK_StimulationId_ExperimentStop");
        return this->check(box);
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};


/**
 * \class CBoxAlgorithmSignalConcatenationDesc
 * \author Laurent Bonnet (INRIA)
 * \date Tue Jun 28 09:52:48 2011
 * \brief Descriptor of the box Signal Concatenation.
 *
 */
class CBoxAlgorithmSignalConcatenationDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal Concatenation"; }
    CString getAuthorName() const override { return "Laurent Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Concatenates multiple signal streams"; }

    CString getDetailedDescription() const override
    {
        return "The signal stream concatenation box reads multiple streams in parallel, and produces a single stream that is the concatenation of all inputs.";
    }

    CString getCategory() const override { return "File reading and writing"; }
    CString getVersion() const override { return "2.0"; }
    CString getStockItemName() const override { return "gtk-add"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_SignalConcatenation; }
    IPluginObject* create() override { return new CBoxAlgorithmSignalConcatenation; }


    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmSignalConcatenationListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal 1",OV_TypeId_Signal);
        prototype.addInput("Input stimulations 1",OV_TypeId_Stimulations);
        prototype.addInput("Input signal 2",OV_TypeId_Signal);
        prototype.addInput("Input stimulations 2",OV_TypeId_Stimulations);

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        prototype.addOutput("Signal",OV_TypeId_Signal);
        prototype.addOutput("Stimulations",OV_TypeId_Stimulations);

        prototype.addOutput("Status",OV_TypeId_Stimulations);

        prototype.addSetting("Time out before assuming end-of-file (in sec)",OV_TypeId_Integer, "5");
        prototype.addSetting("End-of-file stimulation for input 1",OV_TypeId_Stimulation, "OVTK_StimulationId_ExperimentStop");
        prototype.addSetting("End-of-file stimulation for input 2",OV_TypeId_Stimulation, "OVTK_StimulationId_ExperimentStop");

        //prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_SignalConcatenationDesc)
};
}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
