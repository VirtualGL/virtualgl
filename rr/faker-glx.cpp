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

// Map a client-side drawable to a server-side drawable

GLXDrawable ServerDrawable(Display *dpy, GLXDrawable draw)
{
	pbwin *pb=NULL;
	if((pb=winh.findpb(dpy, draw))!=NULL) return pb->getdrawable();
	else return draw;
}

// This attempts to look up a visual in the hash or match it using 2D functions
// if (for some reason) it wasn't obtained with glXChooseVisual()

GLXFBConfig _MatchConfig(Display *dpy, XVisualInfo *vis)
{
	GLXFBConfig c=0, *configs=NULL;  int n=0;
	if(!dpy || !vis) return 0;
	if(!(c=vish.getpbconfig(dpy, vis)))
	{
		int defattribs[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE,
			GLX_PBUFFER_BIT, None};
		int rgbattribs[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE,
			GLX_PBUFFER_BIT, None};
		int ciattribs[]={GLX_DOUBLEBUFFER, 1, GLX_BUFFER_SIZE, 8,
			GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT, GLX_DRAWABLE_TYPE,
			GLX_PBUFFER_BIT, None};
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
		if(((configs=glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), attribs, &n))==NULL || n<1)
			&& ((configs=glXChooseFBConfig(_localdpy, DefaultScreen(_localdpy), defattribs, &n))==NULL || n<1))
			return 0;
		c=configs[0];
		XFree(configs);
		if(c) vish.add(dpy, vis, c);
	}
	return c;
}

extern "C" {

shimfuncdpy4( GLXFBConfig*, glXChooseFBConfig, Display*, dpy, int, screen, const int*, attrib_list, int*, nelements );
shimfuncdpy4( void, glXCopyContext, Display*, dpy, GLXContext, src, GLXContext, dst, unsigned long, mask );

GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attrib_list)
{
	GLXPbuffer pb=0;
	pb=_glXCreatePbuffer(_localdpy, config, attrib_list);
	if(dpy && pb) glxdh.add(pb, dpy);
	return pb;
}

void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	_glXDestroyPbuffer(_localdpy, pbuf);
	if(pbuf) glxdh.remove(pbuf);
}

shimfuncdpy2( void, glXFreeContextEXT, Display*, dpy, GLXContext, ctx );
shimfuncdpy2( const char*, glXGetClientString, Display*, dpy, int, name );

int glXGetConfig(Display *dpy, XVisualInfo *vis, int attrib, int *value)
{
	TRY();
	GLXFBConfig c;  int glxvalue, err;
	errifnot(c=_MatchConfig(dpy, vis));
	switch(attrib)
	{
		case GLX_USE_GL:
			if(value)
			{
				if(vis->depth>=24 && (vis->c_class==TrueColor || vis->c_class==DirectColor))
					*value=1;
				else *value=0;
				return 0;
			}
			else return GLX_BAD_VALUE;
		case GLX_RGBA:
			if((err=glXGetFBConfigAttrib(_localdpy, c, GLX_RENDER_TYPE, &glxvalue))!=0)
				return err;
			*value=glxvalue==GLX_RGBA_BIT? 1:0;  return 0;
		default:
			return glXGetFBConfigAttrib(_localdpy, c, attrib, value);
	}
	CATCH();
	return GLX_BAD_ATTRIBUTE;
}


Display *glXGetCurrentDisplay(void)
{
	Display *dpy=NULL;  pbwin *pb=NULL;
	if((pb=winh.findpb(_localdpy, _glXGetCurrentDrawable()))!=NULL)
		dpy=pb->getwindpy();
	else dpy=glxdh.getcurrentdpy(_glXGetCurrentDrawable());
	return dpy;
}

GLXDrawable glXGetCurrentDrawable(void)
{
	pbwin *pb=NULL;  GLXDrawable draw=_glXGetCurrentDrawable();
	if((pb=winh.findpb(_localdpy, draw))!=NULL)
		draw=pb->getwin();
	return draw;
}

