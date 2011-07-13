/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2011 D. R. Commander
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

#include "pbwin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef USEMEDIALIB
#include <mlib.h>
#endif
#include "glxvisual.h"
#include "glext-vgl.h"

#define INFAKER
#include "tempctx.h"
#include "fakerconfig.h"

extern Display *_localdpy;
#ifdef USEGLP
extern GLPDevice _localdev;
#endif

#define checkgl(m) if(glerror()) _throw("Could not "m);

#define _isright(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)
#define leye(buf) (buf==GL_BACK? GL_BACK_LEFT: \
	(buf==GL_FRONT? GL_FRONT_LEFT:buf))
#define reye(buf) (buf==GL_BACK? GL_BACK_RIGHT: \
	(buf==GL_FRONT? GL_FRONT_RIGHT:buf))

static inline int _drawingtoright(void)
{
	GLint drawbuf=GL_LEFT;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
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
		rrout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", i);
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

	_cleared=false;  _stereo=false;  _format=0;
	#if 0
	const char *glxext=NULL;
	glxext=_glXQueryExtensionsString(dpy, DefaultScreen(dpy));
	if(!glxext || !strstr(glxext, "GLX_SGIX_pbuffer"))
		_throw("Pbuffer extension not supported on rendering display");
	#endif

	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0,
		GLX_PRESERVED_CONTENTS, True, None};

	_w=w;  _h=h;  _config=config;
	pbattribs[1]=w;  pbattribs[3]=h;
	#ifdef SUNOGL
	tempctx tc(_localdpy, 0, 0, 0);
	#endif
	if(fconfig.usewindow) _drawable=create_window(_localdpy, config, w, h);
	else _drawable=glXCreatePbuffer(_localdpy, config, pbattribs);
	if(__vglServerVisualAttrib(config, GLX_STEREO)) _stereo=true;
	int pixelsize=__vglServerVisualAttrib(config, GLX_RED_SIZE)
		+__vglServerVisualAttrib(config, GLX_GREEN_SIZE)
		+__vglServerVisualAttrib(config, GLX_BLUE_SIZE)
		+__vglServerVisualAttrib(config, GLX_ALPHA_SIZE);
	if(pixelsize==32)
	{
		#ifdef GL_BGRA_EXT
		if(littleendian()) _format=GL_BGRA_EXT;
		else
		#endif
		_format=GL_RGBA;
	}
	else
	{
		#ifdef GL_BGR_EXT
		if(littleendian()) _format=GL_BGR_EXT;
		else
		#endif
		_format=GL_RGB;
	}
	if(!_drawable) _throw("Could not create Pbuffer");
}

