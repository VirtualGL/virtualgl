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

#include <stdio.h>
#include <stdlib.h>
#include "rrsocket.h"
#include "rrutil.h"
#include "rrtimer.h"
#ifdef sun
#include <kstat.h>
#endif
#define PORT 1972
#define MINDATASIZE 1
#define MAXDATASIZE (4*1024*1024)
#define ITER 5

#if defined(sun)||defined(linux)
void benchmark(int interval, char *ifname)
{
	double rMbits=0., wMbits=0.;
	unsigned long long rbytes=0, wbytes=0;

	#ifdef sun

	kstat_ctl_t *kc=NULL;
	kstat_t *kif=NULL;
	if((kc=kstat_open())==NULL) {_throwunix();}
	if((kif=kstat_lookup(kc, NULL, -1, ifname))==NULL) {_throwunix();}

	#elif defined(linux)

	FILE *f=NULL;
	if((f=fopen("/proc/net/dev", "r"))==NULL)
		_throw("Could not open /proc/net/dev");

	#endif

	double tStart=rrtime();
	for(;;)
	{
		double tEnd=rrtime();

		#ifdef sun

		kstat_named_t *data=NULL;
		if(kstat_read(kc, kif, NULL)<0) _throwunix();
		if((data=(kstat_named_t *)kstat_data_lookup(kif, "rbytes64"))==NULL)
			_throwunix();
		if(rbytes!=0)
		{
			if(rbytes>data->value.ui64)
				rMbits+=((double)data->value.ui64+4294967296.-(double)rbytes)*8./1000000.;
			else
				rMbits+=((double)data->value.ui64-(double)rbytes)*8./1000000.;
		}
		rbytes=data->value.ui64;
		if((data=(kstat_named_t *)kstat_data_lookup(kif, "obytes64"))==NULL)
			_throwunix();
		if(wbytes!=0)
		{
			if(wbytes>data->value.ui64)
				wMbits+=((double)data->value.ui64+4294967296.-(double)wbytes)*8./1000000.;
			else
				wMbits+=((double)data->value.ui64-(double)wbytes)*8./1000000.;
		}
		wbytes=data->value.ui64;

		#elif defined(linux)

		char temps[1024];
		if(fseek(f, 0, SEEK_SET)!=0) _throwunix();
		bool read=false;
		while(fgets(temps, 1024, f))
		{
			unsigned long long dummy[16];  char ifstr[80], *ptr;
			if((ptr=strchr(temps, ':'))!=NULL) *ptr=' ';
			if(sscanf(temps,
				"%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
				ifstr, &dummy[0], &dummy[1], &dummy[2], &dummy[3], &dummy[4],
				&dummy[5], &dummy[6], &dummy[7], &dummy[8], &dummy[9], &dummy[10],
				&dummy[11], &dummy[12], &dummy[13], &dummy[14], &dummy[15])
				==17 && !strcmp(ifname, ifstr))
			{
				if(rbytes!=0) rMbits+=((double)dummy[0]-(double)rbytes)*8./1000000.;
				if(wbytes!=0) wMbits+=((double)dummy[8]-(double)wbytes)*8./1000000.;
				rbytes=dummy[0];  wbytes=dummy[8];  read=true;
			}
		}
		if(!read) _throw("Cannot parse statistics for requested interface from /proc/net/dev.");

		#endif

		if((tEnd-tStart)>=interval)
		{
			printf("Read: %f Mbps   Write: %f Mbps   Total: %f Mbps\n",
				rMbits/(tEnd-tStart), wMbits/(tEnd-tStart),
				(rMbits+wMbits)/(tEnd-tStart));
			rMbits=wMbits=0.;
			tStart=tEnd;
		}
		usleep(500000);
	}
}
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

void usage(char **argv)
{
	printf("USAGE: %s -client <server name or IP>", argv[0]);
	#ifdef USESSL
	printf(" [-ssl]");
	#endif
	printf(" [-old]");
	printf("\n or    %s -server", argv[0]);
	#ifdef USESSL
	printf(" [-ssl]");
	#endif
	printf("\n or    %s -findport\n", argv[0]);
	#if defined(sun)||defined(linux)
	printf(" or    %s -bench <interface> [interval]\n", argv[0]);
	printf("\n-bench = measure throughput on selected network interface");
	#endif
	printf("\n-findport = display a free TCP port number and exit");
	printf("\n-old = communicate with NetTest server v2.1.x or earlier\n");
	#ifdef USESSL
	printf("-ssl = use secure tunnel\n");
	#endif
	exit(1);
}

