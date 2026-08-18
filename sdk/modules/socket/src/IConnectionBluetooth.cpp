#include "IConnectionBluetooth.h"

#if defined TARGET_OS_Windows
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <CommCtrl.h>
#include <codecvt>
#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/select.h>
    #include <sys/stat.h>
    #include <sys/ioctl.h>

    #include <unistd.h>
    #include <fcntl.h>
    #include <termios.h>
#else
    #error "Unsupported platform"
#endif

#include <assert.h>
#include <vector>
#include <string>

namespace Socket {
class CConnectionBluetooth final : public IConnectionBluetooth
{
public:

#if defined TARGET_OS_Windows
    CConnectionBluetooth() : m_Socket(INVALID_SOCKET) { }
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
        CConnectionBluetooth() : m_LastError() { }
#endif

#if defined TARGET_OS_Windows
    bool initialize()
    {
        WSADATA wsaData;

        // Ask for Winsock version.
        if (_WINSOCK2API_::WSAStartup(MAKEWORD(WIN_SOCKET_MAJOR_VERSION, WIN_SOCKET_MINOR_VERSION), &wsaData) != 0)
        {
            m_LastError = "Failed to start Winsock " + std::to_string(WIN_SOCKET_MAJOR_VERSION) + "." + std::to_string(WIN_SOCKET_MINOR_VERSION) + ": " + this->
                          getLastErrorFormated();
            return false;
        }

        // Confirm that the WinSock DLL supports version requested.
        // Note that if the DLL supports versions greater than the version requested, in addition to the version requested, it will still return the version requested in wVersion.
        if (LOBYTE(wsaData.wVersion) != WIN_SOCKET_MAJOR_VERSION || HIBYTE(wsaData.wVersion) != WIN_SOCKET_MINOR_VERSION)
        {
            m_LastError = "Could not find a usable version of Winsock.dll.";
            _WINSOCK2API_::WSACleanup();
            return false;
        }
        return true;
    }
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
        bool initialize() { return false; }
#endif

    bool open() override { return false; }

    bool close() override
    {
        if (!this->isConnected())
        {
            m_LastError = "Bluetooth device is not connected.";
            return false;
        }

#if defined TARGET_OS_Windows

        bool isSuccess = true;
        if (m_Socket != INVALID_SOCKET)
        {
            // shutdown the connection since no more data will be sent or received
            if (_WINSOCK2API_::shutdown(m_Socket, SD_BOTH) == SOCKET_ERROR)
            {
                m_LastError = "Failed to shutdown the bluetooth socket:" + this->getLastErrorFormated();
                isSuccess   = false;
            }

            if (_WINSOCK2API_::closesocket(m_Socket) == SOCKET_ERROR)
            {
                m_LastError = "Failed to close the bluetooth socket:" + this->getLastErrorFormated();
                isSuccess   = false;
            }

            if (_WINSOCK2API_::WSACleanup() == SOCKET_ERROR)
            {
                m_LastError = "Failed to cleanup the bluetooth socket:" + this->getLastErrorFormated();
                isSuccess   = false;
            }

            m_Socket = INVALID_SOCKET;
        }

        return isSuccess;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return false;
#endif
    }

    bool isReadyToSend(const size_t /*timeOut*/) const override { return this->isConnected(); }

    bool isReadyToReceive(const size_t /*timeOut*/) const override
    {
        if (!this->isConnected()) { return false; }

#if defined TARGET_OS_Windows
        unsigned long nPendingBytes = 0;

        if (_WINSOCK2API_::ioctlsocket(m_Socket, FIONREAD, &nPendingBytes) == SOCKET_ERROR)
        {
            //m_LastError = "Failed to get the pending bytes count: " + this->getLastErrorFormated();
            return false;
        }

        return nPendingBytes != 0;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return false;
#endif
    }

    size_t getPendingByteCount() override
    {
        if (!this->isConnected())
        {
            m_LastError = "Bluetooth device not connected.";
            return 0;
        }

#if defined TARGET_OS_Windows

        unsigned long nPendingBytes = 0;

        if (_WINSOCK2API_::ioctlsocket(m_Socket, FIONREAD, &nPendingBytes) == SOCKET_ERROR)
        {
            m_LastError = "Failed to get the pending bytes count: " + this->getLastErrorFormated();
            return 0;
        }

        return nPendingBytes;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return 0;
#endif
    }

