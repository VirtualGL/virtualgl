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

// High-Performance Utility Library
// SSL network routines

#include <stdio.h>
#include <string.h>
#include "hpsecnet.h"
#include <pthread.h>
#include <errno.h>
#define INVALID_SOCKET -1
#ifdef _WIN32
 #define badarg() SetLastError(ERROR_INVALID_PARAMETER)
#else
 #define badarg() errno=EINVAL
#endif

static pthread_mutex_t mutex[CRYPTO_NUM_LOCKS];
static char *lasterr="No Error";

void locking_callback(int mode, int type, const char *file, int line)
{
	if(mode&CRYPTO_LOCK) pthread_mutex_lock(&mutex[type]);
	else pthread_mutex_unlock(&mutex[type]);
}

unsigned long thread_id(void)
{
	return (unsigned long)pthread_self();
}


/*******************************************************************
 Secure network routines
 *******************************************************************/

SSL_CTX *hpsecnet_serverinit(char *certfile, char *privkeyfile)
{
	SSL_CTX *sslctx;  int i;
	#if defined(SOLARIS)||defined(sgi)
	char *buf="ajsdluasdb5nq457q67na867qm8v67vq4865j7bnq5q867n";
	#endif
	if(!certfile || !privkeyfile)
	{
		lasterr="Invalid argument";  return NULL;
	}
	SSL_library_init();
	#if defined(SOLARIS)||defined(sgi)
	RAND_seed(buf, strlen(buf));
	#endif
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	for(i=0; i<CRYPTO_NUM_LOCKS; i++)
	{
		if(pthread_mutex_init(&mutex[i], NULL)==-1) {lasterr=strerror(errno);  return NULL;}
	}
	CRYPTO_set_id_callback(thread_id);
	CRYPTO_set_locking_callback(locking_callback);
	if((sslctx=SSL_CTX_new(TLSv1_server_method()))==NULL)
	{
		lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(SSL_CTX_use_certificate_file(sslctx, certfile, SSL_FILETYPE_PEM)<=0)
	{
		SSL_CTX_free(sslctx);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(SSL_CTX_use_PrivateKey_file(sslctx, privkeyfile, SSL_FILETYPE_PEM)<=0)
	{
		SSL_CTX_free(sslctx);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(!SSL_CTX_check_private_key(sslctx))
	{
		SSL_CTX_free(sslctx);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	return sslctx;
}


SSL *hpsecnet_accept(int socket, SSL_CTX *sslctx)
{
	SSL *tempssl;
	if(socket==INVALID_SOCKET || !sslctx)
	{
		lasterr="Invalid argument";  return NULL;
	}
	if(!(tempssl=SSL_new(sslctx)))
	{
		lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(!(SSL_set_fd(tempssl, socket)))
	{
		SSL_free(tempssl);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(SSL_accept(tempssl)!=1)
	{
		SSL_free(tempssl);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	SSL_set_accept_state(tempssl);
	return tempssl;
}


void hpsecnet_disconnect(SSL *ssl)
{
	if(ssl)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
}


void hpsecnet_term(SSL_CTX *sslctx)
{
	int i;
	if(sslctx)
	{
		SSL_CTX_free(sslctx);
	}
	for(i=0; i<CRYPTO_NUM_LOCKS; i++) pthread_mutex_destroy(&mutex[i]);
}


SSL_CTX *hpsecnet_clientinit(void)
{
	SSL_CTX *sslctx;  int i;
	#if defined(SOLARIS)||defined(sgi)
	char *buf="ajsdluasdb5nq457q67na867qm8v67vq4865j7bnq5q867n";
	#endif
	SSL_library_init();

	#if defined(SOLARIS)||defined(sgi)
	RAND_seed(buf, strlen(buf));
	#endif
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ERR_load_crypto_strings();
	for(i=0; i<CRYPTO_NUM_LOCKS; i++)
	{
		if(pthread_mutex_init(&mutex[i], NULL)==-1) {lasterr=strerror(errno);  return NULL;}
	}
	CRYPTO_set_id_callback(thread_id);
	CRYPTO_set_locking_callback(locking_callback);
	if((sslctx=SSL_CTX_new(TLSv1_client_method()))==NULL)
	{
		lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	return sslctx;
}


SSL *hpsecnet_connect(int socket, SSL_CTX *sslctx)
{
	SSL *tempssl;
	if(socket==INVALID_SOCKET || !sslctx)
	{
		lasterr="Invalid argument";  return NULL;
	}
	if((tempssl=SSL_new(sslctx))==NULL)
	{
		lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(!SSL_set_fd(tempssl, socket))
	{
		SSL_free(tempssl);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	if(SSL_connect(tempssl)!=1)
	{
		SSL_free(tempssl);  lasterr=ERR_error_string(ERR_get_error(), NULL);  return NULL;
	}
	SSL_set_connect_state(tempssl);
	return tempssl;
}


//#define SSLMTU 16384  // SSL v3 can only transfer 16k at a time

int hpsecnet_send(SSL *ssl, char *buf, int len)
{
	int bytessent=0, retval, sendsize;
	while(bytessent<len)
	{
//		sendsize=min(len-bytessent, SSLMTU);
		sendsize=len-bytessent;
		retval=SSL_write(ssl, &buf[bytessent], len);
		if(retval<=0) {lasterr=ERR_error_string(ERR_get_error(), NULL);  return retval;}
		bytessent+=retval;
	}
	return bytessent;
}


int hpsecnet_recv(SSL *ssl, char *buf, int len)
{
	int bytesrecd=0, retval, readsize;
	while(bytesrecd<len)
	{
//		readsize=min(len-bytesrecd, SSLMTU);
		readsize=len-bytesrecd;
		retval=SSL_read(ssl, &buf[bytesrecd], len);
		if(retval<=0) {lasterr=ERR_error_string(ERR_get_error(), NULL);  return retval;}
		bytesrecd+=retval;
	}
	return bytesrecd;
}


char *hpsecnet_strerror(void)
{
	return lasterr;
}
