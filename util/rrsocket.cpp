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
 #if defined(sun)||defined(sgi)
 #include <openssl/rand.h>
 #endif
#endif
#ifndef INADDR_NONE
 #define INADDR_NONE ((in_addr_t) 0xffffffff)
#endif

bool rrsocket::sslinit=false;
rrcs rrsocket::mutex, rrsocket::cryptolock[CRYPTO_NUM_LOCKS];
int rrsocket::instancecount=0;

rrsocket::rrsocket(bool _dossl=false) : dossl(_dossl)
{
	rrcs::safelock l(mutex);
	#ifdef _WIN32
	if(instancecount==0)
	{
		WSADATA wsaData;
		if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
			throw(rrerror("rrsocket::rrsocket()", "Winsock initialization failed"));
		if(LOBYTE(wsaData.wVersion)!=2 || HIBYTE(wsaData.wVersion)!=2)
			throw(rrerror("rrsocket::rrsocket()", "Wrong Winsock version"));
	}
	instancecount++;
	#else
	if(signal(SIGPIPE, SIG_IGN)==SIG_ERR) _throwunix();
	#endif
	if(!sslinit && dossl)
	{
		#if defined(sun)||defined(sgi)
		const char *buf="ajsdluasdb5nq457q67na867qm8v67vq4865j7bnq5q867n";
		#endif
		SSL_library_init();
		#if defined(sun)||defined(sgi)
		RAND_seed(buf, strlen(buf));
		#endif
		OpenSSL_add_all_algorithms();
		SSL_load_error_strings();
		ERR_load_crypto_strings();
		CRYPTO_set_id_callback(thread_id);
		CRYPTO_set_locking_callback(locking_callback);
		sslinit=true;
	}
	sd=INVALID_SOCKET;  ssl=NULL;  sslctx=NULL;
}

rrsocket::rrsocket(int _sd, SSL *_ssl)
	: sd(_sd), sslctx(NULL), ssl(_ssl)
{
	if(ssl) dossl=true;  else dossl=false;
}

rrsocket::~rrsocket(void)
{
	close();
	#ifdef _WIN32
	mutex.lock(false);
	instancecount--;  if(instancecount==0) WSACleanup();
	mutex.unlock(false);
	#endif
}

void rrsocket::close(void)
{
	if(ssl)	{SSL_shutdown(ssl);  SSL_free(ssl);  ssl=NULL;}
	if(sslctx) {SSL_CTX_free(sslctx);  sslctx=NULL;}
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

void rrsocket::connect(char *servername, unsigned short port)
{
	struct sockaddr_in servaddr;
	int m=1;  struct hostent *hent;
	if(servername==NULL) _throw("Invalid argument");

	if(sd!=INVALID_SOCKET) _throw("Already connected");
	if(ssl && sslctx && dossl) _throw("SSL already connected");

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(servername);
	servaddr.sin_port=htons(port);

	if(servaddr.sin_addr.s_addr==INADDR_NONE)
	{
		if((hent=gethostbyname(servername))==0) _throwsock();
		memcpy(&(servaddr.sin_addr), hent->h_addr_list[0], hent->h_length);
	}

	trysock( sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) );
	trysock( ::connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) );
	trysock( setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int)) );

	if(dossl)
	{
		if((sslctx=SSL_CTX_new(SSLv23_client_method()))==NULL) _throwssl();
		if((ssl=SSL_new(sslctx))==NULL) _throwssl();
		if(!SSL_set_fd(ssl, sd)) _throwssl();
		int ret=SSL_connect(ssl);
		if(ret!=1) throw(sslerror("rrsocket::connect", ssl, ret));
		SSL_set_connect_state(ssl);
	}
}

