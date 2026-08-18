#include "IConnection.h"
#include "IConnectionServer.h"
#include "IConnection.inl"

#include <cstring>
#include <iostream>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <netdb.h>
 #include <cerrno>
#elif defined TARGET_OS_Windows
#else
#endif

namespace Socket {
class CConnectionServer final : public TConnection<IConnectionServer>
{
public:

    bool listen(const size_t port) override
    {
        if (!open()) { return false; }

        int reuseAddress = 1;
#if defined TARGET_OS_Windows
        setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddress), sizeof(reuseAddress));
#else
            ::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));
#endif

        struct sockaddr_in localHostAddress;
        memset(&localHostAddress, 0, sizeof(localHostAddress));
        localHostAddress.sin_family      = AF_INET;
        localHostAddress.sin_port        = htons(static_cast<unsigned short>(port));
        localHostAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(m_socket, reinterpret_cast<struct sockaddr*>(&localHostAddress), sizeof(localHostAddress)) == -1)
        {
            /*
            switch(errno)
            {
                case EBADF: std::cout << "EBADF" << std::endl; break;
                case ENOTSOCK: std::cout << "ENOTSOCK" << std::endl; break;
                case EADDRINUSE: std::cout << "EADDRINUSE" << std::endl; break;
                case EINVAL: std::cout << "EINVAL" << std::endl; break;
                case EROFS: std::cout << "EROFS" << std::endl; break;
                case EFAULT: std::cout << "EFAULT" << std::endl; break;
                case ENAMETOOLONG: std::cout << "ENAMETOOLONG" << std::endl; break;
                case ENOENT: std::cout << "ENOENT" << std::endl; break;
                case ENOMEM: std::cout << "ENOMEM" << std::endl; break;
                case ENOTDIR: std::cout << "ENOTDIR" << std::endl; break;
                case EACCES: std::cout << "EACCES" << std::endl; break;
                case ELOOP: std::cout << "ELOOP" << std::endl; break;
                default: std::cout << "Bind_unknown" << std::endl;
            }
            */
            return false;
        }

        if (::listen(m_socket, 32) == -1)
        {
            /*
            switch(errno)
            {
                case EADDRINUSE: std::cout << "EADDRINUSE" << std::endl; break;
                case EBADF: std::cout << "EBADF" << std::endl; break;
                case ENOTSOCK: std::cout << "ENOTSOCK" << std::endl; break;
                case EOPNOTSUPP: std::cout << "EOPNOTSUPP" << std::endl; break;
                default: std::cout << "Listen_unknown" << std::endl;
            }
            */
            return false;
        }

        return true;
    }

    IConnection* accept() override
    {
        struct sockaddr_in clientAddress;
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            socklen_t clientAddressSize = sizeof(clientAddress);
#elif defined TARGET_OS_Windows
        int clientAddressSize = sizeof(clientAddress);
#else
#endif
        const int clientSocket = int(::accept(m_socket, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddressSize));
        if (clientSocket == -1) { return nullptr; }
        return new TConnection<IConnection>(int(clientSocket));
    }

    bool getSocketPort(size_t& port) override
    {
        struct sockaddr_in socketInfo;

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            socklen_t socketInfoLength = sizeof(socketInfo);
#elif defined TARGET_OS_Windows
        int socketInfoLength = sizeof(socketInfo);
#endif

        if (getsockname(m_socket, reinterpret_cast<sockaddr*>(&socketInfo), &socketInfoLength) == -1) { return false; }

        port = size_t(ntohs(socketInfo.sin_port));
        return true;
    }
};

IConnectionServer* createConnectionServer() { return new CConnectionServer(); }
}  // namespace Socket
