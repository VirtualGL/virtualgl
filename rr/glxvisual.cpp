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

#define _DefaultScreen(d) ((!fconfig.glp && d) ? DefaultScreen(d):0)

GLXFBConfig glXConfigFromVisAttribs(int attribs[])
{
	int glxattribs[257], j=0, nelements;  GLXFBConfig *fbconfigs, c=0;
	int doublebuffer=0, render_type=GLX_COLOR_INDEX_BIT, stereo=0;

	for(int i=0; attribs[i]!=None && i<=254; i++)
	{
		if(attribs[i]==GLX_DOUBLEBUFFER) doublebuffer=1;
		else if(attribs[i]==GLX_RGBA) render_type=GLX_RGBA_BIT;
		else if(attribs[i]==GLX_STEREO) stereo=1;
		else if(attribs[i]!=GLX_USE_GL)
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
	fbconfigs=glXChooseFBConfig(_localdpy, _DefaultScreen(_localdpy), glxattribs, &nelements);
	if(!nelements || !fbconfigs) return 0;
	c=fbconfigs[0];
	XFree(fbconfigs);
	return c;
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
