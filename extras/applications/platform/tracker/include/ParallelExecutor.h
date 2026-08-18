#pragma once

#include <map>
#include <list>
#include <deque>
#include <vector>
#include <functional>

#include <mutex>
#include <condition_variable>
#include <thread>

/*
 * Class allowing creating a set of worker threads that then pull jobs from a job list.
 *
 * Before this class can be used safely, various components in the OV SDK should be made 
 * thread safe. These are at least
 * - LogManager
 * - CKernelObjectFactory
 * - AlgorithmManager?
 * - ConfigurationManager? (if any plugin adds tokens)
 *
 */
namespace OpenViBE {
namespace Tracker {
typedef std::function<void(size_t)> jobCall;

// Fwd declare
class CWorkerThread;

class ParallelExecutor
{
public:
    ParallelExecutor() { }
    bool initialize(const uint32_t nThreads);
    bool uninitialize();
    uint32_t getNumThreads() const { return m_threads.size(); }

    // Clients call these
    bool pushJob(const jobCall& someJob);                       // add job, pass to threads
    bool pushJobList(const std::deque<jobCall>& vJobList);      // add jobs
    bool clearPendingJobs();                                    // Delete all jobs thath haven't been launched yet      
    bool waitForAll();                                          // wait until all pushed jobs are done
    size_t getJobCount() const;                                 // Number of remaining jobs
    bool isIdle() const;                                        // Nothing running & no tasks in list?

    // Worker threads call these
    bool getJob(jobCall& job);                                  // The call will hang until there is a job available or the executor is told to quit
    bool declareDone();                                         // Declare the previously obtained job as done

    bool launchTest();

private:
    mutable std::mutex m_jobMutex;

    std::deque<jobCall> m_jobList;
    uint32_t m_nJobsRunning = 0;

    std::condition_variable m_haveWork;
    std::condition_variable m_jobDone;

    bool m_quit = false;

    std::vector<CWorkerThread*> m_workerThreads;
    std::vector<std::thread*> m_threads;
};

// Provides the worker threads a limited access to the parallel executor
class ExecutorView
{
public:
    explicit ExecutorView(ParallelExecutor& exec) : m_exec(exec) { }
    bool getJob(jobCall& job) const { return m_exec.getJob(job); }
    bool declareDone() const { return m_exec.declareDone(); }

private:
    ParallelExecutor& m_exec;
};

class CWorkerThread
{
public:
    CWorkerThread() { }

    static void run(const ExecutorView ctx, const uint32_t threadNumber)
    {
        while (true) {
            jobCall job;
            if (!ctx.getJob(job)) { return; }

            // @todo pass job failed to caller
            job(threadNumber);
            ctx.declareDone();
        }
    }

    static void startWorkerThread(CWorkerThread* /*thread*/, const ExecutorView ctx, const uint32_t threadNumber) { run(ctx, threadNumber); }
};
}  // namespace Tracker
}  // namespace OpenViBE
