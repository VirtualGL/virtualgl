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

#include "rrsocket.h"

#ifndef _WIN32
 #include <signal.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>
 #include <netinet/tcp.h>
 #define SOCKET_ERROR -1
 #define INVALID_SOCKET -1
#endif
#ifndef INADDR_NONE
 #define INADDR_NONE ((in_addr_t) 0xffffffff)
#endif

#ifdef USESSL
static void progress_callback(int p, int n, void *arg)
{
}

static EVP_PKEY *newprivkey(int bits)
{
	EVP_PKEY *pk=NULL;
	try
	{
		if(!(pk=EVP_PKEY_new())) _throwssl();
		if(!EVP_PKEY_assign_RSA(pk, RSA_generate_key(bits, 0x10001,
			progress_callback, NULL))) _throwssl();
		return pk;
	}
	catch (...)
	{
		if(pk) EVP_PKEY_free(pk);
		throw;
	}
	return NULL;
}

static X509 *newcert(EVP_PKEY *priv)
{
	X509 *cert=NULL;  X509_NAME *name=NULL;  int nid=NID_undef;
	X509_PUBKEY *pub=NULL;  EVP_PKEY *pk=NULL;

	try
	{
		if((cert=X509_new())==NULL) _throwssl();
		if(!X509_set_version(cert, 2)) _throwssl();
		ASN1_INTEGER_set(X509_get_serialNumber(cert), 0L);

		if((name=X509_NAME_new())==NULL) _throwssl();
		if((nid=OBJ_txt2nid("organizationName"))==NID_undef) _throwssl();
		if(!X509_NAME_add_entry_by_NID(name, nid, MBSTRING_ASC,
			(unsigned char *)"VirtualGL", -1, -1, 0)) _throwssl();
		if((nid=OBJ_txt2nid("commonName"))==NID_undef) _throwssl();
		if(!X509_NAME_add_entry_by_NID(name, nid, MBSTRING_ASC,
			(unsigned char *)"localhost", -1, -1, 0)) _throwssl();
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
	}
	catch(...)
	{
		if(pub) X509_PUBKEY_free(pub);
		if(pk) EVP_PKEY_free(pk);
		if(name) X509_NAME_free(name);
		if(cert) X509_free(cert);
		throw;
	}
	return NULL;
}

bool rrsocket::_Sslinit=false;
rrcs rrsocket::_Cryptolock[CRYPTO_NUM_LOCKS];
#endif
rrcs rrsocket::_Mutex;
int rrsocket::_Instancecount=0;


rrsocket::rrsocket(bool dossl=false)
	#ifdef USESSL
	: _dossl(dossl)
	#endif
{
	rrcs::safelock l(_Mutex);
	#ifdef _WIN32
	if(_Instancecount==0)
	{
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
			throw(rrerror("rrsocket::rrsocket()", "Winsock initialization failed"));
		if(LOBYTE(wsaData.wVersion)!=2 || HIBYTE(wsaData.wVersion)!=2)
			throw(rrerror("rrsocket::rrsocket()", "Wrong Winsock version"));
	}
	_Instancecount++;
	#else
	if(signal(SIGPIPE, SIG_IGN)==SIG_ERR) _throwunix();
	#endif
	#ifdef USESSL
	if(!_Sslinit && _dossl)
	{
		#if defined(sun)||defined(sgi)
		char buf[128];  int i;
		srandom(getpid());
		for(i=0; i<128; i++) buf[i]=(char)((double)random()/(double)RAND_MAX*254.+1.0);
		RAND_seed(buf, 128);
		#endif
		OpenSSL_add_all_algorithms();
		SSL_load_error_strings();
		ERR_load_crypto_strings();
		CRYPTO_set_id_callback(thread_id);
		CRYPTO_set_locking_callback(locking_callback);
		SSL_library_init();
		_Sslinit=true;
		char *env=NULL;
		if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
			&& !strncmp(env, "1", 1))
			fprintf(stderr, "[VGL] Using OpenSSL version %s\n", SSLeay_version(SSLEAY_VERSION));
	}
	_ssl=NULL;  _sslctx=NULL;
	#endif
	_sd=INVALID_SOCKET;
}

