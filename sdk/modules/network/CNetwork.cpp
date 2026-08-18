/**
* @file CNetwork.cpp
* @brief Implementation of the CNetwork class for managing network clients and servers.
* @details This file contains the implementation of the CNetwork class, which provides
* methods to create and manage clients and servers, as well as utility functions for
* address resolution and validation.
* The class is designed to be used in a networked environment where multiple clients
* and servers need to be managed.
* It supports both IPv4 and IPv6 addresses, as well as hostnames.
* The class includes methods for splitting address and port information,
* resolving addresses, and formatting resolved addresses for display.
* @note This class is part of a larger networking library and is intended for internal use.
* @see CClient
* @see CServer
* @see NAL
* @see ClientServerCommon
* @author Florent LERAY
* @date 2025-10-04
*/

#include <CNetwork.h>
#include <NAL.h>
#include <CServer.h>
#include <CClient.h>

#include <list>
#include <string>
#include <regex>
#include <mutex>
#include <algorithm> // Pour std::find

namespace Network
{
    /**
     * @internal
     * @brief Private subclass of CServer for network-specific handling.
     * @details This class allow CNetwork to instanciate a CServer.
     * @note This class is only accessible within the CNetwork implementation.
     * @see CNetwork
     */
    class CServerNet : public CServer
    {
        public:
        CServerNet(CNetwork* network) : CServer(network) {}
    };

    /**
     * @internal
     * @brief Private subclass of CClient for network-specific handling.
     * @details This class allow CNetwork to instanciate a CClient.
     * @note This class is only accessible within the CNetwork implementation.
     * @see CNetwork
     */
    class CClientNet : public CClient
    {
        public:
        CClientNet(CNetwork* network) : CClient(network) {}
    };

    /**
     * @internal
     * @brief Private implementation details for the CNetwork class.
     * @details This class encapsulates the private data and operations of the CNetwork class,
     * including client and server management.
     */
    struct CNetworkPrivate
    {
    public:
        std::recursive_mutex mutexClients;
        std::list<CClient*> clients; // List of clients
        std::recursive_mutex mutexServers;
        std::list<CServer*> servers; // List of servers
    };

