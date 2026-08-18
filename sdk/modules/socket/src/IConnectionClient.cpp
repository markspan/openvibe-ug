#include "IConnection.h"
#include "IConnectionClient.h"
#include "IConnection.inl"

#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <cerrno>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <netdb.h>
 #include <unistd.h>
#elif defined TARGET_OS_Windows
#include <cerrno>

#include <WS2tcpip.h>
#else
#endif

namespace Socket {
class CConnectionClient final : public TConnection<IConnectionClient>
{
public:

    bool connect(const char* sServerName, const size_t serverPort, const size_t timeOut) override
    {
        if (!open()) { return false; }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            long value;
            // Sets non blocking
            if((value = ::fcntl(m_socket, F_GETFL, nullptr)) < 0)
            {
                close();
                return false;
            }
            value|=O_NONBLOCK;
            if(::fcntl(m_socket, F_SETFL, value)<0)
            {
                close();
                return false;
            }

            // Looks up host name
            struct hostent* serverHostEntry = gethostbyname(sServerName);
            if(!serverHostEntry)
            {
                close();
                return false;
            }

#elif defined TARGET_OS_Windows

        // Sets non blocking
        unsigned long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);

        struct addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        PADDRINFOA addr;
        if (getaddrinfo(sServerName, nullptr, &hints, &addr) != 0)
        {
            close();
            return false;
        }
        const auto sockaddrIPV4 = reinterpret_cast<sockaddr_in*>(addr->ai_addr);

#endif

        // Connects
        struct sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port   = htons(static_cast<unsigned short>(serverPort));
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            serverAddress.sin_addr=*((struct in_addr*)serverHostEntry->h_addr);
#elif defined TARGET_OS_Windows
        serverAddress.sin_addr = sockaddrIPV4->sin_addr;
        freeaddrinfo(addr);
#endif
        errno = 0;
        if (::connect(m_socket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(struct sockaddr_in)) < 0)
        {
            bool inProgress;

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
                inProgress = (errno==EINPROGRESS);
#elif defined TARGET_OS_Windows
            inProgress = (WSAGetLastError() == WSAEINPROGRESS || WSAGetLastError() == WSAEWOULDBLOCK);
#else
                inProgress = false;
#endif

            if (inProgress)
            {
                const int t = (timeOut == 0xffffffff) ? 125 : int(timeOut);
                // Performs time out

                struct timeval timeVal;
                timeVal.tv_sec  = (t / 1000);
                timeVal.tv_usec = ((t - timeVal.tv_sec * 1000) * 1000);

                fd_set writeFileDesc;
                FD_ZERO(&writeFileDesc);
                FD_SET(m_socket, &writeFileDesc);

                if (select(int(m_socket + 1), nullptr, &writeFileDesc, nullptr, &timeVal) < 0)
                {
                    close();
                    return false;
                }
                if (!FD_ISSET_PROXY(int(m_socket), &writeFileDesc))
                {
                    close();
                    return false;
                }

                // Checks error status
                int option = 0;
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
                    socklen_t length = sizeof(option);
                    ::getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (void*)(&option), &length);
#elif defined TARGET_OS_Windows
                int length = sizeof(option);
                getsockopt(m_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&option), &length);
#endif
                if (option != 0)
                {
                    close();
                    return false;
                }
            }
            else
            {
                close();
                return false;
            }
        }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS

            // Sets back to blocking
            if((value=::fcntl(m_socket, F_GETFL, nullptr))<0)
            {
                close();
                return false;
            }
            value&=~O_NONBLOCK;
            if(::fcntl(m_socket, F_SETFL, value)<0)
            {
                close();
                return false;
            }

#elif defined TARGET_OS_Windows

        // Sets back to blocking
        mode = 0;
        ioctlsocket(m_socket, FIONBIO, &mode);

#endif

        return true;
    }
};

IConnectionClient* createConnectionClient() { return new CConnectionClient(); }
}  // namespace Socket
