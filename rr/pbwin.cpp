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

#include "pbwin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(sun)||defined(sgi)
#define GL_BGR_EXT GL_RGB
#define GL_BGRA_EXT GL_RGBA
#endif

extern void _fprintf (FILE *f, const char *format, ...);

#include "fakerconfig.h"
extern FakerConfig fconfig;

#include "faker-dpyhash.h"
extern dpyhash *_dpyh;
#define dpyh (*(_dpyh?_dpyh:(_dpyh=new dpyhash())))

extern Display *_localdpy;

#define checkgl(m) if(glerror()) _throw("Could not "m);

// Generic OpenGL error checker (0 = no errors)
int glerror(void)
{
	int i, ret=0;
	i=glGetError();
	while(i!=GL_NO_ERROR)
	{
		ret=1;
		fprintf(stderr, "OpenGL error #%d\n", i);
		i=glGetError();
	}
	return ret;
}

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif


pbuffer::pbuffer(Display *dpy, int w, int h, GLXFBConfig config)
{
	if(!dpy || !config || w<1 || h<1) _throw("Invalid argument");

	cleared=false;
	const char *glxext=NULL;
	glxext=_glXQueryExtensionsString(dpy, DefaultScreen(dpy));
	if(!glxext || !strstr(glxext, "GLX_SGIX_pbuffer"))
		_throw("Pbuffer extension not supported on rendering display");

	this->dpy=dpy;
	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True, None};
	int dw=DisplayWidth(dpy, DefaultScreen(dpy));
	int dh=DisplayHeight(dpy, DefaultScreen(dpy));
	w=min(w, dw);  h=min(h, dh);
	this->w=w;  this->h=h;
	pbattribs[1]=w;  pbattribs[3]=h;
	d=_glXCreatePbuffer(dpy, config, pbattribs);
	if(!d) _throw("Could not create Pbuffer");
}

pbuffer::~pbuffer(void)
{
	if(d && dpy)
		_glXDestroyPbuffer(dpy, d);
}

void pbuffer::clear(void)
{
	if(cleared) return;
	cleared=true;
	GLfloat params[4];
	glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(params[0], params[1], params[2], params[3]);
}

void pbuffer::swap(void)
{
	_glXSwapBuffers(dpy, d);
}


// This is a container class which allows us to temporarily swap in a new
// drawable and then "pop the stack" after we're done with it.  It does nothing
// unless there is already a valid context established

class tempctx
{
	public:

		tempctx(GLXDrawable draw, GLXDrawable read)
		{
			_read=_draw=0;  _ctx=0;  _dpy=NULL;  mc=false;
			if(!read && !draw) return;
			if((_ctx=glXGetCurrentContext())==0) return;
			if((_dpy=glXGetCurrentDisplay())==NULL) return;
			_read=glXGetCurrentReadDrawable();
			_draw=glXGetCurrentDrawable();
			if(!read) read=_read;  if(!draw) draw=_draw;
			if(_read!=read || _draw!=draw)
			{
				_glXMakeContextCurrent(_dpy, draw, read, _ctx);
				mc=true;
			}
		}

		~tempctx(void)
		{
			if(mc && _ctx && _dpy)
				_glXMakeContextCurrent(_dpy, _draw, _read, _ctx);
		}

	private:

		Display *_dpy;
		GLXContext _ctx;
		GLXDrawable _read, _draw;
		bool mc;
};


// This class encapsulates the Pbuffer, its most recent ancestor, and
// information specific to its corresponding X window

pbwin::pbwin(Display *windpy, Window win, Display *pbdpy)
{
	if(!windpy || !win || !pbdpy) _throw("Invalid argument");
	this->windpy=windpy;  this->pbdpy=pbdpy;  this->win=win;
	force=false;
	oldpb=pb=NULL;  neww=newh=-1;
	blitter=NULL;
}

pbwin::~pbwin(void)
{
	mutex.lock(false);
	if(pb) {delete pb;  pb=NULL;}
	if(oldpb) {delete oldpb;  oldpb=NULL;}
	if(blitter) {delete blitter;  blitter=NULL;}
	mutex.unlock(false);
}

void pbwin::init(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	int dw=DisplayWidth(pbdpy, DefaultScreen(pbdpy));
	int dh=DisplayHeight(pbdpy, DefaultScreen(pbdpy));
	w=min(w, dw);  h=min(h, dh);

	rrcs::safelock l(mutex);
	if(pb && pb->width()==w && pb->height()==h) return;
	if((pb=new pbuffer(pbdpy, w, h, config))==NULL)
		_throw("Could not create Pbuffer");

	this->config=config;
	force=true;
}

