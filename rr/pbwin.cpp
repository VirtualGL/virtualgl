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
#if defined(sun)||defined(linux)
#include "rrsunray.h"
#endif
#include "glxvisual.h"

extern void _vglprintf (FILE *f, const char *format, ...);

#define INFAKER
#include "tempctx.h"
#include "fakerconfig.h"
extern FakerConfig fconfig;

#include "faker-dpyhash.h"
extern dpyhash *_dpyh;
#define dpyh (*(_dpyh?_dpyh:(_dpyh=new dpyhash())))

extern Display *_localdpy;
#ifdef USEGLP
extern GLPDevice _localdev;
#endif

#define checkgl(m) if(glerror()) _throw("Could not "m);

#define _isright(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)

static inline int _drawingtoright(void)
{
	GLint drawbuf=GL_LEFT;
	glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isright(drawbuf);
}

// Generic OpenGL error checker (0 = no errors)
static int glerror(void)
{
	int i, ret=0;
	i=glGetError();
	while(i!=GL_NO_ERROR)
	{
		ret=1;
		fprintf(stderr, "[VGL] OpenGL error 0x%.4x\n", i);
		i=glGetError();
	}
	return ret;
}

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif

Window create_window(Display *dpy, GLXFBConfig config, int w, int h)
{
	XVisualInfo *vis;
	Window win;
	XSetWindowAttributes wattrs;
	Colormap cmap;

	if((vis=_glXGetVisualFromFBConfig(dpy, config))==NULL) return 0;
	cmap=XCreateColormap(dpy, RootWindow(dpy, vis->screen), vis->visual,
		AllocNone);
	wattrs.background_pixel = 0;
	wattrs.border_pixel = 0;
	wattrs.colormap = cmap;
	wattrs.event_mask = ExposureMask | StructureNotifyMask;
	win = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0, w, h, 1,
		vis->depth, InputOutput, vis->visual,
		CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &wattrs);
	XMapWindow(dpy, win);
	return win;
}

pbuffer::pbuffer(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	_cleared=false;  _stereo=false;
	#if 0
	const char *glxext=NULL;
	glxext=_glXQueryExtensionsString(dpy, DefaultScreen(dpy));
	if(!glxext || !strstr(glxext, "GLX_SGIX_pbuffer"))
		_throw("Pbuffer extension not supported on rendering display");
	#endif

	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True, None};

	_w=w;  _h=h;
	pbattribs[1]=w;  pbattribs[3]=h;
	#ifdef sun
	tempctx tc(_localdpy, 0, 0, 0);
	#endif
	if(fconfig.usewindow) _drawable=create_window(_localdpy, config, w, h);
	else _drawable=glXCreatePbuffer(_localdpy, config, pbattribs);
	if(__vglServerVisualAttrib(config, GLX_STEREO)) _stereo=true;
	if(!_drawable) _throw("Could not create Pbuffer");
}

pbuffer::~pbuffer(void)
{
	if(_drawable)
	{
		#ifdef sun
		tempctx tc(_localdpy, 0, 0, 0);
		#endif
		if(fconfig.usewindow) XDestroyWindow(_localdpy, _drawable);
		else glXDestroyPbuffer(_localdpy, _drawable);
	}
}

void pbuffer::clear(void)
{
	if(_cleared) return;
	_cleared=true;
	GLfloat params[4];
	glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(params[0], params[1], params[2], params[3]);
}

void pbuffer::swap(void)
{
	if(!fconfig.glp) _glXSwapBuffers(_localdpy, _drawable);
}


// This class encapsulates the Pbuffer, its most recent ancestor, and
// information specific to its corresponding X window

pbwin::pbwin(Display *windpy, Window win)
{
	if(!windpy || !win) _throw("Invalid argument");
	_windpy=windpy;  _win=win;
	_force=false;
	_oldpb=_pb=NULL;  _neww=_newh=-1;
	_blitter=NULL;
	_prof_rb.setname("Readback");
	_syncdpy=false;
	_dirty=false;
	_autotestframecount=0;
	_truecolor=true;
	#if defined(sun)||defined(linux)
	_sunrayhandle=RRSunRayInit(windpy, win);
	#endif
	XWindowAttributes xwa;
	XGetWindowAttributes(windpy, win, &xwa);
	if(xwa.depth<24 || xwa.visual->c_class!=TrueColor) _truecolor=false;
}

