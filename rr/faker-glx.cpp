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

#include "faker-sym.h"
#include "faker-macros.h"

// This file contains stubs for GLX functions which are faked simply by remapping
// their display handle and/or X Visual handle to the server's display


// This attempts to look up a visual in the hash or match it using 2D functions
// if (for some reason) it wasn't obtained with glXChooseVisual()

XVisualInfo *_GetVisual(Display *dpy, int screen, int depth, int c_class)
{
	XVisualInfo vtemp, *v=NULL;  int n;
	if(!XMatchVisualInfo(dpy, screen, depth, c_class, &vtemp)) return NULL;
	if(!(v=XGetVisualInfo(_localdpy, VisualIDMask, &vtemp, &n)) || !n)
		return NULL;
	int supportsgl=0;
	_glXGetConfig(_localdpy, v, GLX_USE_GL, &supportsgl);
	if(!supportsgl) return NULL;
	return v;
}

XVisualInfo *_MatchVisual(Display *dpy, XVisualInfo *vis)
{
	XVisualInfo *v=NULL;
	if(!dpy || !vis) return NULL;
	TRY();
	if(!(v=vish.matchvisual(dpy, vis)))
	{
		if((v=_GetVisual(_localdpy, DefaultScreen(_localdpy), vis->depth, vis->c_class))==NULL
		&& (v=_GetVisual(_localdpy, DefaultScreen(_localdpy), 24, TrueColor))==NULL)
			_throw("Could not find appropriate visual on server's display");
		vish.add(dpy, vis, v);
	}
	CATCH();
	return v;
}

#ifdef USEGLP
GLPFBConfig _MatchConfig(Display *dpy, XVisualInfo *vis)
{
	GLPFBConfig c=0, *configs=NULL;  int n=0;
	if(!dpy || !vis) return 0;
	TRY();
	if(!(c=vish.getpbconfig(dpy, vis)))
	{
		int defattribs[]={GLP_DOUBLEBUFFER, 1, GLP_RED_SIZE, 8, GLP_GREEN_SIZE, 8,
			GLP_BLUE_SIZE, 8, GLP_RENDER_TYPE, GLP_RGBA_BIT, None};
		int rgbattribs[]={GLP_DOUBLEBUFFER, 1, GLP_RED_SIZE, 8, GLP_GREEN_SIZE, 8,
			GLP_BLUE_SIZE, 8, GLP_RENDER_TYPE, GLP_RGBA_BIT, None};
		int ciattribs[]={GLP_DOUBLEBUFFER, 1, GLP_BUFFER_SIZE, 8,
			GLP_RENDER_TYPE, GLP_COLOR_INDEX_BIT, None};
		int *attribs=rgbattribs;
		if(vis->c_class!=TrueColor && vis->c_class!=DirectColor)
		{
			attribs=ciattribs;
			attribs[3]=vis->depth;
		}
		else
		{
			if(vis->depth<8) attribs[3]=attribs[5]=attribs[7]=1;
			else if(vis->depth<16) attribs[3]=attribs[5]=attribs[7]=2;
			else if(vis->depth<24) attribs[3]=attribs[5]=attribs[7]=4;
		}
		if(((configs=glPChooseFBConfig(_localdev, attribs, &n))==NULL || n<1)
			&& ((configs=glPChooseFBConfig(_localdev, defattribs, &n))==NULL || n<1))
			_throw("Could not find appropriate visual on server's display");
		c=configs[0];
	}
	return c;
	CATCH();
}
#endif

extern "C" {

GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen, const int *attrib_list, int *nelements)
{
	#ifdef USEGLP
	if(fconfig.glp)
	{
		// Argh!
		int glpattribs[257], j=0;
		for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
		{
			if(attrib_list[i]!=GLX_DRAWABLE_TYPE)
			{
				glpattribs[j++]=attrib_list[i];  glpattribs[j++]=attrib_list[i+1];
			}
		}
		glpattribs[j]=None;
		return glPChooseFBConfig(_localdev, glpattribs, nelements);
	}
	else
	#endif
	return _glXChooseFBConfig(_localdpy, screen, attrib_list, nelements);
}

