/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011, 2013-2014 D. R. Commander
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
#ifdef FAKEXCB
extern "C" {
#include <xcb/glx.h>
}
#endif


namespace vglfaker
{
	extern void safeExit(int);
	extern void init(void);
}


#define CHECKSYM(s) {  \
	if(!__##s) {  \
		vglfaker::init();  \
		if(!__##s) {  \
			vglout.PRINT("[VGL] ERROR: "#s" symbol not loaded\n");  \
			vglfaker::safeExit(1);  \
		}  \
	}  \
}

#ifdef __LOCALSYM__
#define SYMDEF(f) _##f##Type __##f=NULL
#else
#define SYMDEF(f) extern _##f##Type __##f
#endif


#define FUNCDEF0(RetType, f, ret) \
	typedef RetType (*_##f##Type)(void); \
	SYMDEF(f); \
	static inline RetType _##f(void) { \
		CHECKSYM(f);  ret __##f(); \
	}

#define FUNCDEF1(RetType, f, at1, a1, ret) \
	typedef RetType (*_##f##Type)(at1); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1) { \
		CHECKSYM(f);  ret __##f(a1); \
	}

#define FUNCDEF2(RetType, f, at1, a1, at2, a2, ret) \
	typedef RetType (*_##f##Type)(at1, at2); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2) { \
		CHECKSYM(f);  ret __##f(a1, a2); \
	}

#define FUNCDEF3(RetType, f, at1, a1, at2, a2, at3, a3, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3); \
	}

#define FUNCDEF4(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4); \
	}

#define FUNCDEF5(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4, a5); \
	}

#define FUNCDEF6(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, \
		at6 a6) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4, a5, a6); \
	}

#define FUNCDEF7(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7); \
	}

#define FUNCDEF8(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8); \
	}

#define FUNCDEF9(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9); \
	}

#define FUNCDEF10(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10) { \
		CHECKSYM(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); \
	}

#define FUNCDEF12(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, at11, a11, at12, a12, ret) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10, at11, at12); \
	SYMDEF(f); \
	static inline RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10, at11 a11, at12 a12) { \
		CHECKSYM(f); \
		ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); \
	}


#ifdef __cplusplus
extern "C" {
#endif


// GLX 1.0 functions

FUNCDEF3(XVisualInfo*, glXChooseVisual, Display *, dpy, int, screen,
	int *, attrib_list, return);

FUNCDEF4(void, glXCopyContext, Display *, dpy, GLXContext, src, GLXContext,
	dst, unsigned long, mask,);

FUNCDEF4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct, return);

FUNCDEF3(GLXPixmap, glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
	Pixmap, pixmap, return);

FUNCDEF2(void, glXDestroyContext, Display *, dpy, GLXContext, ctx,);

FUNCDEF2(void, glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pix,);

FUNCDEF4(int, glXGetConfig, Display *, dpy, XVisualInfo *, vis, int, attrib,
	int *, value, return);

FUNCDEF0(GLXDrawable, glXGetCurrentDrawable, return);

FUNCDEF2(Bool, glXIsDirect, Display *, dpy, GLXContext, ctx, return);

FUNCDEF3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx, return);

FUNCDEF3(Bool, glXQueryExtension, Display *, dpy, int *, error_base,
	int *, event_base, return);

FUNCDEF3(Bool, glXQueryVersion, Display *, dpy, int *, major, int *, minor,
	return);

FUNCDEF2(void, glXSwapBuffers, Display *, dpy, GLXDrawable, drawable,);

FUNCDEF4(void, glXUseXFont, Font, font, int, first, int, count, int,
	list_base,);

FUNCDEF0(void, glXWaitGL,);


// GLX 1.1 functions

FUNCDEF2(const char *, glXGetClientString, Display *, dpy, int, name, return);

FUNCDEF3(const char *, glXQueryServerString, Display *, dpy, int, screen, int,
	name, return);

FUNCDEF2(const char *, glXQueryExtensionsString, Display *, dpy, int, screen,
	return);


// GLX 1.3 functions

FUNCDEF4(GLXFBConfig *, glXChooseFBConfig, Display *, dpy, int, screen,
	const int *, attrib_list, int *, nelements, return);

FUNCDEF5(GLXContext, glXCreateNewContext, Display *, dpy, GLXFBConfig, config,
	int, render_type, GLXContext, share_list, Bool, direct, return);

FUNCDEF3(GLXPbuffer, glXCreatePbuffer, Display *, dpy, GLXFBConfig, config,
	const int *, attrib_list, return);

FUNCDEF4(GLXPixmap, glXCreatePixmap, Display *, dpy, GLXFBConfig, config,
	Pixmap, pixmap, const int *, attrib_list, return);

FUNCDEF4(GLXWindow, glXCreateWindow, Display *, dpy, GLXFBConfig, config,
	Window, win, const int *, attrib_list, return);

FUNCDEF2(void, glXDestroyPbuffer, Display *, dpy, GLXPbuffer, pbuf,);

