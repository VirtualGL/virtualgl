// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2009, 2013-2014, 2018-2019, 2021 D. R. Commander
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

#include "EGLXVirtualWin.h"

using namespace util;
using namespace faker;


EGLXVirtualWin::EGLXVirtualWin(Display *dpy_, Window win, EGLDisplay edpy_,
	EGLConfig config_, const EGLint *pbAttribs_) : VirtualWin(dpy_, win)
{
	if(!edpy_ || !config_) THROW("Invalid argument");

	edpy = edpy_;
	config = (VGLFBConfig)config_;
	direct = True;

	for(EGLint i = 0; i < MAX_ATTRIBS + 1; i++) pbAttribs[i] = EGL_NONE;
	int j = 0;
	EGLint glColorspace = -1;
	if(pbAttribs_)
	{
		for(EGLint i = 0; pbAttribs_[i] != EGL_NONE && i < MAX_ATTRIBS - 2; i += 2)
		{
			if(pbAttribs_[i] == EGL_GL_COLORSPACE
				&& pbAttribs_[i + 1] != EGL_DONT_CARE)
				glColorspace = pbAttribs_[i + 1];
			else if(pbAttribs_[i] != EGL_RENDER_BUFFER)
			{
				pbAttribs[j++] = pbAttribs_[i];  pbAttribs[j++] = pbAttribs_[i + 1];
			}
		}
	}
	// EGL_GL_COLORSPACE_LINEAR is the default for window surfaces, but
	// EGL_GL_COLORSPACE_SRGB is the default for Pbuffer surfaces.
	if(glColorspace < 0) glColorspace = EGL_GL_COLORSPACE_LINEAR;
	if(glColorspace != EGL_GL_COLORSPACE_SRGB)
	{
		pbAttribs[j++] = EGL_GL_COLORSPACE;  pbAttribs[j++] = glColorspace;
	}

	XWindowAttributes xwa;
	XGetWindowAttributes(dpy, win, &xwa);

	oglDraw = new OGLDrawable(edpy, xwa.width, xwa.height, config_, pbAttribs);

	// Resizing an X window causes the corresponding Pbuffer surface to change,
	// so we have to give the 3D application a static EGLSurface handle that
	// always corresponds to this EGLXVirtualWin instance.
	EGLint dummyPBAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
	if(!(dummyPB = _eglCreatePbufferSurface(edpy, config_, dummyPBAttribs)))
		THROW_EGL("eglCreatePbufferSurface() while creating dummy Pbuffer surface");
}


EGLXVirtualWin::~EGLXVirtualWin(void)
{
	if(dummyPB) _eglDestroySurface(edpy, dummyPB);
}


GLXDrawable EGLXVirtualWin::updateGLXDrawable(void)
{
	GLXDrawable retval = 0;
	CriticalSection::SafeLock l(mutex);
	if(deletedByWM) THROW("Window has been deleted by window manager");
	if(newWidth > 0 && newHeight > 0)
	{
		OGLDrawable *draw = oglDraw;
		if(oglDraw->getWidth() != newWidth || oglDraw->getHeight() != newHeight)
		{
			oglDraw = new OGLDrawable(edpy, newWidth, newHeight, (EGLConfig)config,
				pbAttribs);
			oldDraw = draw;
		}
		newWidth = newHeight = -1;
	}
	retval = oglDraw->getGLXDrawable();
	return retval;
}
