/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011, 2013-2015 D. R. Commander
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

#ifndef __FAKER_SYM_H__
#define __FAKER_SYM_H__

#include <stdio.h>
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include "glx.h"
#include "Log.h"
#include "GlobalCriticalSection.h"
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


namespace vglfaker
{
	extern void safeExit(int);
	extern void init(void);
	#ifdef FAKEXCB
	extern long getFakerLevel(void);
	extern void setFakerLevel(long level);
	#endif

	void *loadSymbol(const char *name, bool optional=false);
	void unloadSymbols(void);
}


#define CHECKSYM_NONFATAL(s) {  \
	if(!__##s) {  \
		vglfaker::init();  \
		vglfaker::GlobalCriticalSection::SafeLock l(globalMutex);  \
		if(!__##s) __##s=(_##s##Type)vglfaker::loadSymbol(#s, true);  \
	}  \
}

#define CHECKSYM(s) {  \
	if(!__##s) {  \
		vglfaker::init();  \
		vglfaker::GlobalCriticalSection::SafeLock l(globalMutex);  \
		if(!__##s) __##s=(_##s##Type)vglfaker::loadSymbol(#s);  \
	}  \
	if(!__##s) vglfaker::safeExit(1);  \
}

#ifdef __LOCALSYM__
#define SYMDEF(f) _##f##Type __##f=NULL
#else
#define SYMDEF(f) extern _##f##Type __##f
#endif


#ifdef FAKEXCB
#define DISABLE_FAKEXCB() vglfaker::setFakerLevel(vglfaker::getFakerLevel()+1);
#define ENABLE_FAKEXCB()  vglfaker::setFakerLevel(vglfaker::getFakerLevel()-1);
#else
#define DISABLE_FAKEXCB()
#define ENABLE_FAKEXCB()
#endif


#define FUNCDEF0(RetType, f) \
	typedef RetType (*_##f##Type)(void); \
	SYMDEF(f); \
	static inline RetType _##f(void) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF0(f) \
	typedef void (*_##f##Type)(void); \
	SYMDEF(f); \
	static inline void _##f(void) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF1(RetType, f, at1, a1) \
	typedef RetType (*_##f##Type)(at1); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF1(f, at1, a1) \
	typedef void (*_##f##Type)(at1); \
	SYMDEF(f); \
	static inline void _##f(at1 a1) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF2(RetType, f, at1, a1, at2, a2) \
	typedef RetType (*_##f##Type)(at1, at2); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF2(f, at1, a1, at2, a2) \
	typedef void (*_##f##Type)(at1, at2); \
	SYMDEF(f); \
	static inline void _##f(at1 a1, at2 a2) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1, a2); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF3(RetType, f, at1, a1, at2, a2, at3, a3) \
	typedef RetType (*_##f##Type)(at1, at2, at3); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF3(f, at1, a1, at2, a2, at3, a3) \
	typedef void (*_##f##Type)(at1, at2, at3); \
	SYMDEF(f); \
	static inline void _##f(at1 a1, at2 a2, at3 a3) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1, a2, a3); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF4(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF4(f, at1, a1, at2, a2, at3, a3, at4, a4) \
	typedef void (*_##f##Type)(at1, at2, at3, at4); \
	SYMDEF(f); \
	static inline void _##f(at1 a1, at2 a2, at3 a3, at4 a4) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1, a2, a3, a4); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF5(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4, a5); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF5(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5); \
	SYMDEF(f); \
	static inline void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1, a2, a3, a4, a5); \
		ENABLE_FAKEXCB(); \
	}

#define VFUNCDEF6(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	SYMDEF(f); \
	static inline void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1, a2, a3, a4, a5, a6); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF6(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, \
		at6 a6) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4, a5, a6); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define VFUNCDEF7(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, at, at6, a6, \
	at7, a7) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7); \
	SYMDEF(f); \
	static inline void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7) { \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		__##f(a1, a2, a3, a4, a5, a6, a7); \
		ENABLE_FAKEXCB(); \
	}

#define FUNCDEF8(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4, a5, a6, a7, a8); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define FUNCDEF9(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4, a5, a6, a7, a8, a9); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define FUNCDEF10(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}

#define FUNCDEF12(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, at11, a11, at12, a12) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10, at11, at12); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10, at11 a11, at12 a12) { \
		RetType retval; \
		CHECKSYM(f); \
		DISABLE_FAKEXCB(); \
		retval=__##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); \
		ENABLE_FAKEXCB(); \
		return retval; \
	}


#ifdef __cplusplus
extern "C" {
#endif


// GLX 1.0 functions

FUNCDEF3(XVisualInfo*, glXChooseVisual, Display *, dpy, int, screen,
	int *, attrib_list);

VFUNCDEF4(glXCopyContext, Display *, dpy, GLXContext, src, GLXContext, dst,
	unsigned long, mask);

FUNCDEF4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct);

FUNCDEF3(GLXPixmap, glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
	Pixmap, pixmap);

VFUNCDEF2(glXDestroyContext, Display *, dpy, GLXContext, ctx);

VFUNCDEF2(glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pix);

FUNCDEF4(int, glXGetConfig, Display *, dpy, XVisualInfo *, vis, int, attrib,
	int *, value);

FUNCDEF0(GLXDrawable, glXGetCurrentDrawable);

FUNCDEF2(Bool, glXIsDirect, Display *, dpy, GLXContext, ctx);

FUNCDEF3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx);

FUNCDEF3(Bool, glXQueryExtension, Display *, dpy, int *, error_base,
	int *, event_base);

FUNCDEF3(Bool, glXQueryVersion, Display *, dpy, int *, major, int *, minor);

VFUNCDEF2(glXSwapBuffers, Display *, dpy, GLXDrawable, drawable);

VFUNCDEF4(glXUseXFont, Font, font, int, first, int, count, int, list_base);

VFUNCDEF0(glXWaitGL);


// GLX 1.1 functions

FUNCDEF2(const char *, glXGetClientString, Display *, dpy, int, name);

FUNCDEF3(const char *, glXQueryServerString, Display *, dpy, int, screen, int,
	name);

FUNCDEF2(const char *, glXQueryExtensionsString, Display *, dpy, int, screen);


// GLX 1.2 functions

FUNCDEF0(Display*, glXGetCurrentDisplay);


// GLX 1.3 functions

FUNCDEF4(GLXFBConfig *, glXChooseFBConfig, Display *, dpy, int, screen,
	const int *, attrib_list, int *, nelements);

FUNCDEF5(GLXContext, glXCreateNewContext, Display *, dpy, GLXFBConfig, config,
	int, render_type, GLXContext, share_list, Bool, direct);

FUNCDEF3(GLXPbuffer, glXCreatePbuffer, Display *, dpy, GLXFBConfig, config,
	const int *, attrib_list);

FUNCDEF4(GLXPixmap, glXCreatePixmap, Display *, dpy, GLXFBConfig, config,
	Pixmap, pixmap, const int *, attrib_list);

FUNCDEF4(GLXWindow, glXCreateWindow, Display *, dpy, GLXFBConfig, config,
	Window, win, const int *, attrib_list);

VFUNCDEF2(glXDestroyPbuffer, Display *, dpy, GLXPbuffer, pbuf);

VFUNCDEF2(glXDestroyPixmap, Display *, dpy, GLXPixmap, pixmap);

VFUNCDEF2(glXDestroyWindow, Display *, dpy, GLXWindow, win);

FUNCDEF0(GLXDrawable, glXGetCurrentReadDrawable);

FUNCDEF4(int, glXGetFBConfigAttrib, Display *, dpy, GLXFBConfig, config,
	int, attribute, int *, value);

FUNCDEF3(GLXFBConfig *, glXGetFBConfigs, Display *, dpy, int, screen,
	int *, nelements);

VFUNCDEF3(glXGetSelectedEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long *, event_mask);

FUNCDEF2(XVisualInfo *, glXGetVisualFromFBConfig, Display *, dpy,
	GLXFBConfig, config);

FUNCDEF4(Bool, glXMakeContextCurrent, Display *, display, GLXDrawable, draw,
	GLXDrawable, read, GLXContext, ctx);

FUNCDEF4(int, glXQueryContext, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value);

VFUNCDEF4(glXQueryDrawable, Display *, dpy, GLXDrawable, draw, int, attribute,
	unsigned int *, value);

VFUNCDEF3(glXSelectEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long, event_mask);


// GLX 1.4 functions

typedef void (*(*_glXGetProcAddressType)(const GLubyte*))(void);
SYMDEF(glXGetProcAddress);
static inline void (*_glXGetProcAddress(const GLubyte *procName))(void)
{
	CHECKSYM(glXGetProcAddress);  return __glXGetProcAddress(procName);
}


// GLX_ARB_create_context

FUNCDEF5(GLXContext, glXCreateContextAttribsARB, Display *, dpy, GLXFBConfig,
	config, GLXContext, share_context, Bool, direct, const int *, attribs);


// GLX_ARB_get_proc_address

typedef void (*(*_glXGetProcAddressARBType)(const GLubyte*))(void);
SYMDEF(glXGetProcAddressARB);
static inline void (*_glXGetProcAddressARB(const GLubyte *procName))(void)
{
	CHECKSYM(glXGetProcAddressARB);  return __glXGetProcAddressARB(procName);
}


// GLX_EXT_import_context

VFUNCDEF2(glXFreeContextEXT, Display *, dpy, GLXContext, ctx);

FUNCDEF2(GLXContext, glXImportContextEXT, Display *, dpy,
	GLXContextID, contextID);

FUNCDEF4(int, glXQueryContextInfoEXT, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value);


// GLX_EXT_swap_control

VFUNCDEF3(glXSwapIntervalEXT, Display *, dpy, GLXDrawable, drawable, int,
	interval);


// GLX_EXT_texture_from_pixmap

VFUNCDEF4(glXBindTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, const int *, attrib_list);

VFUNCDEF3(glXReleaseTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer)


// GLX_NV_swap_group

FUNCDEF3(Bool, glXJoinSwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint, group);

FUNCDEF3(Bool, glXBindSwapBarrierNV, Display *, dpy, GLuint, group,
	GLuint, barrier);

FUNCDEF4(Bool, glXQuerySwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint *, group, GLuint *, barrier);

FUNCDEF4(Bool, glXQueryMaxSwapGroupsNV, Display *, dpy, int, screen,
	GLuint *, maxGroups, GLuint *, maxBarriers);

FUNCDEF3(Bool, glXQueryFrameCountNV, Display *, dpy, int, screen,
	GLuint *, count);

FUNCDEF2(Bool, glXResetFrameCountNV, Display *, dpy, int, screen);


// GLX_SGI_swap_control

FUNCDEF1(int, glXSwapIntervalSGI, int, interval);


// GLX_SGIX_fbconfig

FUNCDEF2(GLXFBConfigSGIX, glXGetFBConfigFromVisualSGIX, Display *, dpy,
	XVisualInfo *, vis);


// GLX_SUN_get_transparent_index

FUNCDEF4(int, glXGetTransparentIndexSUN, Display *, dpy, Window, overlay,
	Window, underlay, long *, transparentIndex);


// GL functions

VFUNCDEF0(glFinish);

VFUNCDEF0(glFlush);

VFUNCDEF4(glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height);

VFUNCDEF1(glDrawBuffer, GLenum, drawbuf);

VFUNCDEF0(glPopAttrib);


// X11 functions

FUNCDEF3(Bool, XCheckMaskEvent, Display *, dpy, long, event_mask,
	XEvent *, xe);

FUNCDEF3(Bool, XCheckTypedEvent, Display *, dpy, int, event_type,
	XEvent *, xe);

FUNCDEF4(Bool, XCheckTypedWindowEvent, Display *, dpy, Window, win,
	int, event_type, XEvent *, xe);

FUNCDEF4(Bool, XCheckWindowEvent, Display *, dpy, Window, win, long,
	event_mask, XEvent *, xe);

FUNCDEF1(int, XCloseDisplay, Display *, dpy);

FUNCDEF4(int, XConfigureWindow, Display *, dpy, Window, win,
	unsigned int, value_mask, XWindowChanges *, values);

FUNCDEF10(int, XCopyArea, Display *, dpy, Drawable, src, Drawable, dst, GC, gc,
	int, src_x, int, src_y, unsigned int, w, unsigned int, h, int, dest_x,
	int, dest_y);

FUNCDEF9(Window, XCreateSimpleWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height, unsigned int,
	border_width, unsigned long, border, unsigned long, background);

FUNCDEF12(Window, XCreateWindow, Display *, dpy, Window, parent, int, x, int,
	y, unsigned int, width, unsigned int, height, unsigned int, border_width,
	int, depth, unsigned int, c_class, Visual *, visual, unsigned long,
	value_mask, XSetWindowAttributes *, attributes);

FUNCDEF2(int, XDestroySubwindows, Display *, dpy, Window, win);

FUNCDEF2(int, XDestroyWindow, Display *, dpy, Window, win);

FUNCDEF1(int, XFree, void *, data);

FUNCDEF9(Status, XGetGeometry, Display *, display, Drawable, d, Window *,
	root, int *, x, int *, y, unsigned int *, width, unsigned int *, height,
	unsigned int *, border_width, unsigned int *, depth);

FUNCDEF8(XImage *, XGetImage, Display *, display, Drawable, d, int, x, int, y,
	unsigned int, width, unsigned int, height, unsigned long, plane_mask, int,
	format);

FUNCDEF2(char **, XListExtensions, Display *, dpy, int *, next);

FUNCDEF3(int, XMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe);

FUNCDEF6(int, XMoveResizeWindow, Display *, dpy, Window, win, int, x, int, y,
	unsigned int, width, unsigned int, height);

FUNCDEF2(int, XNextEvent, Display *, dpy, XEvent *, xe);

FUNCDEF1(Display *, XOpenDisplay, _Xconst char*, name);

FUNCDEF5(Bool, XQueryExtension, Display *, dpy, _Xconst char*, name, int *,
	major_opcode, int *, first_event, int *, first_error);

FUNCDEF4(int, XResizeWindow, Display *, dpy, Window, win, unsigned int, width,
	unsigned int, height);

FUNCDEF1(char *, XServerVendor, Display *, dpy);

FUNCDEF4(int, XWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe);


// From dlfaker

typedef	void* (*_dlopenType)(const char *, int);
void *_vgl_dlopen(const char *, int);
SYMDEF(dlopen);


#ifdef FAKEXCB

// XCB functions

FUNCDEF2(const xcb_query_extension_reply_t *, xcb_get_extension_data,
	xcb_connection_t *, conn, xcb_extension_t *, ext);

FUNCDEF3(xcb_glx_query_version_cookie_t, xcb_glx_query_version,
	xcb_connection_t *, conn, uint32_t, major_version, uint32_t, minor_version);

FUNCDEF3(xcb_glx_query_version_reply_t *, xcb_glx_query_version_reply,
	xcb_connection_t *, conn, xcb_glx_query_version_cookie_t, cookie,
	xcb_generic_error_t **, error);

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_event, xcb_connection_t *, conn);

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_queued_event, xcb_connection_t *,
	conn);