int main(int argc, char **argv)
{
	int server=0;  char *servername=NULL;
	char *buf;  int i, j, size;
	bool dossl=false, old=false;
	rrtimer timer;
	#if defined(sun)||defined(linux)
	int interval=2;
	#endif

	try {

	if(argc<2) usage(argv);
	if(!stricmp(argv[1], "-client"))
	{
		if(argc<3) usage(argv);
		server=0;  servername=argv[2];
		if(argc>3)
		{
			for(i=3; i<argc; i++)
			{
				#ifdef USESSL
				if(!stricmp(argv[i], "-ssl"))
				{
					printf("Using %s ...\n", SSLeay_version(SSLEAY_VERSION));
					dossl=true;
				}
				#endif
				if(!stricmp(argv[i], "-old"))
				{
					printf("Using old protocol\n");
					old=true;
				}
			}
		}
	}
	else if(!stricmp(argv[1], "-server"))
	{
		server=1;
		#ifdef USESSL
		if(argc>2 && !stricmp(argv[2], "-ssl"))
		{
			dossl=true;
			printf("Using %s ...\n", SSLeay_version(SSLEAY_VERSION));
		}
		#endif
	}
	else if(!stricmp(argv[1], "-findport"))
	{
		rrsocket sd(false);
		printf("%d\n", sd.findport());
		sd.close();
		exit(0);
	}
	#if defined(sun)||defined(linux)
	else if(!stricmp(argv[1], "-bench"))
	{
		int _interval;
		if(argc<3) usage(argv);
		if(argc>3 && ((_interval=atoi(argv[3]))>0))
			interval=_interval;
		benchmark(interval, argv[2]);
		exit(0);
	}
	#endif
	else usage(argv);

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
		sd.listen(PORT, true);
		clientsd=sd.accept();

		printf("Accepted TCP connection from %s\n", clientsd->remotename());

		clientsd->recv(buf, 1);
		if(buf[0]=='V')
		{
			clientsd->recv(&buf[1], 4);
			if(strcmp(buf, "VGL22")) _throw("Invalid header");
			while(1)
			{
				clientsd->recv((char *)&size, (int)sizeof(int));
				if(!littleendian()) size=byteswap(size);
				if(size<1) break;
				while(1)
				{
					clientsd->recv(buf, size);
					if((unsigned char)buf[0]==255) break;
					clientsd->send(buf, size);
				}
			}
		}
		else
		{
			for(i=MINDATASIZE; i<=MAXDATASIZE; i*=2)
			{
				for(j=0; j<ITER; j++)
				{
					if(i!=MINDATASIZE || j!=0) clientsd->recv(buf, i);
					clientsd->send(buf, i);
				}
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
		printf("(bytes)                (msec)    (MBytes/sec)     (Mbits/sec)\n");

		if(old)
		{
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
				if(!cmpbuf(buf, i)) {printf("DATA ERROR\n"); exit(1);}
				printf("%-13d  %14.6f  %14.6f  %14.6f\n", i, elapsed/2.*1000./(double)ITER,
					(double)i*(double)ITER/1048576./(elapsed/2.),
					(double)i*(double)ITER/125000./(elapsed/2.));
			}
			sd.close();
			free(buf);
			return 0;
		}

		char id[6]="VGL22";
		sd.send(id, 5);
		for(i=MINDATASIZE; i<=MAXDATASIZE; i*=2)
		{
			size=i;
			if(!littleendian()) size=byteswap(size);
			sd.send((char *)&size, (int)sizeof(int));
			initbuf(buf, i);
			j=0;
			timer.start();
			do
			{
				sd.send(buf, i);
				sd.recv(buf, i);
				j++;
				elapsed=timer.elapsed();
			} while(elapsed<2.0);
			if(!cmpbuf(buf, i)) {printf("DATA ERROR\n");  exit(1);}
			buf[0]=255;
			sd.send(buf, i);
			printf("%-13d  %14.6f  %14.6f  %14.6f\n", i, elapsed/2.*1000./(double)j,
				(double)i*(double)j/1048576./(elapsed/2.),
				(double)i*(double)j/125000./(elapsed/2.));
		}
		size=0;
		if(!littleendian()) size=byteswap(size);
		sd.send((char *)&size, (int)sizeof(int));
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