pbuffer::~pbuffer(void)
{
	if(_drawable)
	{
		#ifdef SUNOGL
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
	_glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(params[0], params[1], params[2], params[3]);
}

void pbuffer::swap(void)
{
	if(!fconfig.glp) _glXSwapBuffers(_localdpy, _drawable);
	#ifdef USEGLP
	else if(__glPSwapBuffers) _glPSwapBuffers(_drawable);
	#endif
}


// This class encapsulates the Pbuffer, its most recent ancestor, and
// information specific to its corresponding X window

pbwin::pbwin(Display *windpy, Window win)
{
	if(!windpy || !win) _throw("Invalid argument");
	_eventdpy=NULL;
	_windpy=windpy;  _win=win;
	_oldpb=_pb=NULL;  _neww=_newh=-1;
	_blitter=NULL;
	#ifdef USEXV
	_xvtrans=NULL;
	#endif
	_rrdpy=_rrmoviedpy=NULL;
	_prof_rb.setname("Readback  ");
	_prof_gamma.setname("Gamma     ");
	_prof_anaglyph.setname("Anaglyph  ");
	_syncdpy=false;
	_dirty=false;
	_rdirty=false;
	_autotestframecount=0;
	_truecolor=true;
	fconfig_setdefaultsfromdpy(_windpy);
	_plugin=NULL;
	_wmdelete=false;
	_newconfig=false;
	XWindowAttributes xwa;
	XGetWindowAttributes(windpy, win, &xwa);
	if(!(xwa.your_event_mask&StructureNotifyMask))
	{
		if(!(_eventdpy=XOpenDisplay(DisplayString(windpy))))
			_throw("Could not clone X display connection");
		XSelectInput(_eventdpy, win, StructureNotifyMask);
		if(fconfig.verbose)
			rrout.println("[VGL] Selecting structure notify events in window 0x%.8x",
				win);
	}
	if(xwa.depth<24 || xwa.visual->c_class!=TrueColor) _truecolor=false;
	_gammacorrectedvisuals=__vglHasGCVisuals(windpy, DefaultScreen(windpy));
	_stereovisual=__vglClientVisualAttrib(windpy, DefaultScreen(windpy),
		xwa.visual->visualid, GLX_STEREO);
}

pbwin::~pbwin(void)
{
	_mutex.lock(false);
	if(_pb) {delete _pb;  _pb=NULL;}
	if(_oldpb) {delete _oldpb;  _oldpb=NULL;}
	if(_blitter) {delete _blitter;  _blitter=NULL;}
	if(_rrdpy) {delete _rrdpy;  _rrdpy=NULL;}
	#ifdef USEXV
	if(_xvtrans) {delete _xvtrans;  _xvtrans=NULL;}
	#endif
	if(_rrmoviedpy) {delete _rrmoviedpy;  _rrmoviedpy=NULL;}
	if(_plugin)
	{
		try {delete _plugin;}
		catch (rrerror &e)
		{
			if(fconfig.verbose)
				rrout.println("[VGL] WARNING: %s", e.getMessage());
		}
	}
	if(_eventdpy) {XCloseDisplay(_eventdpy);  _eventdpy=NULL;}
	_mutex.unlock(false);
}

int pbwin::init(int w, int h, GLXFBConfig config)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(_pb && _pb->width()==w && _pb->height()==h
		&& _FBCID(_pb->config())==_FBCID(config)) return 0;
	if((_pb=new pbuffer(w, h, config))==NULL)
			_throw("Could not create Pbuffer");
	_config=config;
	return 1;
}

// The resize doesn't actually occur until the next time updatedrawable() is
// called

void pbwin::resize(int w, int h)
{
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(w==0 && _pb) w=_pb->width();
	if(h==0 && _pb) h=_pb->height();
	if(_pb && _pb->width()==w && _pb->height()==h)
	{
		_neww=_newh=-1;
		return;
	}
	_neww=w;  _newh=h;
}

// The FB config change doesn't actually occur until the next time
// updatedrawable() is called

void pbwin::checkconfig(GLXFBConfig config)
{
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(_FBCID(config)!=_FBCID(_config))
	{
		_config=config;  _newconfig=true;
	}
}

void pbwin::clear(void)
{
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(_pb) _pb->clear();
}

void pbwin::cleanup(void)
{
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
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
	if(_wmdelete) _throw("Window has been deleted by window manager");
	retval=_pb->drawable();
	return retval;
}

void pbwin::checkresize(void)
{
	if(_eventdpy)
	{
		if(XPending(_eventdpy)>0)
		{
			XEvent event;
			_XNextEvent(_eventdpy, &event);
			if(event.type==ConfigureNotify && event.xconfigure.window==_win
				&& event.xconfigure.width>0 && event.xconfigure.height>0)
				resize(event.xconfigure.width, event.xconfigure.height);
		}
	}
}

// Get the current Pbuffer drawable, but resize the Pbuffer (or change its
// FB config) first if necessary

GLXDrawable pbwin::updatedrawable(void)
{
	GLXDrawable retval=0;
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(_newconfig)
	{
		if(_neww<=0 && _pb) _neww=_pb->width();
		if(_newh<=0 && _pb) _newh=_pb->height();
		_newconfig=false;
	}
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
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(_pb) _pb->swap();
}

void pbwin::wmdelete(void)
{
	rrcs::safelock l(_mutex);
	_wmdelete=true;
}

