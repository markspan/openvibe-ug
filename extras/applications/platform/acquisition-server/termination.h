/**
 * @file termination.h
 * @brief Cross-platform signal handling with callback support.
 * @details Implements a robust signal handling mechanism. It allows attaching
 * a custom callback to bridge low-level signals with high-level event loops
 * (e.g., Qt, generic loops).
 */

#pragma once // Recommended to avoid multiple inclusions

#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>

// OS-Specific Includes
#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS
    #include <windows.h>
#else
    #define OS_POSIX
    #include <csignal>
#endif

// -----------------------------------------------------------------------------
// Type Definitions
// -----------------------------------------------------------------------------

/**
 * @brief Alias for the stop callback function signature.
 * @details Defines a pointer to a function taking no arguments and returning void.
 */
using StopCallback = void (*)();

// -----------------------------------------------------------------------------
// Global State
// -----------------------------------------------------------------------------

/**
 * @brief Global atomic flag to request application stop.
 * @details std::atomic ensures thread-safety when reading the flag from
 * the main loop while it is being set by the signal handler.
 */
static std::atomic<bool> g_stopRequested(false);

/**
 * @brief Pointer to the optional user-defined callback.
 * @details This function is called immediately when a signal is received.
 * @warning The callback code must be Async-Signal-Safe (especially on POSIX).
 * Avoid memory allocation (malloc/new), heavy I/O, or locking mutexes inside it.
 */
static StopCallback g_stopCallBack = nullptr;

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

/**
 * @brief Registers a custom callback to be executed on signal reception.
 * @param callback A pointer to the function to call.
 */
void RegisterStopCallback(StopCallback callback)
{
    g_stopCallBack = callback;
}

// -----------------------------------------------------------------------------
// Signal Handlers
// -----------------------------------------------------------------------------

#ifdef OS_WINDOWS
/**
 * @brief Windows specific control handler.
 * @param ctrl_type The type of control signal received.
 * @return TRUE if handled, FALSE otherwise.
 */
BOOL WINAPI WindowsCtrlHandler(DWORD ctrl_type)
{
    BOOL bRes = FALSE;

    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT)
    {
        g_stopRequested.store(true);
        
        // Execute the user callback if defined
        if (g_stopCallBack)
        {
            g_stopCallBack();
        }
        
        bRes = TRUE;
    }

    return bRes;
}
#else
/**
 * @brief POSIX specific signal handler.
 * @param signal The signal number received.
 */
void PosixSignalHandler(int signal)
{
    if (signal == SIGINT)
    {
        g_stopRequested.store(true);
        
        // Execute the user callback if defined
        if (g_stopCallBack)
        {
            g_stopCallBack();
        }
    }
}
#endif

/**
 * @brief Registers the cross-platform signal handlers.
 * @return true if registration succeeded, false otherwise.
 */
bool RegisterSignalHandlers()
{
    bool success = true;

#ifdef OS_WINDOWS
    if (!SetConsoleCtrlHandler(WindowsCtrlHandler, TRUE))
    {
        success = false;
    }
#else
    if (std::signal(SIGINT, PosixSignalHandler) == SIG_ERR)
    {
        success = false;
    }
#endif

    return success;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

/**
 * @brief Checks if the application is running and handles lazy initialization.
 * @details On the first call, it attempts to register signal handlers.
 * If a stop signal is received, it returns false.
 * @return true if initialized and not stopped, false otherwise.
 */
bool Running()
{
    bool runningRes = false;    

    static bool init = false;    // Lazy initialization variable
    
    if (!init)
    {
        init = RegisterSignalHandlers();
    }
    
    if (init)
    {
        runningRes = !g_stopRequested.load();
    }
    
    return runningRes;
}
