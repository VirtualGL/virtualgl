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

// This program acts as both a performance and integrity test for the HPNET interfaces
// in the HPUTIL library

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
#endif
#include "hputil.h"
#ifdef SECNET
 #include "hpsecnet.h"
 #define OPENSSL_THREAD_DEFINES
 #include <openssl/opensslconf.h>
#endif

#define PORT 1972
#define MAXCONN 1
#define MINDATASIZE 1
#define MAXDATASIZE (4*1024*1024)
#define ITER 5

#define sock(f) {if((f)==SOCKET_ERROR) {fprintf(stderr, "%s failed in line %d:\n%s\n", #f, __LINE__, hpnet_strerror()); \
	exit(1);}}
#ifdef SECNET
 #define _throwssl(f) {fprintf(stderr, "%s failed in line %d:\n%s\n", f, __LINE__, hpsecnet_strerror()); \
 	exit(1);}
 #define tryssl(f) {if(!(f)) _throwssl(#f);}
#else
 #define _throwssl(f) {}
 #define tryssl(f) {}
#endif

void initbuf(char *buf, int len)
{
	int i;
	for(i=0; i<len; i++) buf[i]=(char)(i%256);
}

int cmpbuf(char *buf, int len)
{
	int i;
	for(i=0; i<len; i++) if(buf[i]!=(char)(i%256)) return 0;
	return 1;
}