#ifdef USESSL
rrsocket::rrsocket(SOCKET sd, SSL *ssl)
	: _sslctx(NULL), _ssl(ssl), _sd(sd)
{
	if(_ssl) _dossl=true;  else _dossl=false;
	#ifdef _WIN32
	rrcs::safelock l(_Mutex);
	_Instancecount++;
	#endif
}
#else
rrsocket::rrsocket(SOCKET sd)
	: _sd(sd)
{
	#ifdef _WIN32
	rrcs::safelock l(_Mutex);
	_Instancecount++;
	#endif
}
#endif

rrsocket::~rrsocket(void)
{
	close();
	#ifdef _WIN32
	_Mutex.lock(false);
	_Instancecount--;  if(_Instancecount==0) WSACleanup();
	_Mutex.unlock(false);
	#endif
}

void rrsocket::close(void)
{
	#ifdef USESSL
	if(_ssl)	{SSL_shutdown(_ssl);  SSL_free(_ssl);  _ssl=NULL;}
	if(_sslctx) {SSL_CTX_free(_sslctx);  _sslctx=NULL;}
	#endif
	if(_sd!=INVALID_SOCKET)
	{
		#ifdef _WIN32
		closesocket(_sd);
		#else
		shutdown(_sd, 2);
		::close(_sd);
		#endif
		_sd=INVALID_SOCKET;
	}
}

void rrsocket::connect(char *servername, unsigned short port)
{
	struct sockaddr_in servaddr;
	int m=1;  struct hostent *hent;
	if(servername==NULL) _throw("Invalid argument");

	if(_sd!=INVALID_SOCKET) _throw("Already connected");
	#ifdef USESSL
	if(_ssl && _sslctx && _dossl) _throw("SSL already connected");
	#endif

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(servername);
	servaddr.sin_port=htons(port);

	if(servaddr.sin_addr.s_addr==INADDR_NONE)
	{
		if((hent=gethostbyname(servername))==0) _throwsock();
		memcpy(&(servaddr.sin_addr), hent->h_addr_list[0], hent->h_length);
	}

	if((_sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==INVALID_SOCKET) _throwsock();
	trysock( ::connect(_sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) );
	trysock( setsockopt(_sd, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int)) );

	#ifdef USESSL
	if(_dossl)
	{
		if((_sslctx=SSL_CTX_new(SSLv23_client_method()))==NULL) _throwssl();
		if((_ssl=SSL_new(_sslctx))==NULL) _throwssl();
		if(!SSL_set_fd(_ssl, (int)_sd)) _throwssl();
		int ret=SSL_connect(_ssl);
		if(ret!=1) throw(sslerror("rrsocket::connect", _ssl, ret));
		SSL_set_connect_state(_ssl);
	}
	#endif
}

#ifdef _WIN32
typedef int SOCKLEN_T;
#else
typedef socklen_t SOCKLEN_T;
#endif

