#pragma once

#include "IConnection.h"

#include <array>
#include <cstdio>

namespace Socket {
class Socket_API IConnectionBluetooth : public IConnection
{
public:

    /**
     * \brief Connect to the bluetooth device.
     * \param[in] address the MAC address of the Bluetooth device.
     * \return If the function succeeds, the return value is true, else false.
     */
    virtual bool connect(const uint64_t address) = 0;

    /**
     * \brief Return the input serial pending byte count.
     * \return The number of pending byte.
     */
    virtual size_t getPendingByteCount() = 0;

    /**
     * \brief Flush the input serial buffer.
     * \return If the function succeeds, the return value is true, else false.
     */
    virtual const char* getLastError() const = 0;

    /**
     * \brief Clear the last error registered.
     */
    virtual void clearError() = 0;

    /** 
     * \brief List the paired bluetooth devices: Name and Bluetooth MAC address.
     *
     * \param[out] nPairedDevices the Bluetooth devices count. 
     * \param[out] names an array of Bluetooth names. 
     * \param[out] addresses an array of Bluetooth addresses. 
     *
     * \return  If the function succeeds, the return value is true, else false.
     */
    virtual bool listPairedBluetoothDevices(size_t* nPairedDevices, char** names, uint64_t** addresses) = 0;

    /**
     * \brief Convert string MAC Bluetooth address to hexadecimal.
     * \param[out] straddr string MAC Bluetooth address.
     * \param[out] btaddr hexadecimal MAC Bluetooth address.
     * \retval true in case of success.
     * \retval false in case of failure: if the address does not match: %02x:%02x:%02x:%02x:%02x:%02x
     */
    static bool string2BluetoothAddress(const char* straddr, uint64_t* btaddr)
    {
        std::array<uint32_t, 6> aaddr = { 0, 0, 0, 0, 0, 0 };

        const int value = sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x", &aaddr[0], &aaddr[1], &aaddr[2], &aaddr[3], &aaddr[4], &aaddr[5]);

        if (value != 6) { return false; }

        *btaddr = 0;

        for (size_t i = 0; i < 6; ++i)
        {
            const uint64_t tmpaddr = static_cast<uint64_t>(aaddr[i] & 0xff);
            *btaddr                = (*btaddr << 8) + tmpaddr;
        }

        return true;
    }


protected:

#if defined TARGET_OS_Windows

    static const unsigned char WIN_SOCKET_MAJOR_VERSION = 2; // Winsock major version to use
    static const unsigned char WIN_SOCKET_MINOR_VERSION = 2; // Winsock minor version to use

    static bool m_isWinsockInitialized;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

#endif
};

extern Socket_API IConnectionBluetooth* createConnectionBluetooth();
}  // namespace Socket
