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

#define CERTF "rrcert.pem"

#ifdef _WIN32
	#define DIRSEP "\\"
	#define ALTDIR "."
#else
	#include <pwd.h>
	#define DIRSEP "/"
	#define ALTDIR "/etc"
#endif


char *gethomedir(void)
{
	#ifdef _WIN32
	return getenv("USERPROFILE");
	#else
	char *homedir=NULL;  struct passwd *pw;
	if((homedir=getenv("HOME"))!=NULL) return homedir;
	if((pw=getpwuid(getuid()))==NULL) return NULL;
	return pw->pw_dir;
	#endif
}


rrlistener::rrlistener(unsigned short port, bool dossl, bool dorecv)
{
	char temppath[1024];

	this->dossl=false;  if(dossl) this->dossl=true;
	this->dorecv=false;  if(dorecv) this->dorecv=true;
	sslctx=NULL;
	listen_socket=INVALID_SOCKET;
	listenthnd=0;
	clients=0;  deadyet=0;

	sock(hpnet_init());

	if(dossl)
	{
		strncpy(temppath, gethomedir(), 1024);  temppath[1024]=0;
		strncat(temppath, DIRSEP CERTF, 1024-strlen(temppath));
		hpprintf("Using certificate file %s\n", temppath);
		if((sslctx=hpsecnet_serverinit(temppath, temppath))==NULL)
		{
			hpprintf("%s\n", hpsecnet_strerror());
			#ifdef _WIN32
			if(GetModuleFileName(NULL, temppath, 1024))
			{
				char *ptr;
				if((ptr=strrchr(temppath, '\\'))!=NULL) *ptr='\0';
			}
			else
			#endif
			strcpy(temppath, ALTDIR);
			strcat(temppath, DIRSEP CERTF);
			hpprintf("Using certificate file %s\n", temppath);
			tryssl(sslctx=hpsecnet_serverinit(temppath, temppath));
		}
	}
	sock(listen_socket=hpnet_createServer(port==0?RR_DEFAULTPORT:port, MAXCONN, SOCK_STREAM));

	memset(cli, 0, sizeof(rrconn*)*MAXCONN);
	memset(clithnd, 0, sizeof(pthread_t)*MAXCONN);
	tryunix(sem_init(&clientsem, 0, 0));
	tryunix(pthread_mutex_init(&clientmutex, NULL));
	tryunix(pthread_create(&listenthnd, NULL, listener, (void *)this));
	clientthreadptr=clientthread;
}


rrlistener::~rrlistener(void)
{
	int i;
	deadyet=1;
	sem_post(&clientsem);
	if(clients) for(i=clients-1; i>=0; i--) removeclient(cli[i], true);
	pthread_mutex_lock(&clientmutex);
	clients=0;
	pthread_mutex_unlock(&clientmutex);
	if(listen_socket!=INVALID_SOCKET) {closesocket(listen_socket);  listen_socket=INVALID_SOCKET;}
	if(listenthnd) {pthread_join(listenthnd, NULL);  listenthnd=0;}
	if(sslctx && dossl) hpsecnet_term(sslctx);
	hpnet_term();
	sem_destroy(&clientsem);
	pthread_mutex_destroy(&clientmutex);
}


// Add a client to the pool
void rrlistener::addclient(rrconn *c)
{
	int i;
	rrltparam *tp=NULL;
	if(!c) return;

	rrlock l(clientmutex);

	if(clients) for(i=0; i<clients; i++)
		if(c==cli[i]) return;
	if(clients>=MAXCONN) _throw("Max. connection count exceeded");
	cli[clients]=c;
	errifnot(tp=new rrltparam);
	tp->rrl=this;  tp->c=c;  tp->clientrank=clients+1;
	tryunix(pthread_create(&clithnd[clients], NULL, clientthreadptr, (void *)tp));
	clients++;

	hpprintf("%s connected.  Number of connections is now %d.\n",
		cli[clients-1]->getremotename(), clients);
	if(dossl) hpprintf("SSL cipher: %s\n", cli[clients-1]->getsslcipher());
	sem_post(&clientsem);
}


// Remove a client from the pool
void rrlistener::removeclient(rrconn *c, bool waitforthread)
{
	int i, j;
	if(!c) return;

	rrlock l(clientmutex);
	if(!clients) return;
	for(i=0; i<clients; i++)
	{
		if(c==cli[i])
		{
			hpprintf("Disconnecting %s.", cli[i]->getremotename());
			delete cli[i];  cli[i]=NULL;
			if(clithnd[i])
			{
				if(waitforthread) pthread_join(clithnd[i], NULL);
				clithnd[i]=0;
			}
			if(i<clients-1)
				for(j=i; j<=clients-2; j++)
				{
					cli[j]=cli[j+1];  clithnd[j]=clithnd[j+1];
				}
			cli[clients-1]=NULL;  clithnd[clients-1]=0;
			clients--;
			tryunix(sem_wait(&clientsem));
			hpprintf("  Number of connections is now %d.\n", clients);
			break;
		}
	}
}


void rrlistener::broadcast(char *buf, int length)
{
	int i;
	rrlock l(clientmutex);
	if(clients) for(i=0; i<clients; i++) cli[i]->send(buf, length);
}


void *rrlistener::clientthread(void *param)
{
	rrltparam *tp=(rrltparam *)param;
	rrlistener *rrl=tp->rrl;
	rrconn *c=tp->c;
	int clientrank=tp->clientrank;
	delete tp;
	try
	{
		while(!rrl->deadyet) rrl->receive(c);
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


void *rrlistener::listener(void *param)
{
	rrlistener *rrl=(rrlistener *)param;
	rrconn *c=NULL;
	while(!rrl->deadyet)
	{
		try
		{
			errifnot(c=new rrconn());
			c->listen(rrl->listen_socket, rrl->sslctx);
			if(rrl->deadyet) break;
			rrl->addclient(c);
			continue;
		}
		catch(RRError e)
		{
			if(!rrl->deadyet)
			{
				hpprintf("Listener- %s (%d):\n%s\n", e.file, e.line, e.message);
				delete c;
				continue;
			}
		}
	}
	if(c) delete c;
	hpprintf("Listener exiting ...\n");
	return 0;
}


void rrlistener::waitforclient(void)
{
	tryunix(sem_wait(&clientsem));
	sem_post(&clientsem);
}
