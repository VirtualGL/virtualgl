/* Copyright (C)2005 Sun Microsystems, Inc.
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

#ifndef __RRGLFRAME_H
#define __RRGLFRAME_H

#include "rrframe.h"
#define GLX11
#include "tempctx.h"
#undef GLX11

// Bitmap drawn using OpenGL

#ifdef INFAKER
 #include "faker-sym.h"
 #define XCloseDisplay _XCloseDisplay
 #define XOpenDisplay _XOpenDisplay
#elif defined(XDK)
 #include "xdk-sym.h"
 #define glGetError _glGetError
 #define glGetIntegerv _glGetIntegerv
 #define glPixelStorei _glPixelStorei
 #define glRasterPos2f _glRasterPos2f
 #define glViewport _glViewport
#else
 #include <GL/glx.h>
#endif

#if defined(INFAKER)||defined(XDK)
 #define glXCreateContext _glXCreateContext
 #define glXDestroyContext _glXDestroyContext
 #define glXMakeCurrent _glXMakeCurrent
 #define glXSwapBuffers _glXSwapBuffers
 #define glDrawBuffer _glDrawBuffer
 #define glDrawPixels _glDrawPixels
 #define glFinish _glFinish
#endif

class rrglframe : public rrframe
{
	public:

	rrglframe(char *dpystring, Window win) : rrframe(), _rbits(NULL),
		_stereo(false), _dpy(NULL), _win(win), _ctx(0), _tjhnd(NULL),
		_newdpy(false)
	{
		if(!dpystring || !win) throw(rrerror("rrglframe::rrglframe", "Invalid argument"));
		if(!(_dpy=XOpenDisplay(dpystring))) _throw("Could not open display");
		_newdpy=true;
		_isgl=true;
		#ifdef XDK
		__vgl_loadsymbols();
		#endif
		init(_dpy, win);
	}

	rrglframe(Display *dpy, Window win) : rrframe(), _rbits(NULL),
		_stereo(false), _dpy(NULL), _win(win), _ctx(0), _tjhnd(NULL),
		_newdpy(false)
	{
		if(!dpy || !win) throw(rrerror("rrglframe::rrglframe", "Invalid argument"));
		_dpy=dpy;
		_isgl=true;
		#ifdef XDK
		__vgl_loadsymbols();
		#endif
		init(_dpy, win);
	}

	void init(Display *dpy, Window win)
	{
		XVisualInfo *v=NULL;
		try
		{
			_pixelsize=3;
			XWindowAttributes xwa;
			memset(&xwa, 0, sizeof(xwa));
			XGetWindowAttributes(_dpy, _win, &xwa);
			XVisualInfo vtemp;  int n=0;
			if(!xwa.visual) _throw("Could not get window attributes");
			vtemp.visualid=xwa.visual->visualid;
			int maj_opcode=-1, first_event=-1, first_error=-1;
			if(!XQueryExtension(_dpy, "GLX", &maj_opcode, &first_event, &first_error)
			|| maj_opcode<0 || first_event<0 || first_error<0)
				_throw("GLX extension not available");
			if(!(v=XGetVisualInfo(_dpy, VisualIDMask, &vtemp, &n)) || n==0)
				_throw("Could not obtain visual");
			if(!(_ctx=glXCreateContext(_dpy, v, NULL, True)))
				_throw("Could not create GLX context");
			XFree(v);  v=NULL;
		}
		catch(...)
		{
			if(_dpy && _ctx) {glXMakeCurrent(_dpy, 0, 0);  glXDestroyContext(_dpy, _ctx);  _ctx=0;}
			if(v) XFree(v);
			if(_dpy) {XCloseDisplay(_dpy);  _dpy=NULL;}
			throw;
		}
	}

	~rrglframe(void)
	{
		if(_ctx && _dpy) {glXMakeCurrent(_dpy, 0, 0);  glXDestroyContext(_dpy, _ctx);  _ctx=0;}
		if(_dpy && _newdpy) {XCloseDisplay(_dpy);  _dpy=NULL;}
		if(_tjhnd) {tjDestroy(_tjhnd);  _tjhnd=NULL;}
		if(_rbits) {delete [] _rbits;  _rbits=NULL;}
	}

	void init(rrframeheader *h, bool truecolor=true)
	{
		_flags=RRBMP_BOTTOMUP;
		#ifdef GL_BGR_EXT
		if(littleendian()) _flags|=RRBMP_BGR;
		#endif
		if(!h) throw(rrerror("rrglframe::init", "Invalid argument"));
		_pixelsize=truecolor? 3:1;  _pitch=_pixelsize*h->framew;
		checkheader(h);
		bool stereo=h->flags==RR_LEFT || h->flags==RR_RIGHT;
		if(h->framew!=_h.framew || h->frameh!=_h.frameh || (stereo && !_rbits))
		{
			if(_bits) delete [] _bits;
			errifnot(_bits=new unsigned char[_pitch*h->frameh+1]);
			if(stereo)
			{
				if(_rbits) delete [] _rbits;
				errifnot(_rbits=new unsigned char[_pitch*h->frameh+1]);
			}
		}
		memcpy(&_h, h, sizeof(rrframeheader));
	}

	rrglframe& operator= (rrjpeg& f)
	{
		int tjflags=TJ_BOTTOMUP;
		if(!f._bits || f._h.size<1) _throw("JPEG not initialized");
		init(&f._h);
		if(!_bits) _throw("Bitmap not initialized");
		if(_flags&RRBMP_BGR) tjflags|=TJ_BGR;
		int width=min(f._h.width, _h.framew-f._h.x);
		int height=min(f._h.height, _h.frameh-f._h.y);
		if(width>0 && height>0 && f._h.width<=width && f._h.height<=height)
		{
			if(!_tjhnd)
			{
				if((_tjhnd=tjInitDecompress())==NULL) throw(rrerror("rrglframe::decompressor", tjGetErrorStr()));
			}
			int y=max(0, _h.frameh-f._h.y-height);
			unsigned char *dstbuf=_bits;
			if(f._h.flags==RR_RIGHT)
			{
				_stereo=true;  dstbuf=_rbits;
			}
			tj(tjDecompress(_tjhnd, f._bits, f._h.size, (unsigned char *)&dstbuf[_pitch*y+f._h.x*_pixelsize],
				width, _pitch, height, _pixelsize, tjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		drawtile(0, 0, _h.framew, _h.frameh);
		sync();
	}

	void drawtile(int x, int y, int w, int h)
	{
		if(x<0 || w<1 || (x+w)>_h.framew || y<0 || h<1 || (y+h)>_h.frameh)
			return;
		int format=GL_RGB;
		#ifdef GL_BGR_EXT
		if(littleendian()) format=GL_BGR_EXT;
		#endif
		if(_pixelsize==1) format=GL_COLOR_INDEX;
		
		tempctx tc(_dpy, _win, _win, _ctx, true);

		int e;
		e=glGetError();
		#ifndef _WIN32
		while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
		#endif
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, _pitch/_pixelsize);
		int oldbuf=-1;
		glGetIntegerv(GL_DRAW_BUFFER, &oldbuf);
		if(_stereo) glDrawBuffer(GL_BACK_LEFT);
		glViewport(0, 0, _h.framew, _h.frameh);
		glRasterPos2f(((float)x/(float)_h.framew)*2.-1., ((float)y/(float)_h.frameh)*2.-1.);
		glDrawPixels(w, h, format, GL_UNSIGNED_BYTE, &_bits[_pitch*y+x*_pixelsize]);
		if(_stereo)
		{
			glDrawBuffer(GL_BACK_RIGHT);
			glRasterPos2f(((float)x/(float)_h.framew)*2.-1., ((float)y/(float)_h.frameh)*2.-1.);
			glDrawPixels(w, h, format, GL_UNSIGNED_BYTE, &_rbits[_pitch*y+x*_pixelsize]);
			glDrawBuffer(oldbuf);
			_stereo=false;
		}

		if(glerror()) _throw("Could not draw pixels");
	}

	void sync(void)
	{
		glFinish();
		glXSwapBuffers(_dpy, _win);
	}

	unsigned char *_rbits;

	private:

	// Generic OpenGL error checker (0 = no errors)
	int glerror(void)
	{
		int i, ret=0;
		i=glGetError();
		if(i!=GL_NO_ERROR)
		{
			ret=1;
			rrout.print("[VGL] OpenGL error 0x%.4x\n", i);
		}
		#ifndef _WIN32
		while(i!=GL_NO_ERROR) i=glGetError();
		#endif
		return ret;
	}

	bool _stereo;
	Display *_dpy;  Window _win;
	GLXContext _ctx;
	tjhandle _tjhnd;
	bool _newdpy;
};

#ifdef INFAKER
 #undef XCloseDisplay
 #undef XOpenDisplay
#elif defined(XDK)
 #undef glGetError
 #undef glGetIntegerv
 #undef glPixelStorei
 #undef glRasterPos2f
 #undef glViewport
#endif

#if defined(INFAKER)||defined(XDK)
 #undef glXCreateContext
 #undef glXDestroyContext
 #undef glXMakeCurrent
 #undef glXSwapBuffers
 #undef glDrawBuffer
 #undef glDrawPixels
 #undef glFinish
#endif

#endif // __RRGLFRAME_H