FUNCDEF1(xcb_generic_event_t *, xcb_wait_for_event, xcb_connection_t *, conn);

#endif


// Functions used by the faker (but not interposed.)  We load the GLX/OpenGL
// functions dynamically to prevent the 3D application from overriding them, as
// well as to ensure that, with 'vglrun -nodl', libGL is not loaded into the
// process until the 3D application actually uses it.

VFUNCDEF2(glBindBuffer, GLenum, target, GLuint, buffer);

VFUNCDEF7(glBitmap, GLsizei, width, GLsizei, height, GLfloat, xorig,
	GLfloat, yorig, GLfloat, xmove, GLfloat, ymove, const GLubyte *, bitmap);

VFUNCDEF4(glBufferData, GLenum, target, GLsizeiptr, size, const GLvoid *, data,
	GLenum, usage);

VFUNCDEF1(glClear, GLbitfield, mask);

VFUNCDEF4(glClearColor, GLclampf, red, GLclampf, green, GLclampf, blue,
	GLclampf, alpha);

VFUNCDEF3(glColor3d, GLdouble, red, GLdouble, green, GLdouble, blue);

VFUNCDEF1(glColor3dv, const GLdouble *, v);

VFUNCDEF3(glColor3f, GLfloat, red, GLfloat, green, GLfloat, blue);

