#pragma once
#include "IConnection.h"

#include <iostream>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <sys/select.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 // #include <netinet/in.h>
 // #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 // #include <netdb.h>
 #include <ctime>

#if defined TARGET_OS_MacOS
  #define Socket_SendFlags    0
  #define Socket_ReceiveFlags 0
#else
  #define Socket_SendFlags    MSG_NOSIGNAL
  #define Socket_ReceiveFlags MSG_NOSIGNAL
#endif
#elif defined TARGET_OS_Windows
#include <WinSock2.h>
#include <Windows.h>

#define Socket_SendFlags    0
#define Socket_ReceiveFlags 0
#else

#endif

namespace Socket {
static bool FD_ISSET_PROXY(const int fd, fd_set* set) { return FD_ISSET(fd, set) ? true : false; }

template <class T>
class TConnection : public T
{
public:

	TConnection() : m_socket(-1)
	{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#elif defined TARGET_OS_Windows
		const int versionHigh     = 2;
		const int versionLow      = 0;
		const WORD winsockVersion = MAKEWORD(versionHigh, versionLow);
		WSADATA wsaData;
		WSAStartup(winsockVersion, &wsaData);
#else
#endif
	}

	explicit TConnection(const int socket) : m_socket(socket)
	{
#if defined TARGET_OS_Windows
		const int versionHigh     = 2;
		const int versionLow      = 0;
		const WORD winsockVersion = MAKEWORD(versionHigh, versionLow);
		WSADATA wsaData;
		WSAStartup(winsockVersion, &wsaData);
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#else
#endif
	}

	virtual ~TConnection()
	{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#elif defined TARGET_OS_Windows
		WSACleanup();
#else
#endif
	}

protected:

	virtual bool open()
	{
		if (isConnected()) { return false; }

		m_socket = int(socket(AF_INET, SOCK_STREAM, 0));
		if (m_socket == size_t(-1)) { return false; }

		return true;
	}

public:

	virtual bool close()
	{
		if (!isConnected()) { return false; }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
			::shutdown(m_socket, SHUT_RDWR);
			::close(m_socket);
#elif defined TARGET_OS_Windows
		shutdown(m_socket, SD_BOTH);
		closesocket(m_socket);
#else
#endif

		m_socket = -1;
		return true;
	}

	virtual bool isReadyToSend(const size_t timeOut = 0) const
	{
		if (!isConnected()) { return false; }

		struct timeval timeVal;
		timeVal.tv_sec  = int(timeOut / 1000);
		timeVal.tv_usec = int((timeOut - timeVal.tv_sec * 1000) * 1000);

		fd_set writeFileDesc;
		FD_ZERO(&writeFileDesc);
		FD_SET(m_socket, &writeFileDesc);

		if (select(int(m_socket + 1), nullptr, &writeFileDesc, nullptr, &timeVal) < 0) { return false; }
		if (!FD_ISSET_PROXY(int(m_socket), &writeFileDesc)) { return false; }
		return true;
	}

	virtual bool isReadyToReceive(const size_t timeOut = 0) const
	{
		if (!isConnected()) { return false; }

		struct timeval timeVal;
		timeVal.tv_sec  = int(timeOut / 1000);
		timeVal.tv_usec = int((timeOut - timeVal.tv_sec * 1000) * 1000);

		fd_set readFileDesc;
		FD_ZERO(&readFileDesc);
		FD_SET(m_socket, &readFileDesc);

		if (select(int(m_socket + 1), &readFileDesc, nullptr, nullptr, &timeVal) < 0) { return false; }
		if (!(FD_ISSET_PROXY(int(m_socket), &readFileDesc))) { return false; }
		return true;
	}

	virtual size_t sendBuffer(const void* buffer, const size_t size)
	{
		if (!isConnected()) { return 0; }
		const int res = send(m_socket, static_cast<const char*>(buffer), int(size), Socket_SendFlags);
		if (size != 0 && res <= 0) { close(); }
		return res <= 0 ? 0 : size_t(res);
	}

	virtual size_t receiveBuffer(void* buffer, const size_t size)
	{
		if (!isConnected() || !size) { return 0; }
		const int res = recv(m_socket, static_cast<char*>(buffer), int(size), Socket_ReceiveFlags);
		if (size != 0 && res <= 0) { close(); }
		return res <= 0 ? 0 : size_t(res);
	}

	virtual bool sendBufferBlocking(const void* buffer, const size_t size)
	{
		size_t leftBytes      = size;
		const char* tmpBuffer = static_cast<const char*>(buffer);
		do
		{
			leftBytes -= sendBuffer(tmpBuffer + size - leftBytes, leftBytes);
			if (!isConnected()) { return false; }
		} while (leftBytes != 0);
		return true;
	}

	virtual bool receiveBufferBlocking(void* buffer, const size_t size)
	{
		size_t leftBytes = size;
		char* tmpBuffer  = static_cast<char*>(buffer);
		do
		{
			leftBytes -= receiveBuffer(tmpBuffer + size - leftBytes, leftBytes);
			if (!isConnected()) { return false; }
		} while (leftBytes != 0);
		return true;
	}

	virtual bool isConnected() const { return m_socket != size_t(-1); }

	virtual void release()
	{
		if (isConnected()) { close(); }
		delete this;
	}

protected:

	size_t m_socket;
};
}  // namespace Socket
