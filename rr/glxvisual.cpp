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
#include "fakerconfig.h"

extern Display *_localdpy;
extern FakerConfig fconfig;

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
#define glx(f) {if((__glxerr=(f))!=None) _throw(glxerr(__glxerr));}

glxvisual::glxvisual(XVisualInfo *visual)
{
	vis=visual;  config=0;
}

glxvisual::glxvisual(GLXFBConfig cfg)
{
	config=cfg;  vis=NULL;
}

glxvisual::~glxvisual(void)
{
	if(vis) XFree(vis);
}

GLXFBConfig glxvisual::getpbconfig(void)
{
	int nelements;  GLXFBConfig *fbconfigs;
	int fbattribs[]={
		GLX_BUFFER_SIZE, 1,
		GLX_LEVEL, 0,
		GLX_DOUBLEBUFFER, 1,
		GLX_AUX_BUFFERS, 0,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 0,
		GLX_ACCUM_RED_SIZE, 0,
		GLX_ACCUM_GREEN_SIZE, 0,
		GLX_ACCUM_BLUE_SIZE, 0,
		GLX_ACCUM_ALPHA_SIZE, 0,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		None, None, None};
	if(config) return config;
	if(!vis) _throw("glxvisual not properly initialized");
	glx(_glXGetConfig(_localdpy, vis, GLX_BUFFER_SIZE, &fbattribs[1]));
	glx(_glXGetConfig(_localdpy, vis, GLX_LEVEL, &fbattribs[3]));
	glx(_glXGetConfig(_localdpy, vis, GLX_DOUBLEBUFFER, &fbattribs[5]));
	glx(_glXGetConfig(_localdpy, vis, GLX_AUX_BUFFERS, &fbattribs[7]));
	glx(_glXGetConfig(_localdpy, vis, GLX_RED_SIZE, &fbattribs[9]));
	glx(_glXGetConfig(_localdpy, vis, GLX_GREEN_SIZE, &fbattribs[11]));
	glx(_glXGetConfig(_localdpy, vis, GLX_BLUE_SIZE, &fbattribs[13]));
	glx(_glXGetConfig(_localdpy, vis, GLX_ALPHA_SIZE, &fbattribs[15]));
	glx(_glXGetConfig(_localdpy, vis, GLX_DEPTH_SIZE, &fbattribs[17]));
	glx(_glXGetConfig(_localdpy, vis, GLX_STENCIL_SIZE, &fbattribs[19]));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_RED_SIZE, &fbattribs[21]));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_GREEN_SIZE, &fbattribs[23]));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_BLUE_SIZE, &fbattribs[25]));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_ALPHA_SIZE, &fbattribs[27]));
	int rgba;  fbattribs[29]=GLX_RGBA_BIT;
	glx(_glXGetConfig(_localdpy, vis, GLX_RGBA, &rgba));
	if(!rgba) fbattribs[29]=GLX_COLOR_INDEX_BIT;
	switch(vis->c_class)
	{
		case StaticGray:  fbattribs[31]=GLX_STATIC_GRAY;  break;
		case GrayScale:   fbattribs[31]=GLX_GRAY_SCALE;  break;
		case StaticColor: fbattribs[31]=GLX_STATIC_COLOR;  break;
		case PseudoColor: fbattribs[31]=GLX_PSEUDO_COLOR;  break;
		case TrueColor:   fbattribs[31]=GLX_TRUE_COLOR;  break;
		case DirectColor: fbattribs[31]=GLX_DIRECT_COLOR;  break;
	}
	#ifndef sun
	fbattribs[34]=GLX_STEREO;  // No stereo?  Denied!
	glx(_glXGetConfig(_localdpy, vis, GLX_STEREO, &fbattribs[35]));
	#endif
	int screen=(_localdpy && !fconfig.glp)? DefaultScreen(_localdpy):0;
	fbconfigs=glXChooseFBConfig(_localdpy, screen, fbattribs, &nelements);
	if(!nelements || !fbconfigs) return 0;
	config=fbconfigs[0];
	return fbconfigs[0];
}