void pbwin::readback(GLint drawbuf, bool spoillast, bool sync)
{
	fconfig_reloadenv();
	bool dostereo=false;  int stereomode=fconfig.stereo;

	if(fconfig.readback==RRREAD_NONE) return;

	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");

	_dirty=false;

	int compress=fconfig.compress;
	if(sync && strlen(fconfig.transport)==0) {compress=RRCOMP_PROXY;}

	if(stereo() && stereomode!=RRSTEREO_LEYE && stereomode!=RRSTEREO_REYE)
	{
		if(_drawingtoright() || _rdirty) dostereo=true;
		_rdirty=false;
		if(dostereo && compress==RRCOMP_YUV && strlen(fconfig.transport)==0)
		{
			static bool message3=false;
			if(!message3)
			{
				rrout.println("[VGL] NOTICE: Quad-buffered stereo cannot be used with YUV encoding.");
				rrout.println("[VGL]    Using anaglyphic stereo instead.");
				message3=true;
			}
			stereomode=RRSTEREO_REDCYAN;				
		}
		else if(dostereo && _Trans[compress]!=RRTRANS_VGL && stereomode==RRSTEREO_QUADBUF
			&& strlen(fconfig.transport)==0)
		{
			static bool message=false;
			if(!message)
			{
				rrout.println("[VGL] NOTICE: Quad-buffered stereo requires the VGL Transport.");
				rrout.println("[VGL]    Using anaglyphic stereo instead.");
				message=true;
			}
			stereomode=RRSTEREO_REDCYAN;				
		}
		else if(dostereo && !_stereovisual && stereomode==RRSTEREO_QUADBUF
			&& strlen(fconfig.transport)==0)
		{
			static bool message2=false;
			if(!message2)
			{
				rrout.println("[VGL] NOTICE: Cannot use quad-buffered stereo because no stereo visuals are");
				rrout.println("[VGL]    available on the client.  Using anaglyphic stereo instead.");
				message2=true;
			}
			stereomode=RRSTEREO_REDCYAN;				
		}
	}

	if(!_truecolor && strlen(fconfig.transport)==0) compress=RRCOMP_PROXY;

	bool sharerrdpy=false;
	if(strlen(fconfig.moviefile)>0)
	{
		if(fconfig.mcompress==compress && fconfig.mqual==fconfig.qual
			&& fconfig.msubsamp==fconfig.subsamp && !fconfig.spoil)
			sharerrdpy=true;

		if(!sharerrdpy)
		{
			if(!_rrmoviedpy)
				errifnot(_rrmoviedpy=new rrdisplayclient());
			_rrmoviedpy->record(true);
			sendvgl(_rrmoviedpy, drawbuf, false, dostereo, RRSTEREO_QUADBUF,
				fconfig.mcompress, fconfig.mqual, fconfig.msubsamp, true);
		}
	}

	if(strlen(fconfig.transport)>0)
	{
		sendplugin(drawbuf, spoillast, sync, dostereo, stereomode);
		return;
	}

	switch(compress)
	{
		case RRCOMP_PROXY:
			sendx11(drawbuf, spoillast, sync, dostereo, stereomode);
			break;

		case RRCOMP_JPEG:
		case RRCOMP_RGB:
		case RRCOMP_YUV:
			if(!_rrdpy)
			{
				errifnot(_rrdpy=new rrdisplayclient());
				_rrdpy->connect(strlen(fconfig.client)>0?
					fconfig.client:DisplayString(_windpy), fconfig.port);
			}
			_rrdpy->record(sharerrdpy);
			sendvgl(_rrdpy, drawbuf, spoillast, dostereo, stereomode,
				(int)compress, fconfig.qual, fconfig.subsamp, sharerrdpy);
			break;
		#ifdef USEXV
		case RRCOMP_XV:
			sendxv(drawbuf, spoillast, sync, dostereo, stereomode);
		#endif
	}
}

void pbwin::sendplugin(GLint drawbuf, bool spoillast, bool sync,
	bool dostereo, int stereomode)
{
	rrframe f;  bool usepbo=(fconfig.readback==RRREAD_PBO);
	int pbw=_pb->width(), pbh=_pb->height();
	RRFrame *frame=NULL;
	static bool alreadywarned=false;

	if(!_plugin)
	{
		_plugin=new rrplugin(_windpy, _win, fconfig.transport);
		_plugin->connect(strlen(fconfig.client)>0?
			fconfig.client:DisplayString(_windpy), fconfig.port);
	}
	if(spoillast && fconfig.spoil && !_plugin->ready())
		return;
	if(!fconfig.spoil) _plugin->synchronize();

	int desiredformat=RRTRANS_RGB;
	#ifdef GL_BGR_EXT
	if(_pb->format()==GL_BGR_EXT) desiredformat=RRTRANS_BGR;
	#endif
	#ifdef GL_BGRA_EXT
	if(_pb->format()==GL_BGRA_EXT) desiredformat=RRTRANS_BGRA;
	#endif
	if(_pb->format()==GL_RGBA) desiredformat=RRTRANS_RGBA;

	frame=_plugin->getframe(pbw, pbh, desiredformat,
		dostereo && stereomode==RRSTEREO_QUADBUF);
	if(usepbo && desiredformat!=frame->format)
	{
		usepbo=false;
		if(fconfig.verbose && !alreadywarned)
		{
			alreadywarned=true;
			rrout.println("[VGL] NOTICE: The pixel format of the buffer returned by the transport plugin");
			rrout.println("[VGL]    does not match the pixel format of the Pbuffer.  Disabling PBO's.");
		}
	}
	f.init(frame->bits, frame->w, frame->pitch, frame->h,
		rrtrans_ps[frame->format], (rrtrans_bgr[frame->format]? RRBMP_BGR:0) |
		(rrtrans_afirst[frame->format]? RRBMP_ALPHAFIRST:0) |
		RRBMP_BOTTOMUP);
	int glformat= (rrtrans_ps[frame->format]==3? GL_RGB:GL_RGBA);
	#ifdef GL_BGR_EXT
	if(frame->format==RRTRANS_BGR) glformat=GL_BGR_EXT;
	#endif
	#ifdef GL_BGRA_EXT
	if(frame->format==RRTRANS_BGRA) glformat=GL_BGRA_EXT;
	#endif
	#ifdef GL_ABGR_EXT
	// FIXME: Implement ARGB properly
	if(frame->format==RRTRANS_ABGR || frame->format==RRTRANS_ARGB)
		glformat=GL_ABGR_EXT;
	#endif
	if(dostereo && stereomode==RRSTEREO_REDCYAN) makeanaglyph(&f, drawbuf);
	else
	{
		GLint buf=drawbuf;
		if(dostereo || stereomode==RRSTEREO_LEYE) buf=leye(drawbuf);
		if(stereomode==RRSTEREO_REYE) buf=reye(drawbuf);
		readpixels(0, 0, frame->w, frame->pitch, frame->h, glformat,
			rrtrans_ps[frame->format], frame->bits, buf, usepbo, dostereo);
		if(dostereo && frame->rbits)
			readpixels(0, 0, frame->w, frame->pitch, frame->h, glformat,
				rrtrans_ps[frame->format], frame->rbits, reye(drawbuf), usepbo,
				dostereo);
	}
	if(!_syncdpy) {XSync(_windpy, False);  _syncdpy=true;}
	if(fconfig.logo) f.addlogo();
	_plugin->sendframe(frame, sync);
}

void pbwin::sendvgl(rrdisplayclient *rrdpy, GLint drawbuf, bool spoillast,
	bool dostereo, int stereomode, int compress, int qual, int subsamp,
	bool domovie)
{
	int pbw=_pb->width(), pbh=_pb->height();
	bool usepbo=(fconfig.readback==RRREAD_PBO);
	static bool alreadywarned=false;

	if(spoillast && fconfig.spoil && !rrdpy->ready() && !domovie)
		return;
	rrframe *b;

	int flags=RRBMP_BOTTOMUP, format=GL_RGB, pixelsize=3;
	if(compress!=RRCOMP_RGB)
	{
		format=_pb->format();
		if(_pb->format()==GL_RGBA) pixelsize=4;
		#ifdef GL_BGR_EXT
		else if(_pb->format()==GL_BGR_EXT) flags|=RRBMP_BGR;
		#endif
		#ifdef GL_BGRA_EXT
		else if(_pb->format()==GL_BGRA_EXT) {flags|=RRBMP_BGR;  pixelsize=4;}
		#endif
	}
	if(usepbo && format!=_pb->format())
	{
		usepbo=false;
		if(fconfig.verbose && !alreadywarned)
		{
			alreadywarned=true;
			rrout.println("[VGL] NOTICE: RGB encoding requires RGB pixel readback, which does not match");
			rrout.println("[VGL}    the pixel format of the Pbuffer.  Disabling PBO's.");
		}
	}

	if(domovie || !fconfig.spoil) rrdpy->synchronize();
	errifnot(b=rrdpy->getbitmap(pbw, pbh, pixelsize, flags,
		dostereo && stereomode==RRSTEREO_QUADBUF));
	if(dostereo && stereomode==RRSTEREO_REDCYAN) makeanaglyph(b, drawbuf);
	else
	{
		GLint buf=drawbuf;
		if(dostereo || stereomode==RRSTEREO_LEYE) buf=leye(drawbuf);
		if(stereomode==RRSTEREO_REYE) buf=reye(drawbuf);
		readpixels(0, 0, b->_h.framew, b->_pitch, b->_h.frameh, format,
			b->_pixelsize, b->_bits, buf, usepbo, dostereo);
		if(dostereo && b->_rbits)
			readpixels(0, 0, b->_h.framew, b->_pitch, b->_h.frameh, format,
				b->_pixelsize, b->_rbits, reye(drawbuf), usepbo, dostereo);
	}
	b->_h.winid=_win;
	b->_h.framew=b->_h.width;
	b->_h.frameh=b->_h.height;
	b->_h.x=0;
	b->_h.y=0;
	b->_h.qual=qual;
	b->_h.subsamp=subsamp;
	b->_h.compress=(unsigned char)compress;
	if(!_syncdpy) {XSync(_windpy, False);  _syncdpy=true;}
	if(fconfig.logo) b->addlogo();
	rrdpy->sendframe(b);
}

void pbwin::sendx11(GLint drawbuf, bool spoillast, bool sync, bool dostereo,
	int stereomode)
{
	int pbw=_pb->width(), pbh=_pb->height();
	bool usepbo=(fconfig.readback==RRREAD_PBO);
	int desiredformat=_pb->format();
	static bool alreadywarned=false;

	rrfb *b;
	if(!_blitter) errifnot(_blitter=new rrblitter());
	if(spoillast && fconfig.spoil && !_blitter->ready()) return;
	if(!fconfig.spoil) _blitter->synchronize();
	errifnot(b=_blitter->getbitmap(_windpy, _win, pbw, pbh));
	b->_flags|=RRBMP_BOTTOMUP;
	if(dostereo && stereomode==RRSTEREO_REDCYAN) makeanaglyph(b, drawbuf);
	else
	{
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
		GLint buf=drawbuf;
		if(stereomode==RRSTEREO_REYE) buf=reye(drawbuf);
		else if(stereomode==RRSTEREO_LEYE) buf=leye(drawbuf);
		if(usepbo && format!=desiredformat)
		{
			usepbo=false;
			if(fconfig.verbose && !alreadywarned)
			{
				alreadywarned=true;
				rrout.println("[VGL] NOTICE: Pixel format of 2D X server does not match pixel format of");
				rrout.println("[VGL}    Pbuffer.  Disabling PBO's.");
			}
		}
		readpixels(0, 0, min(pbw, b->_h.framew), b->_pitch,
			min(pbh, b->_h.frameh), format, b->_pixelsize, bits, buf, usepbo, false);
	}
	if(fconfig.logo) b->addlogo();
	_blitter->sendframe(b, sync);
}

#ifdef USEXV

void pbwin::sendxv(GLint drawbuf, bool spoillast, bool sync, bool dostereo,
	int stereomode)
{
	int pbw=_pb->width(), pbh=_pb->height();
	bool usepbo=(fconfig.readback==RRREAD_PBO);

	rrxvframe *b;
	if(!_xvtrans) errifnot(_xvtrans=new rrxvtrans());
	if(spoillast && fconfig.spoil && !_xvtrans->ready()) return;
	if(!fconfig.spoil) _xvtrans->synchronize();
	errifnot(b=_xvtrans->getbitmap(_windpy, _win, pbw, pbh));
	rrframeheader hdr;
	hdr.height=hdr.frameh=pbh;
	hdr.width=hdr.framew=pbw;
	hdr.x=hdr.y=0;

	int flags=RRBMP_BOTTOMUP, format=_pb->format(), pixelsize=3;
	if(_pb->format()==GL_RGBA) pixelsize=4;
	#ifdef GL_BGR_EXT
	else if(_pb->format()==GL_BGR_EXT) flags|=RRBMP_BGR;
	#endif
	#ifdef GL_BGRA_EXT
	else if(_pb->format()==GL_BGRA_EXT) {flags|=RRBMP_BGR;  pixelsize=4;}
	#endif

	_f.init(hdr, pixelsize, flags, false);

	if(dostereo && stereomode==RRSTEREO_REDCYAN) makeanaglyph(&_f, drawbuf);
	else
	{
		GLint buf=drawbuf;
		if(stereomode==RRSTEREO_REYE) buf=reye(drawbuf);
		else if(stereomode==RRSTEREO_LEYE) buf=leye(drawbuf);
		readpixels(0, 0, min(pbw, _f._h.framew), _f._pitch,
			min(pbh, _f._h.frameh), format, _f._pixelsize, _f._bits, buf, usepbo,
			false);
	}
	
	if(fconfig.logo) _f.addlogo();

	*b=_f;
	_xvtrans->sendframe(b, sync);
}

