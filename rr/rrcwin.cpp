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

#ifdef SUNOGL
static int use_ogl_as_default(int dpynum)
{
	int retval=0;  char dpystr[80];
	snprintf(dpystr, 79, ":%d.0", dpynum);
	Display *dpy=XOpenDisplay(dpystr);
	if(dpy)
	{
		int maj_opcode=-1, first_event=-1, first_error=-1;
		if(XQueryExtension(dpy, "GLX", &maj_opcode, &first_event, &first_error))
		{
			int attribs[]={GLX_RGBA, GLX_DOUBLEBUFFER, 0};
			int sbattribs[]={GLX_RGBA, 0};
			XVisualInfo *vi=NULL;
			if((vi=glXChooseVisual(dpy, DefaultScreen(dpy), attribs))!=NULL
				|| (vi=glXChooseVisual(dpy, DefaultScreen(dpy), sbattribs))!=NULL)
			{
				glXInitThreadsSUN();
				GLXContext ctx=glXCreateContext(dpy, vi, NULL, True);
				if(ctx)
				{
					if(glXMakeCurrent(dpy, DefaultRootWindow(dpy), ctx))
					{
						char *renderer=(char *)glGetString(GL_RENDERER);
						if(renderer && !strstr(renderer, "SUNWpfb")
							&& !strstr(renderer, "software renderer")) retval=1;
						glXMakeCurrent(dpy, 0, 0);
					}
					glXDestroyContext(dpy, ctx);
				}
				XFree(vi);
			}
		}
		XCloseDisplay(dpy);
	}
	return retval;
}
#endif

rrcwin::rrcwin(int dpynum, Window window, int drawmethod, bool stereo) :
	_drawmethod(drawmethod), _reqdrawmethod(drawmethod), _b(NULL), _jpgi(0),
	_deadyet(false), _t(NULL), _stereo(stereo)
{
	if(dpynum<0 || dpynum>65535 || !window)
		throw(rrerror("rrcwin::rrcwin()", "Invalid argument"));
	_dpynum=dpynum;  _window=window;

	if(_drawmethod==RR_DRAWAUTO)
	{
		_drawmethod=RR_DRAWX11;
		#ifdef SUNOGL
		if(use_ogl_as_default(dpynum)) _drawmethod=RR_DRAWOGL;
		#endif
	}
	if(_stereo) _drawmethod=RR_DRAWOGL;
	initgl();
	initx11();

	errifnot(_t=new Thread(this));
	_t->start();
}

rrcwin::~rrcwin(void)
{
	_deadyet=true;
	_q.release();
	if(_t) _t->stop();
	if(_b) delete _b;
	for(int i=0; i<NB; i++) _jpg[i].complete();
	if(_t) {delete _t;  _t=NULL;}
}

void rrcwin::initgl(void)
{
	rrframe *b=NULL;
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "LOCALPC:%d.0", _dpynum);
	#else
	sprintf(dpystr, ":%d.0", _dpynum);
	#endif
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
	if(b) {if(_b) delete _b;  _b=b;}
}

void rrcwin::initx11(void)
{
	rrframe *b=NULL;
	char dpystr[80];
	#ifdef XDK
	sprintf(dpystr, "localhost:%d.0", _dpynum);
	#else
	sprintf(dpystr, ":%d.0", _dpynum);
	#endif
	if(_drawmethod==RR_DRAWX11)
	{
		try
		{
			b=new rrfb(dpystr, _window);
			if(!b) _throw("Could not allocate class instance");
		}
		catch(rrerror &e)
		{
			if(b) {delete b;  b=NULL;}
			throw;
		}
	}
	if(b) {if(_b) delete _b;  _b=b;}
}

int rrcwin::match(int dpynum, Window window)
{
	return (_dpynum==dpynum && _window==window);
}

rrjpeg *rrcwin::getFrame(void)
{
	rrjpeg *j=NULL;
	if(_t) _t->checkerror();
	_jpgmutex.lock();
	j=&_jpg[_jpgi];  _jpgi=(_jpgi+1)%NB;
	_jpgmutex.unlock();
	j->waituntilcomplete();
	if(_t) _t->checkerror();
	return j;
}

void rrcwin::drawFrame(rrjpeg *j)
{
	if(_t) _t->checkerror();
	if((j->_h.flags==RR_RIGHT || j->_h.flags==RR_LEFT) && !_stereo)
	{
		_stereo=true;
		if(_drawmethod!=RR_DRAWOGL)
		{
			_drawmethod=RR_DRAWOGL;
			initgl();
		}
	}
	if((j->_h.flags==0) && _stereo)
	{
		_stereo=false;
		_drawmethod=_reqdrawmethod;
		initx11();
	}
	_q.add(j);
}


void rrcwin::run(void)
{
	rrprofiler pt("Total     "), pb("Blit      "), pd("Decompress");
	rrjpeg *j=NULL;  long bytes=0;

	try {

	while(!_deadyet)
	{
		j=NULL;
		_q.get((void **)&j);  if(_deadyet) break;
		if(!j) throw(rrerror("rrcwin::run()", "Invalid image received from queue"));
		if(j->_h.flags==RR_EOF)
		{
			pb.startframe();
			if(_b->_isgl) ((rrglframe *)_b)->init(&j->_h);
			else ((rrfb *)_b)->init(&j->_h);
			if(_b->_isgl) ((rrglframe *)_b)->redraw();
			else ((rrfb *)_b)->redraw();
			pb.endframe(_b->_h.framew*_b->_h.frameh* (_stereo? 2:1), 0, 1);
			pt.endframe(_b->_h.framew*_b->_h.frameh* (_stereo? 2:1), bytes, 1);
			bytes=0;
			pt.startframe();
		}
		else
		{
			pd.startframe();
			if(_b->_isgl) *((rrglframe *)_b)=*j;
			else *((rrfb *)_b)=*j;
			bool stereo=j->_h.flags==RR_LEFT || j->_h.flags==RR_RIGHT;
			pd.endframe(j->_h.width*j->_h.height, 0, (double)(j->_h.width*j->_h.height)/
				(double)(j->_h.framew*j->_h.frameh) * (stereo? 0.5:1.0));
			bytes+=j->_h.size;
		}
		j->complete();
	}

	} catch(rrerror &e)
	{
		if(_t) _t->seterror(e);  if(j) j->complete();
		throw;
	}
}