FUNCDEF2(void, glXDestroyPixmap, Display *, dpy, GLXPixmap, pixmap,);

FUNCDEF2(void, glXDestroyWindow, Display *, dpy, GLXWindow, win,);

FUNCDEF0(GLXDrawable, glXGetCurrentReadDrawable, return);

FUNCDEF0(Display*, glXGetCurrentDisplay, return);

FUNCDEF4(int, glXGetFBConfigAttrib, Display *, dpy, GLXFBConfig, config,
	int, attribute, int *, value, return);

FUNCDEF3(GLXFBConfig *, glXGetFBConfigs, Display *, dpy, int, screen,
	int *, nelements, return);

FUNCDEF3(void, glXGetSelectedEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long *, event_mask,);

FUNCDEF2(XVisualInfo *, glXGetVisualFromFBConfig, Display *, dpy,
	GLXFBConfig, config, return);

FUNCDEF4(Bool, glXMakeContextCurrent, Display *, display, GLXDrawable, draw,
	GLXDrawable, read, GLXContext, ctx, return);

FUNCDEF4(int, glXQueryContext, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value, return);

FUNCDEF4(void, glXQueryDrawable, Display *, dpy, GLXDrawable, draw,
	int, attribute, unsigned int *, value,);

FUNCDEF3(void, glXSelectEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long, event_mask,);


// EXT_import_context

FUNCDEF2(void, glXFreeContextEXT, Display *, dpy, GLXContext, ctx,);

FUNCDEF2(GLXContext, glXImportContextEXT, Display *, dpy,
	GLXContextID, contextID, return);

FUNCDEF4(int, glXQueryContextInfoEXT, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value, return);


// NV_swap_group

FUNCDEF3(Bool, glXJoinSwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint, group, return);

FUNCDEF3(Bool, glXBindSwapBarrierNV, Display *, dpy, GLuint, group,
	GLuint, barrier, return);

FUNCDEF4(Bool, glXQuerySwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint *, group, GLuint *, barrier, return);

FUNCDEF4(Bool, glXQueryMaxSwapGroupsNV, Display *, dpy, int, screen,
	GLuint *, maxGroups, GLuint *, maxBarriers, return);

FUNCDEF3(Bool, glXQueryFrameCountNV, Display *, dpy, int, screen,
	GLuint *, count, return);

FUNCDEF2(Bool, glXResetFrameCountNV, Display *, dpy, int, screen, return);


// GLX_ARB_get_proc_address

typedef void (*(*_glXGetProcAddressARBType)(const GLubyte*))(void);
SYMDEF(glXGetProcAddressARB);
static inline void (*_glXGetProcAddressARB(const GLubyte *procName))(void)
{
	CHECKSYM(glXGetProcAddressARB);  return __glXGetProcAddressARB(procName);
}

typedef void (*(*_glXGetProcAddressType)(const GLubyte*))(void);
SYMDEF(glXGetProcAddress);
static inline void (*_glXGetProcAddress(const GLubyte *procName))(void)
{
	CHECKSYM(glXGetProcAddress);  return __glXGetProcAddress(procName);
}


// GLX_ARB_create_context

FUNCDEF5(GLXContext, glXCreateContextAttribsARB, Display *, dpy, GLXFBConfig,
	config, GLXContext, share_context, Bool, direct, const int *, attribs,
	return);


// GLX_EXT_texture_from_pixmap

FUNCDEF4(void, glXBindTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, const int *, attrib_list,);

FUNCDEF3(void, glXReleaseTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer,)


// GLX_EXT_swap_control

FUNCDEF3(void, glXSwapIntervalEXT, Display *, dpy, GLXDrawable, drawable, int,
	interval,);


// GLX_SGI_swap_control

FUNCDEF1(int, glXSwapIntervalSGI, int, interval, return);


// GL functions

FUNCDEF0(void, glFinish,);

FUNCDEF0(void, glFlush,);

FUNCDEF4(void, glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei,
	height,);

FUNCDEF1(void, glDrawBuffer, GLenum, drawbuf,);

FUNCDEF0(void, glPopAttrib,);

FUNCDEF7(void, glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei,
	height, GLenum, format, GLenum, type, GLvoid*, pixels,);

FUNCDEF5(void, glDrawPixels, GLsizei, width, GLsizei, height, GLenum, format,
	GLenum, type, const GLvoid*, pixels,);

FUNCDEF1(void, glIndexd, GLdouble, c,);

FUNCDEF1(void, glIndexf, GLfloat, c,);

FUNCDEF1(void, glIndexi, GLint, c,);

FUNCDEF1(void, glIndexs, GLshort, c,);

FUNCDEF1(void, glIndexub, GLubyte, c,);

FUNCDEF1(void, glIndexdv, const GLdouble*, c,);

FUNCDEF1(void, glIndexfv, const GLfloat*, c,);

FUNCDEF1(void, glIndexiv, const GLint*, c,);

FUNCDEF1(void, glIndexsv, const GLshort*, c,);

FUNCDEF1(void, glIndexubv, const GLubyte*, c,);

