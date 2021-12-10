// Copyright (C)2011, 2013-2014, 2017, 2019-2021 D. R. Commander
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

#include "VirtualPixmap.h"
#include "glxvisual.h"
#include "vglutil.h"
#include "faker.h"

using namespace util;
using namespace common;
using namespace faker;


VirtualPixmap::VirtualPixmap(Display *dpy_, Visual *visual, Pixmap pm) :
	VirtualDrawable(dpy_, pm)
{
	CriticalSection::SafeLock l(mutex);
	profPMBlit.setName("PMap Blit ");
	frame = new FBXFrame(dpy_, pm, visual, true);
}


VirtualPixmap::~VirtualPixmap()
{
	CriticalSection::SafeLock l(mutex);
	delete frame;  frame = NULL;
}


int VirtualPixmap::init(int width, int height, int depth, VGLFBConfig config_,
	const int *attribs)
{
	if(!config_ || width < 1 || height < 1) THROW("Invalid argument");

	CriticalSection::SafeLock l(mutex);
	if(oglDraw && oglDraw->getWidth() == width && oglDraw->getHeight() == height
		&& oglDraw->getDepth() == depth
		&& FBCID(oglDraw->getFBConfig()) == FBCID(config_))
		return 0;
	#ifdef EGLBACKEND
	if(fconfig.egl)
		oglDraw = new OGLDrawable(dpy, width, height, config_);
	else
	#endif
		oglDraw = new OGLDrawable(width, height, depth, config_, attribs);
	if(config && FBCID(config_) != FBCID(config) && ctx)
	{
		backend::destroyContext(dpy, ctx);  ctx = 0;
	}
	config = config_;
	return 1;
}


// Returns the X11 Pixmap on the 3D X server corresponding to the GLX Pixmap
Pixmap VirtualPixmap::get3DX11Pixmap(void)
{
	GLXDrawable retval = 0;
	CriticalSection::SafeLock l(mutex);
	retval = oglDraw->getPixmap();
	return retval;
}


void VirtualPixmap::readback(void)
{
	if(!checkRenderMode()) return;

	fconfig_reloadenv();

	CriticalSection::SafeLock l(mutex);
	int width = oglDraw->getWidth(), height = oglDraw->getHeight();

	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.x = hdr.y = 0;
	hdr.width = hdr.framew = width;
	hdr.height = hdr.frameh = height;
	frame->init(hdr);

	frame->flags |= FRAME_BOTTOMUP;
	readPixels(0, 0, min(width, frame->hdr.framew), frame->pitch,
		min(height, frame->hdr.frameh), GL_NONE, frame->pf, frame->bits, GL_FRONT,
		false);

	frame->redraw();
}
