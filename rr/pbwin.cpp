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
#include "hputil.h"

extern void _fprintf (FILE *f, const char *format, ...);

#include "fakerconfig.h"
extern FakerConfig fconfig;

#include "faker-dpyhash.h"
extern dpyhash *_dpyh;
#define dpyh (*(_dpyh?_dpyh:(_dpyh=new dpyhash())))

extern Display *_localdpy;

#include "rrcommon.h"

#define checkgl(m) if(glerror()) _throw("Could not "m);
#define rrtry(f) {if((f)==-1) throw(RRGetError());}

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

#define STRIPH 64


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
	pthread_mutexattr_t ma;
	pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex, &ma);
	memset(&fb, 0, sizeof(fbx_struct));
}

pbwin::~pbwin(void)
{
	rrlock l(mutex);
	if(pb) {delete pb;  pb=NULL;}
	if(oldpb) {delete oldpb;  oldpb=NULL;}
	fbx_term(&fb);
}

void pbwin::init(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	int dw=DisplayWidth(pbdpy, DefaultScreen(pbdpy));
	int dh=DisplayHeight(pbdpy, DefaultScreen(pbdpy));
	w=min(w, dw);  h=min(h, dh);

	rrlock l(mutex);
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
	rrlock l(mutex);
	if(w==0 && pb) w=pb->width();
	if(h==0 && pb) h=pb->height();
	if(pb && pb->width()==w && pb->height()==h) return;
	neww=w;  newh=h;
}

void pbwin::clear(void)
{
	rrlock l(mutex);
	if(pb) pb->clear();
}

void pbwin::cleanup(void)
{
	rrlock l(mutex);
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
	rrlock l(mutex);
	retval=pb->drawable();
	return retval;
}

// Get the current Pbuffer drawable, but resize the Pbuffer first if necessary

GLXDrawable pbwin::updatedrawable(void)
{
	GLXDrawable retval=0;
	rrlock l(mutex);
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
	rrlock l(mutex);
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
	rrbmp *b;  RRDisplay rrdpy=NULL;
	char *ptr=NULL, *dpystring;

	rrlock l(mutex);

	if(fconfig.compress!=RRCOMP_NONE) errifnot(rrdpy=dpyh.findrrdpy(windpy));
	if(this->force) {force=true;  this->force=false;}
	int frameready=0;
	rrtry(frameready=RRFrameReady(rrdpy));
	if(fconfig.spoil && rrdpy && !frameready && !force)
		return;

	int pbw=pb->width(), pbh=pb->height();
	if(fconfig.compress==RRCOMP_NONE || pbw*pbh<1000)
	{
		blit(drawbuf);  return;
	}
	b=RRGetBitmap(rrdpy, pbw, pbh, 3);
	if(b==NULL) throw(RRGetError());

	readpixels(0, 0, pbw, pbw*3, pbh, GL_BGR_EXT, b->bits, drawbuf, false);

	b->h.dpynum=0;
	if((dpystring=fconfig.client)==NULL)
		dpystring=DisplayString(windpy);
	if((ptr=strchr(dpystring, ':'))!=NULL)
	{
		if(strlen(ptr)>1) b->h.dpynum=atoi(ptr+1);
	}

	b->h.winid=win;
	b->h.winw=b->h.bmpw;
	b->h.winh=b->h.bmph;
	b->h.bmpx=0;
	b->h.bmpy=0;
	b->h.qual=fconfig.currentqual;
	b->h.subsamp=fconfig.currentsubsamp;
	b->flags=RRBMP_BGR|RRBMP_BOTTOMUP;
	b->strip_height=STRIPH;
	rrtry(RRSendFrame(rrdpy, b));
}

void pbwin::blit(GLint drawbuf)
{
	fbx_wh wh;  GLenum format;
	wh.dpy=windpy;  wh.win=win;

//	errifnot(fb=new fbx_struct);
//	memset(fb, 0, sizeof(fbx_struct));
//	XSync(windpy, False);
	fbx(fbx_init(&fb, wh, 0, 0, 1));
	if(fb.ps!=3 && fb.ps!=4) _throw("X server not 24-bit or 32-bit");

	format= fb.bgr? (fb.ps==3?GL_BGR_EXT:GL_BGRA_EXT) : (fb.ps==3?GL_RGB:GL_RGBA);

	int pbw=pb->width(), pbh=pb->height();
	if(pbw!=fb.width || pbh!=fb.height) return;
	readpixels(0, 0, pbw, fb.xi->bytes_per_line, pbh, format,
		(unsigned char *)fb.bits, drawbuf, true);

	fbx(fbx_write(&fb, 0, 0, 0, 0, 0, 0));

	#if 0
	int endline, i;  int startline;
	for(i=0; i<fb->height; i+=STRIPH)
	{
		startline=i;
		endline=startline+min(fb->height-i, STRIPH);
		if(fb->height-i<2*STRIPH) {endline=fb->height;  i+=STRIPH;}

		if(lastfb && fb->width==lastfb->width && fb->height==lastfb->height
		&& fb->ps==lastfb->ps && fb->xi->bytes_per_line==lastfb->xi->bytes_per_line
		&& fb->bgr==lastfb->bgr && !memcmp(&fb->bits[fb->xi->bytes_per_line*startline],
			&lastfb->bits[fb->xi->bytes_per_line*startline], fb->xi->bytes_per_line*(endline-startline)))
			continue;

		fbx(fbx_write(fb, 0, startline, 0, startline, fb->width, endline-startline));
	}

	if(lastfb)
	{
		fbx_term(lastfb);  delete lastfb;
	}
	lastfb=fb;
	#endif
}

void pbwin::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h, GLenum format, GLubyte *bits, GLint buf, bool bottomup)
{
	GLint readbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &readbuf);

	tempctx *tc;
	errifnot((tc=new tempctx(0, glXGetCurrentDrawable())));

	glReadBuffer(buf);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	if(bottomup)
	{
		for(int i=0; i<h; i++)
		{
			int _y=h-1-i-y;
			glReadPixels(x, i+y, w, 1, format, GL_UNSIGNED_BYTE, &bits[pitch*_y]);
		}
	}
	else glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, bits);
	checkgl("Read Pixels");

	glPopClientAttrib();
	delete tc;
	glReadBuffer(readbuf);
}