VFUNCDEF1(glColor3fv, const GLfloat *, v);

VFUNCDEF5(glCopyPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	GLenum, type);

VFUNCDEF0(glEndList);

VFUNCDEF2(glGenBuffers, GLsizei, n, GLuint *, buffers);

VFUNCDEF3(glGetBufferParameteriv, GLenum, target, GLenum, value, GLint *,
	data);

FUNCDEF0(GLenum, glGetError);

VFUNCDEF2(glGetFloatv, GLenum, pname, GLfloat *, params);

VFUNCDEF2(glGetIntegerv, GLenum, pname, GLint *, params);

FUNCDEF1(const GLubyte *, glGetString, GLenum, name);

VFUNCDEF0(glLoadIdentity);

FUNCDEF2(void *, glMapBuffer, GLenum, target, GLenum, access);

VFUNCDEF1(glMatrixMode, GLenum, mode);

VFUNCDEF2(glNewList, GLuint, list, GLenum, mode);

VFUNCDEF6(glOrtho, GLdouble, left, GLdouble, right, GLdouble, bottom,
	GLdouble, top, GLdouble, near_val, GLdouble, far_val);

VFUNCDEF2(glPixelStorei, GLenum, pname, GLint, param);

VFUNCDEF0(glPopClientAttrib);