FUNCDEF1(void, glClearIndex, GLfloat, c,);

FUNCDEF2(void, glGetDoublev, GLenum, pname, GLdouble *, params,);

FUNCDEF2(void, glGetFloatv, GLenum, pname, GLfloat *, params,);

FUNCDEF2(void, glGetIntegerv, GLenum, pname, GLint *, params,);

FUNCDEF3(void, glMaterialfv, GLenum, face, GLenum, pname, const GLfloat *,
	params,);

FUNCDEF3(void, glMaterialiv, GLenum, face, GLenum, pname, const GLint *,
	params,);

FUNCDEF2(void, glPixelTransferf, GLenum, pname, GLfloat, param,);

FUNCDEF2(void, glPixelTransferi, GLenum, pname, GLint, param,);


// X11 functions

FUNCDEF3(Bool, XCheckMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	return);

FUNCDEF3(Bool, XCheckTypedEvent, Display *, dpy, int, event_type, XEvent *, xe,
	return);

FUNCDEF4(Bool, XCheckTypedWindowEvent, Display *, dpy, Window, win,
	int, event_type, XEvent *, xe, return);

FUNCDEF4(Bool, XCheckWindowEvent, Display *, dpy, Window, win, long,
	event_mask, XEvent *, xe, return);

FUNCDEF1(int, XCloseDisplay, Display *, dpy, return);

FUNCDEF4(int, XConfigureWindow, Display *, dpy, Window, win,
	unsigned int, value_mask, XWindowChanges *, values, return);

FUNCDEF10(int, XCopyArea, Display *, dpy, Drawable, src, Drawable, dst, GC, gc,
	int, src_x, int, src_y, unsigned int, w, unsigned int, h, int, dest_x,
	int, dest_y, return);

FUNCDEF9(Window, XCreateSimpleWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height, unsigned int,
	border_width, unsigned long, border, unsigned long, background, return);

FUNCDEF12(Window, XCreateWindow, Display *, dpy, Window, parent, int, x, int,
	y, unsigned int, width, unsigned int, height, unsigned int, border_width,
	int, depth, unsigned int, c_class, Visual *, visual, unsigned long,
	value_mask, XSetWindowAttributes *, attributes, return);

FUNCDEF2(int, XDestroySubwindows, Display *, dpy, Window, win, return);

FUNCDEF2(int, XDestroyWindow, Display *, dpy, Window, win, return);

FUNCDEF1(int, XFree, void *, data, return);

FUNCDEF9(Status, XGetGeometry, Display *, display, Drawable, d, Window *,
	root, int *, x, int *, y, unsigned int *, width, unsigned int *, height,
	unsigned int *, border_width, unsigned int *, depth, return);

FUNCDEF8(XImage *, XGetImage, Display *, display, Drawable, d, int, x, int, y,
	unsigned int, width, unsigned int, height, unsigned long, plane_mask, int,
	format, return);

FUNCDEF2(char **, XListExtensions, Display *, dpy, int *, next, return);

FUNCDEF3(int, XMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	return);

FUNCDEF6(int, XMoveResizeWindow, Display *, dpy, Window, win, int, x, int, y,
	unsigned int, width, unsigned int, height, return);

FUNCDEF2(int, XNextEvent, Display *, dpy, XEvent *, xe, return);

FUNCDEF1(Display *, XOpenDisplay, _Xconst char*, name, return);

FUNCDEF5(Bool, XQueryExtension, Display *, dpy, _Xconst char*, name, int *,
	major_opcode, int *, first_event, int *, first_error, return);

FUNCDEF4(int, XResizeWindow, Display *, dpy, Window, win, unsigned int, width,
	unsigned int, height, return);

FUNCDEF1(char *, XServerVendor, Display *, dpy, return);

FUNCDEF4(int, XWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe, return);


// From dlfaker

typedef	void* (*_dlopenType)(const char *, int);
void *_vgl_dlopen(const char *, int);
SYMDEF(dlopen);


#ifdef FAKEXCB

// XCB functions

FUNCDEF2(const xcb_query_extension_reply_t *, xcb_get_extension_data,
	xcb_connection_t *, conn, xcb_extension_t *, ext, return);

FUNCDEF3(xcb_glx_query_version_cookie_t, xcb_glx_query_version,
	xcb_connection_t *, conn, uint32_t, major_version, uint32_t, minor_version,
	return);

FUNCDEF3(xcb_glx_query_version_reply_t *, xcb_glx_query_version_reply,
	xcb_connection_t *, conn, xcb_glx_query_version_cookie_t, cookie,
	xcb_generic_error_t **, error, return);

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_event, xcb_connection_t *, conn,
	return);

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_queued_event, xcb_connection_t *,
	conn, return);

FUNCDEF1(xcb_generic_event_t *, xcb_wait_for_event, xcb_connection_t *, conn,
	return);

#endif


#ifdef __cplusplus
}
#endif


namespace vglfaker
{
	void loadSymbols(void);
	void loadDLSymbols(void);
	void unloadSymbols(void);
}

#endif
