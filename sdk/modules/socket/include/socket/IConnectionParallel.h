#pragma once

#include "IConnection.h"
#include <string>

#if defined TARGET_OS_Windows
#include <windows.h>
#endif

namespace Socket {
/**
 * \brief The IConnectionParallel class provides the possibility to communicate with a parallel port.
 * On Windows, you must have TVicPort library installed (available for free: http://entechtaiwan.com/dev/port/index.shtm).
 */
class Socket_API IConnectionParallel : public IConnection
{
public:

    virtual bool connect(const unsigned short port) = 0;

    virtual std::string getLastError() = 0;

protected:

#if defined TARGET_OS_Windows
    typedef bool (CALLBACK * port_open_t)();
    typedef void (CALLBACK * port_close_t)();
    typedef bool (CALLBACK * port_is_driver_opened_t)();
    typedef bool (CALLBACK * port_write_t)(unsigned short, unsigned char);

    HMODULE m_port = nullptr;

    port_open_t m_portOpen                   = nullptr;
    port_close_t m_portClose                 = nullptr;
    port_is_driver_opened_t m_isDriverOpened = nullptr;
    port_write_t m_portWrite                 = nullptr;
#endif
};

extern Socket_API IConnectionParallel* createConnectionParallel();
}  // namespace Socket
