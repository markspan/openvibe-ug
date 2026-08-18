///-------------------------------------------------------------------------------------------------
/// 
/// \file AbsctractEntropyCalculator.cpp
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

EntropyCalculator::EntropyCalculator(int scale) : m_scale(scale) {
    m_signal.resize(m_scale);
    m_univariateEntropy.resize(m_scale);
    m_multivariateEntropy.resize(m_scale);
}

void EntropyCalculator::initialize(size_t slidingWindowSize, size_t intervalTimeSize, size_t nbChannels) {
    m_slidingWindowSize = slidingWindowSize;
    m_intervalTimeSize = intervalTimeSize;
    m_nbChannels = nbChannels;
    

    for(size_t scaleIdx = 0; scaleIdx < m_scale; scaleIdx++) {
        for(size_t channelIdx = 0; channelIdx < m_nbChannels; channelIdx++) {
            m_signal[scaleIdx].emplace_back(slidingWindowSize / (scaleIdx + 1));
        }
    }
}

}
}
}