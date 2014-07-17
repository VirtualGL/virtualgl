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
#include "Error.h"
#include "Log.h"
#include "vglutil.h"

using namespace vglutil;
using namespace vglcommon;


GLFrame::GLFrame(char *dpystring, Window win_) : Frame(),
	dpy(NULL),
	win(win_), ctx(0), tjhnd(NULL), newdpy(false)
{
	if(!dpystring || !win)
		throw(Error("GLFrame::GLFrame", "Invalid argument"));

	if(!(dpy=XOpenDisplay(dpystring))) _throw("Could not open display");
	newdpy=true;
	isGL=true;
	init();
}


GLFrame::GLFrame(Display *dpy_, Window win_) : Frame(),
	dpy(NULL),
	win(win_), ctx(0), tjhnd(NULL), newdpy(false)
{
	if(!dpy_ || !win_) throw(Error("GLFrame::GLFrame", "Invalid argument"));

	dpy=dpy_;
	isGL=true;
	init();
}


void GLFrame::init(void)
{
	XVisualInfo *v=NULL;

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
		if(dpy)
		{
			XCloseDisplay(dpy);  dpy=NULL;
		}
		throw;
	}
}


GLFrame::~GLFrame(void)
{
	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
	}
	if(dpy && newdpy)
	{
		XCloseDisplay(dpy);  dpy=NULL;
	}
	if(tjhnd)
	{
		tjDestroy(tjhnd);  tjhnd=NULL;
	}
	if(rbits)
	{
		delete [] rbits;  rbits=NULL;
	}
}


void GLFrame::init(rrframeheader &h, bool stereo_)
{
	int flags_=FRAME_BOTTOMUP;

	#ifdef GL_BGR_EXT
	if(littleendian() && h.compress!=RRCOMP_RGB) flags_|=FRAME_BGR;
	#endif
	Frame::init(h, 3, flags_, stereo_);
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
			_tj(tjDecompress(tjhnd, cf.bits, cf.hdr.size,
				(unsigned char *)&bits[pitch*y+cf.hdr.x*pixelSize], width, pitch,
				height, pixelSize, tjflags));
			if(stereo && cf.rbits && rbits)
			{
				_tj(tjDecompress(tjhnd, cf.rbits, cf.rhdr.size,
					(unsigned char *)&rbits[pitch*y+cf.hdr.x*pixelSize],
					width, pitch, height, pixelSize, tjflags));
			}
		}
	}
	return *this;
}


void GLFrame::redraw(void)
{
	drawTile(0, 0, hdr.framew, hdr.frameh);
	sync();
}


void GLFrame::drawTile(int x, int y, int width, int height)
{
	if(x<0 || width<1 || (x+width)>hdr.framew || y<0 || height<1
		|| (y+height)>hdr.frameh)
		return;
	int format=GL_RGB;
	#ifdef GL_BGR_EXT
	if(littleendian()) format=GL_BGR_EXT;
	#endif
	if(pixelSize==1) format=GL_COLOR_INDEX;

	if(!glXMakeCurrent(dpy, win, ctx))
		_throw("Could not bind OpenGL context to window (window may have disappeared)");

	int e;
	e=glGetError();
	while(e!=GL_NO_ERROR) e=glGetError();  // Clear previous error
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch/pixelSize);
	int oldbuf=-1;
	glGetIntegerv(GL_DRAW_BUFFER, &oldbuf);
	if(stereo) glDrawBuffer(GL_BACK_LEFT);
	glViewport(0, 0, hdr.framew, hdr.frameh);
	glRasterPos2f(((float)x/(float)hdr.framew)*2.0f-1.0f,
		((float)y/(float)hdr.frameh)*2.0f-1.0f);
	glDrawPixels(width, height, format, GL_UNSIGNED_BYTE,
		&bits[pitch*y+x*pixelSize]);
	if(stereo)
	{
		glDrawBuffer(GL_BACK_RIGHT);
		glRasterPos2f(((float)x/(float)hdr.framew)*2.0f-1.0f,
			((float)y/(float)hdr.frameh)*2.0f-1.0f);
		glDrawPixels(width, height, format, GL_UNSIGNED_BYTE,
			&rbits[pitch*y+x*pixelSize]);
		glDrawBuffer(oldbuf);
	}

	if(glError()) _throw("Could not draw pixels");
}


void GLFrame::sync(void)
{
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
	while(i!=GL_NO_ERROR) i=glGetError();
	return ret;
}