// The resize doesn't actually occur until the next time updatedrawable() is
// called

void pbwin::resize(int w, int h)
{
	rrcs::safelock l(mutex);
	if(w==0 && pb) w=pb->width();
	if(h==0 && pb) h=pb->height();
	if(pb && pb->width()==w && pb->height()==h) return;
	neww=w;  newh=h;
}

void pbwin::clear(void)
{
	rrcs::safelock l(mutex);
	if(pb) pb->clear();
}

void pbwin::cleanup(void)
{
	rrcs::safelock l(mutex);
	if(oldpb) {delete oldpb;  oldpb=NULL;}
}

void pbwin::initfromwindow(GLXFBConfig config)
{
	XSync(windpy, False);
	XWindowAttributes xwa;
	XGetWindowAttributes(windpy, win, &xwa);
	init(xwa.width, xwa.height, config);
}

// Get the current Pbuffer drawable

GLXDrawable pbwin::getdrawable(void)
{
	GLXDrawable retval=0;
	rrcs::safelock l(mutex);
	retval=pb->drawable();
	return retval;
}

// Get the current Pbuffer drawable, but resize the Pbuffer first if necessary

GLXDrawable pbwin::updatedrawable(void)
{
	GLXDrawable retval=0;
	rrcs::safelock l(mutex);
	if(neww>0 && newh>0)
	{
		oldpb=pb;
		init(neww, newh, config);
		neww=newh=-1;
	}
	retval=pb->drawable();
	return retval;
}

Display *pbwin::getwindpy(void)
{
	return windpy;
}

Display *pbwin::getpbdpy(void)
{
	return pbdpy;
}

Window pbwin::getwin(void)
{
	return win;
}

void pbwin::swapbuffers(void)
{
	rrcs::safelock l(mutex);
	if(pb) pb->swap();
}

void pbwin::readback(bool force)
{
	GLint drawbuf=GL_BACK;
	glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	if(drawbuf==GL_FRONT_AND_BACK) drawbuf=GL_FRONT;
	readback(drawbuf, force);
}

void pbwin::readback(GLint drawbuf, bool force)
{
	rrdisplayclient *rrdpy=NULL;
	char *ptr=NULL, *dpystring;

	rrcs::safelock l(mutex);

	if(fconfig.compress!=RRCOMP_NONE) errifnot(rrdpy=dpyh.findrrdpy(windpy));
	if(this->force) {force=true;  this->force=false;}
	if(fconfig.spoil && rrdpy && !rrdpy->frameready() && !force)
		return;

	int pbw=pb->width(), pbh=pb->height();

	rrframeheader h;
	h.winid=win;
	h.winw=h.bmpw=pbw;
	h.winh=h.bmph=pbh;
	h.bmpx=0;
	h.bmpy=0;
	h.qual=fconfig.currentqual;
	h.subsamp=fconfig.currentsubsamp;
	h.dpynum=0;
	if((dpystring=fconfig.client)==NULL)
		dpystring=DisplayString(windpy);
	if((ptr=strchr(dpystring, ':'))!=NULL)
	{
		if(strlen(ptr)>1) h.dpynum=atoi(ptr+1);
	}

	if(fconfig.compress==RRCOMP_NONE || pbw*pbh<1000)
	{
		rrfb *b;
		if(!blitter) errifnot(blitter=new rrblitter());
		errifnot(b=blitter->getbitmap(windpy, win, pbw, pbh));
		int format= (b->flags&RRBMP_BGR)? (b->pixelsize==3?GL_BGR_EXT:GL_BGRA_EXT) : (b->pixelsize==3?GL_RGB:GL_RGBA);
		readpixels(0, 0, pbw, b->pitch, pbh, format, b->bits, drawbuf, false);
		blitter->sendframe(b);
	}
	else
	{
		rrframe *b;
		errifnot(b=new rrframe());  b->init(&h, 3);
		readpixels(0, 0, pbw, pbw*3, pbh, GL_BGR_EXT, b->bits, drawbuf, false);
		rrdpy->sendframe(b);
	}
}

void pbwin::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, GLubyte *bits, GLint buf, bool bottomup)
{
	GLint readbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &readbuf);

	tempctx *tc;
	errifnot((tc=new tempctx(0, glXGetCurrentDrawable())));

	glReadBuffer(buf);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	for(int i=0; i<h; i++)
	{
		int _y= bottomup? i+y: h-1-i-y;
		glReadPixels(x, i+y, w, 1, format, GL_UNSIGNED_BYTE, &bits[pitch*_y]);
	}
	checkgl("Read Pixels");

	glPopClientAttrib();
	delete tc;
	glReadBuffer(readbuf);
}
