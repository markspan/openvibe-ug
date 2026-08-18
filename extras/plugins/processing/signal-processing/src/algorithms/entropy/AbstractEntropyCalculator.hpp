///-------------------------------------------------------------------------------------------------
/// 
/// \file AbsctractEntropyCalculator.hpp
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

#include "../../defines.hpp"
#include <vector>
#include "SlidingWindowHelper.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

/// <summary> The abstract class EntropyCalculator describes the interface for entropy calculator classes. </summary>
class EntropyCalculator {
public:
    EntropyCalculator(int scale);

    virtual void initialize(size_t slidingWindowSize, size_t intervalTimeSize, size_t nbChannels);

    virtual void calculateUnivariate(double* &buffer, size_t nbSamplesPerChannel, size_t nbChannels, std::vector<CMatrix*> outputMatrices) = 0;
    virtual void calculateMultivariate(double* &buffer, size_t nbSamplesPerChannel, size_t nbChannels, std::vector<CMatrix*> outputMatrices) = 0;

protected:
    int m_scale; // m_scale > 1 for multiscale entropy
    size_t m_slidingWindowSize;
    size_t m_intervalTimeSize;
    size_t m_nbChannels;

    std::vector<std::vector<SlidingWindowChannel>> m_signal;

    std::vector<std::vector<double>> m_univariateEntropy;
    std::vector<double> m_multivariateEntropy;
    
    int m_totSamplesCount = 0;
    int m_totSampCntAtLastUpdate = 0;

};


}
}
}