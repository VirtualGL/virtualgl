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
#ifdef _WIN32
#define snprintf _snprintf
#endif
#define MAXPROCS 4

class rrdisplayclient : public Runnable
{
	public:

	rrdisplayclient(char *servername, unsigned short port, bool dossl, bool domt=false)
		: sd(NULL), bmpi(0), t(NULL), deadyet(false), mt(domt)
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
	void sendcompressedframe(rrframeheader &, unsigned char *);
	void run(void);
	void release(rrframe *bmp) {ready.unlock();}

	rrsocket *sd;

	private:

	static const int NB=3;
	rrcs bmpmutex;  rrframe bmp[NB];  int bmpi;
	rrmutex ready;
	genericQ q;
	Thread *t;  bool deadyet;
	rrprofiler prof_total;
	bool mt;
};

#define endianize(h) { \
	if(!littleendian()) {  \
		h.size=byteswap(h.size);  \
		h.winid=byteswap(h.winid);  \
		h.framew=byteswap16(h.framew);  \
		h.frameh=byteswap16(h.frameh);  \
		h.width=byteswap16(h.width);  \
		h.height=byteswap16(h.height);  \
		h.x=byteswap16(h.x);  \
		h.y=byteswap16(h.y);}}

class rrcompressor : public Runnable
{
	public:

	rrcompressor(int _myrank, int _np, rrsocket *_sd) :
		storedframes(0), frame(NULL), _b(NULL), _lastb(NULL), myrank(_myrank), np(_np),
		sd(_sd), deadyet(false)
	{
		ready.lock();  complete.lock();
		char temps[20];
		snprintf(temps, 19, "Compress %d", myrank);
		prof_comp.setname(temps);
	}

	virtual ~rrcompressor(void)
	{
		shutdown();
		if(frame) {free(frame);  frame=NULL;}
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
		for(int i=0; i<storedframes; i++)
		{
			rrjpeg *j=frame[i];
			errifnot(j);
			unsigned int size=j->h.size;
			endianize(j->h);
			if(sd) sd->send((char *)&j->h, sizeof(rrframeheader));
			if(sd) sd->send((char *)j->bits, size);
			delete j;		
		}
		storedframes=0;
	}

	private:

	void store(rrjpeg *j)
	{
		storedframes++;
		if(!(frame=(rrjpeg **)realloc(frame, sizeof(rrjpeg *)*storedframes)))
			_throw("Memory allocation error");
		frame[storedframes-1]=j;
	}

	int storedframes;  rrjpeg **frame;
	rrframe *_b, *_lastb;
	int myrank, np;
	rrsocket *sd;
	rrmutex ready, complete;  bool deadyet;
	rrcs mutex;
	rrprofiler prof_comp;
};

#endif