    size_t sendBuffer(const void* buffer, const size_t size) override
    {
        if (!this->isConnected())
        {
            m_LastError = "Bluetooth device is not connected.";
            return 0;
        }

#if defined TARGET_OS_Windows

        const int nBytesSent = _WINSOCK2API_::send(m_Socket, reinterpret_cast<const char*>(buffer), int(size), 0);

        if (nBytesSent == SOCKET_ERROR)
        {
            m_LastError = "Failed to write on the bluetooth port: " + getLastErrorFormated();
            this->close();
            return 0;
        }

        return size_t(nBytesSent);

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return 0;
#endif
    }

    size_t receiveBuffer(void* buffer, const size_t size) override
    {
        if (!this->isConnected())
        {
            m_LastError = "Bluetooth device is not connected.";
            return 0;
        }

#if defined TARGET_OS_Windows


        const int nBytesReceived = _WINSOCK2API_::recv(m_Socket, static_cast<char*>(buffer), int(size), 0);

        if (nBytesReceived == SOCKET_ERROR)
        {
            m_LastError = "Failed to receive data from bluetooth: " + getLastErrorFormated();

            this->close();
            return 0;
        }

        return size_t(nBytesReceived);

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return 0;
#endif
    }

    bool sendBufferBlocking(const void* buffer, const size_t size) override
    {
        if (!this->isConnected())
        {
            m_LastError = "Bluetooth device is not connected.";
            return false;
        }

        const char* p    = reinterpret_cast<const char*>(buffer);
        size_t bytesLeft = size;

        while (bytesLeft != 0 && this->isConnected())
        {
            bytesLeft -= this->sendBuffer(p + size - bytesLeft, bytesLeft);

            if (this->isErrorRaised()) { return false; }
        }

        return bytesLeft == 0;
    }

    bool receiveBufferBlocking(void* buffer, const size_t size) override
    {
        if (!this->isConnected())
        {
            m_LastError = "Bluetooth device is not connected.";
            return false;
        }

        char* p          = reinterpret_cast<char*>(buffer);
        size_t bytesLeft = size;

        while (bytesLeft != 0 && this->isConnected())
        {
            bytesLeft -= this->receiveBuffer(p + size - bytesLeft, bytesLeft);

            if (this->isErrorRaised()) { return false; }
        }

        return bytesLeft == 0;
    }

    bool isConnected() const override
    {
#if defined TARGET_OS_Windows

        return m_Socket != INVALID_SOCKET;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return false;
#endif
    }

    void release() override { delete this; }

    bool connect(const uint64_t u64BluetoothAddress) override
    {
        m_LastError.clear();

        if (this->isConnected())
        {
            m_LastError = "Bluetooth device is already connected";
            return false;
        }

#if defined TARGET_OS_Windows

        if (!this->initialize()) { return false; }

        m_Socket = _WINSOCK2API_::socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

        if (m_Socket == INVALID_SOCKET)
        {
            m_LastError = "Failed to create bluetooth socket: " + getLastErrorFormated();
            _WINSOCK2API_::WSACleanup();
            return false;
        }

        SOCKADDR_BTH sockAddressBlutoothServer;
        sockAddressBlutoothServer.btAddr         = u64BluetoothAddress;
        sockAddressBlutoothServer.addressFamily  = AF_BTH;
        sockAddressBlutoothServer.serviceClassId = RFCOMM_PROTOCOL_UUID;
        sockAddressBlutoothServer.port           = BT_PORT_ANY;

        if (_WINSOCK2API_::connect(m_Socket, reinterpret_cast<SOCKADDR*>(&sockAddressBlutoothServer), sizeof(SOCKADDR_BTH)) == SOCKET_ERROR)
        {
            m_LastError = "Failed to connect the socket to the bluetooth address [" + std::to_string(sockAddressBlutoothServer.btAddr) + "]: " +
                          getLastErrorFormated();

            _WINSOCK2API_::closesocket(m_Socket); // Returned code not checked.
            _WINSOCK2API_::WSACleanup(); // Returned code not checked.

            m_Socket = INVALID_SOCKET;
            return false;
        }

        return true;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return false;
#endif
    }

    bool isErrorRaised() const { return !m_LastError.empty(); }

