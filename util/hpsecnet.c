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
#ifdef _WIN32
 #include <windows.h>
 #define badarg() SetLastError(ERROR_INVALID_PARAMETER)
#else
 #define INVALID_SOCKET -1
 #define badarg() errno=EINVAL
#endif
#include "hpsecnet.h"
#include <pthread.h>
#include <errno.h>

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

#define _errorcase(t) case t:  lasterr=#t;  break;

static void sslerror(SSL *ssl, int ret)
{
	switch(SSL_get_error(ssl, ret))
	{
		_errorcase(SSL_ERROR_NONE)
		_errorcase(SSL_ERROR_ZERO_RETURN)
		_errorcase(SSL_ERROR_WANT_READ)
		_errorcase(SSL_ERROR_WANT_WRITE)
		_errorcase(SSL_ERROR_WANT_CONNECT)
		#ifdef SSL_ERROR_WANT_ACCEPT
		_errorcase(SSL_ERROR_WANT_ACCEPT)
		#endif
		_errorcase(SSL_ERROR_WANT_X509_LOOKUP)
		case SSL_ERROR_SYSCALL:
			#ifdef _WIN32
			if(ret==-1)
			{
				if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM
					|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, WSAGetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lasterr, 0, NULL))
					lasterr="Error in FormatMessage()";
				return;
			}
			#else
			if(ret==-1) lasterr=strerror(errno);
			#endif
			else if(ret==0) lasterr="SSL_ERROR_SYSCALL (abnormal termination)";
			else lasterr="SSL_ERROR_SYSCALL";  break;
		case SSL_ERROR_SSL:
			lasterr=ERR_error_string(ERR_get_error(), NULL);  return;
	}
};
	
#define _throwssl() {lasterr=ERR_error_string(ERR_get_error(), NULL);  goto bailout;}

/*******************************************************************
 Secure network routines
 *******************************************************************/

static void progress_callback(int p, int n, void *arg)
{
}

static EVP_PKEY *newprivkey(int bits)
{
	EVP_PKEY *pk=NULL;
	if(!(pk=EVP_PKEY_new())) _throwssl();
	if(!EVP_PKEY_assign_RSA(pk, RSA_generate_key(bits, 0x10001,
		progress_callback, NULL))) _throwssl();
	return pk;

	bailout:
	if(pk) EVP_PKEY_free(pk);
	return NULL;
}

static X509 *newcert(EVP_PKEY *priv)
{
	X509 *cert=NULL;  X509_NAME *name=NULL;  int nid=NID_undef;
	X509_PUBKEY *pub=NULL;  EVP_PKEY *pk=NULL;

	if((cert=X509_new())==NULL) _throwssl();
	if(!X509_set_version(cert, 2)) _throwssl();
	ASN1_INTEGER_set(X509_get_serialNumber(cert), 0L);

	if((name=X509_NAME_new())==NULL) _throwssl();
	if((nid=OBJ_txt2nid("organizationName"))==NID_undef) _throwssl();
	if(!X509_NAME_add_entry_by_NID(name, nid, MBSTRING_ASC, (char *)"VirtualGL",
		-1, -1, 0)) _throwssl();
	if((nid=OBJ_txt2nid("commonName"))==NID_undef) _throwssl();
	if(!X509_NAME_add_entry_by_NID(name, nid, MBSTRING_ASC, (char *)"localhost",
		-1, -1, 0)) _throwssl();
	if(!X509_set_subject_name(cert, name)) _throwssl();
	if(!X509_set_issuer_name(cert, name)) _throwssl();
	X509_NAME_free(name);  name=NULL;

	X509_gmtime_adj(X509_get_notBefore(cert), 0);
	X509_gmtime_adj(X509_get_notAfter(cert), (long)60*60*24*365);

	if((pub=X509_PUBKEY_new())==NULL) _throwssl();
	X509_PUBKEY_set(&pub, priv);
	if((pk=X509_PUBKEY_get(pub))==NULL) _throwssl();
	X509_set_pubkey(cert, pk);
	EVP_PKEY_free(pk);  pk=NULL;
	X509_PUBKEY_free(pub);  pub=NULL;
	if(X509_sign(cert, priv, EVP_md5())<=0) _throwssl();

	return cert;
	
	bailout:
	if(pub) X509_PUBKEY_free(pub);
	if(pk) EVP_PKEY_free(pk);
	if(name) X509_NAME_free(name);
	if(cert) X509_free(cert);
	return NULL;
}


SSL_CTX *hpsecnet_serverinit(void)
{
	SSL_CTX *sslctx=NULL;  int i;
	X509 *cert=NULL;  EVP_PKEY *priv=NULL;
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
		if(pthread_mutex_init(&mutex[i], NULL)==-1) {lasterr=strerror(errno);  goto bailout;}
	}
	CRYPTO_set_id_callback(thread_id);
	CRYPTO_set_locking_callback(locking_callback);
	if((sslctx=SSL_CTX_new(SSLv23_server_method()))==NULL) _throwssl();
	if((priv=newprivkey(1024))==NULL) goto bailout;
	if((cert=newcert(priv))==NULL) goto bailout;
	if(SSL_CTX_use_certificate(sslctx, cert)<=0) _throwssl();
	if(SSL_CTX_use_PrivateKey(sslctx, priv)<=0) _throwssl();
	if(!SSL_CTX_check_private_key(sslctx)) _throwssl();
	if(priv) EVP_PKEY_free(priv);
	if(cert) X509_free(cert);
	return sslctx;

	bailout:
	if(priv) EVP_PKEY_free(priv);
	if(cert) X509_free(cert);
	if(sslctx) SSL_CTX_free(sslctx);
	return NULL;
}


SSL *hpsecnet_accept(int socket, SSL_CTX *sslctx)
{
	SSL *tempssl;  int ret;
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
	if((ret=SSL_accept(tempssl))!=1)
	{
		SSL_free(tempssl);  sslerror(tempssl, ret);  return NULL;
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
	SSL *tempssl;  int ret;
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
	if((ret=SSL_connect(tempssl))!=1)
	{
		SSL_free(tempssl);  sslerror(tempssl, ret);  return NULL;
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
		if(retval<=0) {sslerror(ssl, retval);  return retval;}
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
		if(retval<=0) {sslerror(ssl, retval);  return retval;}
		bytesrecd+=retval;
	}
	return bytesrecd;
}


char *hpsecnet_strerror(void)
{
	return lasterr;
}
