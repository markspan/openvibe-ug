/**
 * SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "Thread.h"

#include <iostream>
#include <cstring>

#if defined (_WIN64) || defined(_WIN32)
#include <Windows.h>
#endif

constexpr static const int inputMinPriority = 1;
constexpr static const int inputMaxPriority = 32;

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NON STATIC
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void thread::init()
{
#if defined(_WIN64) || defined(_WIN32)
    int priority = 0;
    m_thread = native_handle();
    osPriority(m_thread, priority, m_policy);

    m_minPriority = THREAD_PRIORITY_IDLE;
    m_maxPriority = THREAD_PRIORITY_TIME_CRITICAL;

    m_internalPriority = priorityReversMap(priority, m_minPriority, m_maxPriority);

#else

    int priority = 0;
    m_thread = native_handle();
    osPriority(m_thread, priority, m_policy);

    sched_param sch_params;
    if (pthread_getschedparam(m_thread, &m_policy, &sch_params) == 0)
    {
        m_minPriority = sched_get_priority_min(m_policy); // SCHED_OTHER, SCHED_FIFO, SCHED_RR, SCHED_MIN, SCHED_MAX, ...
        m_maxPriority = sched_get_priority_max(m_policy); // SCHED_OTHER, SCHED_FIFO, SCHED_RR, SCHED_MIN, SCHED_MAX, ...
        if (m_policy != SCHED_FIFO && m_policy != SCHED_RR)
        {
            m_policy = SCHED_FIFO;
            pthread_setschedparam(m_thread, m_policy, &sch_params);
        }
    }

    m_internalPriority = priorityReversMap(priority, m_minPriority, m_maxPriority);
#endif
}


/**
 * @brief Sets the priority of the current thread.
 *
 * @param priority The new thread priority (between `inputMinPriority` and `inputMaxPriority`).
 * @return `true` if the priority was successfully set, `false` otherwise.
 */
