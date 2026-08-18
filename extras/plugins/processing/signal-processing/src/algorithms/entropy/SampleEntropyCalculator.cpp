///-------------------------------------------------------------------------------------------------
/// 
/// \file SampleEntropyCalculator.cpp
/// \brief Classes of the Box Entropy Measures.
/// \author Axel Bouneau (Inria).
/// \version 1.0.
/// \date Mon Jan 29 10:00:00 2024.
/// \Copyright (C) 2024 Inria
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

#include "SampleEntropyCalculator.hpp"

#include <boost/math/special_functions/binomial.hpp>


namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

SampleEntropyCalculator::SampleEntropyCalculator(int m, double r, int scale): EntropyCalculator(scale), m_m(m), m_r(r) {
    m_countM.resize(m_scale);
    m_countM1.resize(m_scale);
    m_univariateEntropy.resize(m_scale);
    m_leftOffIndexes.resize(m_scale);
}

void SampleEntropyCalculator::initialize(size_t slidingWindowSize, size_t intervalTimeSize, size_t nbChannels) {
    EntropyCalculator::initialize(slidingWindowSize, intervalTimeSize, nbChannels);

    double maxSimilarVectors = boost::math::binomial_coefficient<double>(slidingWindowSize,2);
    m_maxTheoriticalEntropy = log(maxSimilarVectors);



    for(size_t scaleIdx = 0; scaleIdx < m_scale; scaleIdx++) {

        m_countM[scaleIdx].assign(m_nbChannels, SlidingWindowCounter(slidingWindowSize));
        m_countM1[scaleIdx].assign(m_nbChannels, SlidingWindowCounter(slidingWindowSize));
        m_univariateEntropy[scaleIdx].assign(m_nbChannels, m_maxTheoriticalEntropy);
        m_leftOffIndexes[scaleIdx].assign(m_nbChannels, 0);
    }

}

int SampleEntropyCalculator::similarVectors(size_t chanIdx, int startIdx1, int startIdx2, int scale) {


    const std::deque<double>& timeSeries = m_signal[0][chanIdx].getData();
    if (startIdx1 > startIdx2 - scale) {
        return 0;
    }

    if (startIdx2 > (int)timeSeries.size() - (m_m+1)*scale) {
        return 0;
    }

    for(int k = 0; k < m_m + 1; k++) {
        
        double coarseGrainedVal1 = 0;
        double coarseGrainedVal2 = 0;

        for(int s = 0; s < scale; s++) {
            coarseGrainedVal1 += timeSeries[startIdx1+(k*scale)+s];
            coarseGrainedVal2 += timeSeries[startIdx2+(k*scale)+s];
        }

        coarseGrainedVal1 /= (double)scale;
        coarseGrainedVal2 /= (double)scale;

        if(fabs(coarseGrainedVal1 - coarseGrainedVal2) > m_r) {
            if(k < m_m) { return 0; }
            return 1;
        }
    }
    return 2;
}


int SampleEntropyCalculator::similarVectorsMultivariate(size_t chanIdx1, size_t chanIdx2, int startIdx1, int startIdx2, int scale) {


    const std::deque<double>& timeSeries1 = m_signal[0][chanIdx1].getData();
    const std::deque<double>& timeSeries2 = m_signal[0][chanIdx2].getData();

    if(chanIdx1 == chanIdx2 && startIdx1 >= startIdx2) {
        return 0;
    }

    if (startIdx1 > (int)timeSeries1.size() - (m_m+1)*scale || startIdx2 > (int)timeSeries2.size() - (m_m+1)*scale) {
        return 0;
    }

    for(int k = 0; k < m_m + 1; k++) {
        
        double coarseGrainedVal1 = 0;
        double coarseGrainedVal2 = 0;

        for(int s = 0; s < scale; s++) {
            coarseGrainedVal1 += timeSeries1[startIdx1+(k*scale)+s];
            coarseGrainedVal2 += timeSeries2[startIdx2+(k*scale)+s];
        }

        coarseGrainedVal1 /= (double)scale;
        coarseGrainedVal2 /= (double)scale;

        if(fabs(coarseGrainedVal1 - coarseGrainedVal2) > m_r) {
            if(k < m_m) return 0;
            return 1;
        }
    }
    return 2;
}

