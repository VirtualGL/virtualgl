/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#ifndef __RRSOCKET_H__
#define __RRSOCKET_H__

#ifdef USESSL
#define OPENSSL_NO_KRB5
#ifdef _WIN32
#include <windows.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>
#if defined(sun)||defined(sgi)
#include <openssl/rand.h>
#endif
#endif

#include "rrerror.h"
#include "rrmutex.h"


class sockerror : public rrerror
{
	public:

		#ifdef _WIN32
		sockerror(const char *method, int line) :
			rrerror(method, (char *)NULL, line)
		{
			if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), _message, MLEN, NULL))
				strncpy(_message, "Error in FormatMessage()", MLEN);
		}
		#else
		sockerror(const char *method, int line) :
			rrerror(method, strerror(errno), line) {}
		#endif
};

#define _throwsock() throw(sockerror(__FUNCTION__, __LINE__))
#define trysock(f) {if((f)==SOCKET_ERROR) _throwsock();}


#ifdef USESSL

class sslerror : public rrerror
{
	public:

		sslerror(const char *method, int line) :
			rrerror(method, (char *)NULL, line)
		{
			ERR_error_string_n(ERR_get_error(), &_message[strlen(_message)],
				MLEN-strlen(_message));
		}

		sslerror(const char *method, SSL *ssl, int ret) :
			rrerror(method, (char *)NULL)
		{
			const char *errorstring=NULL;
			switch(SSL_get_error(ssl, ret))
			{
				case SSL_ERROR_NONE:
					errorstring="SSL_ERROR_NONE";  break;
				case SSL_ERROR_ZERO_RETURN:
					errorstring="SSL_ERROR_ZERO_RETURN";  break;
				case SSL_ERROR_WANT_READ:
					errorstring="SSL_ERROR_WANT_READ";  break;
				case SSL_ERROR_WANT_WRITE:
					errorstring="SSL_ERROR_WANT_WRITE";  break;
				case SSL_ERROR_WANT_CONNECT:
					errorstring="SSL_ERROR_WANT_CONNECT";  break;
				#ifdef SSL_ERROR_WANT_ACCEPT
				case SSL_ERROR_WANT_ACCEPT:
					errorstring="SSL_ERROR_WANT_ACCEPT";  break;
				#endif
				case SSL_ERROR_WANT_X509_LOOKUP:
					errorstring="SSL_ERROR_WANT_X509_LOOKUP";  break;
				case SSL_ERROR_SYSCALL:
					#ifdef _WIN32
					if(ret==-1)
					{
						if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
							WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							_message, MLEN, NULL))
							strncpy(_message, "Error in FormatMessage()", MLEN);
						return;
					}
					#else
					if(ret==-1) errorstring=strerror(errno);
					#endif
					else if(ret==0)
						errorstring="SSL_ERROR_SYSCALL (abnormal termination)";
					else errorstring="SSL_ERROR_SYSCALL";  break;
				case SSL_ERROR_SSL:
					ERR_error_string_n(ERR_get_error(), _message, MLEN);  return;
			}
			strncpy(_message, errorstring, MLEN);
		}
};

#define _throwssl() throw(sslerror(__FUNCTION__, __LINE__))

#endif // USESSL


#ifndef _WIN32
typedef int SOCKET;
#endif

class rrsocket
{
	public:

		rrsocket(bool);
		#ifdef USESSL
		rrsocket(SOCKET, SSL *);
		#else
		rrsocket(SOCKET);
		#endif
		~rrsocket(void);
		void close(void);
		void connect(char *, unsigned short);
		unsigned short findport(void);
		unsigned short listen(unsigned short, bool reuseaddr=false);
		rrsocket *accept(void);
		void send(char *, int);
		void recv(char *, int);
		char *remotename(void);

	private:

		unsigned short setuplistener(unsigned short, bool reuseaddr);

		#ifdef USESSL
		static unsigned long thread_id(void)
		{
			#ifdef _WIN32
			return GetCurrentThreadId();
			#else
			return (unsigned long)pthread_self();
			#endif
		}

		static void locking_callback(int mode, int type, const char *file,
			int line)
		{
			if(mode&CRYPTO_LOCK) _Cryptolock[type].lock();
			else _Cryptolock[type].unlock();
		}

		static bool _Sslinit;
		static rrcs _Cryptolock[CRYPTO_NUM_LOCKS];
		bool _dossl;  SSL_CTX *_sslctx;  SSL *_ssl;
		#endif

		static const int MAXCONN=1024;
		static int _Instancecount;  
		static rrcs _Mutex;
		SOCKET _sd;
};


#endif // __RRSOCKET_H__
