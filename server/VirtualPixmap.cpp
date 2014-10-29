/* Copyright (C)2011, 2013-2014 D. R. Commander
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

#include "VirtualPixmap.h"
#include "glxvisual.h"
#include "vglutil.h"
#include "faker.h"

using namespace vglutil;
using namespace vglcommon;
using namespace vglserver;


VirtualPixmap::VirtualPixmap(Display *dpy_, XVisualInfo *vis, Pixmap pm)
	: VirtualDrawable(dpy_, pm)
{
	CriticalSection::SafeLock l(mutex);
	profPMBlit.setName("PMap Blit ");
	_newcheck(frame=new FBXFrame(dpy_, pm, vis->visual, true));
}


VirtualPixmap::~VirtualPixmap()
{
	CriticalSection::SafeLock l(mutex);
	if(frame) { delete frame;  frame=NULL; }
}


int VirtualPixmap::init(int width, int height, int depth, GLXFBConfig config_,
	const int *attribs)
{
	if(!config_ || width<1 || height<1) _throw("Invalid argument");

	CriticalSection::SafeLock l(mutex);
	if(oglDraw && oglDraw->getWidth()==width && oglDraw->getHeight()==height
		&& oglDraw->getDepth()==depth
		&& _FBCID(oglDraw->getConfig())==_FBCID(config_))
		return 0;
	_newcheck(oglDraw=new OGLDrawable(width, height, depth, config_, attribs));
	if(config && _FBCID(config_)!=_FBCID(config) && ctx)
	{
		_glXDestroyContext(_dpy3D, ctx);  ctx=0;
	}
	config=config_;
	return 1;
}


// Returns the X11 Pixmap on the 3D X server corresponding to the GLX Pixmap
Pixmap VirtualPixmap::get3DX11Pixmap(void)
{
	GLXDrawable retval=0;
	CriticalSection::SafeLock l(mutex);
	retval=oglDraw->getPixmap();
	return retval;
}


void VirtualPixmap::readback(void)
{
	fconfig_reloadenv();

	CriticalSection::SafeLock l(mutex);
	int width=oglDraw->getWidth(), height=oglDraw->getHeight();

	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.x=hdr.y=0;
	hdr.width=hdr.framew=width;
	hdr.height=hdr.frameh=height;
	frame->init(hdr);

	frame->flags|=FRAME_BOTTOMUP;
	int format;
	unsigned char *bits=frame->bits;
	switch(frame->pixelSize)
	{
		case 1:  format=GL_COLOR_INDEX;  break;
		case 3:
			format=GL_RGB;
			#ifdef GL_BGR_EXT
			if(frame->flags&FRAME_BGR) format=GL_BGR_EXT;
			#endif
			break;
		case 4:
			format=GL_RGBA;
			#ifdef GL_BGRA_EXT
			if(frame->flags&FRAME_BGR && !(frame->flags&FRAME_ALPHAFIRST))
				format=GL_BGRA_EXT;
			#endif
			if(frame->flags&FRAME_BGR && frame->flags&FRAME_ALPHAFIRST)
			{
				#ifdef GL_ABGR_EXT
				format=GL_ABGR_EXT;
				#elif defined(GL_BGRA_EXT)
				format=GL_BGRA_EXT;  bits=frame->bits+1;
				#endif
			}
			if(!(frame->flags&FRAME_BGR) && frame->flags&FRAME_ALPHAFIRST)
			{
				format=GL_RGBA;  bits=frame->bits+1;
			}
			break;
		default:
			_throw("Unsupported pixel format");
	}
	readPixels(0, 0, min(width, frame->hdr.framew), frame->pitch,
		min(height, frame->hdr.frameh), format, frame->pixelSize, bits, GL_FRONT,
		false);

	frame->redraw();
}
