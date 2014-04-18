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

#include "pbpm.h"
#include "glxvisual.h"
#include "fakerconfig.h"
#include "vglutil.h"

using namespace vglutil;
using namespace vglcommon;


extern Display *_localdpy;


pbpm::pbpm(Display *dpy, XVisualInfo *vis, Pixmap pm) : pbdrawable(dpy, pm)
{
	CS::SafeLock l(_mutex);
	_prof_pmblit.setName("PMap Blit ");
	errifnot(_fb=new FBXFrame(dpy, pm, vis->visual));
}


pbpm::~pbpm()
{
	CS::SafeLock l(_mutex);
	if(_fb) {delete _fb;  _fb=NULL;}
}


int pbpm::init(int w, int h, int depth, GLXFBConfig config, const int *attribs)
{
	if(!config || w<1 || h<1) _throw("Invalid argument");

	CS::SafeLock l(_mutex);
	if(_pb && _pb->width()==w && _pb->height()==h && _pb->depth()==depth
		&& _FBCID(_pb->config())==_FBCID(config)) return 0;
	_pb=new glxdrawable(w, h, depth, config, attribs);
	if(_config && _FBCID(config)!=_FBCID(_config) && _ctx)
	{
		_glXDestroyContext(_localdpy, _ctx);  _ctx=0;
	}
	_config=config;
	return 1;
}


// Returns the X11 Pixmap on the 3D X server corresponding to the GLX Pixmap
Pixmap pbpm::get3dx11drawable(void)
{
	GLXDrawable retval=0;
	CS::SafeLock l(_mutex);
	retval=_pb->pixmap();
	return retval;
}


void pbpm::readback(void)
{
	fconfig_reloadenv();

	CS::SafeLock l(_mutex);
	int pbw=_pb->width(), pbh=_pb->height();

	rrframeheader hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.height=hdr.frameh=pbh;
	hdr.width=hdr.framew=pbw;
	hdr.x=hdr.y=0;
	_fb->init(hdr);

	_fb->flags|=FRAME_BOTTOMUP;
	int format;
	unsigned char *bits=_fb->bits;
	switch(_fb->pixelSize)
	{
		case 1:  format=GL_COLOR_INDEX;  break;
		case 3:
			format=GL_RGB;
			#ifdef GL_BGR_EXT
			if(_fb->flags&FRAME_BGR) format=GL_BGR_EXT;
			#endif
			break;
		case 4:
			format=GL_RGBA;
			#ifdef GL_BGRA_EXT
			if(_fb->flags&FRAME_BGR && !(_fb->flags&FRAME_ALPHAFIRST))
				format=GL_BGRA_EXT;
			#endif
			if(_fb->flags&FRAME_BGR && _fb->flags&FRAME_ALPHAFIRST)
			{
				#ifdef GL_ABGR_EXT
				format=GL_ABGR_EXT;
				#elif defined(GL_BGRA_EXT)
				format=GL_BGRA_EXT;  bits=_fb->bits+1;
				#endif
			}
			if(!(_fb->flags&FRAME_BGR) && _fb->flags&FRAME_ALPHAFIRST)
			{
				format=GL_RGBA;  bits=_fb->bits+1;
			}
			break;
		default:
			_throw("Unsupported pixel format");
	}
	readpixels(0, 0, min(pbw, _fb->hdr.framew), _fb->pitch,
		min(pbh, _fb->hdr.frameh), format, _fb->pixelSize, bits, GL_FRONT, false);

	_fb->redraw();
}
