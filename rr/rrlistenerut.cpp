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

#include "rrlistener.h"
#include <stdio.h>
#include <signal.h>

#define PACKETSIZE 10240
#define ITER 10

class myrrconn : public rraconn
{
	public:

	myrrconn(void) {packetno=0;}
	char buf[PACKETSIZE];

	private:

	int packetno;

	void dispatch(void)
	{
		int i;
		packetno++;
		recv(buf, PACKETSIZE);
		for(i=0; i<PACKETSIZE; i++) if(buf[i]!=(char)i) throw("Recv error");
		send(buf, PACKETSIZE);
		if(packetno>=ITER) hpprintf("%d packets received from %s on %d & sent back\n", ITER, getremotename(), getsd());
	}
};

class myrrlistener : public rrlistener
{
	public:

	myrrlistener(unsigned short port, bool dossl, bool dorecv) : rrlistener(port, dossl, dorecv)
	{
		clientthreadptr=myrrlistener::clientthread;
	}

	private:

	int packetno;

	void receive(rrconn *c) {}

	static void *myrrlistener::clientthread(void *param)
	{
		rrltparam *tp=(rrltparam *)param;
		myrrlistener *rrl=(myrrlistener *)tp->rrl;
		rrconn *c=tp->c;
		int clientrank=tp->clientrank, packetno=0, i;
		char buf[PACKETSIZE];
		delete tp;
		try
		{
			while(!rrl->deadyet)
			{
				packetno++;
				c->recv(buf, PACKETSIZE);
				for(i=0; i<PACKETSIZE; i++) if(buf[i]!=(char)i) throw("Recv error");
				hpprintf("Packet %d received by listener from %s on %d\n", packetno, c->getremotename(), c->getsd());
			}
		}
		catch(RRError e)
		{
			if(!rrl->deadyet)
			{
				hpprintf("Client %d- %s (%d):\n%s\n", clientrank, e.file, e.line, e.message);
				rrl->removeclient(c, false);
			}
		}
		return 0;
	}
};

void sighandler(int sig)
{
	hpprintf("signal %d called\n", sig);
	perror("sig");
}

#define usage(a) { \
	printf("USAGE: %s -server\n", a); \
	printf("       %s -client <server machine> [client count]\n", a); \
	printf("       %s -sslserver\n", a); \
	printf("       %s -sslclient <server machine> [client count]\n", a);  exit(1);}

int main(int argc, char **argv)
{
	int i, server=1, clientcount=5;  bool ssl=false;
	char buf[PACKETSIZE];
	for(i=0; i<PACKETSIZE; i++) buf[i]=(char)i;
	if(argc<2) usage(argv[0]);
	if(!stricmp(argv[1], "-client"))
	{
		if(argc<3) usage(argv[0]);
		if(argc>3 && (clientcount=atoi(argv[3]))<1) usage(argv[0]);
		server=0;
	}
	else if(!stricmp(argv[1], "-sslclient"))
	{
		if(argc<3) usage(argv[0]);  ssl=true;
		if(argc>3 && (clientcount=atoi(argv[3]))<1) usage(argv[0]);
		server=0;
	}
	else if(!stricmp(argv[1], "-server")) server=1;
	else if(!stricmp(argv[1], "-sslserver")) {ssl=true;  server=1;}

	else usage(argv[0]);
	hputil_log(1,0);
	signal(SIGABRT, sighandler);
	try
	{
		if(server)
		{
			myrrlistener *l;
			errifnot(l=new myrrlistener(1492, ssl, true));
			hpprintf("Server active.  Waiting for clients ...\n");
			l->waitforclient();
			sleep(5);
			for(i=0; i<ITER; i++)
			{
				hpprintf("Sending packet %d to all clients\n", i+1);
				l->broadcast(buf, PACKETSIZE);
			}
			sleep(5);
			hpprintf("Stopping listener ...\n");
			delete l;
		}
		else
		{
			myrrconn *c=new myrrconn[clientcount];
			errifnot(c);
			for(i=0; i<clientcount; i++)
			{
				c[i].connect(argv[2], 1492, ssl);
			}
			sleep(15);
			hpprintf("Disconnecting all clients ...\n");
			delete [] c;
		}
		hpprintf("Exiting ...\n");
	}
	catch(RRError e)
	{
		fprintf(stderr, "Main- %s (%d):\n%s\n", e.file, e.line, e.message);
		exit(1);
	}
	return 0;
}
