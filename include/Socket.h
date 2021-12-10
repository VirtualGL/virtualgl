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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2ipdef.h>
#else
	#include <netinet/in.h>
#endif

#include "Error.h"
#include "Mutex.h"


namespace util
{
	class SockError : public Error
	{
		public:

			#ifdef _WIN32

			SockError(const char *method_, int line) :
				Error(method_, (char *)NULL, line)
			{
				if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), message, MLEN, NULL))
					strncpy(message, "Error in FormatMessage()", MLEN);
			}

			#else

			SockError(const char *method_, int line) :
				Error(method_, strerror(errno), line) {}

			#endif
	};
}

#define THROW_SOCK()  throw(SockError(__FUNCTION__, __LINE__))


#ifndef _WIN32
typedef int SOCKET;
#endif

namespace util
{
	class Socket
	{
		public:

			Socket(bool ipv6);
			Socket(SOCKET sd);
			~Socket(void);
			void close(void);
			void connect(char *serverName, unsigned short port);
			unsigned short findPort(void);
			unsigned short listen(unsigned short port, bool reuseAddr = false);
			Socket *accept(void);
			void send(char *buf, int len);
			void recv(char *buf, int len);
			const char *remoteName(void);

		private:

			unsigned short setupListener(unsigned short port, bool reuseAddr);

			static const int MAXCONN = 1024;
			static int instanceCount;
			static CriticalSection mutex;
			SOCKET sd;
			char remoteNameBuf[INET6_ADDRSTRLEN];
			bool ipv6;
	};
}

#endif  // __SOCKET_H__
