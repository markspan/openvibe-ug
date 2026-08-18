///-------------------------------------------------------------------------------------------------
/// 
/// \file SlidingWindowHelper.cpp
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

#include "SlidingWindowHelper.hpp"


namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

// ----------------- SlidingWindowChannel class -----------------
SlidingWindowChannel::SlidingWindowChannel(size_t maxLength): SlidingWindowQueue<double>(maxLength) {}

void SlidingWindowChannel::add(double value) {
    if (m_data.size() > m_maxLength) {
        m_data.pop_front(); // Remove the oldest element if at max length
    }
    m_data.push_back(value); // Add the new value
}

// -----------------SlidingWindowCounter class -----------------
SlidingWindowCounter::SlidingWindowCounter(size_t maxLength): SlidingWindowQueue<int>(maxLength), m_total(0) {}

void SlidingWindowCounter::add(int value) {
    if (m_data.size() > m_maxLength) {
        m_total -= m_data.front(); 
        m_data.pop_front(); 
    }
    m_data.push_back(value); 
    m_total += value; 
}

void SlidingWindowCounter::incrementAt(size_t index) {
    if(index >= m_data.size()) {
        throw std::invalid_argument("Index out of bounds");
    } else {
        m_data[index]++;
        m_total++;
    }
}

int SlidingWindowCounter::getTotal() const {
    return m_total;
}

}
}
}