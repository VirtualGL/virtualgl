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

	rrdisplayclient(char *displayname, unsigned short port, bool dossl,
		int nprocs=1) : _sd(NULL), _bmpi(0), _t(NULL), _deadyet(false), _np(nprocs),
		_dpynum(0), _stereo(true)
	{
		_np=min(_np, min(numprocs(), MAXPROCS));
		if(_np<1) {_np=min(numprocs(), MAXPROCS);  if(_np>1) _np--;}
		rrout.println("[VGL] Using %d / %d CPU's for compression", _np, numprocs());
		char *servername=NULL;
		try
		{
			if(displayname)
			{
				char *ptr=NULL;  servername=strdup(displayname);
				if((ptr=strchr(servername, ':'))!=NULL)
				{
					if(strlen(ptr)>1) _dpynum=atoi(ptr+1);
					if(_dpynum<0 || _dpynum>255) _dpynum=0;
					*ptr='\0';
				}
				if(!strlen(servername)) {free(servername);  servername=strdup("localhost");}
			}
			if(servername && port>0)
			{
				errifnot(_sd=new rrsocket(dossl));
				_sd->connect(servername, port);
			}
			_prof_total.setname("Total   ");
			errifnot(_t=new Thread(this));
			_t->start();
		}
		catch(...)
		{
			if(servername) free(servername);  throw;
		}
		if(servername) free(servername);
	}

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
	void release(rrframe *b) {_ready.unlock();}
	bool stereoenabled(void) {return _stereo;}

	rrsocket *_sd;

	private:

	static const int NB=3;
	rrcs _bmpmutex;  rrframe _bmp[NB];  int _bmpi;
	rrmutex _ready;
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
		h.y=byteswap16(h.y);}}

class rrcompressor : public Runnable
{
	public:

	rrcompressor(int myrank, int np, rrsocket *sd) : _bytes(0),
		_storedframes(0), _frame(NULL), _b(NULL), _lastb(NULL), _myrank(myrank), _np(np),
		_sd(sd), _deadyet(false)
	{
		_ready.lock();  _complete.lock();
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

			_ready.lock();  if(_deadyet) break;
			compresssend(_b, _lastb);
			_complete.unlock();

			} catch (...) {_complete.unlock();  throw;}
		}
	}

	void go(rrframe *b, rrframe *lastb)
	{
		_b=b;  _lastb=lastb;
		_ready.unlock();
	}

	void stop(void)
	{
		_complete.lock();
	}

	void shutdown(void) {_deadyet=true;  _ready.unlock();}
	void compresssend(rrframe *, rrframe *);

	void send(void)
	{
		for(int i=0; i<_storedframes; i++)
		{
			rrjpeg *j=_frame[i];
			errifnot(j);
			unsigned int size=j->_h.size;
			endianize(j->_h);
			if(_sd) _sd->send((char *)&j->_h, sizeof(rrframeheader));
			if(_sd) _sd->send((char *)j->_bits, size);
			delete j;		
		}
		_storedframes=0;
	}

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
	rrmutex _ready, _complete;  bool _deadyet;
	rrcs _mutex;
};

#endif
