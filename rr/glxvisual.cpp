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
#ifdef USEGLP
extern GLPDevice _localdev;
#endif
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

glxvisual::glxvisual(XVisualInfo *vis)
{
	init(vis);
}

glxvisual::glxvisual(GLXFBConfig config)
{
	init(config);
}

glxvisual::~glxvisual(void) {}

void glxvisual::init(XVisualInfo *vis)
{
	if(!vis) _throw("Invalid argument");
	glx(_glXGetConfig(_localdpy, vis, GLX_BUFFER_SIZE, &buffer_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_LEVEL, &level));
	glx(_glXGetConfig(_localdpy, vis, GLX_DOUBLEBUFFER, &doublebuffer));
	glx(_glXGetConfig(_localdpy, vis, GLX_STEREO, &stereo));
	glx(_glXGetConfig(_localdpy, vis, GLX_AUX_BUFFERS, &aux_buffers));
	glx(_glXGetConfig(_localdpy, vis, GLX_RED_SIZE, &red_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_GREEN_SIZE, &green_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_BLUE_SIZE, &blue_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_ALPHA_SIZE, &alpha_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_DEPTH_SIZE, &depth_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_STENCIL_SIZE, &stencil_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_RED_SIZE, &accum_red_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_GREEN_SIZE, &accum_green_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_BLUE_SIZE, &accum_blue_size));
	glx(_glXGetConfig(_localdpy, vis, GLX_ACCUM_ALPHA_SIZE, &accum_alpha_size));
	int rgba;  render_type=GLX_RGBA_BIT;
	glx(_glXGetConfig(_localdpy, vis, GLX_RGBA, &rgba));
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
}

void glxvisual::init(GLXFBConfig config)
{
	if(!config) _throw("Invalid argument");
	// Note: calling faked functions so GLP will work
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_BUFFER_SIZE, &buffer_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_LEVEL, &level));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_DOUBLEBUFFER, &doublebuffer));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_STEREO, &stereo));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_AUX_BUFFERS, &aux_buffers));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_RED_SIZE, &red_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_GREEN_SIZE, &green_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_BLUE_SIZE, &blue_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ALPHA_SIZE, &alpha_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_DEPTH_SIZE, &depth_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_STENCIL_SIZE, &stencil_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_RED_SIZE, &accum_red_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_GREEN_SIZE, &accum_green_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_BLUE_SIZE, &accum_blue_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_ACCUM_ALPHA_SIZE, &accum_alpha_size));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_RENDER_TYPE, &render_type));
	glx(glXGetFBConfigAttrib(_localdpy, config, GLX_X_VISUAL_TYPE, &x_visual_type));
}

GLXFBConfig glxvisual::getpbconfig(void)
{
	int nelements;  GLXFBConfig *fbconfigs;
	int fbattribs[]={
		GLX_BUFFER_SIZE, buffer_size,
		GLX_LEVEL, level,
		GLX_DOUBLEBUFFER, doublebuffer,
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
		None, None, None};
	#ifndef sun
	fbattribs[34]=GLX_STEREO;  fbattribs[35]=stereo;  // No stereo?  Denied!
	#endif
	#ifdef USEGLP
	if(fconfig.glp)
		fbconfigs=glPChooseFBConfig(_localdev, fbattribs, &nelements);
	else
	#endif
	{
		errifnot(_localdpy);
		fbconfigs=_glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), fbattribs, &nelements);
	}

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
	errifnot(_localdpy);
	i=26;
	if(doublebuffer) attribs[i++]=GLX_DOUBLEBUFFER;
	if(stereo) attribs[i++]=GLX_STEREO;
	if(render_type==GLX_RGBA_BIT) attribs[i++]=GLX_RGBA;
	v=_glXChooseVisual(_localdpy, DefaultScreen(_localdpy), attribs);
	return v;
}

#ifdef USEGLP

