/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011, 2014 D. R. Commander
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

#include "clientwin.h"
#include "Error.h"
#include "Log.h"
#include "Profiler.h"
#include "GLFrame.h"


extern Display *maindpy;


clientwin::clientwin(int dpynum, Window window, int drawmethod, bool stereo) :
	_drawmethod(drawmethod), _reqdrawmethod(drawmethod), _fb(NULL), _cfi(0),
	_deadyet(false), _t(NULL), _stereo(stereo)
{
	if(dpynum<0 || dpynum>65535 || !window)
		throw(Error("clientwin::clientwin()", "Invalid argument"));
	_dpynum=dpynum;  _window=window;

	#ifdef USEXV
	for(int i=0; i<NFRAMES; i++) _xvf[i]=NULL;
	#endif
	setdrawmethod();
	if(_stereo) _drawmethod=RR_DRAWOGL;
	initgl();
	initx11();

	errifnot(_t=new Thread(this));
	_t->start();
}


void clientwin::setdrawmethod(void)
{
	if(_drawmethod==RR_DRAWAUTO)
	{
		_drawmethod=RR_DRAWX11;
	}
}


clientwin::~clientwin(void)
{
	_deadyet=true;
	_q.release();
	if(_t) _t->stop();
	if(_fb) delete _fb;
	#ifdef USEXV
	for(int i=0; i<NFRAMES; i++)
	{
		if(_xvf[i])
		{
			_xvf[i]->signalComplete();  delete _xvf[i];  _xvf[i]=NULL;
		}
	}
	#endif
	for(int i=0; i<NFRAMES; i++) _cf[i].signalComplete();
	if(_t) {delete _t;  _t=NULL;}
}


void clientwin::initgl(void)
{
	GLFrame *fb=NULL;
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "LOCALPC:%d.0", _dpynum);
	#else
	sprintf(dpystr, ":%d.0", _dpynum);
	#endif
	CS::SafeLock l(_mutex);
	if(_drawmethod==RR_DRAWOGL)
	{
		try
		{
			fb=new GLFrame(dpystr, _window);
			if(!fb) _throw("Could not allocate class instance");
		}
		catch(Error &e)
		{
			vglout.println("OpenGL error-- %s\nUsing X11 drawing instead",
				e.getMessage());
			if(fb) {delete fb;  fb=NULL;}
			_drawmethod=RR_DRAWX11;
			vglout.PRINTLN("Stereo requires OpenGL drawing.  Disabling stereo.");
			_stereo=false;
			return;
		}
	}
	if(fb)
	{
		if(_fb)
		{
			if(_fb->isGL) delete ((GLFrame *)_fb);
			else delete ((FBXFrame *)_fb);
		}
		_fb=(Frame *)fb;
	}
}


void clientwin::initx11(void)
{
	FBXFrame *fb=NULL;
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "localhost:%d.0", _dpynum);
	#else
	sprintf(dpystr, ":%d.0", _dpynum);
	#endif
	CS::SafeLock l(_mutex);
	if(_drawmethod==RR_DRAWX11)
	{
		try
		{
			fb=new FBXFrame(dpystr, _window);
			if(!fb) _throw("Could not allocate class instance");
		}
		catch(...)
		{
			if(fb) {delete fb;  fb=NULL;}
			throw;
		}
	}
	if(fb)
	{
		if(_fb)
		{
			if(_fb->isGL) {delete ((GLFrame *)_fb);}
			else delete ((FBXFrame *)_fb);
		}
		_fb=(Frame *)fb;
	}
}


int clientwin::match(int dpynum, Window window)
{
	return (_dpynum==dpynum && _window==window);
}


Frame *clientwin::getframe(bool usexv)
{
	Frame *f=NULL;
	if(_t) _t->checkError();
	_cfmutex.lock();
	#ifdef USEXV
	if(usexv)
	{
		if(!_xvf[_cfi])
		{
			char dpystr[80];
			sprintf(dpystr, ":%d.0", _dpynum);
			_xvf[_cfi]=new XVFrame(dpystr, _window);
			if(!_xvf[_cfi]) _throw("Could not allocate class instance");
		}
		f=(Frame *)_xvf[_cfi];
	}
	else
	#endif
	f=(Frame *)&_cf[_cfi];
	_cfi=(_cfi+1)%NFRAMES;
	_cfmutex.unlock();
	f->waitUntilComplete();
	if(_t) _t->checkError();
	return f;
}


void clientwin::drawframe(Frame *f)
{
	if(_t) _t->checkError();
	if(!f->isXV)
	{
		CompressedFrame *c=(CompressedFrame *)f;
		if((c->rhdr.flags==RR_RIGHT || c->hdr.flags==RR_LEFT) && !_stereo)
		{
			_stereo=true;
			if(_drawmethod!=RR_DRAWOGL)
			{
				_drawmethod=RR_DRAWOGL;
				initgl();
			}
		}
		if((c->hdr.flags==0) && _stereo)
		{
			_stereo=false;
			_drawmethod=_reqdrawmethod;
			setdrawmethod();
			initx11();
		}
	}
	_q.add(f);
}


void clientwin::run(void)
{
	Profiler pt("Total     "), pb("Blit      "), pd("Decompress");
	Frame *f=NULL;  long bytes=0;

	try
	{
		while(!_deadyet)
		{
			void *ftemp=NULL;
			_q.get(&ftemp);  f=(Frame *)ftemp;  if(_deadyet) break;
			if(!f) throw(Error("clientwin::run()",
				"Invalid image received from queue"));
			CS::SafeLock l(_mutex);
			#ifdef USEXV
			if(f->isXV)
			{
				if(f->hdr.flags!=RR_EOF)
				{
					pb.startFrame();
					((XVFrame *)f)->redraw();
					pb.endFrame(f->hdr.width*f->hdr.height, 0, 1);
					pt.endFrame(f->hdr.width*f->hdr.height, bytes, 1);
					bytes=0;
					pt.startFrame();
				}
			}
			else
			#endif
			{
				if(f->hdr.flags==RR_EOF)
				{
					pb.startFrame();
					if(_fb->isGL) ((GLFrame *)_fb)->init(f->hdr, _stereo);
					else ((FBXFrame *)_fb)->init(f->hdr);
					if(_fb->isGL) ((GLFrame *)_fb)->redraw();
					else ((FBXFrame *)_fb)->redraw();
					pb.endFrame(_fb->hdr.framew*_fb->hdr.frameh, 0, 1);
					pt.endFrame(_fb->hdr.framew*_fb->hdr.frameh, bytes, 1);
					bytes=0;
					pt.startFrame();
				}
				else
				{
					pd.startFrame();
					if(_fb->isGL) *((GLFrame *)_fb)=*((CompressedFrame *)f);
					else *((FBXFrame *)_fb)=*((CompressedFrame *)f);
					pd.endFrame(f->hdr.width*f->hdr.height, 0,
						(double)(f->hdr.width*f->hdr.height)/
							(double)(f->hdr.framew*f->hdr.frameh));
					bytes+=f->hdr.size;
				}
			}
			f->signalComplete();
		}

	}
	catch(Error &e)
	{
		if(_t) _t->setError(e);  if(f) f->signalComplete();
		throw;
	}
}
