///-------------------------------------------------------------------------------------------------
/// 
/// \file SlidingWindowHelper.hpp
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
#include <toolkit/ovtk_all.h>

#include <queue>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


// SlidingWindowQueue abstract class
template <typename T>
class SlidingWindowQueue {
public:
    SlidingWindowQueue(size_t maxLength) : m_maxLength(maxLength) {}

    size_t size() const { return m_data.size(); }
    const std::deque<T>& getData() const { return m_data; }

    virtual void add(T value) = 0;

protected:
    size_t m_maxLength;
    std::deque<T> m_data;
};

// SlidingWindowChannel class
class SlidingWindowChannel : public SlidingWindowQueue<double> {
public:
    explicit SlidingWindowChannel(size_t maxLength);

    void add(double value) override;
};

// SlidingWindowCounter class
class SlidingWindowCounter : public SlidingWindowQueue<int> {
public:
    explicit SlidingWindowCounter(size_t maxLength);
    
    void add(int value) override;
    void incrementAt(size_t index);
    int getTotal() const;
private:
    int m_total;
};

}
}
}