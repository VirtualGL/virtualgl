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

#ifndef __RRLISTENER_H
#define __RRLISTENER_H

#define MAXCONN 1024

#include "rrconn.h"
#include "rr.h"
#include <semaphore.h>

class rrlistener
{
	public:

	rrlistener(unsigned short, bool, bool);
	virtual ~rrlistener(void);
	void waitforclient(void);
	void broadcast(char *, int);

	protected:

	virtual void receive(rrconn *)=0;
	int listen_socket;
	bool dossl, dorecv;  SSL_CTX *sslctx;
	rrconn *cli[MAXCONN];
	pthread_t listenthnd, clithnd[MAXCONN];
	int clients;
	int deadyet;
	sem_t clientsem;  pthread_mutex_t clientmutex;
	void addclient(rrconn *);
	void removeclient(rrconn *, bool);
	void* (*clientthreadptr)(void *);
	static void *listener(void *);
	static void *clientthread(void *);
};

typedef struct _rrltparam
{
	rrlistener *rrl;
	rrconn *c;
	int clientrank;
} rrltparam;

#endif
