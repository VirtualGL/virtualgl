/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

	int _drawmethod;
	rrsocket *_listensd;
	Thread *_t;
	bool _deadyet;
	bool _dossl;
};

class rrserver : public Runnable
{
	public:

	rrserver(rrsocket *sd, int drawmethod) : _drawmethod(drawmethod),
		_windows(0), _sd(sd), _t(NULL), _remotename(NULL)
	{
		memset(_rrw, 0, sizeof(rrcwin *)*MAXWIN);
		if(_sd) _remotename=_sd->remotename();
		errifnot(_t=new Thread(this));
		_t->start();
	}

	virtual ~rrserver(void)
	{
		int i;
		_winmutex.lock(false);
		for(i=0; i<_windows; i++) {if(_rrw[i]) {delete _rrw[i];  _rrw[i]=NULL;}}
		_windows=0;
		_winmutex.unlock(false);
		if(!_remotename) rrout.PRINTLN("-- Disconnecting\n");
		else rrout.PRINTLN("-- Disconnecting %s", _remotename);
		if(_sd) {delete _sd;  _sd=NULL;}
	}

	void send(char *, int);
	void recv(char *, int);

	private:

	void run(void);

	int _drawmethod;
	rrcwin *_rrw[MAXWIN];
	int _windows;
	rrcwin *addwindow(int, Window, bool stereo=false);
	void delwindow(rrcwin *w);
	rrcs _winmutex;
	rrsocket *_sd;
	Thread *_t;
	char *_remotename;
};

#endif