    const char* getLastError() const override { return m_LastError.c_str(); }

    static std::string getLastErrorFormated()
    {
#if defined TARGET_OS_Windows

        LPTSTR text;
        const DWORD errCode = GetLastError();

        const size_t size = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | // use system message tables to retrieve error text
                                          FORMAT_MESSAGE_ALLOCATE_BUFFER |  // allocate buffer on local heap for error text
                                          FORMAT_MESSAGE_IGNORE_INSERTS, // Important! will fail otherwise, since we're not (and CANNOT) pass insertion parameters
                                          nullptr, // unused with FORMAT_MESSAGE_FROM_SYSTEM
                                          errCode, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                                          LPTSTR(&text), // output
                                          0, // minimum size for output buffer
                                          nullptr);

        // Converts std::wstring to std::string and returns it. 
        const std::wstring message(text, size);
        LocalFree(text);
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(message);

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return "";
#endif
    }

    void clearError() override { m_LastError.clear(); }

    bool listPairedBluetoothDevices(size_t* nPairedBluetoothDevices, char** names, uint64_t** addresses) override
    {
        std::vector<std::string> devicesNames;
        std::vector<uint64_t> devicesAddresses;

#if defined TARGET_OS_Windows

        HANDLE handle;
        WSAQUERYSET wsaQuerySet;

        memset((void*)&wsaQuerySet, 0, sizeof(wsaQuerySet));
        wsaQuerySet.dwSize      = sizeof(wsaQuerySet);
        wsaQuerySet.dwNameSpace = NS_BTH;
        wsaQuerySet.lpcsaBuffer = nullptr;

        if (_WINSOCK2API_::WSALookupServiceBegin(&wsaQuerySet, LUP_CONTAINERS | LUP_RETURN_NAME | LUP_RETURN_ADDR, &handle) == SOCKET_ERROR)
        {
            m_LastError = "Failed to start the Bluetooth lookup service: " + getLastErrorFormated();
            return false;
        }

        char buffer[5000];
        const LPWSAQUERYSET wsaQuerySetW = LPWSAQUERYSET(buffer);
        DWORD size                       = sizeof(buffer);

        memset((void*)wsaQuerySetW, 0, sizeof(WSAQUERYSET));
        wsaQuerySetW->dwSize      = sizeof(WSAQUERYSET);
        wsaQuerySetW->dwNameSpace = NS_BTH;
        wsaQuerySetW->lpBlob      = nullptr;

        bool lookup = true;

        while (lookup)
        {
            // Check next bluetooth device
            const int res = _WINSOCK2API_::WSALookupServiceNext(handle, LUP_RETURN_NAME | LUP_RETURN_ADDR, &size, wsaQuerySetW);

            if (res == SOCKET_ERROR)
            {
                // If it is a "real" error, we trace it and return false.
                if (_WINSOCK2API_::WSAGetLastError() != WSA_E_NO_MORE)
                {
                    m_LastError = "Lookup service next operation failed: " + getLastErrorFormated();
                    return false;
                }

                // Else, it is because there is no more Bluetooth devices available.
                lookup = false;
                break;
            }
            // Get bluetooth MAC address and name
            devicesAddresses.push_back(reinterpret_cast<SOCKADDR_BTH*>(wsaQuerySetW->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr);

            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
            devicesNames.push_back(converterX.to_bytes(wsaQuerySetW->lpszServiceInstanceName));
        }

        if (_WINSOCK2API_::WSALookupServiceEnd(handle) == SOCKET_ERROR)
        {
            m_LastError = "Failed to stop the Bluetooth lookup service: " + getLastErrorFormated();
            return false;
        }

        *nPairedBluetoothDevices = devicesAddresses.size();
        names                    = new char*[*nPairedBluetoothDevices];

        for (size_t i = 0; i < *nPairedBluetoothDevices; ++i)
        {
            names[i] = new char[devicesNames[i].size() + 1];
            std::strcpy(names[i], devicesNames[i].c_str());
        }

        addresses = new uint64_t*[*nPairedBluetoothDevices];

        return true;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return false;
#endif
    }

    std::string m_LastError;

#if defined TARGET_OS_Windows

    SOCKET m_Socket;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

#endif
};

IConnectionBluetooth* createConnectionBluetooth() { return new CConnectionBluetooth(); }
}  // namespace Socket
