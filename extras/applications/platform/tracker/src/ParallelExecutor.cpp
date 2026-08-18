#include <iostream>

#include <system/ovCTime.h>

#include "ParallelExecutor.h"
#include <array>

namespace OpenViBE {
namespace Tracker {

bool ParallelExecutor::initialize(const uint32_t nThreads)
{
    m_quit         = false;
    m_nJobsRunning = 0;
    m_jobList.clear();

    // ExecutorView passes the working threads a few function handles to the executor that holds the state
    ExecutorView ctx(*this);

    for (uint32_t i = 0; i < nThreads; ++i) {
        m_workerThreads.push_back(new CWorkerThread());
        m_threads.push_back(new std::thread(std::bind(&CWorkerThread::startWorkerThread, m_workerThreads[i], ctx, i)));
    }

    return true;
}

bool ParallelExecutor::uninitialize()
{
    // Tell the threads waiting in the cond its time to quit, don't care of pending jobs
    { // scope for lock
        std::unique_lock<std::mutex> lock(m_jobMutex);
        m_jobList.clear();
        m_quit = true;
    }

    m_haveWork.notify_all();

    for (uint32_t i = 0; i < m_threads.size(); ++i) {
        m_threads[i]->join();

        delete m_threads[i];
        delete m_workerThreads[i];
    }

    m_threads.clear();
    m_workerThreads.clear();

    return true;
}

bool ParallelExecutor::pushJob(const jobCall& someJob)
{
    // @fixme to add better concurrency, push a list instead; lock();add list;unlock();notify_all();
    { // lock scope
        std::lock_guard<std::mutex> lock(m_jobMutex);
        m_jobList.push_back(someJob);
    }

    m_haveWork.notify_one();

    return true;
}


bool ParallelExecutor::pushJobList(const std::deque<jobCall>& vJobList)
{
    { // lock scope
        std::lock_guard<std::mutex> lock(m_jobMutex);

        if (!m_jobList.empty()) {
            std::cout << "Error, trying to push list with old jobs pending\n";
            return false;
        }
        m_jobList = vJobList;
    }

    m_haveWork.notify_all();


    return true;
}

bool ParallelExecutor::waitForAll()
{
    std::unique_lock<std::mutex> lock(m_jobMutex);
    while (!m_jobList.empty()) { m_jobDone.wait(lock); }

    return true;
}

bool ParallelExecutor::getJob(jobCall& job)
{
    // Wait until we get a job or are told to quit
    std::unique_lock<std::mutex> lock(m_jobMutex);

    m_haveWork.wait(lock, [this]() { return (this->m_quit || !this->m_jobList.empty()); });

    if (m_quit) { return false; }

    // Ok, we have a job
    job = m_jobList.front();
    m_jobList.pop_front();

    m_nJobsRunning++;

    return true;
}

bool ParallelExecutor::declareDone()
{
    std::unique_lock<std::mutex> lock(m_jobMutex);

    m_nJobsRunning--;
    m_jobDone.notify_one();

    return true;
}

bool ParallelExecutor::clearPendingJobs()
{
    std::unique_lock<std::mutex> lock(m_jobMutex);
    m_jobList.clear();
    return true;
}

size_t ParallelExecutor::getJobCount() const
{
    std::unique_lock<std::mutex> lock(m_jobMutex);
    return m_jobList.size();
}

bool ParallelExecutor::isIdle() const
{
    std::unique_lock<std::mutex> lock(m_jobMutex);
    return (m_jobList.empty() && m_nJobsRunning == 0);
}


//___________________________________________________________________//
//                                                                   //

void testFunction(void* data)
{
    for (uint32_t i = 0; i < 10; ++i) {
        std::cout << "Fun: " << *static_cast<uint32_t*>(data) << "\n";
        System::Time::sleep(*static_cast<uint32_t*>(data));
    }
    //  return true;
}

bool ParallelExecutor::launchTest()
{
    std::array<int, 6> stuff = { 500, 666, 50, 1000, 300, 100 };

    std::cout << "Push test\n";

    this->pushJob(std::bind(testFunction, &stuff[0]));
    this->waitForAll();
    this->pushJob(std::bind(testFunction, &stuff[1]));
    this->pushJob(std::bind(testFunction, &stuff[2]));
    this->pushJob(std::bind(testFunction, &stuff[3]));
    this->pushJob(std::bind(testFunction, &stuff[4]));
    this->pushJob(std::bind(testFunction, &stuff[2]));
    this->waitForAll();
    this->waitForAll();

    std::cout << "Pushlist test\n";

    std::deque<jobCall> jobList;
    jobList.push_back(std::bind(testFunction, &stuff[0]));
    jobList.push_back(std::bind(testFunction, &stuff[1]));
    this->pushJobList(jobList);
    this->waitForAll();

    std::cout << "Done\n";

    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