GLXFBConfigSGIX *glXChooseFBConfigSGIX (Display *dpy, int screen, const int *attrib_list, int *nelements)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPChooseFBConfig(_localdev, attrib_list, nelements);
	else
	#endif
	return _glXChooseFBConfigSGIX(_localdpy, screen, attrib_list, nelements);
}

void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask)
{
	#ifdef USEGLP
	if(fconfig.glp) glPCopyContext(src, dst, mask);
	else
	#endif
	_glXCopyContext(_localdpy, src, dst, mask);
}

GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attrib_list)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPCreateBuffer(config, attrib_list);
	else
	#endif
	return _glXCreatePbuffer(_localdpy, config, attrib_list);
}

GLXPbuffer glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfig config, unsigned int width, unsigned int height, const int *attrib_list)
{
	#ifdef USEGLP
	if(fconfig.glp)
	{
		int glpattribs[257], j=0;
		for(int i=0; attrib_list[i]!=None && i<=254; i+=2)
		{
			glpattribs[j++]=attrib_list[i];  glpattribs[j++]=attrib_list[i+1];
		}
		glpattribs[j++]=GLP_PBUFFER_WIDTH;  glpattribs[j++]=width;
		glpattribs[j++]=GLP_PBUFFER_HEIGHT;  glpattribs[j++]=height;
		glpattribs[j]=None;
		return glPCreateBuffer(config, glpattribs);
	}
	#endif
	return _glXCreateGLXPbufferSGIX(_localdpy, config, width, height, attrib_list);
}

void glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf)
{
	#ifdef USEGLP
	if(fconfig.glp) glPDestroyBuffer(pbuf);
	else
	#endif
	_glXDestroyGLXPbufferSGIX(_localdpy, pbuf);
}

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	#ifdef USEGLP
	if(fconfig.glp) glPDestroyBuffer(pbuf);
	else
	#endif
	_glXDestroyPbuffer(_localdpy, pbuf);
}

void glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
	#ifdef USEGLP
	// Tell me about the rabbits, George ...
	if(fconfig.glp) return;
	else
	#endif
	_glXFreeContextEXT(_localdpy, ctx);
}

const char *glXGetClientString(Display *dpy, int name)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetLibraryString(name);
	else
	#endif
	return _glXGetClientString(_localdpy, name);
}

int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
	TRY();
	#ifdef USEGLP
	if(fconfig.glp)
	{
		GLPFBConfig c;  int glpvalue, err;
		errifnot(c=_MatchConfig(dpy, vis));
		switch(attrib)
		{
			case GLX_USE_GL:
				if(value) {*value=1;  return 0;}
				else return GLX_BAD_VALUE;
			case GLX_RGBA:
				if((err=glPGetFBConfigAttrib(c, GLP_RENDER_TYPE, &glpvalue))!=0)
					return err;
				*value=glpvalue==GLP_RGBA_BIT? 1:0;  return 0;
			default:
				return glPGetFBConfigAttrib(c, attrib, value);
		}
	}
	else
	#endif
	if(dpy!=_localdpy)
	{
		if(vis)
		{
			XVisualInfo *_localvis=_MatchVisual(dpy, vis);
			if(_localvis) vis=_localvis;
		}
	}
	CATCH();
	return _glXGetConfig(_localdpy, vis, attrib, value);
}

GLXContext glXGetCurrentContext(void)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetCurrentContext();
	else
	#endif
	return _glXGetCurrentContext();
}

Display *glXGetCurrentDisplay(void)
{
	#ifdef USEGLP
	static Display *dpy=NULL;
	if(fconfig.glp)
	{
		if(!dpy) dpy=_XOpenDisplay(0);
		return dpy;
	}
	else
	#endif
	return _glXGetCurrentDisplay();
}

GLXDrawable glXGetCurrentDrawable(void)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetCurrentBuffer();
	else
	#endif
	return _glXGetCurrentDrawable();
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetCurrentReadBuffer();
	else
	#endif
	return _glXGetCurrentReadDrawable();
}

int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
	#ifdef USEGLP
	if(fconfig.glp)
	{
		if(attribute==GLX_X_VISUAL_TYPE) {*value=GLX_TRUE_COLOR;  return 0;}
		else return glPGetFBConfigAttrib(config, attribute, value);
	}
	else
	#endif
	return _glXGetFBConfigAttrib(_localdpy, config, attribute, value);
}

int glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value_return)
{
	return glXGetFBConfigAttrib(dpy, config, attribute, value_return);
}

GLXFBConfigSGIX glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
	TRY();
	#ifdef USEGLP
	if(fconfig.glp) return _MatchConfig(dpy, vis);
	else
	#endif
	if(dpy!=_localdpy)
	{
		if(vis)
		{
			XVisualInfo *_localvis=_MatchVisual(dpy, vis);
			if(_localvis) vis=_localvis;
		}
	}
	CATCH();
	return _glXGetFBConfigFromVisualSGIX(_localdpy, vis);
}

GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetFBConfigs(_localdev, nelements);
	else
	#endif
	return _glXGetFBConfigs(_localdpy, screen, nelements);
}

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw, unsigned long *event_mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXGetSelectedEvent(_localdpy, draw, event_mask);
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXGetSelectedEventSGIX(_localdpy, drawable, mask);
}

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
	#ifdef USEGLP
	if(fconfig.glp) return 0;
	else
	#endif
	return _glXImportContextEXT(_localdpy, contextID);
}

Bool glXIsDirect (Display *dpy, GLXContext ctx)
{
	#ifdef USEGLP
	if(fconfig.glp) return True;
	else
	#endif
	return _glXIsDirect(_localdpy, ctx);
}

int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPQueryContext(ctx, attribute, value);
	else
	#endif
	return _glXQueryContext(_localdpy, ctx, attribute, value);
}

int glXQueryContextInfoEXT(Display *dpy, GLXContext ctx, int attribute, int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPQueryContext(ctx, attribute, value);
	else
	#endif
	return _glXQueryContextInfoEXT(_localdpy, ctx, attribute, value);
}

void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPQueryBuffer(draw, attribute, value);
	else
	#endif
	return _glXQueryDrawable(_localdpy, draw, attribute, value);
}

Bool glXQueryExtension(Display *dpy, int *error_base, int *event_base)
{
	#ifdef USEGLP
	if(fconfig.glp)
		{if(error_base) *error_base=0;  if(event_base) *event_base=0;  return True;}
	else
	#endif
	return _glXQueryExtension(_localdpy, error_base, event_base);
}

const char *glXQueryExtensionsString(Display *dpy, int screen)
{
	#ifdef USEGLP
	if(fconfig.glp) return "GLX_SGIX_fbconfig GLX_SGIX_pbuffer";
	else
	#endif
	return _glXQueryExtensionsString(_localdpy, screen);
}

const char *glXQueryServerString(Display *dpy, int screen, int name)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPGetDeviceString(_localdev, name);
	else
	#endif
	return _glXQueryServerString(_localdpy, screen, name);
}

void glXQueryGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value)
{
	#ifdef USEGLP
	if(fconfig.glp) glPQueryBuffer(pbuf, attribute, value);
	else
	#endif
	_glXQueryGLXPbufferSGIX(_localdpy, pbuf, attribute, value);
}

Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
	#ifdef USEGLP
	if(fconfig.glp) return glPQueryVersion(major, minor);
	else
	#endif
	return _glXQueryVersion(_localdpy, major, minor);
}

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXSelectEvent(_localdpy, draw, event_mask);
}

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
	#ifdef USEGLP
	if(fconfig.glp) return;
	else
	#endif
	_glXSelectEventSGIX(_localdpy, drawable, mask);
}

shimfuncdpy3(Bool, glXJoinSwapGroupNV, Display*, dpy, GLXDrawable, drawable, GLuint, group, return );
shimfuncdpy3(Bool, glXBindSwapBarrierNV, Display*, dpy, GLuint, group, GLuint, barrier, return );
shimfuncdpy4(Bool, glXQuerySwapGroupNV, Display*, dpy, GLXDrawable, drawable, GLuint*, group, GLuint*, barrier, return );
shimfuncdpy4(Bool, glXQueryMaxSwapGroupsNV, Display*, dpy, int, screen, GLuint*, maxGroups, GLuint*, maxBarriers, return );
shimfuncdpy3(Bool, glXQueryFrameCountNV, Display*, dpy, int, screen, GLuint*, count, return );
shimfuncdpy2(Bool, glXResetFrameCountNV, Display*, dpy, int, screen, return );

}
