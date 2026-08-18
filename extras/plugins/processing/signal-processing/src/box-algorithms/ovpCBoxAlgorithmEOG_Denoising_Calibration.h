#pragma once

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <Eigen/Dense>
#include <iostream>
#include <fstream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmEOG_Denoising_Calibration
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Fri May 23 15:30:58 2014
 * \brief The class CBoxAlgorithmEOG_Denoising_Calibration describes the box EOG_Denoising_Calibration.
 *
 */
class CBoxAlgorithmEOG_Denoising_Calibration final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;

    // If you want to use processClock, you must provide the clock frequency.
    uint64_t getClockFrequency() override { return 1LL << 32; } // the box clock frequency

    bool process() override;

    //virtual bool openfile();

    // As we do with any class in openvibe, we use the macro below 
    // to associate this box to an unique identifier. 
    // The inheritance information is also made available, 
    // as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_EOG_Denoising_Calibration)

protected:
    Toolkit::TSignalDecoder<CBoxAlgorithmEOG_Denoising_Calibration> m_algo0SignalDecoder;
    Toolkit::TSignalDecoder<CBoxAlgorithmEOG_Denoising_Calibration> m_algo1SignalDecoder;

    Toolkit::TStimulationDecoder<CBoxAlgorithmEOG_Denoising_Calibration> m_algo2StimulationDecoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmEOG_Denoising_Calibration> m_stimulationEncoder;

    CString m_calibrationFilename;

    Eigen::Index m_chunksVerify = 0;
    Eigen::Index m_nChunks      = 0;
    bool m_endProcess     = false;
    std::fstream m_eegFile;
    std::fstream m_eogFile;
    std::ofstream m_matrixFile;

    double m_startTime = 0;
    double m_endTime   = 0;

    Eigen::Index m_startTimeChunks = 0;
    Eigen::Index m_endTimeChunks   = 0;

    uint64_t m_trainDate           = 0;
    uint64_t m_trainChunkStartTime = 0;
    uint64_t m_trainChunkEndTime   = 0;

    double m_time = 0;

    Eigen::Index m_nChannels0 = 0;
    Eigen::Index m_nChannels1 = 0;

    Eigen::Index m_nSamples0 = 0;
    Eigen::Index m_nSamples1 = 0;

    uint64_t m_stimID = 0;

    CString m_eegTempFilename;
    CString m_eogTempFilename;
};

/**
 * \class CBoxAlgorithmEOG_Denoising_CalibrationDesc
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Fri May 23 15:30:58 2014
 * \brief Descriptor of the box EOG_Denoising_Calibration.
 *
 */
class CBoxAlgorithmEOG_Denoising_CalibrationDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "EOG Denoising Calibration"; }
    CString getAuthorName() const override { return "Joao-Pedro Berti-Ligabo"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Algorithm implementation as suggested in Schlogl's article of 2007."; }

    CString getDetailedDescription() const override
    {
        return "Press 'a' to set start point and 'u' to set end point, you can connect the Keyboard Stimulator for that";
    }

    CString getCategory() const override { return "Signal processing/Denoising"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gnome-fs-regular.png"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_EOG_Denoising_Calibration; }
    IPluginObject* create() override { return new CBoxAlgorithmEOG_Denoising_Calibration; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("EEG",OV_TypeId_Signal);
        prototype.addInput("EOG",OV_TypeId_Signal);
        prototype.addInput("Stimulations", OV_TypeId_Stimulations);

        prototype.addSetting("Filename b Matrix", OV_TypeId_Filename, "b-Matrix-EEG.cfg");
        prototype.addSetting("End trigger", OV_TypeId_Stimulation, "OVTK_GDF_End_Of_Session");

        prototype.addOutput("Train-completed Flag",OV_TypeId_Stimulations);

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EOG_Denoising_CalibrationDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
