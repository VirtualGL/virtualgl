/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef USESSL
#define OPENSSL_NO_KRB5
#ifdef _WIN32
#include <windows.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#if defined(sun) || defined(sgi)
#include <openssl/rand.h>
#endif
#endif

#include "Error.h"
#include "Mutex.h"


namespace vglutil
{
	class SockError : public Error
	{
		public:

			#ifdef _WIN32

			SockError(const char *method, int line) :
				Error(method, (char *)NULL, line)
			{
				if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), message, MLEN, NULL))
					strncpy(message, "Error in FormatMessage()", MLEN);
			}

			#else

			SockError(const char *method, int line) :
				Error(method, strerror(errno), line) {}

			#endif
	};
}

#define _throwsock() throw(SockError(__FUNCTION__, __LINE__))
#define _sock(f) { if((f)==SOCKET_ERROR) _throwsock(); }


#ifdef USESSL

namespace vglutil
{
	class SSLError : public Error
	{
		public:

			SSLError(const char *method, int line) :
				Error(method, (char *)NULL, line)
			{
				ERR_error_string_n(ERR_get_error(), &message[strlen(message)],
					MLEN-strlen(message));
			}

			SSLError(const char *method, SSL *ssl, int ret) :
				Error(method, (char *)NULL)
			{
				const char *errorString=NULL;

				switch(SSL_get_error(ssl, ret))
				{
					case SSL_ERROR_NONE:
						errorString="SSL_ERROR_NONE";  break;
					case SSL_ERROR_ZERO_RETURN:
						errorString="SSL_ERROR_ZERO_RETURN";  break;
					case SSL_ERROR_WANT_READ:
						errorString="SSL_ERROR_WANT_READ";  break;
					case SSL_ERROR_WANT_WRITE:
						errorString="SSL_ERROR_WANT_WRITE";  break;
					case SSL_ERROR_WANT_CONNECT:
						errorString="SSL_ERROR_WANT_CONNECT";  break;
					#ifdef SSL_ERROR_WANT_ACCEPT
					case SSL_ERROR_WANT_ACCEPT:
						errorString="SSL_ERROR_WANT_ACCEPT";  break;
					#endif
					case SSL_ERROR_WANT_X509_LOOKUP:
						errorString="SSL_ERROR_WANT_X509_LOOKUP";  break;
					case SSL_ERROR_SYSCALL:
						#ifdef _WIN32
						if(ret==-1)
						{
							if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
								WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
								message, MLEN, NULL))
								strncpy(message, "Error in FormatMessage()", MLEN);
							return;
						}
						#else
						if(ret==-1) errorString=strerror(errno);
						#endif
						else if(ret==0)
							errorString="SSL_ERROR_SYSCALL (abnormal termination)";
						else errorString="SSL_ERROR_SYSCALL";  break;
					case SSL_ERROR_SSL:
						ERR_error_string_n(ERR_get_error(), message, MLEN);  return;
				}
				strncpy(message, errorString, MLEN);
			}
	};
}

#define _throwssl() throw(SSLError(__FUNCTION__, __LINE__))

#endif // USESSL


#ifndef _WIN32
typedef int SOCKET;
#endif

namespace vglutil
{
	class Socket
	{
		public:

			Socket(bool doSSL);
			#ifdef USESSL
			Socket(SOCKET sd, SSL *ssl);
			#else
			Socket(SOCKET sd);
			#endif
			~Socket(void);
			void close(void);
			void connect(char *serverName, unsigned short port);
			unsigned short findPort(void);
			unsigned short listen(unsigned short port, bool reuseAddr=false);
			Socket *accept(void);
			void send(char *buf, int len);
			void recv(char *buf, int len);
			char *remoteName(void);

		private:

			unsigned short setupListener(unsigned short port, bool reuseAddr);

			#ifdef USESSL

			static void lockingCallback(int mode, int type, const char *file,
				int line)
			{
				if(mode&CRYPTO_LOCK) cryptoLock[type].lock();
				else cryptoLock[type].unlock();
			}

			static bool sslInit;
			static CriticalSection cryptoLock[CRYPTO_NUM_LOCKS];
			bool doSSL;  SSL_CTX *sslctx;  SSL *ssl;

			#endif

			static const int MAXCONN=1024;
			static int instanceCount;
			static CriticalSection mutex;
			SOCKET sd;
	};
}

#endif // __SOCKET_H__