unsigned short rrsocket::setuplistener(unsigned short port, bool reuseaddr)
{
	int m=1, m2=reuseaddr? 1:0;  struct sockaddr_in myaddr;

	if(_sd!=INVALID_SOCKET) _throw("Already connected");
	#ifdef USESSL
	if(_ssl && _sslctx && _dossl) _throw("SSL already connected");
	#endif

	if((_sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==INVALID_SOCKET) _throwsock();
	trysock( setsockopt(_sd, IPPROTO_TCP, TCP_NODELAY, (char *)&m, sizeof(int)) );
	trysock( setsockopt(_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&m2, sizeof(int)) );

	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	myaddr.sin_port=(port==0)? 0:htons(port);

	trysock( bind(_sd, (struct sockaddr *)&myaddr, sizeof(myaddr)) );
	SOCKLEN_T n=sizeof(myaddr);
	trysock(getsockname(_sd, (struct sockaddr *)&myaddr, &n));
	unsigned short actualport=ntohs(myaddr.sin_port);
	return actualport;
}

unsigned short rrsocket::findport(void)
{
	return setuplistener(0, false);
}

unsigned short rrsocket::listen(unsigned short port, bool reuseaddr)
{
	unsigned short actualport=port;
	#ifdef USESSL
	X509 *cert=NULL;  EVP_PKEY *priv=NULL;
	#endif

	actualport=setuplistener(port, reuseaddr);

	trysock( ::listen(_sd, MAXCONN) );

	#ifdef USESSL
	if(_dossl)
	{
		try {
		if((_sslctx=SSL_CTX_new(SSLv23_server_method()))==NULL) _throwssl();
		errifnot(priv=newprivkey(1024));
		errifnot(cert=newcert(priv));
		if(SSL_CTX_use_certificate(_sslctx, cert)<=0)
			_throwssl();
		if(SSL_CTX_use_PrivateKey(_sslctx, priv)<=0)
			_throwssl();
		if(!SSL_CTX_check_private_key(_sslctx)) _throwssl();
		if(priv) EVP_PKEY_free(priv);
		if(cert) X509_free(cert);
		} catch (...)
		{
			if(priv) EVP_PKEY_free(priv);
			if(cert) X509_free(cert);
			throw;
		}
	}
	#endif

	return actualport;
}

rrsocket *rrsocket::accept(void)
{
	SOCKET sd_client;
	int m=1;  struct sockaddr_in remoteaddr;  SOCKLEN_T addrlen;
	addrlen=sizeof(remoteaddr);

	if(_sd==INVALID_SOCKET) _throw("Not connected");
	#ifdef USESSL
	if(!_sslctx && _dossl) _throw("SSL not initialized");
	#endif

	trysock( sd_client=::accept(_sd, (struct sockaddr *)&remoteaddr, &addrlen) );
	trysock( setsockopt(sd_client, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int)) );

	#ifdef USESSL
	SSL *tempssl=NULL;
	if(_dossl)
	{
		if(!(tempssl=SSL_new(_sslctx))) _throwssl();
		if(!(SSL_set_fd(tempssl, sd_client))) _throwssl();
		int ret=SSL_accept(tempssl);
		if(ret!=1) throw(sslerror("rrsocket::accept", tempssl, ret));
		SSL_set_accept_state(tempssl);
	}
	return new rrsocket(sd_client, tempssl);
	#else
	return new rrsocket(sd_client);
	#endif
}

char *rrsocket::remotename(void)
{
	struct sockaddr_in remoteaddr;  SOCKLEN_T addrlen=sizeof(remoteaddr);
	char *remotename=NULL;
	trysock( getpeername(_sd, (struct sockaddr *)&remoteaddr, &addrlen) );
	remotename=inet_ntoa(remoteaddr.sin_addr);
	return (remotename? remotename:(char *)"Unknown");
}

void rrsocket::send(char *buf, int len)
{
	if(_sd==INVALID_SOCKET) _throw("Not connected");
	#ifdef USESSL
	if(_dossl && !_ssl) _throw("SSL not connected");
	#endif
	int bytessent=0, retval;
	while(bytessent<len)
	{
		#ifdef USESSL
		if(_dossl)
		{
			retval=SSL_write(_ssl, &buf[bytessent], len);
			if(retval<=0) throw(sslerror("rrsocket::send", _ssl, retval));
		}
		else
		#endif
		{
			retval=::send(_sd, &buf[bytessent], len-bytessent, 0);
			if(retval==SOCKET_ERROR) _throwsock();
			if(retval==0) break;
		}
		bytessent+=retval;
	}
	if(bytessent!=len) _throw("Incomplete send");
}

void rrsocket::recv(char *buf, int len)
{
	if(_sd==INVALID_SOCKET) _throw("Not connected");
	#ifdef USESSL
	if(_dossl && !_ssl) _throw("SSL not connected");
	#endif
	int bytesrecd=0, retval;
	while(bytesrecd<len)
	{
		#ifdef USESSL
		if(_dossl)
		{
			retval=SSL_read(_ssl, &buf[bytesrecd], len);
			if(retval<=0) throw(sslerror("rrsocket::recv", _ssl, retval));
		}
		else
		#endif
		{
			retval=::recv(_sd, &buf[bytesrecd], len-bytesrecd, 0);
			if(retval==SOCKET_ERROR) _throwsock();
			if(retval==0) break;
		}
		bytesrecd+=retval;
	}
	if(bytesrecd!=len) _throw("Incomplete receive");
}

