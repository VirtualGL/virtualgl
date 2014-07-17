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

#include "Socket.h"
#include "Thread.h"

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

using namespace vglutil;


#ifdef _WIN32
typedef int SOCKLEN_T;
#else
typedef socklen_t SOCKLEN_T;
#endif

#ifdef USESSL
bool Socket::sslInit=false;
CriticalSection Socket::cryptoLock[CRYPTO_NUM_LOCKS];
#endif
CriticalSection Socket::mutex;
int Socket::instanceCount=0;


#ifdef USESSL

static void progressCallback(int p, int n, void *arg)
{
}


static EVP_PKEY *newPrivateKey(int bits)
{
	EVP_PKEY *pk=NULL;

	try
	{
		if(!(pk=EVP_PKEY_new())) _throwssl();
		if(!EVP_PKEY_assign_RSA(pk, RSA_generate_key(bits, 0x10001,
			progressCallback, NULL))) _throwssl();
		return pk;
	}
	catch (...)
	{
		if(pk) EVP_PKEY_free(pk);
		throw;
	}
	return NULL;
}


static X509 *newCert(EVP_PKEY *priv)
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

#endif // USESSL


Socket::Socket(bool doSSL_=false)
	#ifdef USESSL
	: doSSL(doSSL_)
	#endif
{
	CriticalSection::SafeLock l(mutex);

	#ifdef _WIN32
	if(instanceCount==0)
	{
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
			throw(Error("Socket::Socket()", "Winsock initialization failed"));
		if(LOBYTE(wsaData.wVersion)!=2 || HIBYTE(wsaData.wVersion)!=2)
			throw(Error("Socket::Socket()", "Wrong Winsock version"));
	}
	instanceCount++;
	#else
	if(signal(SIGPIPE, SIG_IGN)==SIG_ERR) _throwunix();
	#endif

	#ifdef USESSL
	if(!sslInit && doSSL)
	{
		#if defined(sun) || defined(sgi)
		char buf[128];  int i;
		srandom(getpid());
		for(i=0; i<128; i++)
			buf[i]=(char)((double)random()/(double)RAND_MAX*254.+1.0);
		RAND_seed(buf, 128);
		#endif
		OpenSSL_add_all_algorithms();
		SSL_load_error_strings();
		ERR_load_crypto_strings();
		CRYPTO_set_id_callback(Thread::threadID);
		CRYPTO_set_locking_callback(lockingCallback);
		SSL_library_init();
		sslInit=true;
		char *env=NULL;
		if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
			&& !strncmp(env, "1", 1))
			fprintf(stderr, "[VGL] Using OpenSSL version %s\n",
				SSLeay_version(SSLEAY_VERSION));
	}
	ssl=NULL;  sslctx=NULL;
	#endif

	sd=INVALID_SOCKET;
}


#ifdef USESSL
Socket::Socket(SOCKET sd_, SSL *ssl_)
	: sslctx(NULL), ssl(ssl_), sd(sd_)
{
	if(ssl) doSSL=true;  else doSSL=false;
	#ifdef _WIN32
	CriticalSection::SafeLock l(mutex);
	instanceCount++;
	#endif
}
#else
Socket::Socket(SOCKET sd_)
	: sd(sd_)
{
	#ifdef _WIN32
	CriticalSection::SafeLock l(mutex);
	instanceCount++;
	#endif
}
#endif


Socket::~Socket(void)
{
	close();
	#ifdef _WIN32
	mutex.lock(false);
	instanceCount--;  if(instanceCount==0) WSACleanup();
	mutex.unlock(false);
	#endif
}


void Socket::close(void)
{
	#ifdef USESSL
	if(ssl)
	{
		SSL_shutdown(ssl);  SSL_free(ssl);  ssl=NULL;
	}
	if(sslctx)
	{
		SSL_CTX_free(sslctx);  sslctx=NULL;
	}
	#endif
	if(sd!=INVALID_SOCKET)
	{
		#ifdef _WIN32
		closesocket(sd);
		#else
		shutdown(sd, 2);
		::close(sd);
		#endif
		sd=INVALID_SOCKET;
	}
}


void Socket::connect(char *serverName, unsigned short port)
{
	struct sockaddr_in servaddr;
	int m=1;  struct hostent *hent;
	if(serverName==NULL) _throw("Invalid argument");

	if(sd!=INVALID_SOCKET) _throw("Already connected");
	#ifdef USESSL
	if(ssl && sslctx && doSSL) _throw("SSL already connected");
	#endif

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(serverName);
	servaddr.sin_port=htons(port);

	if(servaddr.sin_addr.s_addr==INADDR_NONE)
	{
		if((hent=gethostbyname(serverName))==0) _throwsock();
		memcpy(&(servaddr.sin_addr), hent->h_addr_list[0], hent->h_length);
	}

	if((sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==INVALID_SOCKET)
		_throwsock();
	_sock(::connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)));
	_sock(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int)));

	#ifdef USESSL
	if(doSSL)
	{
		if((sslctx=SSL_CTX_new(SSLv23_client_method()))==NULL) _throwssl();
		if((ssl=SSL_new(sslctx))==NULL) _throwssl();
		if(!SSL_set_fd(ssl, (int)sd)) _throwssl();
		int ret=SSL_connect(ssl);
		if(ret!=1) throw(SSLError("Socket::connect", ssl, ret));
		SSL_set_connect_state(ssl);
	}
	#endif
}


