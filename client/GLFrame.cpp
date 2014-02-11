/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2014 D. R. Commander
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

// Frame drawn using OpenGL

#include "GLFrame.h"
#include "vglutil.h"
#include "Error.h"
#include "Log.h"

using namespace vglcommon;

#ifdef XDK
 #define glGetError _glGetError
 #define glGetIntegerv _glGetIntegerv
 #define glPixelStorei _glPixelStorei
 #define glRasterPos2f _glRasterPos2f
 #define glViewport _glViewport
 #define glXCreateContext _glXCreateContext
 #define glXDestroyContext _glXDestroyContext
 #define glXMakeCurrent _glXMakeCurrent
 #define glXSwapBuffers _glXSwapBuffers
 #define glDrawBuffer _glDrawBuffer
 #define glDrawPixels _glDrawPixels
 #define glFinish _glFinish
#endif


#ifdef XDK
Display *GLFrame::dpy=NULL;
int GLFrame::instanceCount=0;
#endif


GLFrame::GLFrame(char *dpystring, Window win_) : Frame(),
	#ifndef XDK
	dpy(NULL),
	#endif
	win(win_), ctx(0), tjhnd(NULL), newdpy(false)
{
	if(!dpystring || !win)
		throw(Error("GLFrame::GLFrame", "Invalid argument"));

	#ifdef XDK
	CS::SafeLock l(mutex);
	if(!dpy)
	#endif
	if(!(dpy=XOpenDisplay(dpystring))) _throw("Could not open display");
	newdpy=true;
	isGL=true;
	#ifdef XDK
	instanceCount++;
	__vgl_loadsymbols();
	#endif
	init(dpy, win);
}


GLFrame::GLFrame(Display *dpy_, Window win_) : Frame(),
	#ifndef XDK
	dpy(NULL),
	#endif
	win(win_), ctx(0), tjhnd(NULL), newdpy(false)
{
	if(!dpy_ || !win_) throw(Error("GLFrame::GLFrame", "Invalid argument"));

	#ifdef XDK
	CS::SafeLock l(mutex);
	#endif
	dpy=dpy_;
	isGL=true;
	#ifdef XDK
	__vgl_loadsymbols();
	#endif
	init(dpy, win);
}


void GLFrame::init(Display *dpy, Window win)
{
	XVisualInfo *v=NULL;

	#ifdef XDK
	CS::SafeLock l(mutex);
	#endif
	try
	{
		pixelSize=3;
		XWindowAttributes xwa;
		memset(&xwa, 0, sizeof(xwa));
		XGetWindowAttributes(dpy, win, &xwa);
		XVisualInfo vtemp;  int n=0;
		if(!xwa.visual) _throw("Could not get window attributes");
		vtemp.visualid=xwa.visual->visualid;
		int maj_opcode=-1, first_event=-1, first_error=-1;
		if(!XQueryExtension(dpy, "GLX", &maj_opcode, &first_event, &first_error)
			|| maj_opcode<0 || first_event<0 || first_error<0)
			_throw("GLX extension not available");
		if(!(v=XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n)) || n==0)
			_throw("Could not obtain visual");
		if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
			_throw("Could not create GLX context");
		XFree(v);  v=NULL;
	}
	catch(...)
	{
		if(dpy && ctx)
		{
			glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
		}
		if(v) XFree(v);
		#ifndef XDK
		if(dpy)
		{
			XCloseDisplay(dpy);  dpy=NULL;
		}
		#endif
		throw;
	}
}


GLFrame::~GLFrame(void)
{
	#ifdef XDK
	mutex.lock(false);
	#endif
	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
	}
	#ifdef XDK
	instanceCount--;
	if(instanceCount==0 && dpy)
	{
		XCloseDisplay(dpy);  dpy=NULL;
	}
	mutex.unlock(false);
	#else
	if(dpy && newdpy)
	{
		XCloseDisplay(dpy);  dpy=NULL;
	}
	#endif
	if(tjhnd)
	{
		tjDestroy(tjhnd);  tjhnd=NULL;
	}
	if(rbits)
	{
		delete [] rbits;  rbits=NULL;
	}
}


void GLFrame::init(rrframeheader &h, bool stereo)
{
	int flags=FRAME_BOTTOMUP;

	#ifdef GL_BGR_EXT
	if(littleendian() && h.compress!=RRCOMP_RGB) flags|=FRAME_BGR;
	#endif
	Frame::init(h, 3, flags, stereo);
}


