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

#ifndef __RRCONN_H
#define __RRCONN_H

#include "hputil.h"
#include "hpsecnet.h"
#include <pthread.h>
#include "rrcommon.h"
#include <string.h>
#ifndef _WIN32
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
#endif

#define MAXERRORS 10

// Generic connection, sans asynchronous receive

class rrconn
{
	public:

	rrconn(void) {sock(hpnet_init());  init();}
	virtual ~rrconn(void) {disconnect();}

	void listen(int listen_socket, SSL_CTX *sslctx)
	{
		sock(sd=hpnet_waitForConnection(listen_socket, &sin));
		if(sslctx)
		{
			dossl=true;
			tryssl(ssl=hpsecnet_accept(sd, sslctx));
			hpprintf("SSL connection using %s\n", SSL_get_cipher(ssl));
		}
	}

	void connect(char *servername, unsigned short port, bool dossl)
	{
		if(!servername) _throw("Invalid argument to connect()");
		if(sd!=INVALID_SOCKET) _throw("Already connected");
		sock(sd=hpnet_connect(servername, port, &sin, SOCK_STREAM));
		if(dossl)
		{
			this->dossl=true;
			tryssl(clientctx=hpsecnet_clientinit());
			tryssl(ssl=hpsecnet_connect(sd, clientctx));
		}
	}

	void recv(char *buf, int length)
	{
		int retval=0;
		if(!buf || length<=0) _throw("Invalid argument to recv()");
		if(sd==INVALID_SOCKET || (dossl && !ssl))
			_throw("Cannot receive: connection not initialized");
		if(dossl) retval=hpsecnet_recv(ssl, buf, length);
		else retval=hpnet_recv(sd, buf, length, &sin);
		if(retval<=0 && dossl) {_throw(hpsecnet_strerror());}
		else if(retval==SOCKET_ERROR) {_throw(hpnet_strerror());}
		if(retval!=length) _throw("Incomplete receive");
	}

	void send(char *buf, int length)
	{
		int retval=0;
		if(!buf || length<=0) _throw("Invalid argument to send()");
		if(sd==INVALID_SOCKET || (dossl && !ssl))
			_throw("Cannot send: connection not initialized");
		if(dossl) retval=hpsecnet_send(ssl, buf, length);
		else retval=hpnet_send(sd, buf, length, &sin);
		if(retval<=0 && dossl) {_throw(hpsecnet_strerror());}
		else if(retval==SOCKET_ERROR) {_throw(hpnet_strerror());}
		if(retval!=length) _throw("Incomplete send");
	}

	void disconnect(void)
	{
		if(dossl && ssl) hpsecnet_disconnect(ssl);
		if(sd!=INVALID_SOCKET) closesocket(sd);
		init();
	}

	const char *getremotename(void)
	{
		if(sd!=INVALID_SOCKET) return inet_ntoa(sin.sin_addr);
		else return "Not connected";
	}

	const char *getsslcipher(void)
	{
		if(ssl) return SSL_get_cipher(ssl);
		else return "Not connected with SSL";
	}

	int getsd(void)
	{
		return sd;
	}

	#if 0
	rrconn& operator= (const rrconn& r)
	{
		if(this!=&r)
		{
			sd=r.sd;  ssl=r.ssl;  memcpy(&sin, &r.sin, sizeof(sin));
			clientctx=r.clientctx;  dossl=r.dossl;
		}
		return *this;
	}

	bool operator== (const rrconn& r)
	{
		return(sd==r.sd && ssl==r.ssl && clientctx==r.clientctx && dossl==r.dossl
			&& !memcmp(&sin, &r.sin, sizeof(sin)));
	}
	#endif

	protected:

	void init(void)
	{
		ssl=NULL;  sd=INVALID_SOCKET;  memset(&sin, 0, sizeof(sin));
		dossl=false;  clientctx=NULL;
	}

	bool dossl;
	int sd;
	SSL *ssl;  SSL_CTX *clientctx;
	struct sockaddr_in sin;
};


// This one can do async receives (or sends).  Override dispatch() to insert
// your own handler

class rraconn : public rrconn
{
	public:

	rraconn(void) : rrconn() {init();}
	virtual ~rraconn(void) {disconnect();}

	void connect(char *servername, unsigned short port, bool dossl)
	{
		rrconn::connect(servername, port, dossl);
		startdispatcher();
	}

	void listen(int listen_socket, SSL_CTX *sslctx)
	{
		rrconn::listen(listen_socket, sslctx);
		startdispatcher();
	}

	protected:

	void disconnect(void)
	{
		deadyet=1;
		if(dispatchthnd)
		{
			pthread_join(dispatchthnd, NULL);
			dispatchthnd=0;
		}
		if(dossl && ssl) hpsecnet_disconnect(ssl);
		if(sd!=INVALID_SOCKET) closesocket(sd);
		init();
	}

	void init(void)
	{
		const RRError noerror={0, 0, 0};
		rrconn::init();
		dispatchthnd=0;  deadyet=1;  lasterror=noerror;
	}

	void startdispatcher(void)
	{
		deadyet=0;
		tryunix(pthread_mutex_init(&lemutex, NULL));
		if(!dispatchthnd)
			tryunix(pthread_create(&dispatchthnd, NULL, dispatcher, (void *)this));
	}

	void setlasterror(RRError e)
	{
		rrlock l(lemutex);
		lasterror=e;
	}

	RRError getlasterror(void)
	{
		const RRError noerror={0, 0, 0};
		RRError retval;  retval.message=NULL;
		rrlock l(lemutex);
		retval=lasterror;
		if(lasterror.message) lasterror=noerror;
		return retval;
	}

	// Override me!
	virtual void dispatch(void)=0;

	int deadyet;
	RRError lasterror;
	pthread_mutex_t lemutex;
	static void *dispatcher(void *);
	pthread_t dispatchthnd;
};	

#endif