void SampleEntropyCalculator::updateUnivariate(size_t scaleIdx, size_t chanIdx) {
    // Update counts of similar vectors of queue N°chanIdx
    int chanSize = m_signal[0][chanIdx].size();
    if(chanSize >= (m_m + 2) * (scaleIdx + 1)) { //otherwise the channel is too short for any calculation to be done
        bool flag = false;
        for(int newlyAddedVectIdx = m_leftOffIndexes[scaleIdx][chanIdx]; newlyAddedVectIdx <= (int)chanSize - (m_m+1) * ((int)scaleIdx+1) ; newlyAddedVectIdx += scaleIdx+1) {

            for(int vectIdx = newlyAddedVectIdx - (scaleIdx+1) ; vectIdx >= 0 ; vectIdx-=scaleIdx+1) {
                flag = true;

                int similarityLevel = similarVectors(chanIdx, vectIdx, newlyAddedVectIdx, scaleIdx+1);
                
                if(similarityLevel == SIMILAR_LENGTH_M_PLUS_1) {
                    m_countM[scaleIdx][chanIdx].incrementAt(vectIdx);
                    m_countM1[scaleIdx][chanIdx].incrementAt(vectIdx);
                }

                if(similarityLevel == SIMILAR_LENGTH_M) {
                    m_countM[scaleIdx][chanIdx].incrementAt(vectIdx);
                }
            }
        }
        if(flag) {
            m_leftOffIndexes[scaleIdx][chanIdx] = chanSize - (m_m) * (scaleIdx+1);
        }
    }

    int totM = m_countM[scaleIdx][chanIdx].getTotal();
    int totM1 = m_countM1[scaleIdx][chanIdx].getTotal();

    int a;

    if(totM == 0 || totM1 == 0) { // "infinite" entropy
        // In theory, in that case, the entropy should be infinite. But to avoid setting it to an arbitrary high number,
        // it is set to m_maxTheoriticalEntropy, which is higher than any value the entropy can reach when it's not infinite
        m_univariateEntropy[scaleIdx][chanIdx] = m_maxTheoriticalEntropy;
    } else {
        m_univariateEntropy[scaleIdx][chanIdx] = -log((double)totM1/(double)totM);
    }
}

void SampleEntropyCalculator::updateMultivariate(size_t scaleIdx, size_t nbChannels) {

    int chanSize = m_signal[0][0].size();
    if(chanSize >= (m_m + 2) * (scaleIdx + 1)) { //otherwise the channel is too short for any calculation to be done
        bool flag = false;
        for (size_t newlyAddedVectChanIdx = 0 ; newlyAddedVectChanIdx < nbChannels; ++newlyAddedVectChanIdx) {
            for(int newlyAddedVectIdx = m_leftOffIndexes[scaleIdx][newlyAddedVectChanIdx]; newlyAddedVectIdx <= (int)chanSize - (m_m+1) * ((int)scaleIdx+1) ; newlyAddedVectIdx += scaleIdx+1) {


                for(size_t vectChanIdx = 0 ; vectChanIdx <= newlyAddedVectChanIdx; vectChanIdx++) {
                    for(int vectIdx = newlyAddedVectIdx ; vectIdx >= 0 ; vectIdx-=scaleIdx+1) {

                        if((vectChanIdx < newlyAddedVectChanIdx && vectIdx <= newlyAddedVectIdx) || vectIdx <= newlyAddedVectIdx - scaleIdx) {

                            flag = true;

                            int similarityLevel = similarVectorsMultivariate(vectChanIdx, newlyAddedVectChanIdx, vectIdx, newlyAddedVectIdx, scaleIdx+1);
                            
                            if(similarityLevel == SIMILAR_LENGTH_M_PLUS_1) {
                                m_countM[scaleIdx][vectChanIdx].incrementAt(vectIdx);
                                m_countM1[scaleIdx][vectChanIdx].incrementAt(vectIdx);
                            }

                            if(similarityLevel == SIMILAR_LENGTH_M) {
                                m_countM[scaleIdx][vectChanIdx].incrementAt(vectIdx);
                            }

                        }

                    }
                }
            }

            if(flag) {
                m_leftOffIndexes[scaleIdx][newlyAddedVectChanIdx] = chanSize - (m_m) * (scaleIdx+1);
            }
        }
    }

    int totM = 0;
    int totM1 = 0;

    for(size_t chanIdx = 0; chanIdx < nbChannels; chanIdx++) {
        totM  += m_countM[scaleIdx][chanIdx].getTotal();
        totM1 += m_countM1[scaleIdx][chanIdx].getTotal();
    }

    if(totM == 0 || totM1 == 0) { // "infinite" entropy
        // In theory, in that case, the entropy should be infinite. But to avoid setting it to an arbitrary high number,
        // it is set to m_maxTheoriticalEntropy, which is higher than any value the entropy can reach when it's not infinite
        m_multivariateEntropy[scaleIdx] = m_maxTheoriticalEntropy;
    } else {
        m_multivariateEntropy[scaleIdx] = -log((double)totM1/(double)totM);

    }


}

