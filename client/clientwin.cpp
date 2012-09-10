/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011 D. R. Commander
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
#include "rrerror.h"
#include "rrprofiler.h"
#include "rrglframe.h"


extern Display *maindpy;


clientwin::clientwin(int dpynum, Window window, int drawmethod, bool stereo) :
	_drawmethod(drawmethod), _reqdrawmethod(drawmethod), _fb(NULL), _cfi(0),
	_deadyet(false), _t(NULL), _stereo(stereo)
{
	if(dpynum<0 || dpynum>65535 || !window)
		throw(rrerror("clientwin::clientwin()", "Invalid argument"));
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
			_xvf[i]->complete();  delete _xvf[i];  _xvf[i]=NULL;
		}
	}
	#endif
	for(int i=0; i<NFRAMES; i++) _cf[i].complete();
	if(_t) {delete _t;  _t=NULL;}
}


void clientwin::initgl(void)
{
	rrglframe *fb=NULL;
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "LOCALPC:%d.0", _dpynum);
	#else
	sprintf(dpystr, ":%d.0", _dpynum);
	#endif
	rrcs::safelock l(_mutex);
	if(_drawmethod==RR_DRAWOGL)
	{
		try
		{
			fb=new rrglframe(dpystr, _window);
			if(!fb) _throw("Could not allocate class instance");
		}
		catch(rrerror &e)
		{
			rrout.println("OpenGL error-- %s\nUsing X11 drawing instead",
				e.getMessage());
			if(fb) {delete fb;  fb=NULL;}
			_drawmethod=RR_DRAWX11;
			rrout.PRINTLN("Stereo requires OpenGL drawing.  Disabling stereo.");
			_stereo=false;
			return;
		}
	}
	if(fb)
	{
		if(_fb)
		{
			if(_fb->_isgl) delete ((rrglframe *)_fb);
			else delete ((rrfb *)_fb);
		}
		_fb=(rrframe *)fb;
	}
}


void clientwin::initx11(void)
{
	rrfb *fb=NULL;
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "localhost:%d.0", _dpynum);
	#else
	sprintf(dpystr, ":%d.0", _dpynum);
	#endif
	rrcs::safelock l(_mutex);
	if(_drawmethod==RR_DRAWX11)
	{
		try
		{
			fb=new rrfb(dpystr, _window);
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
			if(_fb->_isgl) {delete ((rrglframe *)_fb);}
			else delete ((rrfb *)_fb);
		}
		_fb=(rrframe *)fb;
	}
}


int clientwin::match(int dpynum, Window window)
{
	return (_dpynum==dpynum && _window==window);
}


rrframe *clientwin::getframe(bool usexv)
{
	rrframe *f=NULL;
	if(_t) _t->checkerror();
	_cfmutex.lock();
	#ifdef USEXV
	if(usexv)
	{
		if(!_xvf[_cfi])
		{
			char dpystr[80];
			sprintf(dpystr, ":%d.0", _dpynum);
			_xvf[_cfi]=new rrxvframe(dpystr, _window);
			if(!_xvf[_cfi]) _throw("Could not allocate class instance");
		}
		f=(rrframe *)_xvf[_cfi];
	}
	else
	#endif
	f=(rrframe *)&_cf[_cfi];
	_cfi=(_cfi+1)%NFRAMES;
	_cfmutex.unlock();
	f->waituntilcomplete();
	if(_t) _t->checkerror();
	return f;
}


void clientwin::drawframe(rrframe *f)
{
	if(_t) _t->checkerror();
	if(!f->_isxv)
	{
		rrcompframe *c=(rrcompframe *)f;
		if((c->_rh.flags==RR_RIGHT || c->_h.flags==RR_LEFT) && !_stereo)
		{
			_stereo=true;
			if(_drawmethod!=RR_DRAWOGL)
			{
				_drawmethod=RR_DRAWOGL;
				initgl();
			}
		}
		if((c->_h.flags==0) && _stereo)
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
	rrprofiler pt("Total     "), pb("Blit      "), pd("Decompress");
	rrframe *f=NULL;  long bytes=0;

	try
	{
		while(!_deadyet)
		{
			void *ftemp=NULL;
			_q.get(&ftemp);  f=(rrframe *)ftemp;  if(_deadyet) break;
			if(!f) throw(rrerror("clientwin::run()",
				"Invalid image received from queue"));
			rrcs::safelock l(_mutex);
			#ifdef USEXV
			if(f->_isxv)
			{
				if(f->_h.flags!=RR_EOF)
				{
					pb.startframe();
					((rrxvframe *)f)->redraw();
					pb.endframe(f->_h.width*f->_h.height, 0, 1);
					pt.endframe(f->_h.width*f->_h.height, bytes, 1);
					bytes=0;
					pt.startframe();
				}
			}
			else
			#endif
			{
				if(f->_h.flags==RR_EOF)
				{
					pb.startframe();
					if(_fb->_isgl) ((rrglframe *)_fb)->init(f->_h, _stereo);
					else ((rrfb *)_fb)->init(f->_h);
					if(_fb->_isgl) ((rrglframe *)_fb)->redraw();
					else ((rrfb *)_fb)->redraw();
					pb.endframe(_fb->_h.framew*_fb->_h.frameh, 0, 1);
					pt.endframe(_fb->_h.framew*_fb->_h.frameh, bytes, 1);
					bytes=0;
					pt.startframe();
				}
				else
				{
					pd.startframe();
					if(_fb->_isgl) *((rrglframe *)_fb)=*((rrcompframe *)f);
					else *((rrfb *)_fb)=*((rrcompframe *)f);
					pd.endframe(f->_h.width*f->_h.height, 0,
						(double)(f->_h.width*f->_h.height)/
							(double)(f->_h.framew*f->_h.frameh));
					bytes+=f->_h.size;
				}
			}
			f->complete();
		}

	}
	catch(rrerror &e)
	{
		if(_t) _t->seterror(e);  if(f) f->complete();
		throw;
	}
}
