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

XVisualInfo *_MatchVisual(Display *dpy, XVisualInfo *vis)
{
	XVisualInfo *v=NULL;
	if(!dpy || !vis) return NULL;
	if(!(v=vish.matchvisual(dpy, vis)))
	{
		XVisualInfo vtemp;  int n;
		if(!XMatchVisualInfo(_localdpy, DefaultScreen(_localdpy), vis->depth, vis->c_class, &vtemp)) return NULL;
		if(!(v=XGetVisualInfo(_localdpy, VisualIDMask, &vtemp, &n)) || !n) return NULL;
		vish.add(dpy, vis, _localdpy, v);
	}
	return v;
}

extern "C" {

shimfuncdpy4( GLXFBConfig*, glXChooseFBConfig, Display*, dpy, int, screen, const int*, attrib_list, int*, nelements, return );
shimfuncdpy4( void, glXCopyContext, Display*, dpy, GLXContext, src, GLXContext, dst, unsigned long, mask, );
shimfuncdpy3( GLXPbuffer, glXCreatePbuffer, Display*, dpy, GLXFBConfig, config, const int*, attrib_list, return );
shimfuncdpy2( void, glXDestroyPbuffer, Display*, dpy, GLXPbuffer, pbuf, );
shimfuncdpy2( void, glXFreeContextEXT, Display*, dpy, GLXContext, ctx, );
shimfuncdpy2( const char*, glXGetClientString, Display*, dpy, int, name, return );
shimfuncdpyvis4( int, glXGetConfig, Display*, dpy, XVisualInfo*, vis, int, attrib, int*, value, return );
shimfuncdpy4( int, glXGetFBConfigAttrib, Display*, dpy, GLXFBConfig, config, int, attribute, int*, value, return );
shimfuncdpy3( GLXFBConfig*, glXGetFBConfigs, Display*, dpy, int, screen, int*, nelements, return );
shimfuncdpy3( void, glXGetSelectedEvent, Display*, dpy, GLXDrawable, draw, unsigned long*, event_mask, );
shimfuncdpy2( GLXContext, glXImportContextEXT, Display*, dpy, GLXContextID, contextID, return );
shimfuncdpy2( Bool, glXIsDirect, Display*, dpy, GLXContext, ctx, return );
shimfuncdpy4( int, glXQueryContext, Display*, dpy, GLXContext, ctx, int, attribute, int*, value, return );
shimfuncdpy4( int, glXQueryContextInfoEXT, Display*, dpy, GLXContext, ctx, int, attribute, int*, value, return );
shimfuncdpy4( void, glXQueryDrawable, Display*, dpy, GLXDrawable, draw, int, attribute, unsigned int*, value, );
shimfuncdpy3( Bool, glXQueryExtension, Display*, dpy, int*, error_base, int*, event_base, return );
shimfuncdpy2( const char*, glXQueryExtensionsString, Display*, dpy, int, screen, return );
shimfuncdpy3( const char*, glXQueryServerString, Display*, dpy, int, screen, int, name, return );
shimfuncdpy3( Bool, glXQueryVersion, Display*, dpy, int*, major, int*, minor, return );
shimfuncdpy3( void, glXSelectEvent, Display*, dpy, GLXDrawable, draw, unsigned long, event_mask, );

shimfuncdpy4( int, glXGetFBConfigAttribSGIX, Display*, dpy, GLXFBConfigSGIX, config, int, attribute, int*, value_return, return );
shimfuncdpy4( GLXFBConfigSGIX*, glXChooseFBConfigSGIX, Display*, dpy, int, screen, const int*, attrib_list, int*, nelements, return );
shimfuncdpyvis2( GLXFBConfigSGIX, glXGetFBConfigFromVisualSGIX, Display*, dpy, XVisualInfo*, vis, return );

shimfuncdpy5( GLXPbuffer, glXCreateGLXPbufferSGIX, Display*, dpy, GLXFBConfig, config, unsigned int, width, unsigned int, height, const int*, attrib_list, return );
shimfuncdpy2( void, glXDestroyGLXPbufferSGIX, Display*, dpy, GLXPbuffer, pbuf, );
shimfuncdpy4( void, glXQueryGLXPbufferSGIX, Display*, dpy, GLXPbuffer, pbuf, int, attribute, unsigned int*, value, );
shimfuncdpy3( void, glXSelectEventSGIX, Display*, dpy, GLXDrawable, drawable, unsigned long, mask, );
shimfuncdpy3( void, glXGetSelectedEventSGIX, Display*, dpy, GLXDrawable, drawable, unsigned long*, mask, );

shimfuncdpy3(Bool, glXJoinSwapGroupNV, Display*, dpy, GLXDrawable, drawable, GLuint, group, return );
shimfuncdpy3(Bool, glXBindSwapBarrierNV, Display*, dpy, GLuint, group, GLuint, barrier, return );
shimfuncdpy4(Bool, glXQuerySwapGroupNV, Display*, dpy, GLXDrawable, drawable, GLuint*, group, GLuint*, barrier, return );
shimfuncdpy4(Bool, glXQueryMaxSwapGroupsNV, Display*, dpy, int, screen, GLuint*, maxGroups, GLuint*, maxBarriers, return );
shimfuncdpy3(Bool, glXQueryFrameCountNV, Display*, dpy, int, screen, GLuint*, count, return );
shimfuncdpy2(Bool, glXResetFrameCountNV, Display*, dpy, int, screen, return );

}
