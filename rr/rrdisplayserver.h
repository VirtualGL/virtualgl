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

#ifndef __RRDISPLAYSERVER_H
#define __RRDISPLAYSERVER_H

#include "rrlistener.h"
#include "rrframe.h"
#include "rrcwin.h"

class rrdisplayserver : rrlistener
{
	public:

	rrdisplayserver(unsigned short port, bool dossl)
		: rrlistener(port, dossl, true)
	{
		hptimer_init();
		clientthreadptr=clientthread;
	}

	private:

	void receive(rrconn *) {}
	static void *clientthread(void *);
};

class rrclient
{
	public:

	rrclient(void)
	{
		memset(rrw, 0, sizeof(rrcwin *)*MAXWIN);
		windows=0;
		tryunix(pthread_mutex_init(&winmutex, NULL));
	}

	~rrclient(void)
	{
		int i;
		pthread_mutex_lock(&winmutex);
		for(i=0; i<windows; i++) {if(rrw[i]) {delete rrw[i];  rrw[i]=NULL;}}
		windows=0;
		pthread_mutex_unlock(&winmutex);  pthread_mutex_destroy(&winmutex);
	}

	private:

	friend class rrdisplayserver;
	rrcwin *rrw[MAXWIN];
	int windows;
	rrcwin *addwindow(int, Window);
	void delwindow(rrcwin *w);
	void receive(rrconn *c);
	pthread_mutex_t winmutex;
};

#endif