XVisualInfo *glxvisual::getvisual(void)
{
	int i, doublebuffer=1, stereo=0, render_type=GLX_RGBA_BIT;
	XVisualInfo *v;
	int attribs[]={
		GLX_BUFFER_SIZE, 1,
		GLX_LEVEL, 0,
		GLX_AUX_BUFFERS, 0,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 0,
		GLX_ACCUM_RED_SIZE, 0,
		GLX_ACCUM_GREEN_SIZE, 0,
		GLX_ACCUM_BLUE_SIZE, 0,
		GLX_ACCUM_ALPHA_SIZE, 0,
		None, None, None,
		None};
	if(vis) return vis;
	if(!config) _throw("glxvisual not properly initialized");
	// Note: calling faked functions so GLP will work
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_BUFFER_SIZE, &attribs[1]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_LEVEL, &attribs[3]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_DOUBLEBUFFER, &doublebuffer));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_STEREO, &stereo));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_AUX_BUFFERS, &attribs[5]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_RED_SIZE, &attribs[7]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_GREEN_SIZE, &attribs[9]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_BLUE_SIZE, &attribs[11]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ALPHA_SIZE, &attribs[13]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_DEPTH_SIZE, &attribs[15]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_STENCIL_SIZE, &attribs[17]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_RED_SIZE, &attribs[19]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_GREEN_SIZE, &attribs[21]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_BLUE_SIZE, &attribs[23]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_ALPHA_SIZE, &attribs[25]));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_RENDER_TYPE, &render_type));
	errifnot(_localdpy);
	i=26;
	if(doublebuffer) attribs[i++]=GLX_DOUBLEBUFFER;
	if(stereo) attribs[i++]=GLX_STEREO;
	if(render_type==GLX_RGBA_BIT) attribs[i++]=GLX_RGBA;
	errifnot(_localdpy);
	v=_glXChooseVisual(_localdpy, DefaultScreen(_localdpy), attribs);
	if(v) vis=v;
	return v;
}

GLXFBConfig glXConfigFromVisAttribs(int attribs[])
{
	int glxattribs[257], j=0, nelements;  GLXFBConfig *fbconfigs;
	int doublebuffer=0, render_type=GLX_COLOR_INDEX_BIT, stereo=0;

	for(int i=0; attribs[i]!=None && i<=254; i++)
	{
		if(attribs[i]==GLX_DOUBLEBUFFER) doublebuffer=1;
		else if(attribs[i]==GLX_RGBA) render_type=GLX_RGBA_BIT;
		else if(attribs[i]==GLX_STEREO) stereo=1;
		else
		{
			glxattribs[j++]=attribs[i];  glxattribs[j++]=attribs[i+1];
			i++;
		}
	}
	glxattribs[j++]=GLX_DOUBLEBUFFER;  glxattribs[j++]=doublebuffer;
	glxattribs[j++]=GLX_RENDER_TYPE;  glxattribs[j++]=render_type;
	#ifndef sun
	glxattribs[j++]=GLX_STEREO;  glxattribs[j++]=stereo;
	#endif
	glxattribs[j++]=GLX_DRAWABLE_TYPE;  glxattribs[j++]=GLX_PBUFFER_BIT;
	glxattribs[j]=None;

	int screen=(_localdpy && !fconfig.glp)? DefaultScreen(_localdpy):0;
	fbconfigs=glXChooseFBConfig(_localdpy, screen, glxattribs, &nelements);
	if(!nelements || !fbconfigs) return 0;
	return fbconfigs[0];
}

int glXConfigDepth(GLXFBConfig c)
{
	int depth, render_type, r, g, b;
	errifnot(c);
	glx(glXGetFBConfigAttrib(_localdpy, c, GLX_RENDER_TYPE, &render_type));
	if(render_type==GLX_RGBA_BIT)
	{
		glx(glXGetFBConfigAttrib(_localdpy, c, GLX_RED_SIZE, &r));
		glx(glXGetFBConfigAttrib(_localdpy, c, GLX_GREEN_SIZE, &g));
		glx(glXGetFBConfigAttrib(_localdpy, c, GLX_BLUE_SIZE, &b));
		depth=r+g+b;
		if(depth<8) depth=1;  // Monochrome
	}
	else
	{
		glx(glXGetFBConfigAttrib(_localdpy, c, GLX_BUFFER_SIZE, &depth));
	}
	return depth;
}

int glXConfigClass(GLXFBConfig c)
{
	int rendertype;
	errifnot(c);
	glx(glXGetFBConfigAttrib(_localdpy, c, GLX_RENDER_TYPE, &rendertype));
	if(rendertype==GLX_RGBA_BIT) return TrueColor;
	else return PseudoColor;
}
