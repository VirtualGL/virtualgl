/* Copyright (C)2004 Landmark Graphics
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

#include "glxvisual.h"
#include <stdio.h>
#include <stdlib.h>

// This class attempts to manage visual properties across Windows and Pbuffers
// and across GLX 1.0 and 1.3 in a sane manner

#include "rrerror.h"

#define _case(ec) case ec: return "GLX Error: "#ec;
static const char *glxerr(int glxerror)
{
	switch(glxerror)
	{
		_case(GLX_BAD_SCREEN)
		_case(GLX_BAD_ATTRIBUTE)
		_case(GLX_NO_EXTENSION)
		_case(GLX_BAD_VISUAL)
		_case(GLX_BAD_CONTEXT)
		_case(GLX_BAD_VALUE)
		_case(GLX_BAD_ENUM)
		default: return "No GLX Error";
	}
}
static int __glxerr=0;
#define glx(f) {if((__glxerr=(f))!=0) _throw(glxerr(__glxerr));}

glxvisual::glxvisual(Display *dpy, XVisualInfo *vis)
{
	this->dpy=NULL;
	init(dpy, vis);
}

glxvisual::glxvisual(Display *dpy, GLXFBConfig config)
{
	this->dpy=NULL;
	init(dpy, config);
}

glxvisual::~glxvisual(void) {}

void glxvisual::init(Display *dpy, XVisualInfo *vis)
{
	if(!dpy || !vis) _throw("Invalid argument");
	glx(_glXGetConfig(dpy, vis, GLX_BUFFER_SIZE, &buffer_size));
	glx(_glXGetConfig(dpy, vis, GLX_LEVEL, &level));
	glx(_glXGetConfig(dpy, vis, GLX_DOUBLEBUFFER, &doublebuffer));
	glx(_glXGetConfig(dpy, vis, GLX_STEREO, &stereo));
	glx(_glXGetConfig(dpy, vis, GLX_AUX_BUFFERS, &aux_buffers));
	glx(_glXGetConfig(dpy, vis, GLX_RED_SIZE, &red_size));
	glx(_glXGetConfig(dpy, vis, GLX_GREEN_SIZE, &green_size));
	glx(_glXGetConfig(dpy, vis, GLX_BLUE_SIZE, &blue_size));
	glx(_glXGetConfig(dpy, vis, GLX_ALPHA_SIZE, &alpha_size));
	glx(_glXGetConfig(dpy, vis, GLX_DEPTH_SIZE, &depth_size));
	glx(_glXGetConfig(dpy, vis, GLX_STENCIL_SIZE, &stencil_size));
	glx(_glXGetConfig(dpy, vis, GLX_ACCUM_RED_SIZE, &accum_red_size));
	glx(_glXGetConfig(dpy, vis, GLX_ACCUM_GREEN_SIZE, &accum_green_size));
	glx(_glXGetConfig(dpy, vis, GLX_ACCUM_BLUE_SIZE, &accum_blue_size));
	glx(_glXGetConfig(dpy, vis, GLX_ACCUM_ALPHA_SIZE, &accum_alpha_size));
	int rgba;  render_type=GLX_RGBA_BIT;
	glx(_glXGetConfig(dpy, vis, GLX_RGBA, &rgba));
	if(!rgba) render_type=GLX_COLOR_INDEX_BIT;
	switch(vis->c_class)
	{
		case StaticGray:  x_visual_type=GLX_STATIC_GRAY;  break;
		case GrayScale:   x_visual_type=GLX_GRAY_SCALE;  break;
		case StaticColor: x_visual_type=GLX_STATIC_COLOR;  break;
		case PseudoColor: x_visual_type=GLX_PSEUDO_COLOR;  break;
		case TrueColor:   x_visual_type=GLX_TRUE_COLOR;  break;
		case DirectColor: x_visual_type=GLX_DIRECT_COLOR;  break;
	}
	this->dpy=dpy;
}

void glxvisual::init(Display *dpy, GLXFBConfig config)
{
	if(!dpy || !config) _throw("Invalid argument");
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_BUFFER_SIZE, &buffer_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_LEVEL, &level));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_DOUBLEBUFFER, &doublebuffer));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_STEREO, &stereo));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_AUX_BUFFERS, &aux_buffers));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_RED_SIZE, &red_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_GREEN_SIZE, &green_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_BLUE_SIZE, &blue_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_ALPHA_SIZE, &alpha_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_DEPTH_SIZE, &depth_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_STENCIL_SIZE, &stencil_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_ACCUM_RED_SIZE, &accum_red_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_ACCUM_GREEN_SIZE, &accum_green_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_ACCUM_BLUE_SIZE, &accum_blue_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_ACCUM_ALPHA_SIZE, &accum_alpha_size));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_RENDER_TYPE, &render_type));
	glx(_glXGetFBConfigAttrib(dpy, config, GLX_X_VISUAL_TYPE, &x_visual_type));
	this->dpy=dpy;
}

GLXFBConfig glxvisual::getpbconfig(void)
{
	int nelements;  GLXFBConfig *fbconfigs;
	int fbattribs[]={
		GLX_BUFFER_SIZE, buffer_size,
		GLX_LEVEL, level,
		GLX_DOUBLEBUFFER, doublebuffer,
		GLX_STEREO, stereo,
		GLX_AUX_BUFFERS, aux_buffers,
		GLX_RED_SIZE, red_size,
		GLX_GREEN_SIZE, green_size,
		GLX_BLUE_SIZE, blue_size,
		GLX_ALPHA_SIZE, alpha_size,
		GLX_DEPTH_SIZE, depth_size,
		GLX_STENCIL_SIZE, stencil_size,
		GLX_ACCUM_RED_SIZE, accum_red_size,
		GLX_ACCUM_GREEN_SIZE, accum_green_size,
		GLX_ACCUM_BLUE_SIZE, accum_blue_size,
		GLX_ACCUM_ALPHA_SIZE, accum_alpha_size,
		GLX_RENDER_TYPE, render_type,
		GLX_X_VISUAL_TYPE, x_visual_type,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		None};
	errifnot(dpy);
	fbconfigs=_glXChooseFBConfig(dpy, DefaultScreen(dpy), fbattribs, &nelements);
	if(!nelements || !fbconfigs) return 0;
	return fbconfigs[0];
}

GLXFBConfig glxvisual::getfbconfig(void)
{
	int nelements;  GLXFBConfig *fbconfigs;
	int fbattribs[]={
		GLX_BUFFER_SIZE, buffer_size,
		GLX_LEVEL, level,
		GLX_DOUBLEBUFFER, doublebuffer,
		GLX_STEREO, stereo,
		GLX_AUX_BUFFERS, aux_buffers,
		GLX_RED_SIZE, red_size,
		GLX_GREEN_SIZE, green_size,
		GLX_BLUE_SIZE, blue_size,
		GLX_ALPHA_SIZE, alpha_size,
		GLX_DEPTH_SIZE, depth_size,
		GLX_STENCIL_SIZE, stencil_size,
		GLX_ACCUM_RED_SIZE, accum_red_size,
		GLX_ACCUM_GREEN_SIZE, accum_green_size,
		GLX_ACCUM_BLUE_SIZE, accum_blue_size,
		GLX_ACCUM_ALPHA_SIZE, accum_alpha_size,
		GLX_RENDER_TYPE, render_type,
		GLX_X_VISUAL_TYPE, x_visual_type,
		None};
	errifnot(dpy);
	fbconfigs=_glXChooseFBConfig(dpy, DefaultScreen(dpy), fbattribs, &nelements);
	if(!nelements || !fbconfigs) return 0;
	return fbconfigs[0];
}

XVisualInfo *glxvisual::getvisual(void)
{
	int i;  XVisualInfo *v;
	int attribs[]={
		GLX_BUFFER_SIZE, buffer_size,
		GLX_LEVEL, level,
		GLX_AUX_BUFFERS, aux_buffers,
		GLX_RED_SIZE, red_size,
		GLX_GREEN_SIZE, green_size,
		GLX_BLUE_SIZE, blue_size,
		GLX_ALPHA_SIZE, alpha_size,
		GLX_DEPTH_SIZE, depth_size,
		GLX_STENCIL_SIZE, stencil_size,
		GLX_ACCUM_RED_SIZE, accum_red_size,
		GLX_ACCUM_GREEN_SIZE, accum_green_size,
		GLX_ACCUM_BLUE_SIZE, accum_blue_size,
		GLX_ACCUM_ALPHA_SIZE, accum_alpha_size,
		None, None, None,
		None};
	errifnot(dpy);
	i=26;
	if(doublebuffer) attribs[i++]=GLX_DOUBLEBUFFER;
	if(stereo) attribs[i++]=GLX_STEREO;
	if(render_type==GLX_RGBA_BIT) attribs[i++]=GLX_RGBA;
	v=_glXChooseVisual(dpy, DefaultScreen(dpy), attribs);
	return v;
}
