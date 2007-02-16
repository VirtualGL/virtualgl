/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include "rrsocket.h"
#include "rrutil.h"
#include "rrtimer.h"

#define PORT 1972
#define MINDATASIZE 1
#define MAXDATASIZE (4*1024*1024)
#define ITER 5

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
	bool dossl=false;
	rrtimer timer;

	#ifdef USESSL
	#define usage() {printf("USAGE: %s -client <server name or IP> [-ssl]\n or    %s -server [-ssl]\n or    %s -findport\n\n-ssl = use secure tunnel\n-findport = display a free TCP port number and exit\n", argv[0], argv[0], argv[0]);  exit(1);}
	#else
	#define usage() {printf("USAGE: %s -client <server name or IP>\n or    %s -server\n or    %s -findport\n\n-findport = display a free TCP port number and exit\n", argv[0], argv[0], argv[0]);  exit(1);}
	#endif

	if(argc<2) usage();
	if(!stricmp(argv[1], "-client"))
	{
		if(argc<3) usage();
		server=0;  servername=argv[2];
		#ifdef USESSL
		if(argc>3 && !stricmp(argv[3], "-ssl")) {puts("Using SSL ...");  dossl=true;}
		#endif
	}
	else if(!stricmp(argv[1], "-server"))
	{
		server=1;
		#ifdef USESSL
		if(argc>2 && !stricmp(argv[2], "-ssl"))
		{
			dossl=true;
			puts("Using SSL ...");
		}
		#endif
	}
	else if(!stricmp(argv[1], "-findport"))
	{
		rrsocket sd(false);
		printf("%d\n", sd.listen(0, true));
		sd.close();
		exit(0);
	}
	else usage();

	try {

	rrsocket sd(dossl);
	if((buf=(char *)malloc(sizeof(char)*MAXDATASIZE))==NULL)
		{printf("Buffer allocation error.\n");  exit(1);}

	#ifdef USESSL
	if(dossl)
	{
		#if defined(OPENSSL_THREADS)
		printf("OpenSSL threads supported\n");
		#else
		printf("OpenSSL threads not supported\n");
		#endif
	}
	#endif

	if(server)
	{
		rrsocket *clientsd=NULL;

		printf("Listening on TCP port %d\n", PORT);
		sd.listen(PORT);
		clientsd=sd.accept();

		printf("Accepted TCP connection from %s\n", clientsd->remotename());

		for(i=MINDATASIZE; i<=MAXDATASIZE; i*=2)
		{
			for(j=0; j<ITER; j++)
			{
				clientsd->recv(buf, i);
				clientsd->send(buf, i);
			}
		}
		printf("Shutting down TCP connection ...\n");
		delete clientsd;
	}
	else
	{
		double elapsed;
		sd.connect(servername, PORT);

		printf("TCP transfer performance between localhost and %s:\n\n", sd.remotename());
		printf("Transfer size  1/2 Round-Trip      Throughput      Throughput\n");
		printf("(bytes)                (msec)        (MB/sec)     (Mbits/sec)\n");
		for(i=MINDATASIZE; i<=MAXDATASIZE; i*=2)
		{
			initbuf(buf, i);
			timer.start();
			for(j=0; j<ITER; j++)
			{
				sd.send(buf, i);
				sd.recv(buf, i);
			}
			elapsed=timer.elapsed();
			if(!cmpbuf(buf, i)) {printf("DATA ERROR\n");  exit(1);}
			printf("%-13d  %14.6f  %14.6f  %14.6f\n", i, elapsed/2.*1000./(double)ITER, (double)i*(double)ITER/1048576./(elapsed/2.), (double)i*(double)ITER/125000./(elapsed/2.));
		}
	}

	sd.close();
	free(buf);

	}
	catch(rrerror &e)
	{
		printf("Error in %s--\n%s\n", e.getMethod(), e.getMessage());
	}
	return 0;
}

