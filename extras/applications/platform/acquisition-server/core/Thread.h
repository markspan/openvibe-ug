#pragma once
/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * @file Thread.h
 * @brief Implementation of a thread class with priority management.
 *
 * This class inherits from `std::thread` and provides functionalities to modify the thread priority,
 * both as an instance member and as a static function.
 */

#ifndef THREAD_H
#define THREAD_H

#include <thread>

 /**
  * @class thread
  * @brief Thread class with priority management.
  *
  * This class inherits from `std::thread` and adds functionalities to manage thread priority.
  * It allows setting and retrieving the thread priority, both for the current instance and for other threads.
  */
class thread : public std::thread
{
public:
    using threadId = std::thread::native_handle_type;

public:
    template <class _Fn, class... _Args>
    explicit thread(_Fn&& _Fx, _Args&&... _Ax);
    thread(thread&&) = default;
    ~thread() = default;

    thread& operator=(thread&&) = default;

    bool setPriority(int priority);
    bool increasePriority() { return setPriority(m_internalPriority + 1); }
    bool decreasePriority() { return setPriority(m_internalPriority - 1); }

    static bool setPriority(threadId id, int priority);
    static bool priority(threadId id, int &priority, int &processClass);

    static bool increasePriority(threadId id);
    static bool decreasePriority(threadId id);


private:
    thread(const thread&) = delete;
    thread() = delete;
    thread& operator=(const thread&) = delete;
    
    void init();

    static bool osPriority(threadId id, int &priority, int &processClass);
    static bool setOsPriority(threadId id, int priority, int processClass);

    static int priorityMap(int priority, int ostMinPriority, int osMaxPriority);
    static int priorityReversMap(int OSPriority, int ostMinPriority, int osMaxPriority);

    static bool incDecPriority(thread::threadId id, bool increment);


private:
    std::thread::native_handle_type m_thread;

    int m_internalPriority;
    int m_minPriority;
    int m_maxPriority;

    int m_policy;
};

#endif // THREAD_H

template<class _Fn, class ..._Args>
inline thread::thread(_Fn&& _Fx, _Args && ..._Ax) : std::thread(std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...), m_policy(0), m_minPriority(0), m_maxPriority(0)
{
    init();
}

