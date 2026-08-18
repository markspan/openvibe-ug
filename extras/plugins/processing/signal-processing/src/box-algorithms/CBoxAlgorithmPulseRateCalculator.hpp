///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmPulseRateCalculator.hpp
/// \brief Classes of the Box Pulse Rate Calculator.
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Tue June 13 16:05:42 2023.
/// \Copyright (C) 2023 Inria
/// 
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
/// 
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include <queue>
//You may have to change this path to match your folder organisation
#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>


// The unique identifiers for the box and its descriptor.
// Identifier are randomly chosen by the skeleton-generator.
#define OV_AttributeId_Box_FlagIsUnstable                   OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/// <summary> The class CBoxAlgorithmPulseRateCalculator describes the box Pulse Rate Calculator. </summary>
class CBoxAlgorithmPulseRateCalculator final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    //Here is the different process callbacks possible
    // - On clock ticks :
    //bool processClock(Kernel::CMessageClock& msg) override;       
    // - On new input received (the most common behaviour for signal processing) :
    bool processInput(const size_t index) override;

    // If you want to use processClock, you must provide the clock frequency.
    //uint64_t getClockFrequency() override;

    bool process() override;

    // As we do with any class in openvibe, we use the macro below to associate this box to an unique identifier. 
    // The inheritance information is also made available, as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_PulseRateCalculator)

protected:
    // Input decoder:
    Toolkit::TSignalDecoder<CBoxAlgorithmPulseRateCalculator> m_inputDecoder;
    // Output decoder:
    Toolkit::TSignalEncoder<CBoxAlgorithmPulseRateCalculator> m_outputEncoder;

    Toolkit::TStimulationEncoder<CBoxAlgorithmPulseRateCalculator> m_outputStimEncoder;


private:
    uint64_t m_start = 0;
    uint64_t m_end = 0;

    double m_samplingRate = 0.0;

    // Stimulations
    const CStimulationSet* m_stimSet = nullptr;
    bool m_sendStim = false;

    // Threshold calibration
    std::deque<double> m_ppgSignal; // contains the last seconds of the signal, on which the threshold will be based

    // Peak detection
    uint32_t m_nbSamples = 0;

    double m_lastValue = 0.0;
    bool m_increasing = false;

    int m_timeWindow = 0;
    std::deque<std::pair<uint32_t, double>> m_peaks; // contains the timestamps and values peaks recorded less than m_timeWindow ago

    uint64_t m_nextPeakEstimSampleIndex = 0; // The index of the sample that will likely contain the next peak
    uint64_t m_nextPeakEstimSampleIndexUpperBound = 0; // The next peak should not have a sample index greater than this

    // IBI and HR calculation
    std::deque<double> m_interPeakIntervals;
    double m_lastIPIAverage = 0.0;

    // Methods

    /**
    * @brief Defines an amplitude threshold according the previous 3 seconds of signal.
    * @return The amplitude threshold, that will be used to detect peaks.
    */
    double calibrateThreshold();

    /**
    * @brief Populates the m_peaks deque with newly detected peaks, and removes from m_peaks and m_interPeakIntervals the peaks and intervals that were recorded too long ago.
    *         Some actual peaks might be missed by the algorithm when the threshold is too high, in which case they will be recovered on the next call.
    * @param threshold The amplitude of signal above which peaks will be considered as peaks. Any peak detected below that threshold will be discarded.
    * @return The number of detected peaks.
    */
    int detectPeaks(double threshold);

    /**
    * @brief Populates the m_interPeakIntervals queue with the time intervals separating the most recently detected peaks,
    * and removes the false peaks (peaks that are too close to another higher peak).
    * @param nbNewPeaks The numbers of new peaks that were detected in the detectPeaks method.
    */
    void calculateInterPeakIntervals(int nbNewPeaks);

    /**
    * @brief Computes the average of the input vector.
    *
    * @param interPeakIntervals The vector that will be averaged.
    * @return The average of the input vector.
    */
    double vectorAverage(const std::deque<double>& interPeakIntervals);


};

/// <summary> Descriptor of the box Pulse Rate Calculator. </summary>
class CBoxAlgorithmPulseRateCalculatorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return CString("Pulse Rate Calculator"); }
    CString getAuthorName() const override { return CString("Axel Bouneau"); }
    CString getAuthorCompanyName() const override { return CString("Inria"); }
    CString getShortDescription() const override { return CString("Computes the Pulse Rate Variability (PRV) of a PPG signal"); }
    CString getDetailedDescription() const override { return CString("Detects the peaks (corresponding to the subject's pulse) in the PPG signal, in order to compute the pulse rate variability."); }
    CString getCategory() const override { return CString("Physio/PPG"); }
    CString getVersion() const override { return CString("1.0"); }
    CString getStockItemName() const override { return CString("gtk-remove"); }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_PulseRateCalculator; }
    IPluginObject* create() override { return new CBoxAlgorithmPulseRateCalculator; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Signal",OV_TypeId_Signal);

        prototype.addOutput("PRV",OV_TypeId_Signal);
        prototype.addOutput("Heart beats", OV_TypeId_Stimulations);

        prototype.addSetting("Time window (s)",OV_TypeId_Integer, "12");
        prototype.addSetting("Send stimulation when beat detected", OV_TypeId_Boolean, "false");

        return true;
    }
    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_PulseRateCalculatorDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
