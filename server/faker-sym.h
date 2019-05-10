// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011, 2013-2016, 2018-2019 D. R. Commander
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

#ifndef __FAKER_SYM_H__
#define __FAKER_SYM_H__

#include <stdio.h>
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include "glx.h"
#include "Log.h"
#include "GlobalCriticalSection.h"
#include "vglinline.h"
#ifdef FAKEXCB
extern "C" {
#ifdef SYSXCBHEADERS
	#include <xcb/xcb.h>
	#include <xcb/xcbext.h>
	#include <xcb/xcb_keysyms.h>
	#include <xcb/glx.h>
	#include <X11/Xlib-xcb.h>
#else
	#include "xcb_headers/xcb.h"
	#include "xcb_headers/xcbext.h"
	#include "xcb_headers/xcb_keysyms.h"
	#include "xcb_headers/glx.h"
	#include "xcb_headers/Xlib-xcb.h"
#endif
}
#endif
#include "faker.h"


#define CHECKSYM_NONFATAL(s) \
{ \
	if(!__##s) \
	{ \
		vglfaker::init(); \
		vglfaker::GlobalCriticalSection::SafeLock l(globalMutex); \
		if(!__##s) __##s = (_##s##Type)vglfaker::loadSymbol(#s, true); \
	} \
}

#define CHECKSYM(s, fake_s) \
{ \
	if(!__##s) \
	{ \
		vglfaker::init(); \
		vglfaker::GlobalCriticalSection::SafeLock l(globalMutex); \
		if(!__##s) __##s = (_##s##Type)vglfaker::loadSymbol(#s); \
	} \
	if(!__##s) vglfaker::safeExit(1); \
	if(__##s == fake_s) \
	{ \
		vglout.print("[VGL] ERROR: VirtualGL attempted to load the real\n"); \
		vglout.print("[VGL]   " #s " function and got the fake one instead.\n"); \
		vglout.print("[VGL]   Something is terribly wrong.  Aborting before chaos ensues.\n"); \
		vglfaker::safeExit(1); \
	} \
}

#ifdef __LOCALSYM__
#define SYMDEF(f)  _##f##Type __##f = NULL
#else
#define SYMDEF(f)  extern _##f##Type __##f
#endif


#define DISABLE_FAKER() \
	vglfaker::setFakerLevel(vglfaker::getFakerLevel() + 1);
#define ENABLE_FAKER() \
	vglfaker::setFakerLevel(vglfaker::getFakerLevel() - 1);


#define FUNCDEF0(RetType, f, fake_f) \
	typedef RetType (*_##f##Type)(void); \
	SYMDEF(f); \
	static INLINE RetType _##f(void) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF0(f, fake_f) \
	typedef void (*_##f##Type)(void); \
	SYMDEF(f); \
	static INLINE void _##f(void) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF1(RetType, f, at1, a1, fake_f) \
	typedef RetType (*_##f##Type)(at1); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF1(f, at1, a1, fake_f) \
	typedef void (*_##f##Type)(at1); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF2(RetType, f, at1, a1, at2, a2, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF2(f, at1, a1, at2, a2, fake_f) \
	typedef void (*_##f##Type)(at1, at2); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF3(RetType, f, at1, a1, at2, a2, at3, a3, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF3(f, at1, a1, at2, a2, at3, a3, fake_f) \
	typedef void (*_##f##Type)(at1, at2, at3); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2, at3 a3) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2, a3); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF4(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF4(f, at1, a1, at2, a2, at3, a3, at4, a4, fake_f) \
	typedef void (*_##f##Type)(at1, at2, at3, at4); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2, at3 a3, at4 a4) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2, a3, a4); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF5(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF5(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, fake_f) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2, a3, a4, a5); \
		ENABLE_FAKER(); \
	}

#define VFUNCDEF6(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, \
	fake_f) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2, a3, a4, a5, a6); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF6(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5, a6); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define VFUNCDEF7(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, at, at6, a6, \
	at7, a7, fake_f) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2, a3, a4, a5, a6, a7); \
		ENABLE_FAKER(); \
	}

#define FUNCDEF8(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5, a6, a7, a8); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define FUNCDEF9(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define FUNCDEF10(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); \
		ENABLE_FAKER(); \
		return retval; \
	}

#define FUNCDEF12(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, at11, a11, at12, a12, \
	fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10, at11, at12); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10, at11 a11, at12 a12) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); \
		ENABLE_FAKER(); \
		return retval; \
	}


#ifdef __cplusplus
extern "C" {
#endif


// GLX 1.0 functions

FUNCDEF3(XVisualInfo *, glXChooseVisual, Display *, dpy, int, screen,
	int *, attrib_list, glXChooseVisual);

VFUNCDEF4(glXCopyContext, Display *, dpy, GLXContext, src, GLXContext, dst,
	unsigned long, mask, glXCopyContext);

FUNCDEF4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct, glXCreateContext);

FUNCDEF3(GLXPixmap, glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
	Pixmap, pixmap, glXCreateGLXPixmap);

VFUNCDEF2(glXDestroyContext, Display *, dpy, GLXContext, ctx,
	glXDestroyContext);

VFUNCDEF2(glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pix,
	glXDestroyGLXPixmap);

FUNCDEF4(int, glXGetConfig, Display *, dpy, XVisualInfo *, vis, int, attrib,
	int *, value, glXGetConfig);

FUNCDEF0(GLXDrawable, glXGetCurrentDrawable, glXGetCurrentDrawable);

FUNCDEF2(Bool, glXIsDirect, Display *, dpy, GLXContext, ctx, glXIsDirect);

FUNCDEF3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx, glXMakeCurrent);

FUNCDEF3(Bool, glXQueryExtension, Display *, dpy, int *, error_base,
	int *, event_base, glXQueryExtension);

FUNCDEF3(Bool, glXQueryVersion, Display *, dpy, int *, major, int *, minor,
	glXQueryVersion);

VFUNCDEF2(glXSwapBuffers, Display *, dpy, GLXDrawable, drawable,
	glXSwapBuffers);

VFUNCDEF4(glXUseXFont, Font, font, int, first, int, count, int, list_base,
	glXUseXFont);

VFUNCDEF0(glXWaitGL, glXWaitGL);


// GLX 1.1 functions

FUNCDEF2(const char *, glXGetClientString, Display *, dpy, int, name,
	glXGetClientString);

FUNCDEF3(const char *, glXQueryServerString, Display *, dpy, int, screen, int,
	name, glXQueryServerString);

FUNCDEF2(const char *, glXQueryExtensionsString, Display *, dpy, int, screen,
	glXQueryExtensionsString);


// GLX 1.2 functions

FUNCDEF0(Display *, glXGetCurrentDisplay, glXGetCurrentDisplay);


// GLX 1.3 functions

FUNCDEF4(GLXFBConfig *, glXChooseFBConfig, Display *, dpy, int, screen,
	const int *, attrib_list, int *, nelements, glXChooseFBConfig);

FUNCDEF5(GLXContext, glXCreateNewContext, Display *, dpy, GLXFBConfig, config,
	int, render_type, GLXContext, share_list, Bool, direct, glXCreateNewContext);

FUNCDEF3(GLXPbuffer, glXCreatePbuffer, Display *, dpy, GLXFBConfig, config,
	const int *, attrib_list, glXCreatePbuffer);

FUNCDEF4(GLXPixmap, glXCreatePixmap, Display *, dpy, GLXFBConfig, config,
	Pixmap, pixmap, const int *, attrib_list, glXCreatePixmap);

FUNCDEF4(GLXWindow, glXCreateWindow, Display *, dpy, GLXFBConfig, config,
	Window, win, const int *, attrib_list, glXCreateWindow);

VFUNCDEF2(glXDestroyPbuffer, Display *, dpy, GLXPbuffer, pbuf,
	glXDestroyPbuffer);

VFUNCDEF2(glXDestroyPixmap, Display *, dpy, GLXPixmap, pixmap,
	glXDestroyPixmap);

VFUNCDEF2(glXDestroyWindow, Display *, dpy, GLXWindow, win, glXDestroyWindow);

FUNCDEF0(GLXDrawable, glXGetCurrentReadDrawable, glXGetCurrentReadDrawable);

FUNCDEF4(int, glXGetFBConfigAttrib, Display *, dpy, GLXFBConfig, config,
	int, attribute, int *, value, glXGetFBConfigAttrib);

FUNCDEF3(GLXFBConfig *, glXGetFBConfigs, Display *, dpy, int, screen,
	int *, nelements, glXGetFBConfigs);

VFUNCDEF3(glXGetSelectedEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long *, event_mask, glXGetSelectedEvent);

FUNCDEF2(XVisualInfo *, glXGetVisualFromFBConfig, Display *, dpy,
	GLXFBConfig, config, glXGetVisualFromFBConfig);

FUNCDEF4(Bool, glXMakeContextCurrent, Display *, display, GLXDrawable, draw,
	GLXDrawable, read, GLXContext, ctx, glXMakeContextCurrent);

FUNCDEF4(int, glXQueryContext, Display *, dpy, GLXContext, ctx, int, attribute,
	int *, value, glXQueryContext);

VFUNCDEF4(glXQueryDrawable, Display *, dpy, GLXDrawable, draw, int, attribute,
	unsigned int *, value, glXQueryDrawable);

VFUNCDEF3(glXSelectEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long, event_mask, glXSelectEvent);


// GLX 1.4 functions

typedef void (*(*_glXGetProcAddressType)(const GLubyte *))(void);
SYMDEF(glXGetProcAddress);
static INLINE void (*_glXGetProcAddress(const GLubyte *procName))(void)
{
	CHECKSYM(glXGetProcAddress, glXGetProcAddress);
	return __glXGetProcAddress(procName);
}


// GLX_ARB_create_context

FUNCDEF5(GLXContext, glXCreateContextAttribsARB, Display *, dpy, GLXFBConfig,
	config, GLXContext, share_context, Bool, direct, const int *, attribs,
	glXCreateContextAttribsARB);


// GLX_ARB_get_proc_address

typedef void (*(*_glXGetProcAddressARBType)(const GLubyte *))(void);
SYMDEF(glXGetProcAddressARB);
static INLINE void (*_glXGetProcAddressARB(const GLubyte *procName))(void)
{
	CHECKSYM(glXGetProcAddressARB, glXGetProcAddressARB);
	return __glXGetProcAddressARB(procName);
}


// GLX_EXT_import_context

VFUNCDEF2(glXFreeContextEXT, Display *, dpy, GLXContext, ctx,
	glXFreeContextEXT);

FUNCDEF2(GLXContext, glXImportContextEXT, Display *, dpy,
	GLXContextID, contextID, glXImportContextEXT);

FUNCDEF4(int, glXQueryContextInfoEXT, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value, glXQueryContextInfoEXT);


// GLX_EXT_swap_control

VFUNCDEF3(glXSwapIntervalEXT, Display *, dpy, GLXDrawable, drawable, int,
	interval, glXSwapIntervalEXT);


// GLX_EXT_texture_from_pixmap

VFUNCDEF4(glXBindTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, const int *, attrib_list, glXBindTexImageEXT);

VFUNCDEF3(glXReleaseTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, glXReleaseTexImageEXT)


// GLX_NV_swap_group

FUNCDEF3(Bool, glXJoinSwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint, group, glXJoinSwapGroupNV);

FUNCDEF3(Bool, glXBindSwapBarrierNV, Display *, dpy, GLuint, group,
	GLuint, barrier, glXBindSwapBarrierNV);

FUNCDEF4(Bool, glXQuerySwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint *, group, GLuint *, barrier, glXQuerySwapGroupNV);

FUNCDEF4(Bool, glXQueryMaxSwapGroupsNV, Display *, dpy, int, screen,
	GLuint *, maxGroups, GLuint *, maxBarriers, glXQueryMaxSwapGroupsNV);

FUNCDEF3(Bool, glXQueryFrameCountNV, Display *, dpy, int, screen,
	GLuint *, count, glXQueryFrameCountNV);

FUNCDEF2(Bool, glXResetFrameCountNV, Display *, dpy, int, screen,
	glXResetFrameCountNV);


// GLX_SGI_swap_control

FUNCDEF1(int, glXSwapIntervalSGI, int, interval, glXSwapIntervalSGI);


// GLX_SGIX_fbconfig

FUNCDEF2(GLXFBConfigSGIX, glXGetFBConfigFromVisualSGIX, Display *, dpy,
	XVisualInfo *, vis, glXGetFBConfigFromVisualSGIX);


// GLX_SUN_get_transparent_index

FUNCDEF4(int, glXGetTransparentIndexSUN, Display *, dpy, Window, overlay,
	Window, underlay, long *, transparentIndex, glXGetTransparentIndexSUN);


// GL functions

VFUNCDEF0(glFinish, glFinish);

VFUNCDEF0(glFlush, glFlush);

VFUNCDEF4(glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	glViewport);

VFUNCDEF1(glDrawBuffer, GLenum, drawbuf, glDrawBuffer);

VFUNCDEF0(glPopAttrib, glPopAttrib);


// X11 functions

FUNCDEF3(Bool, XCheckMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	XCheckMaskEvent);

FUNCDEF3(Bool, XCheckTypedEvent, Display *, dpy, int, event_type, XEvent *, xe,
	XCheckTypedEvent);

FUNCDEF4(Bool, XCheckTypedWindowEvent, Display *, dpy, Window, win,
	int, event_type, XEvent *, xe, XCheckTypedWindowEvent);

FUNCDEF4(Bool, XCheckWindowEvent, Display *, dpy, Window, win, long,
	event_mask, XEvent *, xe, XCheckWindowEvent);

FUNCDEF1(int, XCloseDisplay, Display *, dpy, XCloseDisplay);

FUNCDEF4(int, XConfigureWindow, Display *, dpy, Window, win,
	unsigned int, value_mask, XWindowChanges *, values, XConfigureWindow);

FUNCDEF10(int, XCopyArea, Display *, dpy, Drawable, src, Drawable, dst, GC, gc,
	int, src_x, int, src_y, unsigned int, w, unsigned int, h, int, dest_x,
	int, dest_y, XCopyArea);

FUNCDEF9(Window, XCreateSimpleWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height, unsigned int,
	border_width, unsigned long, border, unsigned long, background,
	XCreateSimpleWindow);

FUNCDEF12(Window, XCreateWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height,
	unsigned int, border_width, int, depth, unsigned int, c_class,
	Visual *, visual, unsigned long, value_mask,
	XSetWindowAttributes *, attributes, XCreateWindow);

FUNCDEF2(int, XDestroySubwindows, Display *, dpy, Window, win,
	XDestroySubwindows);

FUNCDEF2(int, XDestroyWindow, Display *, dpy, Window, win, XDestroyWindow);

FUNCDEF1(int, XFree, void *, data, XFree);

FUNCDEF9(Status, XGetGeometry, Display *, display, Drawable, d, Window *, root,
	int *, x, int *, y, unsigned int *, width, unsigned int *, height,
	unsigned int *, border_width, unsigned int *, depth, XGetGeometry);

FUNCDEF8(XImage *, XGetImage, Display *, display, Drawable, d, int, x, int, y,
	unsigned int, width, unsigned int, height, unsigned long, plane_mask,
	int, format, XGetImage);

FUNCDEF2(char **, XListExtensions, Display *, dpy, int *, next,
	XListExtensions);

FUNCDEF3(int, XMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	XMaskEvent);

FUNCDEF6(int, XMoveResizeWindow, Display *, dpy, Window, win, int, x, int, y,
	unsigned int, width, unsigned int, height, XMoveResizeWindow);

FUNCDEF2(int, XNextEvent, Display *, dpy, XEvent *, xe, XNextEvent);

FUNCDEF1(Display *, XOpenDisplay, _Xconst char *, name, XOpenDisplay);

FUNCDEF5(Bool, XQueryExtension, Display *, dpy, _Xconst char *, name,
	int *, major_opcode, int *, first_event, int *, first_error,
	XQueryExtension);

FUNCDEF4(int, XResizeWindow, Display *, dpy, Window, win, unsigned int, width,
	unsigned int, height, XResizeWindow);

FUNCDEF1(char *, XServerVendor, Display *, dpy, XServerVendor);

FUNCDEF4(int, XWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe, XWindowEvent);


// From dlfaker

typedef void *(*_dlopenType)(const char *, int);
void *_vgl_dlopen(const char *, int);
SYMDEF(dlopen);


#ifdef FAKEXCB

// XCB functions

FUNCDEF2(const xcb_query_extension_reply_t *, xcb_get_extension_data,
	xcb_connection_t *, conn, xcb_extension_t *, ext, xcb_get_extension_data);

FUNCDEF3(xcb_glx_query_version_cookie_t, xcb_glx_query_version,
	xcb_connection_t *, conn, uint32_t, major_version, uint32_t, minor_version,
	xcb_glx_query_version);

FUNCDEF3(xcb_glx_query_version_reply_t *, xcb_glx_query_version_reply,
	xcb_connection_t *, conn, xcb_glx_query_version_cookie_t, cookie,
	xcb_generic_error_t **, error, xcb_glx_query_version_reply);

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_event, xcb_connection_t *, conn,
	xcb_poll_for_event);

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_queued_event, xcb_connection_t *,
	conn, xcb_poll_for_queued_event);

FUNCDEF1(xcb_generic_event_t *, xcb_wait_for_event, xcb_connection_t *, conn,
	xcb_wait_for_event);

#endif


// Functions used by the faker (but not interposed.)  We load the GLX/OpenGL
// functions dynamically to prevent the 3D application from overriding them, as
// well as to ensure that, with 'vglrun -nodl', libGL is not loaded into the
// process until the 3D application actually uses it.

VFUNCDEF2(glBindBuffer, GLenum, target, GLuint, buffer, NULL);

VFUNCDEF7(glBitmap, GLsizei, width, GLsizei, height, GLfloat, xorig,
	GLfloat, yorig, GLfloat, xmove, GLfloat, ymove, const GLubyte *, bitmap,
	NULL);

VFUNCDEF4(glBufferData, GLenum, target, GLsizeiptr, size, const GLvoid *, data,
	GLenum, usage, NULL);

VFUNCDEF1(glClear, GLbitfield, mask, NULL);

VFUNCDEF4(glClearColor, GLclampf, red, GLclampf, green, GLclampf, blue,
	GLclampf, alpha, NULL);

VFUNCDEF5(glCopyPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	GLenum, type, NULL);

VFUNCDEF0(glEndList, NULL);

VFUNCDEF2(glGenBuffers, GLsizei, n, GLuint *, buffers, NULL);

VFUNCDEF3(glGetBufferParameteriv, GLenum, target, GLenum, value, GLint *, data,
	NULL);

FUNCDEF0(GLenum, glGetError, NULL);

VFUNCDEF2(glGetFloatv, GLenum, pname, GLfloat *, params, NULL);

VFUNCDEF2(glGetIntegerv, GLenum, pname, GLint *, params, NULL);

FUNCDEF1(const GLubyte *, glGetString, GLenum, name, NULL);

VFUNCDEF0(glLoadIdentity, NULL);

FUNCDEF2(void *, glMapBuffer, GLenum, target, GLenum, access, NULL);

VFUNCDEF1(glMatrixMode, GLenum, mode, NULL);

VFUNCDEF2(glNewList, GLuint, list, GLenum, mode, NULL);

VFUNCDEF6(glOrtho, GLdouble, left, GLdouble, right, GLdouble, bottom,
	GLdouble, top, GLdouble, near_val, GLdouble, far_val, NULL);

VFUNCDEF2(glPixelStorei, GLenum, pname, GLint, param, NULL);

VFUNCDEF0(glPopMatrix, NULL);

VFUNCDEF0(glPushMatrix, NULL);

VFUNCDEF2(glRasterPos2i, GLint, x, GLint, y, NULL);

VFUNCDEF1(glReadBuffer, GLenum, mode, NULL);

VFUNCDEF7(glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	GLenum, format, GLenum, type, GLvoid *, pixels, NULL);

FUNCDEF1(GLboolean, glUnmapBuffer, GLenum, target, NULL);

FUNCDEF0(GLXContext, glXGetCurrentContext, NULL);

// We load all XCB functions dynamically, so that the same VirtualGL binary
// can be used to support systems with and without XCB libraries.

#ifdef FAKEXCB

FUNCDEF1(xcb_connection_t *, XGetXCBConnection, Display *, dpy, NULL);

VFUNCDEF2(XSetEventQueueOwner, Display *, dpy, enum XEventQueueOwner, owner,
	XSetEventQueueOwner);

typedef xcb_extension_t *_xcb_glx_idType;
SYMDEF(xcb_glx_id);
static INLINE xcb_extension_t *_xcb_glx_id(void)
{
	CHECKSYM(xcb_glx_id, NULL);
	return __xcb_glx_id;
}

FUNCDEF4(xcb_intern_atom_cookie_t, xcb_intern_atom, xcb_connection_t *, conn,
	uint8_t, only_if_exists, uint16_t, name_len, const char *, name, NULL);

FUNCDEF3(xcb_intern_atom_reply_t *, xcb_intern_atom_reply, xcb_connection_t *,
	conn, xcb_intern_atom_cookie_t, cookie, xcb_generic_error_t **, e, NULL);

FUNCDEF1(xcb_key_symbols_t *, xcb_key_symbols_alloc, xcb_connection_t *, c,
	NULL);

VFUNCDEF1(xcb_key_symbols_free, xcb_key_symbols_t *, syms, NULL);

FUNCDEF3(xcb_keysym_t, xcb_key_symbols_get_keysym, xcb_key_symbols_t *, syms,
	xcb_keycode_t, keycode, int, col, NULL);

#endif

#ifdef __cplusplus
}
#endif


static INLINE int DrawingToFront(void)
{
	GLint drawbuf = GL_BACK;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return IS_FRONT(drawbuf);
}


static INLINE int DrawingToRight(void)
{
	GLint drawbuf = GL_LEFT;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return IS_RIGHT(drawbuf);
}

#endif  // __FAKER_SYM_H__
