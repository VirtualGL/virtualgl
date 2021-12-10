// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2014, 2017-2019, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include <stdio.h>
#include <stdlib.h>
#include "Socket.h"
#include "vglutil.h"
#include "Timer.h"
#ifdef sun
#include <kstat.h>
#endif

using namespace util;


#define PORT  1972
#define MINDATASIZE  1
#define MAXDATASIZE  (4 * 1024 * 1024)
#define ITER  5


double benchTime = 2.0;


#if defined(sun) || defined(linux)

void benchmark(int interval, char *ifname)
{
	double mbitsRead = 0., mbitsSent = 0.;
	unsigned long long bytesRead = 0, bytesSent = 0;

	#ifdef sun

	kstat_ctl_t *kc = NULL;
	kstat_t *kif = NULL;
	if((kc = kstat_open()) == NULL) { THROW_UNIX(); }
	if((kif = kstat_lookup(kc, NULL, -1, ifname)) == NULL) { THROW_UNIX(); }

	#elif defined(linux)

	FILE *f = NULL;
	if((f = fopen("/proc/net/dev", "r")) == NULL)
		THROW("Could not open /proc/net/dev");

	#endif

	double tStart = GetTime();
	for(;;)
	{
		double tEnd = GetTime();

		#ifdef sun

		kstat_named_t *data = NULL;
		if(kstat_read(kc, kif, NULL) < 0) THROW_UNIX();
		if((data =
			(kstat_named_t *)kstat_data_lookup(kif, (char *)"rbytes64")) == NULL)
			THROW_UNIX();
		if(bytesRead != 0)
		{
			if(bytesRead > data->value.ui64)
				mbitsRead += ((double)data->value.ui64 + 4294967296. -
					(double)bytesRead) * 8. / 1000000.;
			else
				mbitsRead += ((double)data->value.ui64 -
					(double)bytesRead) * 8. / 1000000.;
		}
		bytesRead = data->value.ui64;
		if((data =
			(kstat_named_t *)kstat_data_lookup(kif, (char *)"obytes64")) == NULL)
			THROW_UNIX();
		if(bytesSent != 0)
		{
			if(bytesSent > data->value.ui64)
				mbitsSent += ((double)data->value.ui64 + 4294967296. -
					(double)bytesSent) * 8. / 1000000.;
			else
				mbitsSent += ((double)data->value.ui64 -
					(double)bytesSent) * 8. / 1000000.;
		}
		bytesSent = data->value.ui64;

		#elif defined(linux)

		char temps[1024];
		if(fseek(f, 0, SEEK_SET) != 0) THROW_UNIX();
		bool isRead = false;
		while(fgets(temps, 1024, f))
		{
			unsigned long long dummy[16];  char ifstr[80], *ptr;
			if((ptr = strchr(temps, ':')) != NULL) *ptr = ' ';
			if(sscanf(temps,
				"%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
				ifstr, &dummy[0], &dummy[1], &dummy[2], &dummy[3], &dummy[4],
				&dummy[5], &dummy[6], &dummy[7], &dummy[8], &dummy[9], &dummy[10],
				&dummy[11], &dummy[12], &dummy[13], &dummy[14], &dummy[15]) == 17
				&& !strcmp(ifname, ifstr))
			{
				if(bytesRead != 0)
					mbitsRead += ((double)dummy[0] - (double)bytesRead) * 8. / 1000000.;
				if(bytesSent != 0)
					mbitsSent += ((double)dummy[8] - (double)bytesSent) * 8. / 1000000.;
				bytesRead = dummy[0];  bytesSent = dummy[8];  isRead = true;
			}
		}
		if(!isRead)
			THROW("Cannot parse statistics for requested interface from /proc/net/dev.");

		#endif

		if((tEnd - tStart) >= interval)
		{
			printf("Read: %f Mbps   Write: %f Mbps   Total: %f Mbps\n",
				mbitsRead / (tEnd - tStart), mbitsSent / (tEnd - tStart),
				(mbitsRead + mbitsSent) / (tEnd - tStart));
			mbitsRead = mbitsSent = 0.;
			tStart = tEnd;
		}
		usleep(500000);
	}
}

#endif  // defined(sun) || defined(linux)


void initBuf(char *buf, int len)
{
	int i;
	for(i = 0; i < len; i++) buf[i] = (char)(i % 256);
}


