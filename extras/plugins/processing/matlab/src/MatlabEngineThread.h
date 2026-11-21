#pragma once
// UG fork: new file, not in upstream OpenViBE. See UG.md, 2.58.
//
// The MATLAB engine talks to MATLAB over COM (Windows), and a COM connection works only from the thread that opened
// it. Designer initializes and uninitializes the player on its window thread, but plays it on a thread of its own, so
// the engine opened in initialize() failed at the first call from process() or processClock(). Every engine call
// now runs on one thread, this one, whichever thread asks: the asking thread waits for the answer.
//
// The macros at the end route the box's and the helper's engine calls here, unchanged at the call site. A call
// written with the name in parentheses, (engOpen)(...), is not routed: for code that already runs on this thread.

#include <engine.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>

#if defined TARGET_OS_Windows
#include <windows.h>
#include <objbase.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace Matlab {

class CEngineThread
{
public:
    /// Starts the engine thread for its first user (a MATLAB box being initialized). Pair with release().
    static void acquire()
    {
        State& s = state();
        std::lock_guard<std::mutex> lock(s.mutex);
        if (s.users++ > 0) { return; }
        s.stopping = false;
#if defined TARGET_OS_Windows
        s.wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
#endif
        s.thread = std::thread(loop);
        s.id     = s.thread.get_id();
    }

    /// Ends the engine thread with its last user, after the calls waiting for it have run.
    static void release()
    {
        State& s = state();
        std::thread ending;
        {
            std::lock_guard<std::mutex> lock(s.mutex);
            if (s.users == 0 || --s.users > 0) { return; }
            s.stopping = true;
            ending.swap(s.thread);
        }
        wakeUp();
        if (ending.joinable()) { ending.join(); }
        std::lock_guard<std::mutex> lock(s.mutex);
        s.id = std::thread::id();
#if defined TARGET_OS_Windows
        if (s.wake) { CloseHandle(s.wake); }
        s.wake = nullptr;
#endif
    }

    /// Runs f on the engine thread and returns what it returns. On the engine thread itself, or with no engine thread
    /// running, f runs at once on the calling thread.
    template <typename F>
    static auto run(F&& f) -> std::invoke_result_t<F&>
    {
        using Result = std::invoke_result_t<F&>;
        State& s     = state();
        std::packaged_task<Result()> task([&f]() -> Result { return f(); });
        std::future<Result> result = task.get_future();
        bool here;
        {
            std::lock_guard<std::mutex> lock(s.mutex);
            here = s.users == 0 || std::this_thread::get_id() == s.id;
            if (!here) { s.jobs.emplace_back([&task]() { task(); }); }
        }
        if (here) { task(); }
        else { wakeUp(); }
        return result.get();
    }

private:
    struct State
    {
        std::mutex mutex;
        std::condition_variable wakeCondition;   // used where there are no Windows messages to pump
        std::deque<std::function<void()>> jobs;
        std::thread thread;
        std::thread::id id;
        size_t users  = 0;
        bool stopping = false;
#if defined TARGET_OS_Windows
        HANDLE wake = nullptr;
#endif
    };

    static State& state()
    {
        static State s;
        return s;
    }

    static void wakeUp()
    {
        State& s = state();
#if defined TARGET_OS_Windows
        if (s.wake) { SetEvent(s.wake); }
#endif
        s.wakeCondition.notify_all();
    }

    static void loop()
    {
        State& s = state();
#if defined TARGET_OS_Windows
        // A single-threaded apartment, as the engine's own examples have on their main thread. It pumps its messages
        // while it waits, as such a thread must.
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
        while (true)
        {
            std::deque<std::function<void()>> jobs;
            bool stop;
            {
                std::unique_lock<std::mutex> lock(s.mutex);
#if !defined TARGET_OS_Windows
                s.wakeCondition.wait(lock, [&s]() { return s.stopping || !s.jobs.empty(); });
#endif
                jobs.swap(s.jobs);
                stop = s.stopping;   // no job comes after that: with no users left, run() runs its own
            }
            for (auto& job : jobs) { job(); }
            if (stop) { break; }
#if defined TARGET_OS_Windows
            MsgWaitForMultipleObjects(1, &s.wake, FALSE, INFINITE, QS_ALLINPUT);
            MSG msg;
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
#endif
        }
#if defined TARGET_OS_Windows
        if (SUCCEEDED(com)) { CoUninitialize(); }
#endif
    }
};

}  // namespace Matlab
}  // namespace Plugins
}  // namespace OpenViBE

#define engEvalString(...)   ::OpenViBE::Plugins::Matlab::CEngineThread::run([&]() { return ::engEvalString(__VA_ARGS__); })
#define engGetVariable(...)  ::OpenViBE::Plugins::Matlab::CEngineThread::run([&]() { return ::engGetVariable(__VA_ARGS__); })
#define engPutVariable(...)  ::OpenViBE::Plugins::Matlab::CEngineThread::run([&]() { return ::engPutVariable(__VA_ARGS__); })
#define engOutputBuffer(...) ::OpenViBE::Plugins::Matlab::CEngineThread::run([&]() { return ::engOutputBuffer(__VA_ARGS__); })
#define engOpen(...)         ::OpenViBE::Plugins::Matlab::CEngineThread::run([&]() { return ::engOpen(__VA_ARGS__); })
#define engClose(...)        ::OpenViBE::Plugins::Matlab::CEngineThread::run([&]() { return ::engClose(__VA_ARGS__); })
