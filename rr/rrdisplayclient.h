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

class rrdisplayclient : public Runnable
{
	public:

	rrdisplayclient(char *);

	virtual ~rrdisplayclient(void)
	{
		_deadyet=true;  _q.release();
		if(_t) {_t->stop();  delete _t;  _t=NULL;}
		if(_sd) {delete _sd;  _sd=NULL;}
	}

	rrframe *getbitmap(int, int, int);
	bool frameready(void);
	void sendframe(rrframe *);
	void sendcompressedframe(rrframeheader &, unsigned char *);
	void run(void);
	void release(rrframe *b) {_ready.signal();}
	bool stereoenabled(void) {return _stereo;}

	rrsocket *_sd;

	private:

	static const int NB=3;
	rrcs _bmpmutex;  rrframe _bmp[NB];  int _bmpi;
	rrevent _ready;
	genericQ _q;
	Thread *_t;  bool _deadyet;
	rrprofiler _prof_total;
	int _np, _dpynum;
	bool _stereo;
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
		h.y=byteswap16(h.y);  \
		h.dpynum=byteswap16(h.dpynum);}}

#define endianize_v1(h) { \
	if(!littleendian()) {  \
		h.size=byteswap(h.size);  \
		h.winid=byteswap(h.winid);  \
		h.framew=byteswap16(h.framew);  \
		h.frameh=byteswap16(h.frameh);  \
		h.width=byteswap16(h.width);  \
		h.height=byteswap16(h.height);  \
		h.x=byteswap16(h.x);  \
		h.y=byteswap16(h.y);}}

#define cvthdr_v1(h, h1) {  \
	h1.size=h.size;  \
	h1.winid=h.winid;  \
	h1.framew=h.framew;  \
	h1.frameh=h.frameh;  \
	h1.width=h.width;  \
	h1.height=h.height;  \
	h1.x=h.x;  \
	h1.y=h.y;  \
	h1.qual=h.qual;  \
	h1.subsamp=h.subsamp;  \
	h1.flags=h.flags;  \
	h1.dpynum=(unsigned char)h.dpynum;}

class rrcompressor : public Runnable
{
	public:

	rrcompressor(int myrank, int np, rrsocket *sd) : _bytes(0),
		_storedframes(0), _frame(NULL), _b(NULL), _lastb(NULL), _myrank(myrank), _np(np),
		_sd(sd), _deadyet(false)
	{
		_ready.wait();  _complete.wait();
	}

	virtual ~rrcompressor(void)
	{
		shutdown();
		if(_frame) {free(_frame);  _frame=NULL;}
	}

	void run(void)
	{
		while(!_deadyet)
		{
			try {

			_ready.wait();  if(_deadyet) break;
			compresssend(_b, _lastb);
			_complete.signal();

			} catch (...) {_complete.signal();  throw;}
		}
	}

	void go(rrframe *b, rrframe *lastb)
	{
		_b=b;  _lastb=lastb;
		_ready.signal();
	}

	void stop(void)
	{
		_complete.wait();
	}

	void shutdown(void) {_deadyet=true;  _ready.signal();}
	void compresssend(rrframe *, rrframe *);
	void send(void);

	long _bytes;

	private:

	void store(rrjpeg *j)
	{
		_storedframes++;
		if(!(_frame=(rrjpeg **)realloc(_frame, sizeof(rrjpeg *)*_storedframes)))
			_throw("Memory allocation error");
		_frame[_storedframes-1]=j;
	}

	int _storedframes;  rrjpeg **_frame;
	rrframe *_b, *_lastb;
	int _myrank, _np;
	rrsocket *_sd;
	rrevent _ready, _complete;  bool _deadyet;
	rrcs _mutex;
};

#endif
