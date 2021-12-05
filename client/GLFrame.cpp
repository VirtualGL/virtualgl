// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2014, 2017-2019, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

// Frame drawn using OpenGL

#include "GLFrame.h"
#include "Error.h"
#include "Log.h"
#include "vglutil.h"

using namespace util;
using namespace common;


static int tjpf[PIXELFORMATS] =
{
	TJPF_RGB, TJPF_RGBX, -1, TJPF_BGR, TJPF_BGRX, -1, TJPF_XBGR, -1, TJPF_XRGB,
	-1, TJPF_GRAY
};


GLFrame::GLFrame(char *dpystring, Window win_) : Frame(), dpy(NULL), win(win_),
	ctx(0), tjhnd(NULL), newdpy(false)
{
	if(!dpystring || !win)
		throw(Error("GLFrame::GLFrame", "Invalid argument"));

	if(!(dpy = XOpenDisplay(dpystring))) THROW("Could not open display");
	newdpy = true;
	isGL = true;
	init();
}


GLFrame::GLFrame(Display *dpy_, Window win_) : Frame(), dpy(NULL), win(win_),
	ctx(0), tjhnd(NULL), newdpy(false)
{
	if(!dpy_ || !win_) throw(Error("GLFrame::GLFrame", "Invalid argument"));

	dpy = dpy_;
	isGL = true;
	init();
}


void GLFrame::init(void)
{
	XVisualInfo *v = NULL;

	try
	{
		pf = pf_get(PF_RGB);
		XWindowAttributes xwa;
		memset(&xwa, 0, sizeof(xwa));
		XGetWindowAttributes(dpy, win, &xwa);
		XVisualInfo vtemp;  int n = 0;
		if(!xwa.visual) THROW("Could not get window attributes");
		vtemp.visualid = xwa.visual->visualid;
		int maj_opcode = -1, first_event = -1, first_error = -1;
		if(!XQueryExtension(dpy, "GLX", &maj_opcode, &first_event, &first_error)
			|| maj_opcode < 0 || first_event < 0 || first_error < 0)
			THROW("GLX extension not available");
		if(!(v = XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n)) || n == 0)
			THROW("Could not obtain visual");
		if(!(ctx = glXCreateContext(dpy, v, NULL, True)))
			THROW("Could not create GLX context");
		XFree(v);  v = NULL;
	}
	catch(...)
	{
		if(dpy && ctx)
		{
			glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx = 0;
		}
		if(v) XFree(v);
		if(dpy)
		{
			XCloseDisplay(dpy);  dpy = NULL;
		}
		throw;
	}
}


GLFrame::~GLFrame(void)
{
	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx = 0;
	}
	if(dpy && newdpy)
	{
		XCloseDisplay(dpy);  dpy = NULL;
	}
	if(tjhnd)
	{
		tjDestroy(tjhnd);  tjhnd = NULL;
	}
	delete [] rbits;  rbits = NULL;
}


void GLFrame::init(rrframeheader &h, bool stereo_)
{
	int format = PF_RGB;
	if(LittleEndian() && h.compress != RRCOMP_RGB) format = PF_BGR;
	Frame::init(h, format, FRAME_BOTTOMUP, stereo_);
}


GLFrame &GLFrame::operator= (CompressedFrame &cf)
{
	int tjflags = TJ_BOTTOMUP;

	if(!cf.bits || cf.hdr.size < 1) THROW("JPEG not initialized");
	init(cf.hdr, cf.stereo);
	if(!bits) THROW("Frame not initialized");
	int width = min(cf.hdr.width, hdr.framew - cf.hdr.x);
	int height = min(cf.hdr.height, hdr.frameh - cf.hdr.y);
	if(width > 0 && height > 0 && cf.hdr.width <= width
		&& cf.hdr.height <= height)
	{
		if(cf.hdr.compress == RRCOMP_RGB)
		{
			decompressRGB(cf, width, height, false);
			if(stereo && cf.rbits && rbits)
				decompressRGB(cf, width, height, true);
		}
		else
		{
			if(!tjhnd)
			{
				if((tjhnd = tjInitDecompress()) == NULL)
					throw(Error("GLFrame::decompressor", tjGetErrorStr()));
			}
			int y = max(0, hdr.frameh - cf.hdr.y - height);
			TRY_TJ(tjDecompress2(tjhnd, cf.bits, cf.hdr.size,
				&bits[pitch * y + cf.hdr.x * pf->size], width, pitch, height,
				tjpf[pf->id], tjflags));
			if(stereo && cf.rbits && rbits)
			{
				TRY_TJ(tjDecompress2(tjhnd, cf.rbits, cf.rhdr.size,
					&rbits[pitch * y + cf.hdr.x * pf->size], width, pitch, height,
					tjpf[pf->id], tjflags));
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
	if(x < 0 || width < 1 || (x + width) > hdr.framew || y < 0 || height < 1
		|| (y + height) > hdr.frameh)
		return;
	int glFormat = (pf->id == PF_BGR ? GL_BGR : GL_RGB);

	if(!glXMakeCurrent(dpy, win, ctx))
		THROW("Could not bind OpenGL context to window (window may have disappeared)");

	int e;
	e = glGetError();
	while(e != GL_NO_ERROR) e = glGetError();  // Clear previous error
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / pf->size);
	int oldbuf = -1;
	glGetIntegerv(GL_DRAW_BUFFER, &oldbuf);
	if(stereo) glDrawBuffer(GL_BACK_LEFT);
	glViewport(0, 0, hdr.framew, hdr.frameh);
	glRasterPos2f(((float)x / (float)hdr.framew) * 2.0f - 1.0f,
		((float)y / (float)hdr.frameh) * 2.0f - 1.0f);
	glDrawPixels(width, height, glFormat, GL_UNSIGNED_BYTE,
		&bits[pitch * y + x * pf->size]);
	if(stereo)
	{
		glDrawBuffer(GL_BACK_RIGHT);
		glRasterPos2f(((float)x / (float)hdr.framew) * 2.0f - 1.0f,
			((float)y / (float)hdr.frameh) * 2.0f - 1.0f);
		glDrawPixels(width, height, glFormat, GL_UNSIGNED_BYTE,
			&rbits[pitch * y + x * pf->size]);
		glDrawBuffer(oldbuf);
	}

	if(glError()) THROW("Could not draw pixels");
}


void GLFrame::sync(void)
{
	glFinish();
	glXSwapBuffers(dpy, win);
	glXMakeCurrent(dpy, 0, 0);
}


int GLFrame::glError(void)
{
	int i, ret = 0;

	i = glGetError();
	if(i != GL_NO_ERROR)
	{
		ret = 1;
		char *env = NULL;
		if((env = getenv("VGL_VERBOSE")) != NULL && strlen(env) > 0
			&& !strncmp(env, "1", 1))
			vglout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", i);
	}
	while(i != GL_NO_ERROR) i = glGetError();
	return ret;
}
