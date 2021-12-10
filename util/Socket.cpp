// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2014, 2016, 2018-2019, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include "Socket.h"
#include "Thread.h"

#ifdef _WIN32
	#include <ws2tcpip.h>
	#include "vglutil.h"
#else
	#include <signal.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <netinet/tcp.h>
	#define SOCKET_ERROR  -1
	#define INVALID_SOCKET  -1
#endif
#ifndef INADDR_NONE
	#define INADDR_NONE  ((in_addr_t)0xffffffff)
#endif

using namespace util;


#define TRY_SOCK(f)  { if((f) == SOCKET_ERROR) THROW_SOCK(); }

#ifdef _WIN32
typedef int SOCKLEN_T;
#else
typedef socklen_t SOCKLEN_T;
#endif

namespace util {

typedef struct
{
	union
	{
		struct sockaddr sa;
		struct sockaddr_storage ss;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} u;
} SockAddr;

}

CriticalSection Socket::mutex;
int Socket::instanceCount = 0;


Socket::Socket(bool ipv6_) : ipv6(ipv6_)
{
	CriticalSection::SafeLock l(mutex);

	#ifdef _WIN32
	if(instanceCount == 0)
	{
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			throw(Error("Socket::Socket()", "Winsock initialization failed"));
		if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
			throw(Error("Socket::Socket()", "Wrong Winsock version"));
	}
	instanceCount++;
	#else
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) THROW_UNIX();
	#endif

	sd = INVALID_SOCKET;
}


Socket::Socket(SOCKET sd_) :
	sd(sd_)
{
	#ifdef _WIN32
	CriticalSection::SafeLock l(mutex);
	instanceCount++;
	#endif
}


Socket::~Socket(void)
{
	close();
	#ifdef _WIN32
	mutex.lock(false);
	instanceCount--;  if(instanceCount == 0) WSACleanup();
	mutex.unlock(false);
	#endif
}


void Socket::close(void)
{
	if(sd != INVALID_SOCKET)
	{
		#ifdef _WIN32
		closesocket(sd);
		#else
		shutdown(sd, 2);
		::close(sd);
		#endif
		sd = INVALID_SOCKET;
	}
}


void Socket::connect(char *serverName, unsigned short port)
{
	struct addrinfo hints, *addr = NULL;
	int m = 1;
	char portName[10];

	if(serverName == NULL || strlen(serverName) < 1) THROW("Invalid argument");
	if(sd != INVALID_SOCKET) THROW("Already connected");

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	snprintf(portName, 10, "%d", port);
	int err = getaddrinfo(serverName, portName, &hints, &addr);
	if(err != 0)
		throw(Error(__FUNCTION__, gai_strerror(err), __LINE__));

	try
	{
		if((sd = socket(addr->ai_family, SOCK_STREAM,
			IPPROTO_TCP)) == INVALID_SOCKET)
			THROW_SOCK();
		TRY_SOCK(::connect(sd, addr->ai_addr, (SOCKLEN_T)addr->ai_addrlen));
		TRY_SOCK(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&m,
			sizeof(int)));
		freeaddrinfo(addr);
	}
	catch(...)
	{
		freeaddrinfo(addr);
		throw;
	}
}


unsigned short Socket::setupListener(unsigned short port, bool reuseAddr)
{
	int one = 1, reuse = reuseAddr ? 1 : 0;
	#ifdef __CYGWIN__
	int zero = 0;
	#endif
	SockAddr myaddr;  SOCKLEN_T addrlen;

	if(sd != INVALID_SOCKET) THROW("Already connected");

	if((sd = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM,
		IPPROTO_TCP)) == INVALID_SOCKET)
		THROW_SOCK();
	TRY_SOCK(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&one,
		sizeof(int)));
	TRY_SOCK(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
		sizeof(int)));
	#ifdef __CYGWIN__
	TRY_SOCK(setsockopt(sd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&zero,
		sizeof(int)));
	#endif

	memset(&myaddr, 0, sizeof(myaddr));
	if(ipv6)
	{
		myaddr.u.sin6.sin6_family = AF_INET6;
		myaddr.u.sin6.sin6_addr = in6addr_any;
		myaddr.u.sin6.sin6_port = (port == 0) ? 0 : htons(port);
		addrlen = sizeof(struct sockaddr_in6);
	}
	else
	{
		myaddr.u.sin.sin_family = AF_INET;
		myaddr.u.sin.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.u.sin.sin_port = (port == 0) ? 0 : htons(port);
		addrlen = sizeof(struct sockaddr_in);
	}
	TRY_SOCK(bind(sd, &myaddr.u.sa, addrlen));
	TRY_SOCK(getsockname(sd, &myaddr.u.sa, &addrlen));

	return ipv6 ? ntohs(myaddr.u.sin6.sin6_port) : ntohs(myaddr.u.sin.sin_port);
}


unsigned short Socket::findPort(void)
{
	return setupListener(0, false);
}


unsigned short Socket::listen(unsigned short port, bool reuseAddr)
{
	unsigned short actualPort = port;

	actualPort = setupListener(port, reuseAddr);

	TRY_SOCK(::listen(sd, MAXCONN));

	return actualPort;
}


Socket *Socket::accept(void)
{
	SOCKET clientsd;
	int m = 1;
	SockAddr remoteaddr;
	SOCKLEN_T addrlen = sizeof(struct sockaddr_storage);

	if(sd == INVALID_SOCKET) THROW("Not connected");

	TRY_SOCK(clientsd = ::accept(sd, &remoteaddr.u.sa, &addrlen));
	TRY_SOCK(setsockopt(clientsd, IPPROTO_TCP, TCP_NODELAY, (char *)&m,
		sizeof(int)));

	return new Socket(clientsd);
}


const char *Socket::remoteName(void)
{
	SockAddr remoteaddr;
	SOCKLEN_T addrlen = sizeof(struct sockaddr_storage);
	const char *remoteName;

	TRY_SOCK(getpeername(sd, &remoteaddr.u.sa, &addrlen));
	if(remoteaddr.u.ss.ss_family == AF_INET6)
		remoteName = inet_ntop(remoteaddr.u.ss.ss_family,
			&remoteaddr.u.sin6.sin6_addr, remoteNameBuf, INET6_ADDRSTRLEN);
	else
		remoteName = inet_ntop(remoteaddr.u.ss.ss_family,
			&remoteaddr.u.sin.sin_addr, remoteNameBuf, INET6_ADDRSTRLEN);

	return remoteName ? remoteName : (char *)"Unknown";
}


void Socket::send(char *buf, int len)
{
	if(sd == INVALID_SOCKET) THROW("Not connected");
	int bytesSent = 0, retval;
	while(bytesSent < len)
	{
		retval = ::send(sd, &buf[bytesSent], len - bytesSent, 0);
		if(retval == SOCKET_ERROR) THROW_SOCK();
		if(retval == 0) break;
		bytesSent += retval;
	}
	if(bytesSent != len) THROW("Incomplete send");
}


void Socket::recv(char *buf, int len)
{
	if(sd == INVALID_SOCKET) THROW("Not connected");
	int bytesRead = 0, retval;
	while(bytesRead < len)
	{
		retval = ::recv(sd, &buf[bytesRead], len - bytesRead, 0);
		if(retval == SOCKET_ERROR) THROW_SOCK();
		if(retval == 0) break;
		bytesRead += retval;
	}
	if(bytesRead != len) THROW("Incomplete receive");
}
