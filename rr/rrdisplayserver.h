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

#include "rrsocket.h"
#include "rrcwin.h"

#define MAXWIN 1024

class rrdisplayserver : public Runnable
{
	public:

	rrdisplayserver(unsigned short, bool, int _drawmethod);
	virtual ~rrdisplayserver(void);

	private:

	void run(void);

	int drawmethod;
	rrsocket *listensd;
	Thread *t;
	bool deadyet;
};

class rrserver : public Runnable
{
	public:

	rrserver(rrsocket *_sd, int _drawmethod) : drawmethod(_drawmethod),
		windows(0), sd(_sd), t(NULL)
	{
		memset(rrw, 0, sizeof(rrcwin *)*MAXWIN);
		errifnot(t=new Thread(this));
		t->start();
	}

	virtual ~rrserver(void)
	{
		int i;
		winmutex.lock(false);
		for(i=0; i<windows; i++) {if(rrw[i]) {delete rrw[i];  rrw[i]=NULL;}}
		windows=0;
		winmutex.unlock(false);
		rrout.println("-- Disconnecting %s", sd->remotename());
		if(sd) {delete sd;  sd=NULL;}
	}

	private:

	void run(void);

	int drawmethod;
	rrcwin *rrw[MAXWIN];
	int windows;
	rrcwin *addwindow(int, Window);
	void delwindow(rrcwin *w);
	rrcs winmutex;
	rrsocket *sd;
	Thread *t;
};

#endif
