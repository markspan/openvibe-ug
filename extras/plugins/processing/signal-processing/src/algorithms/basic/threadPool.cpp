///-------------------------------------------------------------------------------------------------
///
/// \file threadPool.cpp
/// \brief Thread Pools implementation
/// \author Arthur Desbois
/// \version 1.0
///
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///-------------------------------------------------------------------------------------------------

#include "threadPool.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void ThreadPool::Start(const uint32_t num_threads) {
    m_threads.resize(num_threads);
    for (uint32_t i = 0; i < num_threads; i++) {
        m_threads.at(i) = std::thread(&ThreadPool::ThreadLoop, this);
    }
}

void ThreadPool::ThreadLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_mutexCondition.wait(lock, [this] {
                return !m_jobs.empty() || m_shouldTerminate;
            });
            if (m_shouldTerminate) {
                return;
            }
            job = m_jobs.front();
            m_jobs.pop();
        }
        job();

        {
            std::unique_lock<std::mutex> lock(m_mainMutex);
            if ( --m_numJobsPending == 0 ) {
                m_mainCondition.notify_one();
            }
        }
    }
}

void ThreadPool::QueueJob(const std::function<void()>& job) {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_numJobsPending++;
        m_jobs.push(job);
    }
    m_mutexCondition.notify_one();
}

void ThreadPool::waitUntilCompleted() {
    std::unique_lock<std::mutex> lock(m_mainMutex);
    m_mainCondition.wait(lock, [this] {
      return m_numJobsPending <= 0;
    });
 }

void ThreadPool::Stop() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_shouldTerminate = true;
    }
    m_mutexCondition.notify_all();
    for (std::thread& active_thread : m_threads) {
        active_thread.join();
    }

    m_threads.clear();
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE

