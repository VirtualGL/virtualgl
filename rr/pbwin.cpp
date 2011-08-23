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
#include "fakerconfig.h"
#include "glxvisual.h"

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


// This class encapsulates the Pbuffer, its most recent ancestor, and
// information specific to its corresponding X window

pbwin::pbwin(Display *dpy, Window win) : pbdrawable(dpy, win)
{
	_eventdpy=NULL;
	_oldpb=NULL;  _neww=_newh=-1;
	_blitter=NULL;
	#ifdef USEXV
	_xvtrans=NULL;
	#endif
	_rrdpy=NULL;
	_prof_gamma.setname("Gamma     ");
	_prof_anaglyph.setname("Anaglyph  ");
	_syncdpy=false;
	_dirty=false;
	_rdirty=false;
	_truecolor=true;
	fconfig_setdefaultsfromdpy(_dpy);
	_plugin=NULL;
	_wmdelete=false;
	_newconfig=false;
	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, win, &xwa);
	if(!(xwa.your_event_mask&StructureNotifyMask))
	{
		if(!(_eventdpy=XOpenDisplay(DisplayString(dpy))))
			_throw("Could not clone X display connection");
		XSelectInput(_eventdpy, win, StructureNotifyMask);
		if(fconfig.verbose)
			rrout.println("[VGL] Selecting structure notify events in window 0x%.8x",
				win);
	}
	if(xwa.depth<24 || xwa.visual->c_class!=TrueColor) _truecolor=false;
	_gammacorrectedvisuals=__vglHasGCVisuals(dpy, DefaultScreen(dpy));
	_stereovisual=__vglClientVisualAttrib(dpy, DefaultScreen(dpy),
		xwa.visual->visualid, GLX_STEREO);
}

pbwin::~pbwin(void)
{
	_mutex.lock(false);
	if(_oldpb) {delete _oldpb;  _oldpb=NULL;}
	if(_blitter) {delete _blitter;  _blitter=NULL;}
	if(_rrdpy) {delete _rrdpy;  _rrdpy=NULL;}
	#ifdef USEXV
	if(_xvtrans) {delete _xvtrans;  _xvtrans=NULL;}
	#endif
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
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	return pbdrawable::init(w, h, config);
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
	pbdrawable::clear();
}

void pbwin::cleanup(void)
{
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	if(_oldpb) {delete _oldpb;  _oldpb=NULL;}
}

void pbwin::initfromwindow(GLXFBConfig config)
{
	XSync(_dpy, False);
	XWindowAttributes xwa;
	XGetWindowAttributes(_dpy, _drawable, &xwa);
	init(xwa.width, xwa.height, config);
}

// Get the current Pbuffer drawable

GLXDrawable pbwin::getglxdrawable(void)
{
	rrcs::safelock l(_mutex);
	if(_wmdelete) _throw("Window has been deleted by window manager");
	return pbdrawable::getglxdrawable();
}

void pbwin::checkresize(void)
{
	if(_eventdpy)
	{
		if(XPending(_eventdpy)>0)
		{
			XEvent event;
			_XNextEvent(_eventdpy, &event);
			if(event.type==ConfigureNotify && event.xconfigure.window==_drawable
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
					fconfig.client:DisplayString(_dpy), fconfig.port);
			}
			sendvgl(_rrdpy, drawbuf, spoillast, dostereo, stereomode,
				(int)compress, fconfig.qual, fconfig.subsamp);
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
		_plugin=new rrplugin(_dpy, _drawable, fconfig.transport);
		_plugin->connect(strlen(fconfig.client)>0?
			fconfig.client:DisplayString(_dpy), fconfig.port);
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
	if(!_truecolor) desiredformat=RRTRANS_INDEX;

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
	if(frame->format==RRTRANS_INDEX) glformat=GL_COLOR_INDEX;

	if(dostereo && stereomode==RRSTEREO_QUADBUF && frame->rbits==NULL)
	{
		static bool message=false;
		if(!message)
		{
			rrout.println("[VGL] NOTICE: Quad-buffered stereo is not supported by the plugin.");
			rrout.println("[VGL]    Using anaglyphic stereo instead.");
			message=true;
		}
		stereomode=RRSTEREO_REDCYAN;				
	}
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
	if(!_syncdpy) {XSync(_dpy, False);  _syncdpy=true;}
	if(fconfig.logo) f.addlogo();
	_plugin->sendframe(frame, sync);
}

void pbwin::sendvgl(rrdisplayclient *rrdpy, GLint drawbuf, bool spoillast,
	bool dostereo, int stereomode, int compress, int qual, int subsamp)
{
	int pbw=_pb->width(), pbh=_pb->height();
	bool usepbo=(fconfig.readback==RRREAD_PBO);
	static bool alreadywarned=false;

	if(spoillast && fconfig.spoil && !rrdpy->ready())
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

	if(!fconfig.spoil) rrdpy->synchronize();
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
	b->_h.winid=_drawable;
	b->_h.framew=b->_h.width;
	b->_h.frameh=b->_h.height;
	b->_h.x=0;
	b->_h.y=0;
	b->_h.qual=qual;
	b->_h.subsamp=subsamp;
	b->_h.compress=(unsigned char)compress;
	if(!_syncdpy) {XSync(_dpy, False);  _syncdpy=true;}
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
	errifnot(b=_blitter->getbitmap(_dpy, _drawable, pbw, pbh));
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
	errifnot(b=_xvtrans->getbitmap(_dpy, _drawable, pbw, pbh));
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
	pbdrawable::readpixels(x, y, w, pitch, h, format, ps, bits, buf, usepbo,
		stereo);

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
}

bool pbwin::stereo(void)
{
	return (_pb && _pb->stereo());
}
