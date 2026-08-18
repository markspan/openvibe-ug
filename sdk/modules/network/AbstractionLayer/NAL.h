#pragma once

/**
 * @file NAL.h
 * @brief Network Abstraction Layer (NAL) header file.
 * @details This header file provides platform-specific includes and definitions.
 * @author Florent LERAY
 * @date 2025-10-04
 */

#ifndef NAL_H
#define NAL_H

#include <stdio.h> // For snprintf / _snprintf_s

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")


#define LAST_NETWORK_ERROR (WSAGetLastError()) /// @brief Retrieves the last network-related error code for the current thread.
#define NETWORK_ERROR_TIMEOUT (LAST_NETWORK_ERROR == WSAETIMEDOUT || LAST_NETWORK_ERROR == WSAEWOULDBLOCK) /// @brief Checks if the current network error represents a non-fatal "would block" or "timeout" condition.
#define NAL_SNPRINTF(buffer, size, ...) _snprintf_s((buffer), (size), _TRUNCATE, __VA_ARGS__) /// @brief Platform-independent secure string formatting macro.


typedef DWORD TIMEOUT;
typedef int SOCKLEN_T;


/**
 * @brief Creates a timeout value.
 * @param timeoutSec The timeout in seconds.
 * @return TIMEOUT The platform-specific timeout representation.
 */
inline TIMEOUT GetTimeout(int timeoutSec) { TIMEOUT timeout = (TIMEOUT)(timeoutSec * 1000); return timeout; }

/**
 * @brief Converts a network error code into a human-readable string.
 * @param errorCode The error code to convert.
 * @param outBuffer The buffer where the string will be written.
 * @param bufferSize The maximum size of the buffer.
 */
inline void GetNetworkErrorString(int errorCode, char* outBuffer, size_t bufferSize)
{
    if (outBuffer != NULL && bufferSize > 0)
    {
        DWORD length = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            (DWORD)errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            outBuffer,
            (DWORD)bufferSize,
            NULL
        );

        if (length == 0)
        {
            NAL_SNPRINTF(outBuffer, bufferSize, "Unknown error (%d)", errorCode);
        }
        else
        {
            if (length >= 2 && outBuffer[length - 2] == '\r' && outBuffer[length - 1] == '\n')
            {
                outBuffer[length - 2] = '\0';
            }
        }
    }
}


#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> // For TCP_NODELAY
#include <arpa/inet.h>
#include <unistd.h>      // For close
#include <netdb.h>       // For gethostbyname
#include <errno.h>
#include <string.h>      // For strerror


#define INVALID_SOCKET -1          /// @brief Defines an invalid socket value.
#define SOCKET_ERROR -1            /// @brief Defines a socket error value.
#define SD_BOTH SHUT_RDWR          /// @brief Defines the shutdown both (read and write) flag.
#define closesocket(s) close(s)    /// @brief Maps the closesocket function to the POSIX close function.
#define LAST_NETWORK_ERROR (errno) /// @brief Retrieves the last network-related error code for the current thread.
#define NETWORK_ERROR_TIMEOUT (LAST_NETWORK_ERROR == EAGAIN || LAST_NETWORK_ERROR == EWOULDBLOCK || LAST_NETWORK_ERROR == ETIMEDOUT) /// @brief Checks if the current network error represents a non-fatal "would block" or "timeout" condition.
#define NAL_SNPRINTF(buffer, size, ...) snprintf((buffer), (size), __VA_ARGS__) /// @brief Platform-independent secure string formatting macro.


typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in6 SOCKADDR_IN6;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef struct timeval TIMEOUT;
typedef socklen_t SOCKLEN_T;


/**
 * @brief  Creates a timeout value.
 * @param  timeoutSec The timeout in seconds.
 * @return TIMEOUT The platform-specific timeout representation.
 */
inline TIMEOUT GetTimeout(int timeoutSec) { TIMEOUT timeout; timeout.tv_sec = timeoutSec; timeout.tv_usec = 0; return timeout; }

/**
 * @brief Converts a network error code into a human-readable string.
 * @param errorCode The error code to convert.
 * @param outBuffer The buffer where the string will be written.
 * @param bufferSize The maximum size of the buffer.
 */
inline void GetNetworkErrorString(int errorCode, char* outBuffer, size_t bufferSize)
{
    if (outBuffer != NULL && bufferSize > 0)
    {
        const char* errorMsg = strerror(errorCode);
        NAL_SNPRINTF(outBuffer, bufferSize, "%s", errorMsg);
    }
}

#else

#error not defined for this platform

#endif

 /**
  * @brief   Configures standard socket options for performance and reliability.
  * @details Disables Nagle's algorithm (TCP_NODELAY) and sets send/receive timeouts.
  * @param   s The socket descriptor to configure.
  * @param   timeoutSec The timeout duration in seconds for both send and receive.
  * @return  int 0 on success, other on failure.
  */
inline int ConfigureSocket(SOCKET s, int timeoutSec)
{
    int result = 0;
    int flag = 1;

    // 1. Disable Nagle's Algorithm (TCP_NODELAY)
    result = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));

    if (result != SOCKET_ERROR) // 2. Set Timeouts
    {
        TIMEOUT timeout = GetTimeout(timeoutSec);
        result = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        if (result == 0)
        {
            result = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
        }
    }

    return result;
}

#endif /* NAL_H */
