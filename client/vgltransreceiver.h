/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2011 D. R. Commander
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

#ifndef __VGLTRANSRECEIVER_H
#define __VGLTRANSRECEIVER_H

#include "rrsocket.h"
#include "clientwin.h"

#define MAXWIN 1024

class vgltransreceiver : public Runnable
{
	public:

	vgltransreceiver(bool, int _drawmethod);
	void listen(unsigned short);
	unsigned short port(void) {return _port;}
	virtual ~vgltransreceiver(void);

	private:

	void run(void);

	int _drawmethod;
	rrsocket *_listensd;
  rrcs _listensdmutex;
	Thread *_t;
	bool _deadyet;
	bool _dossl;
	unsigned short _port;
};

class vgltransserver : public Runnable
{
	public:

	vgltransserver(rrsocket *sd, int drawmethod) : _drawmethod(drawmethod),
		_nwin(0), _sd(sd), _t(NULL), _remotename(NULL)
	{
		memset(_win, 0, sizeof(clientwin *)*MAXWIN);
		if(_sd) _remotename=_sd->remotename();
		errifnot(_t=new Thread(this));
		_t->start();
	}

	virtual ~vgltransserver(void)
	{
		int i;
		_winmutex.lock(false);
		for(i=0; i<_nwin; i++) {if(_win[i]) {delete _win[i];  _win[i]=NULL;}}
		_nwin=0;
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
	clientwin *_win[MAXWIN];
	int _nwin;
	clientwin *addwindow(int, Window, bool stereo=false);
	void delwindow(clientwin *w);
	rrcs _winmutex;
	rrsocket *_sd;
	Thread *_t;
	char *_remotename;
};

#endif