int main(int argc, char **argv)
{
	int server;  char *servername=NULL;
	char *buf;  int i, j;
	#ifdef SECNET
	int dossl=0;
	#endif

	#ifdef SECNET
	#define usage() {printf("USAGE: %s -client <server name or IP> [-ssl]\n or    %s -server [-ssl]\n\n-ssl = use secure tunnel\n", argv[0], argv[0]);  exit(1);}
	#else
	#define usage() {printf("USAGE: %s -client <server name or IP>\n or    %s -server\n", argv[0], argv[0]);  exit(1);}
	#endif
	if(argc<2) usage();
	if(!stricmp(argv[1], "-client"))
	{
		if(argc<3) usage();
		server=0;  servername=argv[2];
		#ifdef SECNET
		if(argc>3 && !stricmp(argv[3], "-ssl")) {puts("Using SSL ...");  dossl=1;}
		#endif
	}
	else if(!stricmp(argv[1], "-server"))
	{
		server=1;
		#ifdef SECNET
		if(argc>2 && !stricmp(argv[2], "-ssl"))
		{
			dossl=1;
			puts("Using SSL ...");
		}
		#endif
	}
	else usage();

	sock(hpnet_init());
	if((buf=(char *)malloc(sizeof(char)*MAXDATASIZE))==NULL)
		{printf("Buffer allocation error.\n");  exit(1);}

	if(server)
	{
		int sd, clientsd;  struct sockaddr_in clientaddr;
		int retval, bufsize;
		#ifdef SECNET
		SSL_CTX *sslctx=NULL;  SSL *ssl=NULL;
		#endif

		printf("Listening on TCP port %d\n", PORT);
		#ifdef SECNET
		if(dossl) tryssl(sslctx=hpsecnet_serverinit());
		#endif
		sock(sd=hpnet_createServer(PORT, MAXCONN, SOCK_STREAM));
		sock(clientsd=hpnet_waitForConnection(sd, &clientaddr));
		printf("Accepted TCP connection from %s\n", inet_ntoa(clientaddr.sin_addr));
		#ifdef SECNET
		if(dossl)
		{
			tryssl(ssl=hpsecnet_accept(clientsd, sslctx));
			#if defined(THREADS)
			printf("OpenSSL threads supported\n");
			#else
			printf("OpenSSL threads not supported\n");
			#endif
		}
		#endif

		for(i=MINDATASIZE; i<=MAXDATASIZE; i*=2)
		{
			for(j=0; j<ITER; j++)
			{
				#ifdef SECNET
				if(dossl)
				{
					if((retval=hpsecnet_recv(ssl, buf, i))<=0) _throwssl("hpsecnet_recv");
					if(retval!=i) {printf("RECV error: expected %d bytes, got %d\n", i, retval);  exit(1);}
					if((retval=hpsecnet_send(ssl, buf, i))<=0) _throwssl("hpsecnet_send");
					if(retval!=i) {printf("SEND error: expected %d bytes, sent %d\n", i, retval);  exit(1);}
				}
				else
				{
				#endif
					sock(retval=hpnet_recv(clientsd, buf, i, &clientaddr));
					if(retval!=i) {printf("RECV error: expected %d bytes, got %d\n", i, retval);  exit(1);}
					sock(retval=hpnet_send(clientsd, buf, i, &clientaddr));
					if(retval!=i) {printf("SEND error: expected %d bytes, sent %d\n", i, retval);  exit(1);}
				#ifdef SECNET
				}
				#endif
			}
		}
		printf("Shutting down TCP connection ...\n");
		#ifdef SECNET
		if(dossl)
		{
			hpsecnet_disconnect(ssl);  hpsecnet_term(sslctx);
		}
		#endif
		sock(hpnet_killClient(clientsd));  sock(closesocket(sd));

		#ifdef SECNET
		if(!dossl)
		{
		#endif
		printf("Listening on UDP port %d\n", PORT);
		sock(sd=hpnet_createServer(PORT, MAXCONN, SOCK_DGRAM));
		sock(bufsize=hpnet_serverSyncBufsize(sd, &clientaddr));
		for(i=MINDATASIZE; i<=bufsize; i*=2)
		{
			for(j=0; j<ITER; j++)
			{
				sock(retval=hpnet_recv(sd, buf, i, &clientaddr));
				if(retval!=i) {printf("RECV error: expected %d bytes, got %d\n", i, retval);  exit(1);}
				sock(retval=hpnet_send(sd, buf, i, &clientaddr));
				if(retval!=i) {printf("SEND error: expected %d bytes, sent %d\n", i, retval);  exit(1);}
			}
		}
		printf("Shutting down UDP connection ...\n");
		sock(closesocket(sd));
		#ifdef SECNET
		}
		#endif
	}
	else
	{
		int sd;  struct sockaddr_in servaddr;
		int retval;  double elapsed;  int bufsize;
		#ifdef SECNET
		SSL_CTX *sslctx=NULL;  SSL *ssl=NULL;
		#endif

		#ifdef SECNET
		if(dossl) tryssl(sslctx=hpsecnet_clientinit());
		#endif
		sock(sd=hpnet_connect(servername, PORT, &servaddr, SOCK_STREAM));
		#ifdef SECNET
		if(dossl)
		{
			tryssl(ssl=hpsecnet_connect(sd, sslctx));
			#if defined(THREADS)
			printf("OpenSSL threads supported\n");
			#else
			printf("OpenSSL threads not supported\n");
			#endif
		}
		#endif

		printf("TCP transfer performance between localhost and %s:\n\n", inet_ntoa(servaddr.sin_addr));
		printf("Transfer size  1/2 Round-Trip      Throughput\n");
		printf("(bytes)                (msec)        (MB/sec)\n");
		hptimer_init();
		for(i=MINDATASIZE; i<=MAXDATASIZE; i*=2)
		{
			initbuf(buf, i);
			elapsed=hptime();
			for(j=0; j<ITER; j++)
			{
				#ifdef SECNET
				if(dossl)
				{
					if((retval=hpsecnet_send(ssl, buf, i))<=0) _throwssl("hpsecnet_send");
					if(retval!=i) {printf("SEND error: expected %d bytes, sent %d\n", i, retval);  exit(1);}
					if((retval=hpsecnet_recv(ssl, buf, i))<=0) _throwssl("hpsecnet_recv");
					if(retval!=i) {printf("RECV error: expected %d bytes, got %d\n", i, retval);  exit(1);}
				}
				else
				{
				#endif
					sock(retval=hpnet_send(sd, buf, i, &servaddr));
					if(retval!=i) {printf("SEND error: expected %d bytes, sent %d\n", i, retval);  exit(1);}
					sock(retval=hpnet_recv(sd, buf, i, &servaddr));
					if(retval!=i) {printf("RECV error: expected %d bytes, got %d\n", i, retval);  exit(1);}
				#ifdef SECNET
				}
				#endif
			}
			elapsed=hptime()-elapsed;
			if(!cmpbuf(buf, i)) {printf("DATA ERROR\n");  exit(1);}
			printf("%-13d  %14.6f  %14.6f\n", i, elapsed/2.*1000./(double)ITER, (double)i*(double)ITER/1048576./(elapsed/2.));
		}
		#ifdef SECNET
		if(dossl)
		{
			hpsecnet_disconnect(ssl);  hpsecnet_term(sslctx);
		}
		#endif
		sock(closesocket(sd));

		#ifdef SECNET
		if(!dossl)
		{
		#endif
		sock(sd=hpnet_connect(servername, PORT, &servaddr, SOCK_DGRAM));
		printf("\nUDP transfer performance between localhost and %s:\n\n", inet_ntoa(servaddr.sin_addr));
		printf("Transfer size  1/2 Round-Trip      Throughput\n");
		printf("(bytes)                (msec)        (MB/sec)\n");
		sock(bufsize=hpnet_clientSyncBufsize(sd, &servaddr));
		for(i=MINDATASIZE; i<=bufsize; i*=2)
		{
			initbuf(buf, i);
			elapsed=hptime();
			for(j=0; j<ITER; j++)
			{
				sock(retval=hpnet_send(sd, buf, i, &servaddr));
				if(retval!=i) {printf("SEND error: expected %d bytes, sent %d\n", i, retval);  exit(1);}
				sock(retval=hpnet_recv(sd, buf, i, &servaddr));
				if(retval!=i) {printf("RECV error: expected %d bytes, got %d\n", i, retval);  exit(1);}
			}
			elapsed=hptime()-elapsed;
			if(!cmpbuf(buf, i)) {printf("DATA ERROR\n");  exit(1);}
			printf("%-13d  %14.6f  %14.6f\n", i, elapsed/2.*1000./(double)ITER, (double)i*(double)ITER/1048576./(elapsed/2.));
		}
		sock(closesocket(sd));
		#ifdef SECNET
		}
		#endif
	}

	sock(hpnet_term());
	free(buf);
	return 0;
}