    /**
     * @brief Default constructor for the CNetwork class.
     * @details Initializes the networking environment, including Winsock on Windows.
     */
    CNetwork::CNetwork() : d(new CNetworkPrivate)
    {
#if _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    }

    /**
     * @brief Destructor for the CNetwork class.
     * @details Cleans up the networking environment, including Winsock on Windows.
     */
    CNetwork::~CNetwork()
    {
#if _WIN32
        WSACleanup();
#endif
        while (!d->clients.empty())
        {
            removeClient(*d->clients.begin());
        }
        while (!d->servers.empty())
        {
            removeServer(*d->servers.begin());
        }
        delete d;
    }

    /**
     * @brief Creates a new client instance and adds it to the network.
     * @return A pointer to the newly created CClient instance.
     */
    CClient* CNetwork::createClient()
    {
        auto newClient = new CClientNet(this);
        std::lock_guard <std::recursive_mutex> lock(d->mutexClients);
        d->clients.push_back(newClient);
        return newClient;
    }

    /**
     * @brief Creates a new server instance and adds it to the network.
     * @return A pointer to the newly created CServer instance.
     */
    CServer* CNetwork::createServer()
    {
        auto newServer = new CServerNet(this);
        std::lock_guard<std::recursive_mutex> lock(d->mutexServers);
        d->servers.push_back(newServer);
        return newServer;
    }

    /**
     * @brief Removes a client from the network and deletes it.
     * @param client A pointer to the CClient instance to be removed.
     */
    void CNetwork::removeClient(CClient* client)
    {
        std::lock_guard <std::recursive_mutex> lock(d->mutexClients);
        CClientNet* clientNet = dynamic_cast<CClientNet*>(client);
        if (clientNet && std::find(d->clients.begin(), d->clients.end(), client) != d->clients.end())
        {
            d->clients.remove(clientNet);
            if (clientNet->getNetwork() == this)
            {
                clientNet->detachNetwork();
                delete clientNet;
            }
        }
    }

    /**
     * @brief Removes a server from the network and deletes it.
     * @param server A pointer to the CServer instance to be removed.
     */
    void CNetwork::removeServer(CServer* server)
    {
        std::lock_guard<std::recursive_mutex> lock(d->mutexServers);
        CServerNet* serverNet = dynamic_cast<CServerNet*>(server);
        if (serverNet && std::find(d->servers.begin(), d->servers.end(), serverNet) != d->servers.end())
        {
            d->servers.remove(serverNet);
            if (serverNet->getNetwork() == this)
            {
                serverNet->detachNetwork();
                delete serverNet;
            }
        }
    }








    //----------------------------------------------------------------
    // Address and port splitter API with validation
    //----------------------------------------------------------------

    /**
     * @brief Splits an address string into its address/hostname part and its port number.
     *
     * This function attempts to find and extract the port number from the input
     * string. The port is expected to be after the last ':' character, unless
     * the address is an IPv6 in square brackets '[]', in which case the port
     * is expected after the closing bracket and the final colon.
     *
     * @param addressWithPort The string containing the address (IPv4, IPv6, or hostname)
     * and an optional port number.
     * @param outAddress The extracted address or hostname without the port.
     * @param outPort The extracted port number (uint16_t). Set to 0 if no port is found.
     * @return bool True if the input string was successfully processed (even if no port was found),
     * false if an invalid port format or conversion error occurred.
     */
    bool CNetwork::splitAddressAndPort(const std::string& addressWithPort, std::string& outAddress, uint16_t& outPort)
    {
        bool bRes = true;

        outPort = 0;
        outAddress = "";
        std::string portStr;

        // NAPS | EMPTY || XOR | bRes | 
        // ________________________________
        // 0    | 0     || 0   | 1    | N'arrive jamais
        // 0    | 1     || 1   | 0    | Bad
        // 1    | 0     || 1   | 0    | 
        // 1    | 1     || 0   | 1    | 


        bRes = nameAndPortSplitter(addressWithPort, outAddress, portStr) ^ portStr.empty();
        bRes = bRes && portExtract(portStr, outPort) && (isIpv4Address(outAddress) || isIpv6Address(outAddress) || isHostname(outAddress));

        return bRes;
    }

    /**
     * @brief Resolves a hostname or IP address string into a list of socket addresses.
     *
     * This function uses getaddrinfo to perform the lookup, supporting both IPv4 and IPv6.
     * It is the recommended way to prepare for creating a socket connection.
     *
     * @param address The hostname (e.g., "localhost") or IP address string (e.g., "127.0.0.1").
     * @param port The port number.
     * @return A unique_ptr to the head of the addrinfo linked list.
     * The pointer will be empty (nullptr) on failure.
     * The memory is automatically managed by the unique_ptr's custom deleter.
     */
    CNetwork::AddrInfoSmartPtr CNetwork::resolveAddress(const std::string& address, uint16_t port)
    {
        addrinfo hints = { 0 };
        addrinfo* result = nullptr;

        hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // We want a TCP socket

        std::string service = std::to_string(port);

        if (getaddrinfo(address.c_str(), service.c_str(), &hints, &result) != 0)
        {
            return CNetwork::AddrInfoSmartPtr(nullptr, &freeaddrinfo);
        }

        return CNetwork::AddrInfoSmartPtr(result, &freeaddrinfo);
    }


    /**
    * @brief Formats the raw addrinfo list into a human-readable vector.
    * @param resolvedAddresses The smart pointer to the addrinfo list from resolveAddress.
    * @return A vector of AddressDetails structures for display or logging.
    */
    std::vector<CNetwork::AddressDetails> CNetwork::formatResolvedAddresses(const AddrInfoSmartPtr& resolvedAddresses)
    {
        std::vector<AddressDetails> formattedList;
        uint16_t port = 0;

        for (addrinfo* p = resolvedAddresses.get(); p != nullptr; p = p->ai_next)
        {
            AddressDetails entry;
            entry.family = p->ai_family;
            entry.hostname = p->ai_canonname ? p->ai_canonname : "";

            void* binaryAddress = nullptr;
            char ipStr[INET6_ADDRSTRLEN] = { 0 };

            if (p->ai_family == AF_INET)
            {
                auto* sockaddr_ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
                binaryAddress = &sockaddr_ipv4->sin_addr;
                port = ntohs(sockaddr_ipv4->sin_port);
            }
            else // AF_INET6
            {
                auto* sockaddr_ipv6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
                binaryAddress = &sockaddr_ipv6->sin6_addr;
                port = ntohs(sockaddr_ipv6->sin6_port);
            }

            inet_ntop(p->ai_family, binaryAddress, ipStr, sizeof(ipStr));

            entry.ip = ipStr;
            entry.port = port;

            formattedList.push_back(entry);
        }
        return formattedList;
    }

    /**
     * @brief  Resolves an address and port, then formats the results into address structures.
     * @param  address The address to resolve (e.g., a hostname or IP address).
     * @param  port The port associated with the address.
     * @param  resolvedAddresses Reference to a pointer where the resolved addresses will be stored.
     * @param  formattedAddresses Reference to a vector where the formatted addresses will be stored.
     * @return true if the resolution and formatting were successful, false otherwise.
     */
    bool CNetwork::resolvedAndFormat(const std::string& address, uint16_t port, AddrInfoSmartPtr& resolvedAddresses, std::vector<AddressDetails>& formattedAddresses)
    {
        bool bRes = false;

        resolvedAddresses = resolveAddress(address, port);

        if (resolvedAddresses)
        {
            formattedAddresses = formatResolvedAddresses(resolvedAddresses);
            bRes = !formattedAddresses.empty();
        }

        return bRes;
    }

    /**
     * @brief  Resolves an address and port, then formats the results into address structures.
     * @param  address The address to resolve (e.g., a hostname or IP address) with or without port.
     * @param  resolvedAddresses Reference to a pointer where the resolved addresses will be stored.
     * @param  formattedAddresses Reference to a vector where the formatted addresses will be stored.
     * @return true if the resolution and formatting were successful, false otherwise.
     */
    bool CNetwork::resolvedAndFormat(const std::string& address, AddrInfoSmartPtr& resolvedAddresses, std::vector<AddressDetails>& formattedAddresses)
    {
        bool bRes = false;

        uint16_t port = 0;
        std::string addr;

        bRes = splitAddressAndPort(address, addr, port);
        if (bRes)
        {
            bRes = resolvedAndFormat(addr, port, resolvedAddresses, formattedAddresses);
        }

        return bRes;
    }



    /*******************************************************************************************************************
    ********************************************************************************************************************
    **  PRIVATE CODE                                                                                                  **
    ********************************************************************************************************************
    *******************************************************************************************************************/


    //----------------------------------------------------------------
    // Address and port validators
    //----------------------------------------------------------------

    /**
     * @brief Checks if a string is a valid IPv4 address.
     *
     * This function uses a comprehensive regular expression to validate the IPv4 format,
     * ensuring four decimal segments (octets) separated by dots, where each octet is
     * in the range 0-255.
     *
     * @param address The string to validate.
     * @return bool True if the string is a valid IPv4 address, false otherwise.
     */
    bool CNetwork::isIpv4Address(const std::string& address)
    {
        const char* octet_pattern = "([0-9]|""[1-9][0-9]|""1[0-9]{2}|""2[0-4][0-9]|""25[0-5])";
        const std::regex ipv4Pattern("^" + std::string(octet_pattern) + "\\." + std::string(octet_pattern) + "\\." + std::string(octet_pattern) + "\\." + std::string(octet_pattern) + "$");
        return std::regex_match(address, ipv4Pattern);
    }

    /**
     * @brief Checks if a string is a valid IPv6 address.
     *
     * This function uses a comprehensive regular expression to validate the IPv6 format,
     * covering various representations including full, compressed, and mixed notation.
     *
     * @param address The string to validate.
     * @return bool True if the string is a valid IPv6 address, false otherwise.
     */
    bool CNetwork::isIpv6Address(const std::string& address)
    {
        const std::string h_grp = "[0-9a-fA-F]{1,4}";
        const std::regex ipv6Pattern("^""(\\[)?""(""(((" + h_grp + ":){7}" + h_grp + "))""|""(((" + h_grp + ":){1,7}:))""|""(((" + h_grp + ":){0,6}" + h_grp + ")?::((" + h_grp + ":){0,6}" + h_grp + ")?)"")""(\\])?""$");

        return std::regex_match(address, ipv6Pattern);
    }

    /**
     * @brief Checks if a string is a valid DNS hostname.
     *
     * This function uses a regular expression to validate the string according to
     * general DNS hostname rules (RFC 952/1123). It verifies that:
     * 1. The name consists of labels separated by dots.
     * 2. Each label starts and ends with an alphanumeric character.
     * 3. Each label contains only alphanumeric characters or hyphens.
     * 4. Hyphens are not allowed at the beginning or end of a label.
     *
     * Note: The regex does not explicitly check the 253-character total length limit
     * or the 63-character label limit, which should be checked separately in code
     * if strict compliance is required, but it covers the character set and structure.
     *
     * @param address The string to validate.
     * @return bool True if the string matches the valid hostname structure, false otherwise.
     */
    bool CNetwork::isHostname(const std::string& address)
    {
        bool bRes = false;

        if (!address.empty() && address.length() < 254)
        {
            const char* label_pattern = "[a-zA-Z0-9]([a-zA-Z0-9\\-]*[a-zA-Z0-9])?";
            const std::regex hostnamePattern("^" + std::string(label_pattern) + "(\\." + std::string(label_pattern) + ")*" + "$");
            bRes = std::regex_match(address, hostnamePattern);
        }

        return bRes;
    }

    /**
     * @brief Extracts and validates a port number from a string.
     *
     * This function checks if the input string represents a valid port, then number or an empty string.
     * If it's a number, it must be  a positive integer between 1 and 65535.
     * It uses a regular expression to ensure the string contains only digits.
     *
     * @param portStr The string containing the port number to validate and extract.
     * @param outPort A reference to a uint16_t variable where the extracted port number will be stored.
     * @return bool True if the port number is valid and successfully extracted.
     * True is also returned if the input string is empty, leaving outPort unchanged. Because port is always optional.
     * False is returned if the string is non-empty but invalid.
     */
    bool CNetwork::portExtract(std::string& portStr, uint16_t& outPort)
    {
        bool bRes = true;

        if (!portStr.empty())
        {
            const std::regex numberPattern(R"(^[0-9]{1,5}$)");
            if (std::regex_match(portStr, numberPattern))
            {
                unsigned long portInt = std::stoul(portStr);
                if (portInt > 0 && portInt <= 65535)
                {
                    outPort = static_cast<uint16_t>(portInt);
                }
                else
                {
                    bRes = false;
                }
            }
        }

        return bRes;
    }


    //----------------------------------------------------------------
    // Address and port splitters
    // These functions split an address string into its address/hostname part and its port string.
    //----------------------------------------------------------------

    /**
     * @brief Splits an IPv4 address or hostname with an optional port.
     *
     * This function looks for the last ':' character to separate the address
     * from the port. It does not validate the address format; it simply splits
     * the string.
     *
     * @param addressWithPort The input string containing the address and optional port.
     * @param outAddress The extracted address or hostname without the port.
     * @param portStr The extracted port as a string. Empty if no port is found.
     * @return bool True if a port was found and extracted, false otherwise.
     */
    bool CNetwork::IPv4OrHostnameSplitter(const std::string& addressWithPort, std::string& outAddress, std::string& portStr)
    {
        bool bRes = false;

        auto sep = addressWithPort.rfind(':');
        outAddress = addressWithPort.substr(0, sep); // Default to the full string if no port is found
        if (sep != std::string::npos)
        {
            portStr = addressWithPort.substr(sep + 1);
            bRes = true;
        }

        return bRes;
    }

    /**
     * @brief Splits an IPv6 address in square brackets with an optional port.
     *
     * This function looks for the closing ']' character and expects the port
     * to follow immediately after a ':' character. It extracts the address
     * without the brackets and the port if present.
     *
     * @param addressWithPort The input string containing the IPv6 address in brackets and optional port.
     * @param outAddress The extracted IPv6 address without brackets.
     * @param portStr The extracted port as a string. Empty if no port is found.
     * @return bool True if a port was found and extracted, false otherwise.
     */
    bool CNetwork::IPv6Splitter(const std::string& addressWithPort, std::string& outAddress, std::string& portStr)
    {
        bool bRes = false;

        size_t startOfPort = std::string::npos;
        size_t endBracket = addressWithPort.find(']');
        if (endBracket != std::string::npos && endBracket < addressWithPort.length() - 1)
        {
            // The colon for the port should be immediately after the ']'
            if (addressWithPort[endBracket + 1] == ':')
            {
                startOfPort = endBracket + 2;

                outAddress = addressWithPort.substr(1, endBracket);
                portStr = addressWithPort.substr(startOfPort);
                bRes = true;
            }
        }

        return bRes;
    }


    //----------------------------------------------------------------
    // General address and port splitter
    //----------------------------------------------------------------

    /**
     * @brief Splits an address string into its address/hostname part and its port string.
     *
     * This function determines if the address is an IPv6 in square brackets '[]'
     * or an IPv4/hostname, and calls the appropriate splitter function.
     *
     * @param addressWithPort The string containing the address (IPv4, IPv6, or hostname)
     * and an optional port number.
     * @param outAddress The extracted address or hostname without the port.
     * @param portStr The extracted port as a string. Empty if no port is found.
     * @return bool True if a port was found and extracted, false otherwise.
     */
    bool CNetwork::nameAndPortSplitter(const std::string& addressWithPort, std::string& outAddress, std::string& portStr)
    {
        bool bRes = false;

        if (addressWithPort.front() == '[' && addressWithPort.find(']') != std::string::npos)
        {
            bRes = IPv6Splitter(addressWithPort, outAddress, portStr);
        }
        else
        {
            bRes = IPv4OrHostnameSplitter(addressWithPort, outAddress, portStr);
        }

        return bRes;
    }
} // namespace Network
