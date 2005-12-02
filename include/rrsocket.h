/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
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
#ifndef _WIN32
#pragma weak ASN1_INTEGER_set
#pragma weak CRYPTO_set_id_callback
#pragma weak CRYPTO_set_locking_callback
#pragma weak ERR_error_string_n
#pragma weak ERR_get_error
#pragma weak ERR_load_crypto_strings
#pragma weak EVP_md5
#pragma weak EVP_PKEY_assign
#pragma weak EVP_PKEY_free
#pragma weak EVP_PKEY_new
#pragma weak OBJ_txt2nid
#pragma weak OPENSSL_add_all_algorithms_conf
#pragma weak OPENSSL_add_all_algorithms_noconf
#if defined(sun)||defined(sgi)
#pragma weak RAND_seed
#endif
#pragma weak RSA_generate_key
#pragma weak SSL_accept
#pragma weak SSL_connect
#pragma weak SSL_CTX_check_private_key
#pragma weak SSL_CTX_free
#pragma weak SSL_CTX_new
#pragma weak SSL_CTX_use_certificate
#pragma weak SSL_CTX_use_PrivateKey
#pragma weak SSL_free
#pragma weak SSL_get_error
#pragma weak SSL_library_init
#pragma weak SSL_load_error_strings
#pragma weak SSL_new
#pragma weak SSL_read
#pragma weak SSL_set_accept_state
#pragma weak SSL_set_connect_state
#pragma weak SSL_set_fd
#pragma weak SSL_shutdown
#pragma weak SSL_write
#pragma weak SSLv23_client_method
#pragma weak SSLv23_server_method
#pragma weak X509_free
#pragma weak X509_get_serialNumber
#pragma weak X509_gmtime_adj
#pragma weak X509_NAME_add_entry_by_NID
#pragma weak X509_NAME_free
#pragma weak X509_NAME_new
#pragma weak X509_new
#pragma weak X509_PUBKEY_free
#pragma weak X509_PUBKEY_get
#pragma weak X509_PUBKEY_new
#pragma weak X509_PUBKEY_set
#pragma weak X509_set_issuer_name
#pragma weak X509_set_pubkey
#pragma weak X509_set_subject_name
#pragma weak X509_set_version
#pragma weak X509_sign
#endif
#endif

#include "rrerror.h"
#include "rrmutex.h"

class sockerror : public rrerror
{
	public:

		#ifdef _WIN32
		sockerror(const char *method, int line) : rrerror(method, (char *)NULL, line)
		{
			if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), _message, MLEN, NULL))
				strncpy(_message, "Error in FormatMessage()", MLEN);
		}
		#else
		sockerror(const char *method, int line) : rrerror(method, strerror(errno), line) {}
		#endif
};

#define _throwsock() throw(sockerror(__FUNCTION__, __LINE__))
#define trysock(f) {if((f)==SOCKET_ERROR) _throwsock();}

#ifdef USESSL
class sslerror : public rrerror
{
	public:

		sslerror(const char *method, int line) : rrerror(method, (char *)NULL, line)
		{
			ERR_error_string_n(ERR_get_error(), &_message[strlen(_message)], MLEN-strlen(_message));
		}

		sslerror(const char *method, SSL *ssl, int ret) : rrerror(method, (char *)NULL)
		{
			const char *errorstring=NULL;
			switch(SSL_get_error(ssl, ret))
			{
				case SSL_ERROR_NONE:  errorstring="SSL_ERROR_NONE";  break;
				case SSL_ERROR_ZERO_RETURN:  errorstring="SSL_ERROR_ZERO_RETURN";  break;
				case SSL_ERROR_WANT_READ:  errorstring="SSL_ERROR_WANT_READ";  break;
				case SSL_ERROR_WANT_WRITE:  errorstring="SSL_ERROR_WANT_WRITE";  break;
				case SSL_ERROR_WANT_CONNECT:  errorstring="SSL_ERROR_WANT_CONNECT";  break;
				#ifdef SSL_ERROR_WANT_ACCEPT
				case SSL_ERROR_WANT_ACCEPT:  errorstring="SSL_ERROR_WANT_ACCEPT";  break;
				#endif
				case SSL_ERROR_WANT_X509_LOOKUP:  errorstring="SSL_ERROR_WANT_X509_LOOKUP";  break;
				case SSL_ERROR_SYSCALL:
					#ifdef _WIN32
					if(ret==-1)
					{
						if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), _message, MLEN, NULL))
							strncpy(_message, "Error in FormatMessage()", MLEN);
						return;
					}
					#else
					if(ret==-1) errorstring=strerror(errno);
					#endif
					else if(ret==0) errorstring="SSL_ERROR_SYSCALL (abnormal termination)";
					else errorstring="SSL_ERROR_SYSCALL";  break;
				case SSL_ERROR_SSL:
					ERR_error_string_n(ERR_get_error(), _message, MLEN);  return;
			}
			strncpy(_message, errorstring, MLEN);
		}
};

#define _throwssl() throw(sslerror(__FUNCTION__, __LINE__))
#endif

class rrsocket
{
	public:

		rrsocket(bool);
		#ifdef USESSL
		rrsocket(int, SSL *);
		#else
		rrsocket(int);
		#endif
		~rrsocket(void);
		void close(void);
		void connect(char *, unsigned short);
		void listen(unsigned short);
		rrsocket *accept(void);
		void send(char *, int);
		void recv(char *, int);
		char *remotename(void);

	private:

		#ifdef USESSL
		static unsigned long thread_id(void)
		{
			#ifdef _WIN32
			return GetCurrentThreadId();
			#else
			return (unsigned long)pthread_self();
			#endif
		}

		static void locking_callback(int mode, int type, const char *file, int line)
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
		#ifdef _WIN32
		SOCKET _sd;
		#else
		int _sd;
		#endif
};

#endif