pbwin::~pbwin(void)
{
	_mutex.lock(false);
	if(_pb) {delete _pb;  _pb=NULL;}
	if(_oldpb) {delete _oldpb;  _oldpb=NULL;}
	if(_blitter) {delete _blitter;  _blitter=NULL;}
	_mutex.unlock(false);
}

int pbwin::init(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	rrcs::safelock l(_mutex);
	if(_pb && _pb->width()==w && _pb->height()==h) return 0;
	if((_pb=new pbuffer(w, h, config))==NULL)
			_throw("Could not create Pbuffer");
	_config=config;
	_force=true;
	return 1;
}

// The resize doesn't actually occur until the next time updatedrawable() is
// called

void pbwin::resize(int w, int h)
{
	rrcs::safelock l(_mutex);
	if(w==0 && _pb) w=_pb->width();
	if(h==0 && _pb) h=_pb->height();
	if(_pb && _pb->width()==w && _pb->height()==h) return;
	_neww=w;  _newh=h;
}

void pbwin::clear(void)
{
	rrcs::safelock l(_mutex);
	if(_pb) _pb->clear();
}

void pbwin::cleanup(void)
{
	rrcs::safelock l(_mutex);
	if(_oldpb) {delete _oldpb;  _oldpb=NULL;}
}

void pbwin::initfromwindow(GLXFBConfig config)
{
	XSync(_windpy, False);
	XWindowAttributes xwa;
	XGetWindowAttributes(_windpy, _win, &xwa);
	init(xwa.width, xwa.height, config);
}

// Get the current Pbuffer drawable

GLXDrawable pbwin::getdrawable(void)
{
	GLXDrawable retval=0;
	rrcs::safelock l(_mutex);
	retval=_pb->drawable();
	return retval;
}

// Get the current Pbuffer drawable, but resize the Pbuffer first if necessary

GLXDrawable pbwin::updatedrawable(void)
{
	GLXDrawable retval=0;
	rrcs::safelock l(_mutex);
	if(_neww>0 && _newh>0)
	{
		pbuffer *oldpb=_pb;
		if(init(_neww, _newh, _config)) _oldpb=oldpb;
		_neww=_newh=-1;
	}
	retval=_pb->drawable();
	return retval;
}

Display *pbwin::getwindpy(void)
{
	return _windpy;
}

Window pbwin::getwin(void)
{
	return _win;
}

void pbwin::swapbuffers(void)
{
	rrcs::safelock l(_mutex);
	if(_pb) _pb->swap();
}