unsigned short Socket::setupListener(unsigned short port, bool reuseAddr)
{
	int m=1, m2=reuseAddr? 1:0;  struct sockaddr_in myaddr;

	if(sd!=INVALID_SOCKET) _throw("Already connected");
	#ifdef USESSL
	if(ssl && sslctx && doSSL) _throw("SSL already connected");
	#endif

	if((sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==INVALID_SOCKET)
		_throwsock();
	_sock(setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&m,
		sizeof(int)));
	_sock(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&m2,
		sizeof(int)));

	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	myaddr.sin_port=(port==0)? 0:htons(port);

	_sock(bind(sd, (struct sockaddr *)&myaddr, sizeof(myaddr)));
	SOCKLEN_T n=sizeof(myaddr);
	_sock(getsockname(sd, (struct sockaddr *)&myaddr, &n));
	unsigned short actualPort=ntohs(myaddr.sin_port);
	return actualPort;
}


unsigned short Socket::findPort(void)
{
	return setupListener(0, false);
}


unsigned short Socket::listen(unsigned short port, bool reuseAddr)
{
	unsigned short actualPort=port;
	#ifdef USESSL
	X509 *cert=NULL;  EVP_PKEY *priv=NULL;
	#endif

	actualPort=setupListener(port, reuseAddr);

	_sock(::listen(sd, MAXCONN));

	#ifdef USESSL
	if(doSSL)
	{
		try
		{
			if((sslctx=SSL_CTX_new(SSLv23_server_method()))==NULL) _throwssl();
			_errifnot(priv=newPrivateKey(1024));
			_errifnot(cert=newCert(priv));
			if(SSL_CTX_use_certificate(sslctx, cert)<=0)
				_throwssl();
			if(SSL_CTX_use_PrivateKey(sslctx, priv)<=0)
				_throwssl();
			if(!SSL_CTX_check_private_key(sslctx)) _throwssl();
			if(priv) EVP_PKEY_free(priv);
			if(cert) X509_free(cert);
		}
		catch (...)
		{
			if(priv) EVP_PKEY_free(priv);
			if(cert) X509_free(cert);
			throw;
		}
	}
	#endif

	return actualPort;
}


Socket *Socket::accept(void)
{
	SOCKET clientsd;
	int m=1;  struct sockaddr_in remoteaddr;  SOCKLEN_T addrlen;
	addrlen=sizeof(remoteaddr);

	if(sd==INVALID_SOCKET) _throw("Not connected");
	#ifdef USESSL
	if(!sslctx && doSSL) _throw("SSL not initialized");
	#endif

	_sock(clientsd=::accept(sd, (struct sockaddr *)&remoteaddr,
		&addrlen));
	_sock(setsockopt(clientsd, IPPROTO_TCP, TCP_NODELAY, (char*)&m,
		sizeof(int)));

	#ifdef USESSL
	SSL *tempssl=NULL;
	if(doSSL)
	{
		if(!(tempssl=SSL_new(sslctx))) _throwssl();
		if(!(SSL_set_fd(tempssl, (int)clientsd))) _throwssl();
		int ret=SSL_accept(tempssl);
		if(ret!=1) throw(SSLError("Socket::accept", tempssl, ret));
		SSL_set_accept_state(tempssl);
	}
	return new Socket(clientsd, tempssl);
	#else
	return new Socket(clientsd);
	#endif
}


char *Socket::remoteName(void)
{
	struct sockaddr_in remoteaddr;  SOCKLEN_T addrlen=sizeof(remoteaddr);
	char *remoteName=NULL;

	_sock(getpeername(sd, (struct sockaddr *)&remoteaddr, &addrlen));
	remoteName=inet_ntoa(remoteaddr.sin_addr);
	return (remoteName? remoteName:(char *)"Unknown");
}


void Socket::send(char *buf, int len)
{
	if(sd==INVALID_SOCKET) _throw("Not connected");
	#ifdef USESSL
	if(doSSL && !ssl) _throw("SSL not connected");
	#endif
	int bytesSent=0, retval;
	while(bytesSent<len)
	{
		#ifdef USESSL
		if(doSSL)
		{
			retval=SSL_write(ssl, &buf[bytesSent], len);
			if(retval<=0) throw(SSLError("Socket::send", ssl, retval));
		}
		else
		#endif
		{
			retval=::send(sd, &buf[bytesSent], len-bytesSent, 0);
			if(retval==SOCKET_ERROR) _throwsock();
			if(retval==0) break;
		}
		bytesSent+=retval;
	}
	if(bytesSent!=len) _throw("Incomplete send");
}


void Socket::recv(char *buf, int len)
{
	if(sd==INVALID_SOCKET) _throw("Not connected");
	#ifdef USESSL
	if(doSSL && !ssl) _throw("SSL not connected");
	#endif
	int bytesRead=0, retval;
	while(bytesRead<len)
	{
		#ifdef USESSL
		if(doSSL)
		{
			retval=SSL_read(ssl, &buf[bytesRead], len);
			if(retval<=0) throw(SSLError("Socket::recv", ssl, retval));
		}
		else
		#endif
		{
			retval=::recv(sd, &buf[bytesRead], len-bytesRead, 0);
			if(retval==SOCKET_ERROR) _throwsock();
			if(retval==0) break;
		}
		bytesRead+=retval;
	}
	if(bytesRead!=len) _throw("Incomplete receive");
}