GLFrame &GLFrame::operator= (CompressedFrame &cf)
{
	int tjflags=TJ_BOTTOMUP;

	if(!cf.bits || cf.hdr.size<1) _throw("JPEG not initialized");
	init(cf.hdr, cf.stereo);
	if(!bits) _throw("Frame not initialized");
	if(flags&FRAME_BGR) tjflags|=TJ_BGR;
	int width=min(cf.hdr.width, hdr.framew-cf.hdr.x);
	int height=min(cf.hdr.height, hdr.frameh-cf.hdr.y);
	if(width>0 && height>0 && cf.hdr.width<=width && cf.hdr.height<=height)
	{
		if(cf.hdr.compress==RRCOMP_RGB)
		{
			decompressRGB(cf, width, height, false);
			if(stereo && cf.rbits && rbits)
				decompressRGB(cf, width, height, true);
		}
		else
		{
			if(!tjhnd)
			{
				if((tjhnd=tjInitDecompress())==NULL)
					throw(Error("GLFrame::decompressor", tjGetErrorStr()));
			}
			int y=max(0, hdr.frameh-cf.hdr.y-height);
			tj(tjDecompress(tjhnd, cf.bits, cf.hdr.size,
				(unsigned char *)&bits[pitch*y+cf.hdr.x*pixelSize], width, pitch,
				height, pixelSize, tjflags));
			if(stereo && cf.rbits && rbits)
			{
				tj(tjDecompress(tjhnd, cf.rbits, cf.rhdr.size,
					(unsigned char *)&rbits[pitch*y+cf.hdr.x*pixelSize],
					width, pitch, height, pixelSize, tjflags));
			}
		}
	}
	return *this;
}


void GLFrame::redraw(void)
{
	#ifdef XDK
	CS::SafeLock l(mutex);
	#endif
	drawTile(0, 0, hdr.framew, hdr.frameh);
	sync();
}


void GLFrame::drawTile(int x, int y, int w, int h)
{
	if(x<0 || w<1 || (x+w)>hdr.framew || y<0 || h<1 || (y+h)>hdr.frameh)
		return;
	#ifdef XDK
	CS::SafeLock l(mutex);
	#endif
	int format=GL_RGB;
	#ifdef GL_BGR_EXT
	if(littleendian()) format=GL_BGR_EXT;
	#endif
	if(pixelSize==1) format=GL_COLOR_INDEX;
	
	if(!glXMakeCurrent(dpy, win, ctx))
		_throw("Could not bind OpenGL context to window (window may have disappeared)");

	int e;
	e=glGetError();
	#ifndef _WIN32
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	#endif
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch/pixelSize);
	int oldbuf=-1;
	glGetIntegerv(GL_DRAW_BUFFER, &oldbuf);
	if(stereo) glDrawBuffer(GL_BACK_LEFT);
	glViewport(0, 0, hdr.framew, hdr.frameh);
	glRasterPos2f(((float)x/(float)hdr.framew)*2.0f-1.0f,
		((float)y/(float)hdr.frameh)*2.0f-1.0f);
	glDrawPixels(w, h, format, GL_UNSIGNED_BYTE, &bits[pitch*y+x*pixelSize]);
	if(stereo)
	{
		glDrawBuffer(GL_BACK_RIGHT);
		glRasterPos2f(((float)x/(float)hdr.framew)*2.0f-1.0f,
			((float)y/(float)hdr.frameh)*2.0f-1.0f);
		glDrawPixels(w, h, format, GL_UNSIGNED_BYTE,
			&rbits[pitch*y+x*pixelSize]);
		glDrawBuffer(oldbuf);
	}

	if(glError()) _throw("Could not draw pixels");
}


void GLFrame::sync(void)
{
	#ifdef XDK
	CS::SafeLock l(mutex);
	#endif
	glFinish();
	glXSwapBuffers(dpy, win);
	glXMakeCurrent(dpy, 0, 0);
}


int GLFrame::glError(void)
{
	int i, ret=0;

	i=glGetError();
	if(i!=GL_NO_ERROR)
	{
		ret=1;
		char *env=NULL;
		if((env=getenv("VGL_VERBOSE"))!=NULL && strlen(env)>0
			&& !strncmp(env, "1", 1))
			vglout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", i);
	}
	#ifndef _WIN32
	while(i!=GL_NO_ERROR) i=glGetError();
	#endif
	return ret;
}
