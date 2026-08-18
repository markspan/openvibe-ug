#include "IConnectionParallel.h"

#if defined TARGET_OS_Windows
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#else
#error "Unsupported platform"
#endif

#if defined TARGET_OS_Linux && !defined TARGET_OS_Android
#include <linux/ppdev.h>
#include <linux/parport.h>
#endif

#include <assert.h>

namespace Socket {
class CConnectionParallel final : public IConnectionParallel
{
protected:
    unsigned short m_portNumber;
    std::string m_lastError;
public:

    CConnectionParallel()

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            :m_file(0)
#endif
    {
#if defined TARGET_OS_Windows

        m_portNumber = 0;
        m_port       = LoadLibrary(TEXT("TVicPort.dll"));

        if (m_port != nullptr)
        {
            m_isDriverOpened = port_is_driver_opened_t(GetProcAddress(m_port, "IsDriverOpened"));
            m_portOpen       = port_open_t(GetProcAddress(m_port, "OpenTVicPort"));
            m_portClose      = port_close_t(GetProcAddress(m_port, "CloseTVicPort"));
            m_portWrite      = port_write_t(GetProcAddress(m_port, "WritePort"));

            if (!m_isDriverOpened || !m_portOpen || !m_portClose || !m_portWrite)
            {
                m_lastError = "Cannot load function from TVicPort.dll: " + this->getLastErrorFormated();
                this->release();
            }
        }
        else { m_lastError = "Cannot found or open TVicPort.dll: " + this->getLastErrorFormated(); }
#endif
    }

    bool open() override
    {
        // Should never be used
        return false;
    }

    bool close() override
    {
#if defined TARGET_OS_Windows

        if (m_port != nullptr)
        {
            if (m_isDriverOpened())
            {
                m_portNumber = 0;
                m_portClose();
                return true;
            }
            m_lastError = "Cannot close the TVicPort library because it is not opened.";
            return false;
        }
        return false;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

            return false;

            /*if (ioctl(fd, PPRELEASE) < 0)  { return false; }

            if (close(m_file) < 0) { return false; }
            else
            {
                m_file = -1;
            }*/
#endif
    }

    bool isReadyToSend(const size_t /*timeOut*/  = 0) const override { return this->isConnected(); }
    bool isReadyToReceive(const size_t /*timeOut*/  = 0) const override { return this->isConnected(); }
    size_t getPendingByteCount() const { return (this->isConnected() ? 0 : 1); }

    size_t sendBuffer(const void* buffer, const size_t size = 8) override
    {
        if (!this->isConnected()) { return 0; }

#if defined TARGET_OS_Windows
        const uint8_t value = *(static_cast<const uint8_t*>(buffer));

        m_portWrite(m_portNumber, value);
        return size;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

            return 0;

            /*if (ioctl(m_file, PPWDATA, &value) < 0) { return size; }*/

#endif
    }

    size_t receiveBuffer(void* /*buffer*/, const size_t /*size*/  = 8) override
    {
        if (!this->isConnected()) { return 0; }

#if defined TARGET_OS_Windows

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

        /*
        unsigned char valin;
        int dirin=1;
        int dirout=0;

        if (ioctl (fd, PPDATADIR, &dirin)  < 0) { return 0; }
        if (ioctl(fd, PPRDATA, &valin) < 0) { return 0; }

        int res = ::read(m_file, buffer, size);
        if(res < 0)
        {
            this->close();
            return 0;
        }

        return res;
        */

#endif
        return 0;
    }

    bool sendBufferBlocking(const void* buffer, const size_t size) override
    {
        const char* p    = reinterpret_cast<const char*>(buffer);
        size_t bytesLeft = size;

        while (bytesLeft != 0 && this->isConnected()) { bytesLeft -= this->sendBuffer(p + size - bytesLeft, bytesLeft); }

        return this->isConnected();
    }

    bool receiveBufferBlocking(void* buffer, const size_t size) override
    {
        char* p          = reinterpret_cast<char*>(buffer);
        size_t bytesLeft = size;

        while (bytesLeft != 0 && this->isConnected()) { bytesLeft -= this->receiveBuffer(p + size - bytesLeft, bytesLeft); }

        return this->isConnected();
    }

#if defined TARGET_OS_Windows
    bool isConnected() const override { return m_portNumber != 0; }
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
        bool isConnected() const override { return m_file != 0; }
#endif

    void release() override
    {
#if defined TARGET_OS_Windows
        if (m_port != nullptr && !FreeLibrary(m_port)) { m_lastError = getLastErrorFormated(); }
#endif
        delete this;
    }

    bool connect(const unsigned short portNumber) override
    {
        if (this->isConnected()) { return false; }

#if defined TARGET_OS_Windows
        if (m_port != nullptr)
        {
            if (m_portOpen())
            {
                m_lastError  = "No error";
                m_portNumber = portNumber;
                return true;
            }
            m_lastError  = "Cannot open the TVic library";
            m_portNumber = 0;
            return false;
        }
        m_lastError = "TVicPort library is not loaded.";
        return false;

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS

            return false;

            /*
            std::string url = "/dev/parport" + std::to_string(portNumber);
            if ((m_file = open(url.c_str() , O_RDWR)) < 0) 
            {
                this->close();
                return false;
            }
            else
            {
                if (ioctl(m_file, PPCLAIM) < 0)
                {
                    this->close();
                    return false;
                }
            }
            */

#endif
    }

    std::string getLastError() override { return m_lastError; }

    std::string getLastErrorFormated()
    {
#if defined TARGET_OS_Windows

        LPTSTR errorText;
        const DWORD error = GetLastError();

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM                    // use system message tables to retrieve error text
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER              // allocate buffer on local heap for error text
                      | FORMAT_MESSAGE_IGNORE_INSERTS,              // Important! will fail otherwise, since we're not (and CANNOT) pass insertion parameters 
                      nullptr,                                          // unused with FORMAT_MESSAGE_FROM_SYSTEM
                      error,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      LPTSTR(&errorText),                       // output 
                      0,                                            // minimum size for output buffer
                      nullptr);                                     // arguments - see note

        return errorText + std::to_string(static_cast<uint64_t>(error));

#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
            return "Not implemented.";
#endif
    }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    int m_file;
#endif
};

IConnectionParallel* createConnectionParallel() { return new CConnectionParallel(); }
}  // namespace Socket