void pbwin::readback(GLint drawbuf, bool force, bool sync)
{
	rrdisplayclient *rrdpy=NULL;
	fconfig.reloadenv();
	int compress=fconfig.compress;  bool dostereo=false;
	int pbw=_pb->width(), pbh=_pb->height();

	rrcs::safelock l(_mutex);

	_dirty=false;

	#if defined(sun)||defined(linux)
	// If this is a SunRay session, then use the SunRay compressor to send data
	if(_sunrayhandle && fconfig.compress==RRCOMP_DEFAULT)
	{
		unsigned char *bitmap=NULL;  int pitch, bottomup, format;
		if(!(bitmap=RRSunRayGetFrame(_sunrayhandle, pbw, pbh, &pitch, &format,
			&bottomup))) _throw(RRSunRayGetError(_sunrayhandle));
		int glformat= (rrsunray_ps[format]==3? GL_RGB:GL_RGBA);
		#ifdef GL_BGR_EXT
		if(format==RRSUNRAY_BGR) glformat=GL_BGR_EXT;
		#endif
		#ifdef GL_BGRA_EXT
		if(format==RRSUNRAY_BGRA) glformat=GL_BGRA_EXT;
		#endif
		#ifdef GL_ABGR_EXT
		if(format==RRSUNRAY_ABGR) glformat=GL_ABGR_EXT;
		#endif
		readpixels(0, 0, pbw, pitch, pbh, glformat, rrsunray_ps[format], bitmap,
			drawbuf, bottomup);
		if(RRSunRaySendFrame(_sunrayhandle, bitmap, pbw, pbh, pitch, format,
			bottomup)==-1) _throw(RRSunRayGetError(_sunrayhandle));
		return;
	}
	#endif

	if(compress==RRCOMP_DEFAULT) compress=RRCOMP_JPEG;

	if(_force) {force=true;  _force=false;}
	if(sync) {compress=RRCOMP_NONE;  force=true;}
	if(pbw*pbh<1000) compress=RRCOMP_NONE;

	if(stereo())
	{
		if(_drawingtoright() || _rdirty) dostereo=true;
		_rdirty=false;
		compress=RRCOMP_JPEG;
	}

	if(!_truecolor) compress=RRCOMP_NONE;

	switch(compress)
	{
		case RRCOMP_JPEG:
		{
			errifnot(rrdpy=dpyh.findrrdpy(_windpy));
			if(fconfig.spoil && rrdpy && !rrdpy->frameready() && !force)
				return;
			if(!rrdpy->stereoenabled() && !fconfig.autotest) dostereo=false;
			rrframe *b;
			errifnot(b=rrdpy->getbitmap(pbw, pbh, 3));
			#ifdef GL_BGR_EXT
			if(littleendian())
			{
				readpixels(0, 0, pbw, pbw*3, pbh, GL_BGR_EXT, 3, b->_bits, drawbuf, true, dostereo);
				b->_flags=RRBMP_BGR;
			}
			else
			#endif
			{
				readpixels(0, 0, pbw, pbw*3, pbh, GL_RGB, 3, b->_bits, drawbuf, true, dostereo);
				b->_flags=0;
			}
			b->_h.winid=_win;
			b->_h.framew=b->_h.width;
			b->_h.frameh=b->_h.height;
			b->_h.x=0;
			b->_h.y=0;
			b->_h.qual=fconfig.currentqual;
			b->_h.subsamp=fconfig.currentsubsamp;
			b->_h.flags=dostereo? RR_LEFT:0;
			b->_flags|=RRBMP_BOTTOMUP;
			if(!_syncdpy) {XSync(_windpy, False);  _syncdpy=true;}
			rrdpy->sendframe(b);

			if(dostereo)
			{
				errifnot(b=rrdpy->getbitmap(pbw, pbh, 3));
				if(drawbuf==GL_FRONT) drawbuf=GL_FRONT_RIGHT;
				else if(drawbuf==GL_BACK) drawbuf=GL_BACK_RIGHT;
				#ifdef GL_BGR_EXT
				if(littleendian())
				{
					readpixels(0, 0, pbw, pbw*3, pbh, GL_BGR_EXT, 3, b->_bits, drawbuf, true, dostereo);
					b->_flags=RRBMP_BGR;
				}
				else
				#endif
				{
					readpixels(0, 0, pbw, pbw*3, pbh, GL_RGB, 3, b->_bits, drawbuf, true, dostereo);
					b->_flags=0;
				}
				b->_h.winid=_win;
				b->_h.framew=b->_h.width;
				b->_h.frameh=b->_h.height;
				b->_h.x=0;
				b->_h.y=0;
				b->_h.qual=fconfig.currentqual;
				b->_h.subsamp=fconfig.currentsubsamp;
				b->_h.flags=RR_RIGHT;
				b->_flags|=RRBMP_BOTTOMUP;
				rrdpy->sendframe(b);
			}

			break;
		}

		case RRCOMP_NONE:
		{
			rrfb *b;
			if(!_blitter) errifnot(_blitter=new rrblitter());
			if(fconfig.spoil && !_blitter->frameready() && !force) return;
			errifnot(b=_blitter->getbitmap(_windpy, _win, pbw, pbh));
			b->_flags|=RRBMP_BOTTOMUP;
			int format;
			unsigned char *bits=b->_bits;
			switch(b->_pixelsize)
			{
				case 1:  format=GL_COLOR_INDEX;  break;
				case 3:
					format=GL_RGB;
					#ifdef GL_BGR_EXT
					if(b->_flags&RRBMP_BGR) format=GL_BGR_EXT;
					#endif
					break;
				case 4:
					format=GL_RGBA;
					#ifdef GL_BGRA_EXT
					if(b->_flags&RRBMP_BGR && !(b->_flags&RRBMP_ALPHAFIRST))
						format=GL_BGRA_EXT;
					#endif
					if(b->_flags&RRBMP_BGR && b->_flags&RRBMP_ALPHAFIRST)
					{
						#ifdef GL_ABGR_EXT
						format=GL_ABGR_EXT;
						#elif defined(GL_BGRA_EXT)
						format=GL_BGRA_EXT;  bits=b->_bits+1;
						#endif
					}
					if(!(b->_flags&RRBMP_BGR) && b->_flags&RRBMP_ALPHAFIRST)
					{
						format=GL_RGBA;  bits=b->_bits+1;
					}
					break;
				default:
					_throw("Unsupported pixel format");
			}
			readpixels(0, 0, min(pbw, b->_h.framew), b->_pitch, min(pbh, b->_h.frameh), format, b->_pixelsize, bits, drawbuf, true);
			_blitter->sendframe(b, sync);
			break;
		}
	}
}