int cmpBuf(char *buf, int len)
{
	int i;
	for(i = 0; i < len; i++)
		if(buf[i] != (char)(i % 256)) return 0;
	return 1;
}


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s -client <server name or IP>", argv[0]);
	fprintf(stderr, " [-old] [-time <t>]");
	fprintf(stderr, "\n or    %s -server [-ipv6]", argv[0]);
	fprintf(stderr, "\n or    %s -findport\n", argv[0]);
	#if defined(sun) || defined(linux)
	fprintf(stderr, " or    %s -bench <interface> [interval]\n", argv[0]);
	fprintf(stderr, "\n-bench = Measure throughput on selected network interface");
	#endif
	fprintf(stderr, "\n-findport = Display a free TCP port number and exit");
	fprintf(stderr, "\n-old = Communicate with NetTest server v2.1.x or earlier\n");
	fprintf(stderr, "-ipv6 = Use IPv6 sockets\n");
	fprintf(stderr, "-time <t> = Run each benchmark for <t> seconds (default: %.1f)\n",
		benchTime);
	fprintf(stderr, "            (NOTE: this must be specified on the client)\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int server = 0;  char *serverName = NULL;
	Socket *clientSocket = NULL;
	char *buf = NULL;  int i, j, size;
	bool ipv6 = false, old = false;
	Timer timer;
	#if defined(sun) || defined(linux)
	int interval = 2;
	#endif

	try
	{
		if(argc < 2) usage(argv);

		if(!stricmp(argv[1], "-client"))
		{
			if(argc < 3) usage(argv);
			server = 0;  serverName = argv[2];
			if(argc > 3) for(i = 3; i < argc; i++)
			{
				if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
				else if(!stricmp(argv[i], "-old"))
				{
					printf("Using old protocol\n");
					old = true;
				}
				else if(!stricmp(argv[i], "-time") && i < argc - 1)
				{
					if(sscanf(argv[++i], "%lf", &benchTime) < 1 || benchTime <= 0.0)
						usage(argv);
				}
				else usage(argv);
			}
		}
		else if(!stricmp(argv[1], "-server"))
		{
			server = 1;
			if(argc > 2) for(i = 2; i < argc; i++)
			{
				if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
				else if(!stricmp(argv[i], "-ipv6"))
				{
					ipv6 = true;
					printf("Using IPv6 sockets ...\n");
				}
				else usage(argv);
			}
		}
		else if(!stricmp(argv[1], "-findport"))
		{
			Socket socket(false);
			printf("%d\n", socket.findPort());
			socket.close();
			exit(0);
		}
		#if defined(sun) || defined(linux)
		else if(!stricmp(argv[1], "-bench"))
		{
			if(argc < 3 || argc > 4) usage(argv);
			if(argc > 3)
			{
				int interval_ = atoi(argv[3]);
				if(interval_ <= 0) usage(argv);
				interval = interval_;
			}
			benchmark(interval, argv[2]);
			exit(0);
		}
		#endif
		else usage(argv);

		Socket socket(ipv6);
		if((buf = (char *)malloc(sizeof(char) * MAXDATASIZE)) == NULL)
		{
			printf("Buffer allocation error.\n");  exit(1);
		}

		if(server)
		{
			printf("Listening on TCP port %d\n", PORT);
			socket.listen(PORT, true);
			clientSocket = socket.accept();

			printf("Accepted TCP connection from %s\n", clientSocket->remoteName());

			clientSocket->recv(buf, 1);
			if(buf[0] == 'V')
			{
				clientSocket->recv(&buf[1], 4);
				buf[5] = 0;
				if(strcmp(buf, "VGL22")) THROW("Invalid header");
				while(1)
				{
					clientSocket->recv((char *)&size, (int)sizeof(int));
					if(!LittleEndian()) size = BYTESWAP(size);
					if(size < 1) break;
					while(1)
					{
						clientSocket->recv(buf, size);
						if((unsigned char)buf[0] == 255) break;
						clientSocket->send(buf, size);
					}
				}
			}
			else
			{
				for(i = MINDATASIZE; i <= MAXDATASIZE; i *= 2)
				{
					for(j = 0; j < ITER; j++)
					{
						if(i != MINDATASIZE || j != 0) clientSocket->recv(buf, i);
						clientSocket->send(buf, i);
					}
				}
			}
			printf("Shutting down TCP connection ...\n");
			delete clientSocket;
		}
		else
		{
			double elapsed;
			socket.connect(serverName, PORT);

			printf("TCP transfer performance between localhost and %s:\n\n",
				socket.remoteName());
			printf("Transfer size  1/2 Round-Trip      Throughput      Throughput\n");
			printf("(bytes)                (msec)    (MBytes/sec)     (Mbits/sec)\n");

			if(old)
			{
				for(i = MINDATASIZE; i <= MAXDATASIZE; i *= 2)
				{
					initBuf(buf, i);
					timer.start();
					for(j = 0; j < ITER; j++)
					{
						socket.send(buf, i);
						socket.recv(buf, i);
					}
					elapsed = timer.elapsed();
					if(!cmpBuf(buf, i))
					{
						printf("DATA ERROR\n");  exit(1);
					}
					printf("%-13d  %14.6f  %14.6f  %14.6f\n", i,
						elapsed / 2. * 1000. / (double)ITER,
						(double)i * (double)ITER / 1048576. / (elapsed / 2.),
						(double)i * (double)ITER / 125000. / (elapsed / 2.));
				}
				socket.close();
				free(buf);
				return 0;
			}

			char id[6] = "VGL22";
			socket.send(id, 5);
			for(i = MINDATASIZE; i <= MAXDATASIZE; i *= 2)
			{
				size = i;
				if(!LittleEndian()) size = BYTESWAP(size);
				socket.send((char *)&size, (int)sizeof(int));
				initBuf(buf, i);
				j = 0;
				timer.start();
				do
				{
					socket.send(buf, i);
					socket.recv(buf, i);
					j++;
					elapsed = timer.elapsed();
				} while(elapsed < benchTime);
				if(!cmpBuf(buf, i))
				{
					printf("DATA ERROR\n");  exit(1);
				}
				buf[0] = (char)255;
				socket.send(buf, i);
				printf("%-13d  %14.6f  %14.6f  %14.6f\n", i,
					elapsed / 2. * 1000. / (double)j,
					(double)i * (double)j / 1048576. / (elapsed / 2.),
					(double)i * (double)j / 125000. / (elapsed / 2.));
			}
			size = 0;
			if(!LittleEndian()) size = BYTESWAP(size);
			socket.send((char *)&size, (int)sizeof(int));
		}

		socket.close();
		free(buf);
	}
	catch(std::exception &e)
	{
		printf("Error in %s--\n%s\n", GET_METHOD(e), e.what());
		free(buf);
		delete clientSocket;
		return -1;
	}
	return 0;
}