#endif

void pbwin::makeanaglyph(rrframe *b, int drawbuf)
{
	_r.init(b->_h, 1, b->_flags, false);
	readpixels(0, 0, _r._h.framew, _r._pitch, _r._h.frameh, GL_RED,
		_r._pixelsize, _r._bits, leye(drawbuf), false, false);
	_g.init(b->_h, 1, b->_flags, false);
	readpixels(0, 0, _g._h.framew, _g._pitch, _g._h.frameh, GL_GREEN,
		_g._pixelsize, _g._bits, reye(drawbuf), false, false);
	_b.init(b->_h, 1, b->_flags, false);
	readpixels(0, 0, _b._h.framew, _b._pitch, _b._h.frameh, GL_BLUE,
		_b._pixelsize, _b._bits, reye(drawbuf), false, false);
	_prof_anaglyph.startframe();
	b->makeanaglyph(_r, _g, _b);
	_prof_anaglyph.endframe(b->_h.framew*b->_h.frameh, 0, 1);
}

void pbwin::readpixels(GLint x, GLint y, GLint w, GLint pitch, GLint h,
	GLenum format, int ps, GLubyte *bits, GLint buf, bool usepbo, bool stereo)
{

	GLint readbuf=GL_BACK, oldrendermode=GL_RENDER;
	GLint fbr=0, fbw=0;
	#ifdef GL_VERSION_1_5
	static GLuint pbo=0;  int boundbuffer=0;
	#endif
	static const char *ext=NULL;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &fbr);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &fbw);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	_glGetIntegerv(GL_READ_BUFFER, &readbuf);
	_glGetIntegerv(GL_RENDER_MODE, &oldrendermode);

	tempctx tc(_localdpy, EXISTING_DRAWABLE, GetCurrentDrawable());

	glReadBuffer(buf);
	glRenderMode(GL_RENDER);
	glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

	if(pitch%8==0) glPixelStorei(GL_PACK_ALIGNMENT, 8);
	else if(pitch%4==0) glPixelStorei(GL_PACK_ALIGNMENT, 4);
	else if(pitch%2==0) glPixelStorei(GL_PACK_ALIGNMENT, 2);
	else if(pitch%1==0) glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glPushAttrib(GL_PIXEL_MODE_BIT);
	_glPixelTransferf(GL_RED_SCALE, 1.0);
	_glPixelTransferf(GL_RED_BIAS, 0.0);
	_glPixelTransferf(GL_GREEN_SCALE, 1.0);
	_glPixelTransferf(GL_GREEN_BIAS, 0.0);
	_glPixelTransferf(GL_BLUE_SCALE, 1.0);
	_glPixelTransferf(GL_BLUE_BIAS, 0.0);
	_glPixelTransferf(GL_ALPHA_SCALE, 1.0);
	_glPixelTransferf(GL_ALPHA_BIAS, 0.0);

	if(usepbo)
	{
		if(!ext)
		{
			ext=(const char *)glGetString(GL_EXTENSIONS);
			if(!ext || !strstr(ext, "GL_ARB_pixel_buffer_object"))
				_throw("GL_ARB_pixel_buffer_object extension not available");
		}
		#ifdef GL_VERSION_1_5
		if(!pbo)
		{
			glGenBuffers(1, &pbo);
			if(!pbo) _throw("Could not generate pixel buffer object");
			if(fconfig.verbose)
				rrout.println("[VGL] Using pixel buffer objects for readback (GL format = 0x%.4x)",
				format);
		}
		glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_EXT, &boundbuffer);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, pbo);
		int size=0;
		glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size!=pitch*h)
			glBufferData(GL_PIXEL_PACK_BUFFER_EXT, pitch*h, NULL, GL_STREAM_READ);
		glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE, &size);
		if(size!=pitch*h)
			_throw("Could not set PBO size");
		#else
		_throw("PBO support not compiled in.  Rebuild VGL on a system that has OpenGL 1.5.");
		#endif
	}
	else
	{
		static bool alreadyprinted=false;
		if(!alreadyprinted && fconfig.verbose)
		{
			rrout.println("[VGL] Using synchronous readback (GL format = 0x%.4x)",
				format);
			alreadyprinted=true;
		}
	}

	int e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	_prof_rb.startframe();
	glReadPixels(x, y, w, h, format, GL_UNSIGNED_BYTE, usepbo? NULL:bits);

	if(usepbo)
	{
		#ifdef GL_VERSION_1_5
		unsigned char *pbobits=NULL;
		pbobits=(unsigned char *)glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT,
			GL_READ_ONLY);
		if(!pbobits) _throw("Could not map pixel buffer object");
		memcpy(bits, pbobits, pitch*h);
		if(!glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT))
			_throw("Could not unmap pixel buffer object");
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, boundbuffer);
		#endif
	}

	_prof_rb.endframe(w*h, 0, stereo? 0.5 : 1);
	checkgl("Read Pixels");

	// Gamma correction
	if((!_gammacorrectedvisuals || !fconfig.gamma_usesun)
		&& fconfig.gamma!=0.0 && fconfig.gamma!=1.0 && fconfig.gamma!=-1.0)
	{
		_prof_gamma.startframe();
		static bool first=true;
		#if defined(USEMEDIALIB) && defined(sun)
		if(first)
		{
			first=false;
			if(fconfig.verbose)
				rrout.println("[VGL] Using mediaLib gamma correction (correction factor=%f)\n",
					fconfig.gamma);
		}
		mlib_image *image=NULL;
		if((image=mlib_ImageCreateStruct(MLIB_BYTE, ps, w, h, pitch, bits))!=NULL)
		{
			unsigned char *luts[4]={fconfig.gamma_lut, fconfig.gamma_lut,
				fconfig.gamma_lut, fconfig.gamma_lut};
			mlib_ImageLookUp_Inp(image, (const void **)luts);
			mlib_ImageDelete(image);
		}
		else
		{
		#endif
		if(first)
		{
			first=false;
			if(fconfig.verbose)
				rrout.println("[VGL] Using software gamma correction (correction factor=%f)\n",
					fconfig.gamma);
		}
		unsigned short *ptr1, *ptr2=(unsigned short *)(&bits[pitch*h]);
		for(ptr1=(unsigned short *)bits; ptr1<ptr2; ptr1++)
			*ptr1=fconfig.gamma_lut16[*ptr1];
		if((pitch*h)%2!=0) bits[pitch*h-1]=fconfig.gamma_lut[bits[pitch*h-1]];
		#if defined(USEMEDIALIB) && defined(sun)
		}
		#endif
		_prof_gamma.endframe(w*h, 0, stereo?0.5 : 1);
	}

	// If automatic faker testing is enabled, store the FB color in an
	// environment variable so the test program can verify it
	if(fconfig.autotest)
	{
		unsigned char *rowptr, *pixel;  int match=1;
		int color=-1, i, j, k;
		color=-1;
		if(buf!=GL_FRONT_RIGHT && buf!=GL_BACK_RIGHT) _autotestframecount++;
		for(j=0, rowptr=bits; j<h && match; j++, rowptr+=pitch)
			for(i=1, pixel=&rowptr[ps]; i<w && match; i++, pixel+=ps)
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

	glRenderMode(oldrendermode);
	_glPopAttrib();
	glPopClientAttrib();
	tc.restore();

	glReadBuffer(readbuf);
	if(fbr) glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbr);
	if(fbw) glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fbw);
}

bool pbwin::stereo(void)
{
	return (_pb && _pb->stereo());
}