void rrsocket::listen(unsigned short port, char *certfile, char *privkeyfile)
{
	int m=1;  struct sockaddr_in myaddr;

	if(sd!=INVALID_SOCKET) _throw("Already connected");
	if(ssl && sslctx && dossl) _throw("SSL already connected");

	trysock( sd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) );
	trysock( setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&m, sizeof(int)) );
	trysock( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&m, sizeof(int)) );

	memset(&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	myaddr.sin_port=htons(port);

	trysock( bind(sd, (struct sockaddr *)&myaddr, sizeof(myaddr)) );
	trysock( ::listen(sd, MAXCONN) );

	if(dossl)
	{
		if((sslctx=SSL_CTX_new(SSLv23_server_method()))==NULL) _throwssl();
		if(SSL_CTX_use_certificate_file(sslctx, certfile, SSL_FILETYPE_PEM)<=0)
			_throwssl();
		if(SSL_CTX_use_PrivateKey_file(sslctx, privkeyfile, SSL_FILETYPE_PEM)<=0)
			_throwssl();
		if(!SSL_CTX_check_private_key(sslctx)) _throwssl();
	}
}

#if (defined(__GLIBC__)&&(__GLIBC__>1)||defined(sun))
typedef socklen_t SOCKLEN_T;
#else
typedef int SOCKLEN_T;
#endif

rrsocket *rrsocket::accept(void)
{
	int sd_client, m=1;  struct sockaddr_in remoteaddr;  SOCKLEN_T addrlen;
	addrlen=sizeof(remoteaddr);

	if(sd==INVALID_SOCKET) _throw("Not connected");
	if(!sslctx && dossl) _throw("SSL not initialized");

	trysock( sd_client=::accept(sd, (struct sockaddr *)&remoteaddr, &addrlen) );
	trysock( setsockopt(sd_client, IPPROTO_TCP, TCP_NODELAY, (char*)&m, sizeof(int)) );

	SSL *tempssl=NULL;
	if(dossl)
	{
		if(!(tempssl=SSL_new(sslctx))) _throwssl();
		if(!(SSL_set_fd(tempssl, sd_client))) _throwssl();
		int ret=SSL_accept(tempssl);
		if(ret!=1) throw(sslerror("rrsocket::accept", tempssl, ret));
		SSL_set_accept_state(tempssl);
	}
	return new rrsocket(sd_client, tempssl);
}

char *rrsocket::remotename(void)
{
	struct sockaddr_in remoteaddr;  SOCKLEN_T addrlen=sizeof(remoteaddr);
	char *remotename=NULL;
	trysock( getpeername(sd, (struct sockaddr *)&remoteaddr, &addrlen) );
	remotename=inet_ntoa(remoteaddr.sin_addr);
	return (remotename? remotename:(char *)"Unknown");
}

void rrsocket::send(char *buf, int len)
{
	if(sd==INVALID_SOCKET) _throw("Not connected");
	if(dossl && !ssl) _throw("SSL not connected");
	int bytessent=0, retval;
	while(bytessent<len)
	{
		if(dossl)
		{
			retval=SSL_write(ssl, &buf[bytessent], len);
			if(retval<=0) throw(sslerror("rrsocket::send", ssl, retval));
		}
		else
		{
			retval=::send(sd, &buf[bytessent], len-bytessent, 0);
			if(retval==SOCKET_ERROR) _throwsock();
			if(retval==0) break;
		}
		bytessent+=retval;
	}
	if(bytessent!=len) _throw("Incomplete send");
}

void rrsocket::recv(char *buf, int len)
{
	if(sd==INVALID_SOCKET) _throw("Not connected");
	if(dossl && !ssl) _throw("SSL not connected");
	int bytesrecd=0, retval;
	while(bytesrecd<len)
	{
		if(dossl)
		{
			retval=SSL_read(ssl, &buf[bytesrecd], len);
			if(retval<=0) throw(sslerror("rrsocket::recv", ssl, retval));
		}
		else
		{
			retval=::recv(sd, &buf[bytesrecd], len-bytesrecd, 0);
			if(retval==SOCKET_ERROR) _throwsock();
			if(retval==0) break;
		}
		bytesrecd+=retval;
	}
	if(bytesrecd!=len) _throw("Incomplete receive");
}