bool thread::setPriority(int priority)
{
    bool bRes = true;

    auto osPriority = thread::priorityMap(priority, m_minPriority, m_maxPriority);
    bRes = setOsPriority(m_thread, osPriority, m_policy);
    if (bRes)
    {
        m_internalPriority = priority;
    }
    else
    {
        std::cerr << "Failed to set Thread scheduling : " << strerror(errno) << std::endl;
    }

    return bRes;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// STATIC
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
//// ****************************************************************************
//// Windows
//// ****************************************************************************
int osMinPriority(int policy)
{
    (void)policy;
    return THREAD_PRIORITY_IDLE;
}

int osMaxPriority(int policy)
{
    (void)policy;
    return THREAD_PRIORITY_TIME_CRITICAL;
}

#else

// ****************************************************************************
// Linux
// ****************************************************************************
int osMinPriority(int policy)
{
    return sched_get_priority_min(policy);
}

int osMaxPriority(int policy)
{
    return sched_get_priority_max(policy);
}
#endif

// ****************************************************************************
// Common
// ****************************************************************************

// ------------------------------------
// PUBLIC

/**
 * @brief Sets the priority of a given thread.
 *
 * @param id The thread identifier.
 * @param priority The new thread priority (between `inputMinPriority` and `inputMaxPriority`).
 * @return `true` if the priority was successfully set, `false` otherwise.
 */
bool thread::setPriority(threadId id, int priority)
{
    bool bRes = false;

    int minPriority  = 0;
    int maxPriority  = 0;
    int processClass = 0;

    bRes = osPriority(id, priority, processClass);
    if (bRes)
    {
        minPriority = osMinPriority(processClass);
        maxPriority = osMaxPriority(processClass);
        bRes = setOsPriority(id, priorityMap(priority, minPriority, maxPriority), processClass) != 0;
    }
    if (!bRes)
    {
        std::cerr << "Failed to set Thread scheduling : " << std::strerror(errno) << std::endl;
    }

    return bRes;
}

/**
 * @brief Retrieves the priority and process class of a given thread.
 *
 * @param id The thread identifier.
 * @param priority The thread priority (output).
 * @param processClass The thread process class (output).
 * @return `true` if the priority was successfully retrieved, `false` otherwise.
 */
bool thread::priority(threadId id, int& priority, int& processClass)
{
    bool bRes = osPriority(id, priority, processClass);

    if (bRes)
    {
        int minPriority = osMinPriority(processClass);
        int maxPriority = osMaxPriority(processClass);
        priority = thread::priorityReversMap(priority, minPriority, maxPriority);
    }

    return bRes;
}

/**
 * @brief Increases the priority of a given thread.
 *
 * @param id The thread identifier.
 * @return `true` if the priority was successfully increased, `false` otherwise.
 */
bool thread::increasePriority(threadId id)
{
    return thread::incDecPriority(id, true);
}

/**
 * @brief Decreases the priority of a given thread.
 *
 * @param id The thread identifier.
 * @return `true` if the priority was successfully decreased, `false` otherwise.
 */
bool thread::decreasePriority(threadId id)
{
    return thread::incDecPriority(id, false);
}

// ------------------------------------
// PRIVATE

/**
 * @brief Increases or decreases the priority of a given thread.
 *
 * @param id The thread identifier.
 * @param increment `true` to increase the priority, `false` to decrease it.
 * @return `true` if the priority was successfully modified, `false` otherwise.
 */
bool thread::incDecPriority(threadId id, bool increment)
{
    bool bRes = false;

    int priority;
    int processClass;

    bRes = thread::priority(id, priority, processClass);

    if (bRes)
    {
        int minPriority = osMinPriority(processClass);
        int maxPriority = osMaxPriority(processClass);

        auto currentOsPriority = thread::priorityMap(priority, minPriority, maxPriority);
        auto osPriority = currentOsPriority;
        do
        {
            increment ? priority++ : priority--;
            currentOsPriority = thread::priorityMap(priority, minPriority, maxPriority);

        } while (currentOsPriority == osPriority);

        bRes = thread::setPriority(id, priority);
    }

    return bRes;
}

/**
 * @brief Retrieves the operating system priority of a given thread.
 *
 * @param id The thread identifier.
 * @param priority The operating system priority of the thread (output).
 * @param processClass The thread process class (output).
 * @return `true` if the priority was successfully retrieved, `false` otherwise.
 */
bool thread::osPriority(threadId id, int& priority, int& processClass)
{
#ifdef WIN32
    processClass = static_cast<int>( GetPriorityClass(OpenProcess(PROCESS_ALL_ACCESS, TRUE,GetProcessIdOfThread(id))) );
    priority = GetThreadPriority(id) ;
    return (processClass != 0) && (priority != 0);
#else
    return pthread_getschedparam(id, &processClass, (struct sched_param*) & priority) == 0;
#endif
}

/**
 * @brief Sets the operating system priority of a given thread.
 *
 * @param id The thread identifier.
 * @param osPriority The operating system priority.
 * @param processClass The thread process class.
 * @return `true` if the priority was successfully set, `false` otherwise.
 */
bool thread::setOsPriority(threadId id, int osPriority, int processClass)
{
#ifdef WIN32
    (void)processClass;
    return SetThreadPriority(id, osPriority) != 0;
#else
    return pthread_setschedparam(id, processClass, (struct sched_param*) & osPriority) != 0;
#endif
}


/**
 * @brief Converts an internal priority to an operating system priority.
 *
 * @param priority The internal priority.
 * @param minPriority The minimum operating system priority.
 * @param maxPriority The maximum operating system priority.
 * @return The corresponding operating system priority.
 */
int thread::priorityMap(int priority, int minPriority, int maxPriority)
{
    return (priority - inputMinPriority) * (maxPriority - minPriority) / (inputMaxPriority - inputMinPriority) + minPriority;
}

/**
 * @brief Converts an operating system priority to an internal priority.
 *
 * @param OSPriority The operating system priority.
 * @param minPriority The minimum operating system priority.
 * @param maxPriority The maximum operating system priority.
 * @return The corresponding internal priority.
 */
int thread::priorityReversMap(int OSPriority, int minPriority, int maxPriority)
{
    return (OSPriority - minPriority) * (inputMaxPriority - inputMinPriority) / (maxPriority - minPriority) + inputMinPriority;
}

