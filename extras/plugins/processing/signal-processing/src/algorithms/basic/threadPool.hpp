///-------------------------------------------------------------------------------------------------
///
/// \file threadPool.hpp
/// \brief Thread Pool implementation
/// \author Arthur Desbois
/// \version 1.0
///
/// \copyright Copyright(C) 2022 Inria
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
#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {


// ThreadPool Class - based on https://stackoverflow.com/questions/15752659/thread-pooling-in-c11
class ThreadPool {
public:
    ThreadPool() : m_numJobsPending(0) {};
    void Start(const uint32_t num_threads);
    void QueueJob(const std::function<void()>& job);
    void Stop();
    void waitUntilCompleted();

private:
    void ThreadLoop();

    bool m_shouldTerminate = false;           // Tells threads to stop looking for jobs
    std::mutex m_queueMutex;                  // Prevents data races to the job queue
    std::condition_variable m_mutexCondition; // Allows threads to wait on new jobs or termination
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_jobs;

    std::atomic<int> m_numJobsPending;
    std::mutex m_mainMutex;
    std::condition_variable m_mainCondition;

public:
    uint64_t getNumThreads(){return m_threads.size();}
};


}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