void pbwin::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h,
	GLenum format, int ps, GLubyte *bits, GLint buf, bool bottomup, bool stereo)
{

	GLint readbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &readbuf);

	tempctx tc(_localdpy, EXISTING_DRAWABLE, GetCurrentDrawable());

	glReadBuffer(buf);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

	if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
	else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
	else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
	else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	_prof_rb.startframe();
	if(!bottomup)
	{
		for(int i=0; i<h; i++)
		{
			int yr=h-1-i-y;
			glReadPixels(x, i+y, w, 1, format, GL_UNSIGNED_BYTE, &bits[pitch*yr]);
		}
	}
	else glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, bits);
	_prof_rb.endframe(w*h, 0, stereo? 0.5 : 1);
	checkgl("Read Pixels");

	// If automatic faker testing is enabled, store the FB color in an
	// environment variable so the test program can verify it
	if(fconfig.autotest)
	{
		unsigned char *rowptr, *pixel;  int match=1;
		int color=-1, i, j, k;
		color=-1;
		if(buf!=GL_FRONT_RIGHT && buf!=GL_BACK_RIGHT) _autotestframecount++;
		for(j=0, rowptr=bits; j<h; j++, rowptr+=pitch)
			for(i=1, pixel=&rowptr[ps]; i<w; i++, pixel+=ps)
				for(k=0; k<ps; k++)
				{
					if(pixel[k]!=rowptr[k]) {match=0;  break;}
				}
		if(match)
		{
			if(format==GL_COLOR_INDEX)
			{
				unsigned char index;
				glReadPixels(0, 0, 1, 1, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &index);
				color=index;
			}
			else
			{
				unsigned char rgb[3];
				glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
				color=rgb[0]+(rgb[1]<<8)+(rgb[2]<<16);
			}
		}
		if(buf==GL_FRONT_RIGHT || buf==GL_BACK_RIGHT)
		{
			snprintf(_autotestrclr, 79, "__VGL_AUTOTESTRCLR%x=%d", (unsigned int)_win, color);
			putenv(_autotestrclr);
		}
		else
		{
			snprintf(_autotestclr, 79, "__VGL_AUTOTESTCLR%x=%d", (unsigned int)_win, color);
			putenv(_autotestclr);
		}
		snprintf(_autotestframe, 79, "__VGL_AUTOTESTFRAME%x=%d", (unsigned int)_win, _autotestframecount);
		putenv(_autotestframe);
	}

	glPopClientAttrib();
	tc.restore();
	glReadBuffer(readbuf);
}

bool pbwin::stereo(void)
{
	return (_pb && _pb->stereo());
}
