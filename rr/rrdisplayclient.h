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

#ifndef __RRDISPLAYCLIENT_H
#define __RRDISPLAYCLIENT_H

#include "rrsocket.h"
#include "rrthread.h"
#include "rr.h"
#include "rrframe.h"
#include "genericQ.h"
#include "rrprofiler.h"

#define MAXPROCS 4

class rrdisplayclient : public Runnable
{
	public:

	rrdisplayclient(char *servername, unsigned short port, bool dossl)
		: bmpi(0), t(NULL), deadyet(false), sd(NULL)
	{
		if(servername && port>0)
		{
			errifnot(sd=new rrsocket(dossl));
			sd->connect(servername, port);
		}
		prof_total.setname("Total");
		errifnot(t=new Thread(this));
		t->start();
	}

	virtual ~rrdisplayclient(void)
	{
		deadyet=true;  q.release();
		if(t) {t->stop();  delete t;  t=NULL;}
		if(sd) {delete sd;  sd=NULL;}
	}

	rrframe *getbitmap(int, int, int);
	bool frameready(void);
	void sendframe(rrframe *);
	void run(void);

	private:

	static const int NB=3;
	rrcs bmpmutex;  rrframe bmp[NB];  int bmpi;
	rrmutex ready;
	genericQ q;
	Thread *t;  bool deadyet;
	rrsocket *sd;
	rrprofiler prof_total;
};

class rrcompressor : public Runnable
{
	public:

	rrcompressor(int _myrank, int _np, rrsocket *_sd) :
		_b(NULL), _lastb(NULL), myrank(_myrank), np(_np), sd(_sd), deadyet(false)
	{
		ready.lock();  complete.lock();
		sendbuf=NULL;  bufsize=0;
		prof_comp.setname("Compress");
	}

	virtual ~rrcompressor(void)
	{
		shutdown();
		if(sendbuf) {free(sendbuf);  sendbuf=NULL;  bufsize=0;}
	}

	void run(void)
	{
		while(!deadyet)
		{
			try {

			ready.lock();  if(deadyet) break;
			compresssend(_b, _lastb);
			complete.unlock();

			} catch (...) {complete.unlock();  throw;}
		}
	}

	void go(rrframe *b, rrframe *lastb)
	{
		_b=b;  _lastb=lastb;
		ready.unlock();
	}

	void stop(void)
	{
		complete.lock();
	}

	void shutdown(void) {deadyet=true;  ready.unlock();}
	void compresssend(rrframe *, rrframe *);

	void send(void)
	{
		if(sd && sendbuf && bufsize) sd->send(sendbuf, bufsize);
		if(sendbuf) {free(sendbuf);  sendbuf=NULL;  bufsize=0;}
	}

	private:

	void store(char *buf, int size)
	{
		if(!(sendbuf=(char *)realloc(sendbuf, bufsize+size))) _throw("Memory allocation error");
		memcpy(&sendbuf[bufsize], buf, size);
		bufsize+=size;
	}

	int bufsize;  char *sendbuf;
	rrframe *_b, *_lastb;
	int myrank, np;
	rrsocket *sd;
	rrmutex ready, complete;  bool deadyet;
	rrcs mutex;
	rrprofiler prof_comp;
};

#endif