GLXDrawable glXGetCurrentReadDrawable(void)
{
	pbwin *pb=NULL;  GLXDrawable read=_glXGetCurrentReadDrawable();
	if((pb=winh.findpb(_localdpy, read))!=NULL)
		read=pb->getwin();
	return read;
}

shimfuncdpy4( int, glXGetFBConfigAttrib, Display*, dpy, GLXFBConfig, config, int, attribute, int*, value );
shimfuncdpy3( GLXFBConfig*, glXGetFBConfigs, Display*, dpy, int, screen, int*, nelements );

void glXGetSelectedEvent(Display *dpy, GLXDrawable draw, unsigned long *event_mask)
{
	_glXGetSelectedEvent(_localdpy, ServerDrawable(dpy, draw), event_mask);
}

shimfuncdpy2( GLXContext, glXImportContextEXT, Display*, dpy, GLXContextID, contextID );
shimfuncdpy2( Bool, glXIsDirect, Display*, dpy, GLXContext, ctx );
shimfuncdpy4( int, glXQueryContext, Display*, dpy, GLXContext, ctx, int, attribute, int*, value );
shimfuncdpy4( int, glXQueryContextInfoEXT, Display*, dpy, GLXContext, ctx, int, attribute, int*, value );

void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
	return _glXQueryDrawable(_localdpy, ServerDrawable(dpy, draw), attribute, value);
}

shimfuncdpy3( Bool, glXQueryExtension, Display*, dpy, int*, error_base, int*, event_base );
shimfuncdpy2( const char*, glXQueryExtensionsString, Display*, dpy, int, screen );
shimfuncdpy3( const char*, glXQueryServerString, Display*, dpy, int, screen, int, name );
shimfuncdpy3( Bool, glXQueryVersion, Display*, dpy, int*, major, int*, minor );

void glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask)
{
	_glXSelectEvent(_localdpy, ServerDrawable(dpy, draw), event_mask);
}

shimfuncdpy4( int, glXGetFBConfigAttribSGIX, Display*, dpy, GLXFBConfigSGIX, config, int, attribute, int*, value_return );
shimfuncdpy4( GLXFBConfigSGIX*, glXChooseFBConfigSGIX, Display*, dpy, int, screen, const int*, attrib_list, int*, nelements );

GLXFBConfigSGIX glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
	return _MatchConfig(dpy, vis);
}

GLXPbuffer glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, const int *attrib_list)
{
	GLXPbuffer pb=0;
	pb=_glXCreateGLXPbufferSGIX(_localdpy, config, width, height, attrib_list);
	if(dpy && pb) glxdh.add(pb, dpy);
	return pb;
}

void glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbuffer pbuf)
{
	_glXDestroyGLXPbufferSGIX(_localdpy, pbuf);
	if(pbuf) glxdh.remove(pbuf);
}

shimfuncdpy4( void, glXQueryGLXPbufferSGIX, Display*, dpy, GLXPbuffer, pbuf, int, attribute, unsigned int*, value );

void glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
	_glXSelectEventSGIX(_localdpy, ServerDrawable(dpy, drawable), mask);
}

void glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
	_glXGetSelectedEventSGIX(_localdpy, ServerDrawable(dpy, drawable), mask);
}

Bool glXJoinSwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint group)
{
	return _glXJoinSwapGroupNV(_localdpy, ServerDrawable(dpy, drawable), group);
}

shimfuncdpy3(Bool, glXBindSwapBarrierNV, Display*, dpy, GLuint, group, GLuint, barrier );

Bool glXQuerySwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier)
{
	return _glXQuerySwapGroupNV(_localdpy, ServerDrawable(dpy, drawable), group, barrier);
}

shimfuncdpy4(Bool, glXQueryMaxSwapGroupsNV, Display*, dpy, int, screen, GLuint*, maxGroups, GLuint*, maxBarriers );
shimfuncdpy3(Bool, glXQueryFrameCountNV, Display*, dpy, int, screen, GLuint*, count );
shimfuncdpy2(Bool, glXResetFrameCountNV, Display*, dpy, int, screen );

}
