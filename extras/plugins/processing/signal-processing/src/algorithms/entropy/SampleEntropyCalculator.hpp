///-------------------------------------------------------------------------------------------------
/// 
/// \file SampleEntropyCalculator.hpp
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

#include "AbstractEntropyCalculator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

/// <summary> The class SampleEntropyCalculator describes one implementation of the Entropy Calculator abstract class that computes sample entropy. </summary>
class SampleEntropyCalculator : public EntropyCalculator {
public:
    SampleEntropyCalculator(int m, double r, int scale = 1);

    void initialize(size_t slidingWindowSize, size_t intervalTimeSize, size_t nbChannels) override;

    /**
    * @brief Updates m_univariateEntropy so it contains, for each channel, the values of the corresponding sample entropy 
    */
    void calculateUnivariate(double* &buffer, size_t nbSamplesPerChannel, size_t nbChannels, std::vector<CMatrix*> outputMatrices) override;
    /**
    * @brief Updates m_multivariateEntropy so it contains the values of the multivariate sample entropy 
    */
    void calculateMultivariate(double* &buffer, size_t nbSamplesPerChannel, size_t nbChannels, std::vector<CMatrix*> outputMatrices) override;
    


private:
    int m_m;
    double m_r;

    std::vector<std::vector<SlidingWindowCounter>> m_countM;
    std::vector<std::vector<SlidingWindowCounter>> m_countM1;

    std::vector<std::vector<size_t>> m_leftOffIndexes;
    double m_maxTheoriticalEntropy;

    enum SimilarityLevel {NOT_SIMILAR, SIMILAR_LENGTH_M, SIMILAR_LENGTH_M_PLUS_1};
    /**
    * @brief Checks if vectors of channel chanIdx that start at startIdx1 and startIdx2 and have length m_m have a distance lower that m_r, for scale scale
    */
    int similarVectors(size_t chanIdx, int startIdx1, int startIdx2, int scale);
    int similarVectorsMultivariate(size_t chanIdx1, size_t chanIdx2, int startIdx1, int startIdx2, int scale);
    
    /**
    * @brief Updates the values of the univariate entropy so they can be added to the output buffer
    */
    void updateUnivariate(size_t scaleIdx, size_t chanIdx);
    /**
    * @brief Updates the values of the multivariate entropy so they can be added to the output buffer
    */
    void updateMultivariate(size_t scaleIdx, size_t nbChannels);

};

}
}
}