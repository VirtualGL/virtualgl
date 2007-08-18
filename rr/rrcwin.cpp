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

#include "rrcwin.h"
#include "rrerror.h"
#include "rrprofiler.h"
#include "rrglframe.h"

extern Display *maindpy;

#ifdef SUNOGL
static int use_ogl_as_default(void)
{
	int retval=0;
	if(dpy)
	{
		int maj_opcode=-1, first_event=-1, first_error=-1;
		if(XQueryExtension(maindpy, "GLX", &maj_opcode, &first_event, &first_error))
		{
			int attribs[]={GLX_RGBA, GLX_DOUBLEBUFFER, 0};
			int sbattribs[]={GLX_RGBA, 0};
			XVisualInfo *vi=NULL;
			if((vi=glXChooseVisual(maindpy, DefaultScreen(dpy), attribs))!=NULL
				|| (vi=glXChooseVisual(maindpy, DefaultScreen(dpy), sbattribs))!=NULL)
			{
				GLXContext ctx=glXCreateContext(maindpy, vi, NULL, True);
				if(ctx)
				{
					if(glXMakeCurrent(maindpy, DefaultRootWindow(dpy), ctx))
					{
						char *renderer=(char *)glGetString(GL_RENDERER);
						if(renderer && !strstr(renderer, "SUNWpfb")
							&& !strstr(renderer, "SUNWm64") && !strstr(renderer, "SUNWnfb")
							&& !strstr(renderer, "Sun dpa")
							&& !strstr(renderer, "software renderer")) retval=1;
						glXMakeCurrent(dpy, 0, 0);
					}
					glXDestroyContext(maindpy, ctx);
				}
				XFree(vi);
			}
		}
	}
	return retval;
}
#endif

rrcwin::rrcwin(int dpynum, Window window, int drawmethod, bool stereo) :
	_drawmethod(drawmethod), _reqdrawmethod(drawmethod), _b(NULL), _cfi(0),
	_deadyet(false), _t(NULL), _stereo(stereo)
{
	if(dpynum<0 || dpynum>65535 || !window)
		throw(rrerror("rrcwin::rrcwin()", "Invalid argument"));
	_dpynum=dpynum;  _window=window;

	setdrawmethod();
	if(_stereo) _drawmethod=RR_DRAWOGL;
	initgl();
	initx11();

	errifnot(_t=new Thread(this));
	_t->start();
}

void rrcwin::setdrawmethod(void)
{
	if(_drawmethod==RR_DRAWAUTO)
	{
		_drawmethod=RR_DRAWX11;
		#ifdef SUNOGL
		if(use_ogl_as_default(_dpynum)) _drawmethod=RR_DRAWOGL;
		#endif
	}
}

rrcwin::~rrcwin(void)
{
	_deadyet=true;
	_q.release();
	if(_t) _t->stop();
	if(_b) delete _b;
	for(int i=0; i<NB; i++) _cf[i].complete();
	if(_t) {delete _t;  _t=NULL;}
}

void rrcwin::initgl(void)
{
	rrglframe *b=NULL;
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
			b=new rrglframe(dpystr, _window);
			if(!b) _throw("Could not allocate class instance");
		}
		catch(rrerror &e)
		{
			rrout.println("OpenGL error-- %s\nUsing X11 drawing instead",
				e.getMessage());
			if(b) {delete b;  b=NULL;}
			_drawmethod=RR_DRAWX11;
			rrout.PRINTLN("Stereo requires OpenGL drawing.  Disabling stereo.");
			_stereo=false;
			return;
		}
	}
	if(b)
	{
		if(_b)
		{
			if(_b->_isgl) delete ((rrglframe *)_b);
			else delete ((rrfb *)_b);
		}
		_b=(rrframe *)b;
	}
}

void rrcwin::initx11(void)
{
	rrfb *b=NULL;
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
			b=new rrfb(dpystr, _window);
			if(!b) _throw("Could not allocate class instance");
		}
		catch(...)
		{
			if(b) {delete b;  b=NULL;}
			throw;
		}
	}
	if(b)
	{
		if(_b)
		{
			if(_b->_isgl) {delete ((rrglframe *)_b);}
			else delete ((rrfb *)_b);
		}
		_b=(rrframe *)b;
	}
}

int rrcwin::match(int dpynum, Window window)
{
	return (_dpynum==dpynum && _window==window);
}

rrcompframe *rrcwin::getFrame(void)
{
	rrcompframe *c=NULL;
	if(_t) _t->checkerror();
	_cfmutex.lock();
	c=&_cf[_cfi];  _cfi=(_cfi+1)%NB;
	_cfmutex.unlock();
	c->waituntilcomplete();
	if(_t) _t->checkerror();
	return c;
}

void rrcwin::drawFrame(rrcompframe *c)
{
	if(_t) _t->checkerror();
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
	_q.add(c);
}


void rrcwin::run(void)
{
	rrprofiler pt("Total     "), pb("Blit      "), pd("Decompress");
	rrcompframe *c=NULL;  long bytes=0;

	try {

	while(!_deadyet)
	{
		c=NULL;
		_q.get((void **)&c);  if(_deadyet) break;
		if(!c) throw(rrerror("rrcwin::run()", "Invalid image received from queue"));
		rrcs::safelock l(_mutex);
		if(c->_h.flags==RR_EOF)
		{
			pb.startframe();
			if(_b->_isgl) ((rrglframe *)_b)->init(c->_h, _stereo);
			else ((rrfb *)_b)->init(c->_h);
			if(_b->_isgl) ((rrglframe *)_b)->redraw();
			else ((rrfb *)_b)->redraw();
			pb.endframe(_b->_h.framew*_b->_h.frameh, 0, 1);
			pt.endframe(_b->_h.framew*_b->_h.frameh, bytes, 1);
			bytes=0;
			pt.startframe();
		}
		else
		{
			pd.startframe();
			if(_b->_isgl) *((rrglframe *)_b)=*c;
			else *((rrfb *)_b)=*c;
			pd.endframe(c->_h.width*c->_h.height, 0, (double)(c->_h.width*c->_h.height)/
				(double)(c->_h.framew*c->_h.frameh));
			bytes+=c->_h.size;
		}
		c->complete();
	}

	} catch(rrerror &e)
	{
		if(_t) _t->seterror(e);  if(c) c->complete();
		throw;
	}
}