void SampleEntropyCalculator::calculateUnivariate(double* &inputBuffer, size_t nbSamplesPerChannel, size_t nbChannels, std::vector<CMatrix*> outputMatrices) {
    bool mustUpdateEntropy = false;

    for(size_t sampleIdx = 0 ; sampleIdx < nbSamplesPerChannel; ++sampleIdx) { 
            
        if(m_totSamplesCount + sampleIdx >= m_totSampCntAtLastUpdate + m_intervalTimeSize) {
            m_totSampCntAtLastUpdate = m_totSamplesCount + sampleIdx;
            mustUpdateEntropy = true;
        }
        for (size_t chanIdx = 0 ; chanIdx < nbChannels; ++ chanIdx) {

            // Update m_signal so it contains the latest m_timeWindow seconds of signal 
            m_signal[0][chanIdx].add(inputBuffer[chanIdx*nbSamplesPerChannel + sampleIdx]);

            for(size_t scaleIdx = 0; scaleIdx < m_scale; scaleIdx++) {

                m_countM[scaleIdx][chanIdx].add(0);
                m_countM1[scaleIdx][chanIdx].add(0);
                
                // Remove samples older than m_timeWindow from the queue N°chanIdx
                if (m_signal[0][chanIdx].size() > m_slidingWindowSize) {
                    if(m_leftOffIndexes[scaleIdx][chanIdx] != 0) {
                        m_leftOffIndexes[scaleIdx][chanIdx]--;
                    }
                }
                
                if (mustUpdateEntropy) {
                    updateUnivariate(scaleIdx, chanIdx);
                }
                
                outputMatrices[scaleIdx]->getBuffer()[chanIdx*nbSamplesPerChannel + sampleIdx] = m_univariateEntropy[scaleIdx][chanIdx];

            }
            
        }
        mustUpdateEntropy = false;
        
    }
    m_totSamplesCount += nbSamplesPerChannel;
}

void SampleEntropyCalculator::calculateMultivariate(double* &inputBuffer, size_t nbSamplesPerChannel, size_t nbChannels, std::vector<CMatrix*> outputMatrices) {
    bool mustUpdateEntropy = false;

    for(size_t sampleIdx = 0 ; sampleIdx < nbSamplesPerChannel; ++sampleIdx) { 
        
        if(m_totSamplesCount + sampleIdx >= m_totSampCntAtLastUpdate + m_intervalTimeSize) {
            m_totSampCntAtLastUpdate = m_totSamplesCount + sampleIdx;
            mustUpdateEntropy = true;
        }
        for (size_t chanIdx = 0 ; chanIdx < nbChannels; ++ chanIdx) {

            // Update m_signal so it contains the latest m_timeWindow seconds of signal 
            m_signal[0][chanIdx].add(inputBuffer[chanIdx*nbSamplesPerChannel + sampleIdx]);

            for(size_t scaleIdx = 0; scaleIdx < m_scale; scaleIdx++) {

                m_countM[scaleIdx][chanIdx].add(0);
                m_countM1[scaleIdx][chanIdx].add(0);
                
                // Remove samples older than m_timeWindow from the queue N°chanIdx
                if (m_signal[0][chanIdx].size() > m_slidingWindowSize) {
                    if(m_leftOffIndexes[scaleIdx][chanIdx] != 0) {
                        m_leftOffIndexes[scaleIdx][chanIdx]--;
                    }
                }
                
            }
        }

        for(size_t scaleIdx = 0; scaleIdx < m_scale; scaleIdx++) {
            if (mustUpdateEntropy) {
                updateMultivariate(scaleIdx, nbChannels);
            }
            
            outputMatrices[scaleIdx]->getBuffer()[sampleIdx] = m_multivariateEntropy[scaleIdx];
        }

            
        mustUpdateEntropy = false;
        
    }
    m_totSamplesCount += nbSamplesPerChannel;
}


}
}
}