GLPFBConfig glPConfigFromVisAttribs(GLPDevice dev, int attribs[])
{
	int i=0, glpattribs[40], nelements;  GLPFBConfig *fbconfigs;
	int buffer_size=-1, level=-1, doublebuffer=0, render_type=GLP_COLOR_INDEX_BIT,
		stereo=0, aux_buffers=-1, red_size=-1, green_size=-1, blue_size=-1,
		alpha_size=-1, depth_size=-1, stencil_size=-1, accum_red_size=-1,
		accum_green_size=-1, accum_blue_size=-1, accum_alpha_size=-1;

	while(attribs[i]!=None)
	{
		if(attribs[i]==GLX_BUFFER_SIZE && attribs[++i]!=None)
			buffer_size=attribs[i];
		else if(attribs[i]==GLX_LEVEL && attribs[++i]!=None)
			level=attribs[i];
		else if(attribs[i]==GLX_DOUBLEBUFFER) doublebuffer=1;
		else if(attribs[i]==GLX_RGBA) render_type=GLP_RGBA_BIT;
		else if(attribs[i]==GLX_STEREO) stereo=1;
		else if(attribs[i]==GLX_AUX_BUFFERS && attribs[++i]!=None)
			aux_buffers=attribs[i];
		else if(attribs[i]==GLX_RED_SIZE && attribs[++i]!=None)
			red_size=attribs[i];
		else if(attribs[i]==GLX_GREEN_SIZE && attribs[++i]!=None)
			green_size=attribs[i];
		else if(attribs[i]==GLX_BLUE_SIZE && attribs[++i]!=None)
			blue_size=attribs[i];
		else if(attribs[i]==GLX_ALPHA_SIZE && attribs[++i]!=None)
			alpha_size=attribs[i];
		else if(attribs[i]==GLX_DEPTH_SIZE && attribs[++i]!=None)
			depth_size=attribs[i];
		else if(attribs[i]==GLX_STENCIL_SIZE && attribs[++i]!=None)
			stencil_size=attribs[i];
		else if(attribs[i]==GLX_ACCUM_RED_SIZE && attribs[++i]!=None)
			accum_red_size=attribs[i];
		else if(attribs[i]==GLX_ACCUM_GREEN_SIZE && attribs[++i]!=None)
			accum_green_size=attribs[i];
		else if(attribs[i]==GLX_ACCUM_BLUE_SIZE && attribs[++i]!=None)
			accum_blue_size=attribs[i];
		else if(attribs[i]==GLX_ACCUM_ALPHA_SIZE && attribs[++i]!=None)
			accum_alpha_size=attribs[i];
		i++;
	}

	i=0;
	if(buffer_size>=0)
		{glpattribs[i++]=GLP_BUFFER_SIZE;  glpattribs[i++]=buffer_size;}
	if(level>=0)
		{glpattribs[i++]=GLP_LEVEL;  glpattribs[i++]=level;}
	glpattribs[i++]=GLP_DOUBLEBUFFER;  glpattribs[i++]=doublebuffer;
	glpattribs[i++]=GLP_RENDER_TYPE;  glpattribs[i++]=render_type;
	#ifndef sun
	glpattribs[i++]=GLP_STEREO;  glpattribs[i++]=stereo;
	#endif
	if(aux_buffers>=0)
		{glpattribs[i++]=GLP_AUX_BUFFERS;  glpattribs[i++]=aux_buffers;}
	if(red_size>=0)
		{glpattribs[i++]=GLP_RED_SIZE;  glpattribs[i++]=red_size;}
	if(green_size>=0)
		{glpattribs[i++]=GLP_GREEN_SIZE;  glpattribs[i++]=green_size;}
	if(blue_size>=0)
		{glpattribs[i++]=GLP_BLUE_SIZE;  glpattribs[i++]=blue_size;}
	if(alpha_size>=0)
		{glpattribs[i++]=GLP_ALPHA_SIZE;  glpattribs[i++]=alpha_size;}
	if(depth_size>=0)
		{glpattribs[i++]=GLP_DEPTH_SIZE;  glpattribs[i++]=depth_size;}
	if(stencil_size>=0)
		{glpattribs[i++]=GLP_STENCIL_SIZE;  glpattribs[i++]=stencil_size;}
	if(accum_red_size>=0)
		{glpattribs[i++]=GLP_ACCUM_RED_SIZE;  glpattribs[i++]=accum_red_size;}
	if(accum_green_size>=0)
		{glpattribs[i++]=GLP_ACCUM_GREEN_SIZE;  glpattribs[i++]=accum_green_size;}
	if(accum_blue_size>=0)
		{glpattribs[i++]=GLP_ACCUM_BLUE_SIZE;  glpattribs[i++]=accum_blue_size;}
	if(accum_alpha_size>=0)
		{glpattribs[i++]=GLP_ACCUM_ALPHA_SIZE;  glpattribs[i++]=accum_alpha_size;}
	glpattribs[i]=None;

	fbconfigs=glPChooseFBConfig(_localdev, glpattribs, &nelements);
	if(!nelements || !fbconfigs) return 0;
	return fbconfigs[0];
}

int glPConfigDepth(GLPFBConfig c)
{
	int depth, render_type, r, g, b;
	errifnot(c);
	glx(glPGetFBConfigAttrib(c, GLP_RENDER_TYPE, &render_type));
	if(render_type==GLP_RGBA_BIT)
	{
		glx(glPGetFBConfigAttrib(c, GLP_RED_SIZE, &r));
		glx(glPGetFBConfigAttrib(c, GLP_GREEN_SIZE, &g));
		glx(glPGetFBConfigAttrib(c, GLP_BLUE_SIZE, &b));
		depth=r+g+b;
		if(depth<8) depth=1;  // Monochrome
	}
	else
	{
		glx(glPGetFBConfigAttrib(c, GLP_BUFFER_SIZE, &depth));
	}
	return depth;
}

int glPConfigClass(GLPFBConfig c)
{
	int rendertype;
	errifnot(c);
	glx(glPGetFBConfigAttrib(c, GLP_RENDER_TYPE, &rendertype));
	if(rendertype==GLP_RGBA_BIT) return TrueColor;
	else return PseudoColor;
}

#endif