VFUNCDEF0(glPopMatrix);

VFUNCDEF1(glPushClientAttrib, GLbitfield, mask);

VFUNCDEF0(glPushMatrix);

VFUNCDEF2(glRasterPos2i, GLint, x, GLint, y);

VFUNCDEF1(glReadBuffer, GLenum, mode);

VFUNCDEF7(glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	GLenum, format, GLenum, type, GLvoid*, pixels);

FUNCDEF1(GLboolean, glUnmapBuffer, GLenum, target);

FUNCDEF0(GLXContext, glXGetCurrentContext);

// We load all XCB functions dynamically, so that the same VirtualGL binary
// can be used to support systems with and without XCB libraries.

#ifdef FAKEXCB

FUNCDEF1(xcb_connection_t *, XGetXCBConnection, Display *, dpy);

VFUNCDEF2(XSetEventQueueOwner, Display *, dpy, enum XEventQueueOwner, owner);

typedef xcb_extension_t* _xcb_glx_idType;
SYMDEF(xcb_glx_id);
static inline xcb_extension_t *_xcb_glx_id(void)
{
	CHECKSYM(xcb_glx_id);
	return __xcb_glx_id;
}

FUNCDEF4(xcb_intern_atom_cookie_t, xcb_intern_atom, xcb_connection_t *, conn,
	uint8_t, only_if_exists, uint16_t, name_len, const char *, name);

FUNCDEF3(xcb_intern_atom_reply_t *, xcb_intern_atom_reply, xcb_connection_t *,
	conn, xcb_intern_atom_cookie_t, cookie, xcb_generic_error_t **, e);

FUNCDEF1(xcb_key_symbols_t *, xcb_key_symbols_alloc, xcb_connection_t *, c);

VFUNCDEF1(xcb_key_symbols_free, xcb_key_symbols_t *, syms);

FUNCDEF3(xcb_keysym_t, xcb_key_symbols_get_keysym, xcb_key_symbols_t *, syms,
	xcb_keycode_t, keycode, int, col);

#endif

#ifdef __cplusplus
}
#endif


#endif
