///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmPulseRateCalculator.cpp
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

#include "CBoxAlgorithmPulseRateCalculator.hpp"

#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmPulseRateCalculator::initialize()
{
    m_inputDecoder.initialize(*this, 0);
    m_outputEncoder.initialize(*this, 0);
    m_outputStimEncoder.initialize(*this, 1);

    m_outputEncoder.getInputMatrix().setReferenceTarget(m_inputDecoder.getOutputMatrix());
    m_outputEncoder.getInputSamplingRate().setReferenceTarget(m_inputDecoder.getOutputSamplingRate());

    m_timeWindow = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    if (m_timeWindow < 2) {
        m_timeWindow = 2;
        this->getLogManager() << Kernel::LogLevel_Warning << "Time windows shouldn't be shorter than 2 seconds. Time window overwritten to 2 seconds." << "\n";
    }
    m_sendStim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    this->getLogManager() << Kernel::LogLevel_Info << "Waiting to have some peaks before any calculation.\n";

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmPulseRateCalculator::uninitialize()
{
    m_inputDecoder.uninitialize();
    m_outputEncoder.uninitialize();
    m_outputStimEncoder.uninitialize();

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmPulseRateCalculator::processInput(const size_t index)
{
    // some pre-processing code if needed...
    m_samplingRate = m_inputDecoder.getOutputSamplingRate();
    // ready to process !
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

/*******************************************************************************/


bool CBoxAlgorithmPulseRateCalculator::process()
{
    // the static box context describes the box inputs, outputs, settings structures
    const Kernel::IBox& staticBoxContext = this->getStaticBoxContext();

    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& dynamicBoxContext = this->getDynamicBoxContext();
    m_stimSet = m_outputStimEncoder.getInputStimulationSet();



    for (uint32_t i = 0; i < dynamicBoxContext.getInputChunkCount(0); i++)
    {
        m_start = dynamicBoxContext.getInputChunkStartTime(0, i);
        m_end = dynamicBoxContext.getInputChunkEndTime(0, i);

        // decode the chunk i 
        m_inputDecoder.decode(i);

        if (m_inputDecoder.isHeaderReceived())
        {
            // Pass the header to the next boxes, by encoding a header:
            m_outputEncoder.encodeHeader();
            m_outputStimEncoder.encodeHeader();
        }

        if (m_inputDecoder.isBufferReceived())
        {
            double threshold = calibrateThreshold();

            int nbNewPeaks = detectPeaks(threshold);

            calculateInterPeakIntervals(nbNewPeaks);

            m_lastIPIAverage = vectorAverage(m_interPeakIntervals);

            IMatrix* PRVSignal = m_outputEncoder.getInputMatrix();
            for (uint32_t j = 0; j < PRVSignal->getBufferElementCount(); j++) {
                if (m_nbSamples > m_samplingRate && m_interPeakIntervals.size() >= 2) { // start outputing signal after 1 second of recording, and only if there are at least 2 intervals
                    // Take into account not the latest interval but the one before, as the latest one could be due to a false peak that will be removed in the next iteration
                    PRVSignal->getBuffer()[j] = m_interPeakIntervals[m_interPeakIntervals.size() - 2] * 1000;
                }
                else {
                    PRVSignal->getBuffer()[j] = 0; // before 1 second of recording, send a signal with IBI of 0ms
                }
            }

            m_outputEncoder.encodeBuffer();
            m_outputStimEncoder.encodeBuffer();

            m_stimSet->clear();
            m_stimSet->resize(0);
        }

        if (m_inputDecoder.isEndReceived())
        {
            m_outputEncoder.encodeEnd();
            m_outputStimEncoder.encodeEnd();
        }

        dynamicBoxContext.markOutputAsReadyToSend(0, m_start, m_end);
        dynamicBoxContext.markOutputAsReadyToSend(1, m_start, m_end);
    } // for


    return true;
}

double CBoxAlgorithmPulseRateCalculator::calibrateThreshold()
{
    double* inputBuffer = m_inputDecoder.getOutputMatrix()->getBuffer();
    uint32_t inputBufferSize = m_inputDecoder.getOutputMatrix()->getDimensionSize(1);

    for (int i = 0; i < inputBufferSize; i++) {
        m_ppgSignal.push_back(inputBuffer[i]);
        if (m_ppgSignal.size() > m_samplingRate * 3) { // adapt the threshold to the latest 3 seconds of signal
            m_ppgSignal.pop_front();
        }
    }

    // The threshold can only be computed after 3 seconds of signal
    // to make sure the signal has enough peaks
    if (m_ppgSignal.size() < m_samplingRate * 3) {
        return std::numeric_limits<double>::max();
    }

    // set the threshold so that 80% of the samples are below it
    // so that the threshold will adapt in case of variability in the PPG amplitude
    std::deque<double> tmp;
    tmp.resize(m_ppgSignal.size());
    std::copy(m_ppgSignal.begin(), m_ppgSignal.end(), tmp.begin());
    std::sort(tmp.begin(), tmp.end());
    double threshold = tmp[(int)tmp.size() * 0.80];
    
    return threshold;
}

int CBoxAlgorithmPulseRateCalculator::detectPeaks(double threshold) {

    double*  inputBuffer        = m_inputDecoder.getOutputMatrix()->getBuffer();
    uint32_t inputBufferSize    = m_inputDecoder.getOutputMatrix()->getDimensionSize(1);

    if (m_nbSamples == 0) {
        m_lastValue = inputBuffer[0]; // first sample of the first buffer following the initial threshold calibration
    }

    int nbNewPeaks = 0;

    for (int i = 0; i < inputBufferSize; i++) {

        // if there hasn't been a peak for too long, the algorithm must have missed a peak because it was lower than the threshold
        // Only valid if at least 3 peaks have been recorded in order to have a decent estimation of the IPI
        if (m_nbSamples + i >= m_nextPeakEstimSampleIndexUpperBound && m_peaks.size() >= 3) {
            // the last recorded peak is then duplicated, at the date that was estimated
            std::pair<uint32_t, double> p = { m_nextPeakEstimSampleIndex, m_peaks.back().second };
            m_peaks.push_back(p);
            nbNewPeaks++;
            m_nextPeakEstimSampleIndex = m_nextPeakEstimSampleIndex + m_lastIPIAverage * m_samplingRate;
            m_nextPeakEstimSampleIndexUpperBound = m_nextPeakEstimSampleIndex + 0.5 * m_lastIPIAverage * m_samplingRate;

            if (m_sendStim) { // if the user wants to have a stimulation for every peak detected
                m_stimSet->push_back(OVTK_StimulationId_Label_00, m_start + (m_end - m_start) / inputBufferSize * i, 0);
                this->getLogManager() << Kernel::LogLevel_Info << "A peak was probably missed, so the previous one was duplicated" << "\n";
            }
        }

        // if signal increasing
        if (m_lastValue < inputBuffer[i]) {
            m_increasing = true;
        }
        // if signal decreasing
        if (m_lastValue > inputBuffer[i]) {
            //if signal was increasing just before and signal above threshold, then it's a peak
            if (m_increasing && m_lastValue > threshold) {
                std::pair<uint32_t, double> p = { m_nbSamples + i, inputBuffer[i] };
                m_peaks.push_back(p); // add index of current sample to m_peaks
                nbNewPeaks++;

                m_nextPeakEstimSampleIndex = m_nbSamples + (i - 1) + m_lastIPIAverage * m_samplingRate;
                m_nextPeakEstimSampleIndexUpperBound = m_nextPeakEstimSampleIndex + 0.5 * m_lastIPIAverage * m_samplingRate;

                if (m_sendStim) { // if the user wants to have a stimulation for every peak detected
                    m_stimSet->push_back(OVTK_StimulationId_Label_00, m_start + (m_end - m_start)/inputBufferSize * i, 0);
                }
            }
            m_increasing = false;
        }
        m_lastValue = inputBuffer[i];
    }

    m_nbSamples += inputBufferSize;

    // remove the peaks that were recorded more than m_timeWindow seconds ago (and the corresponding intervals)
    while (!m_peaks.empty() && m_nbSamples - m_peaks.front().first > m_timeWindow * m_samplingRate) {
        m_peaks.pop_front();
        if (!m_interPeakIntervals.empty()) {
            m_interPeakIntervals.pop_front();
        }
    }

    return nbNewPeaks;
    
}

void CBoxAlgorithmPulseRateCalculator::calculateInterPeakIntervals(int nbNewPeaks)
{
    std::vector<double> interPeakIntervals;

    if (m_peaks.empty() || m_peaks.size() < nbNewPeaks) {
        return;
    }

    if (m_peaks.size() == nbNewPeaks) {
        // if there are as many peaks as new peaks, reduce the number of new peaks by 1 
        // for the algorithm to process the intervals
        nbNewPeaks--;
    }

    // Iterate over the peaks of m_peaks that form new intervals
    for (int i = (int)m_peaks.size() - nbNewPeaks - 1; i < (int)m_peaks.size() - 1; i++) {
        
        // Calculate the inter-peak interval as the difference of the samples they were recorded at
        uint32_t nbSamplesBetweenPeaks = m_peaks[i + 1].first - m_peaks[i].first;

        // Convert the inter-peak interval to time
        double intervalTime = ((double) nbSamplesBetweenPeaks) / m_samplingRate;

        //If two beats are too close, there must be one false peak (a peak that isn't actually a heart beat)
        if (intervalTime < m_lastIPIAverage / 2) {  // if a peak is detected less than half the averageIPI away from another peak, it's probably a P or T peak (or an artifact), and should be removed
            
            double removedPeakDate;
            // Remove the smallest peak between i and i+1 (P and T peaks are lower than R peaks)
            if (m_peaks[i].second <= m_peaks[i + 1].second) {
                // in that case, the interval previously computed between peaks[i-1] and peaks[i] must be replaced by the interval between peaks[i-1] and peaks[i+1]
                if (i > 0) {
                    m_interPeakIntervals[i - 1] = (m_peaks[i + 1].first - m_peaks[i - 1].first) / m_samplingRate;
                }
                // and peaks[i] is removed
                removedPeakDate = m_peaks[i].first / m_samplingRate;
                m_peaks.erase(m_peaks.begin() + i);
            }
            else {
                // in that case, the false peak is the one that was just added, so nothing was added to m_interPeakIntervals yet, so nothing to replace
                removedPeakDate = m_peaks[i + 1].first / m_samplingRate;
                m_peaks.erase(m_peaks.begin() + i + 1);
            }



            // Send a stimulation to communicate that a peak was removed
            if (m_sendStim) {
                uint32_t inputBufferSize = m_inputDecoder.getOutputMatrix()->getDimensionSize(1);
                m_stimSet->push_back(OVTK_StimulationId_Label_01, m_start + (m_end - m_start) / inputBufferSize * i, 0);
                this->getLogManager() << Kernel::LogLevel_Info << "A false peak (recorded at time [" << removedPeakDate << " sec]) was removed" << "\n";
            }
            
            i--; // Correct the index to account for the removed peak
        }

        // If the two peaks are not too close, interpret them as actual heartbeats
        else {
            // Add the inter-peak interval to the vector
            m_interPeakIntervals.push_back(intervalTime);
        }

    }

}

double CBoxAlgorithmPulseRateCalculator::vectorAverage(const std::deque<double>& interPeakIntervals)
{
    double sum = 0.0;

    for (const double interval : interPeakIntervals) {
        sum += interval;
    }

    double average = 0.0;

    if (!interPeakIntervals.empty()) {
        average = sum / interPeakIntervals.size();
    }

    return average;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
