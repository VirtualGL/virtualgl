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


namespace vglfaker
{
	extern void safeExit(int);
	extern void init(void);
}


#define checksym(s) {  \
	if(!__##s) {  \
		vglfaker::init();  \
		if(!__##s) {  \
			vglout.PRINT("[VGL] ERROR: "#s" symbol not loaded\n");  \
			vglfaker::safeExit(1);  \
		}  \
	}  \
}

#ifdef __LOCALSYM__
#define symdef(f) _##f##Type __##f=NULL
#else
#define symdef(f) extern _##f##Type __##f
#endif


#define funcdef0(rettype, f, ret) \
	typedef rettype (*_##f##Type)(void); \
	symdef(f); \
	static inline rettype _##f(void) { \
		checksym(f);  ret __##f(); \
	}

#define funcdef1(rettype, f, at1, a1, ret) \
	typedef rettype (*_##f##Type)(at1); \
	symdef(f); \
	static inline rettype _##f(at1 a1) { \
		checksym(f);  ret __##f(a1); \
	}

#define funcdef2(rettype, f, at1, a1, at2, a2, ret) \
	typedef rettype (*_##f##Type)(at1, at2); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2) { \
		checksym(f);  ret __##f(a1, a2); \
	}

#define funcdef3(rettype, f, at1, a1, at2, a2, at3, a3, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3) { \
		checksym(f);  ret __##f(a1, a2, a3); \
	}

#define funcdef4(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4) { \
		checksym(f);  ret __##f(a1, a2, a3, a4); \
	}

#define funcdef5(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) { \
		checksym(f);  ret __##f(a1, a2, a3, a4, a5); \
	}

#define funcdef6(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, \
		at6 a6) { \
		checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6); \
	}

#define funcdef7(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7) { \
		checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7); \
	}

#define funcdef8(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8) { \
		checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8); \
	}

#define funcdef9(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9) { \
		checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9); \
	}

#define funcdef10(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10) { \
		checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); \
	}

#define funcdef12(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, at11, a11, at12, a12, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10, at11, at12); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10, at11 a11, at12 a12) { \
		checksym(f); \
		ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12); \
	}


#ifdef __cplusplus
extern "C" {
#endif


// GLX 1.0 functions

funcdef3(XVisualInfo*, glXChooseVisual, Display *, dpy, int, screen,
	int *, attrib_list, return);

funcdef4(void, glXCopyContext, Display *, dpy, GLXContext, src, GLXContext,
	dst, unsigned long, mask,);

funcdef4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct, return);

funcdef3(GLXPixmap, glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
	Pixmap, pixmap, return);

funcdef2(void, glXDestroyContext, Display *, dpy, GLXContext, ctx,);

funcdef2(void, glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pix,);

funcdef4(int, glXGetConfig, Display *, dpy, XVisualInfo *, vis, int, attrib,
	int *, value, return);

funcdef0(GLXDrawable, glXGetCurrentDrawable, return);

funcdef2(Bool, glXIsDirect, Display *, dpy, GLXContext, ctx, return);

funcdef3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx, return);

funcdef3(Bool, glXQueryExtension, Display *, dpy, int *, error_base,
	int *, event_base, return);

funcdef3(Bool, glXQueryVersion, Display *, dpy, int *, major, int *, minor,
	return);

funcdef2(void, glXSwapBuffers, Display *, dpy, GLXDrawable, drawable,);

funcdef4(void, glXUseXFont, Font, font, int, first, int, count, int,
	list_base,);

funcdef0(void, glXWaitGL,);


// GLX 1.1 functions

funcdef2(const char *, glXGetClientString, Display *, dpy, int, name, return);

funcdef3(const char *, glXQueryServerString, Display *, dpy, int, screen, int,
	name, return);

funcdef2(const char *, glXQueryExtensionsString, Display *, dpy, int, screen,
	return);


// GLX 1.3 functions

funcdef4(GLXFBConfig *, glXChooseFBConfig, Display *, dpy, int, screen,
	const int *, attrib_list, int *, nelements, return);

funcdef5(GLXContext, glXCreateNewContext, Display *, dpy, GLXFBConfig, config,
	int, render_type, GLXContext, share_list, Bool, direct, return);

funcdef3(GLXPbuffer, glXCreatePbuffer, Display *, dpy, GLXFBConfig, config,
	const int *, attrib_list, return);

funcdef4(GLXPixmap, glXCreatePixmap, Display *, dpy, GLXFBConfig, config,
	Pixmap, pixmap, const int *, attrib_list, return);

funcdef4(GLXWindow, glXCreateWindow, Display *, dpy, GLXFBConfig, config,
	Window, win, const int *, attrib_list, return);

funcdef2(void, glXDestroyPbuffer, Display *, dpy, GLXPbuffer, pbuf,);

funcdef2(void, glXDestroyPixmap, Display *, dpy, GLXPixmap, pixmap,);

funcdef2(void, glXDestroyWindow, Display *, dpy, GLXWindow, win,);

funcdef0(GLXDrawable, glXGetCurrentReadDrawable, return);

funcdef0(Display*, glXGetCurrentDisplay, return);

funcdef4(int, glXGetFBConfigAttrib, Display *, dpy, GLXFBConfig, config,
	int, attribute, int *, value, return);

funcdef3(GLXFBConfig *, glXGetFBConfigs, Display *, dpy, int, screen,
	int *, nelements, return);

funcdef3(void, glXGetSelectedEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long *, event_mask,);

funcdef2(XVisualInfo *, glXGetVisualFromFBConfig, Display *, dpy,
	GLXFBConfig, config, return);

funcdef4(Bool, glXMakeContextCurrent, Display *, display, GLXDrawable, draw,
	GLXDrawable, read, GLXContext, ctx, return);

funcdef4(int, glXQueryContext, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value, return);

funcdef4(void, glXQueryDrawable, Display *, dpy, GLXDrawable, draw,
	int, attribute, unsigned int *, value,);

funcdef3(void, glXSelectEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long, event_mask,);


// EXT_import_context

funcdef2(void, glXFreeContextEXT, Display *, dpy, GLXContext, ctx,);

funcdef2(GLXContext, glXImportContextEXT, Display *, dpy,
	GLXContextID, contextID, return);

funcdef4(int, glXQueryContextInfoEXT, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value, return);


// NV_swap_group

funcdef3(Bool, glXJoinSwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint, group, return);

funcdef3(Bool, glXBindSwapBarrierNV, Display *, dpy, GLuint, group,
	GLuint, barrier, return);

funcdef4(Bool, glXQuerySwapGroupNV, Display *, dpy, GLXDrawable, drawable,
	GLuint *, group, GLuint *, barrier, return);

funcdef4(Bool, glXQueryMaxSwapGroupsNV, Display *, dpy, int, screen,
	GLuint *, maxGroups, GLuint *, maxBarriers, return);

funcdef3(Bool, glXQueryFrameCountNV, Display *, dpy, int, screen,
	GLuint *, count, return);

funcdef2(Bool, glXResetFrameCountNV, Display *, dpy, int, screen, return);


// GLX_ARB_get_proc_address

typedef void (*(*_glXGetProcAddressARBType)(const GLubyte*))(void);
symdef(glXGetProcAddressARB);
static inline void (*_glXGetProcAddressARB(const GLubyte *procName))(void)
{
	checksym(glXGetProcAddressARB);  return __glXGetProcAddressARB(procName);
}

typedef void (*(*_glXGetProcAddressType)(const GLubyte*))(void);
symdef(glXGetProcAddress);
static inline void (*_glXGetProcAddress(const GLubyte *procName))(void)
{
	checksym(glXGetProcAddress);  return __glXGetProcAddress(procName);
}


// GLX_ARB_create_context

funcdef5(GLXContext, glXCreateContextAttribsARB, Display *, dpy, GLXFBConfig,
	config, GLXContext, share_context, Bool, direct, const int *, attribs,
	return);


// GLX_EXT_texture_from_pixmap

funcdef4(void, glXBindTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, const int *, attrib_list,);

funcdef3(void, glXReleaseTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer,)


// GLX_EXT_swap_control

funcdef3(void, glXSwapIntervalEXT, Display *, dpy, GLXDrawable, drawable, int,
	interval,);


// GLX_SGI_swap_control

funcdef1(int, glXSwapIntervalSGI, int, interval, return);


// GL functions

funcdef0(void, glFinish,);

funcdef0(void, glFlush,);

funcdef4(void, glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei,
	height,);

funcdef1(void, glDrawBuffer, GLenum, drawbuf,);

funcdef0(void, glPopAttrib,);

funcdef7(void, glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei,
	height, GLenum, format, GLenum, type, GLvoid*, pixels,);

funcdef5(void, glDrawPixels, GLsizei, width, GLsizei, height, GLenum, format,
	GLenum, type, const GLvoid*, pixels,);

funcdef1(void, glIndexd, GLdouble, c,);

funcdef1(void, glIndexf, GLfloat, c,);

funcdef1(void, glIndexi, GLint, c,);

funcdef1(void, glIndexs, GLshort, c,);

funcdef1(void, glIndexub, GLubyte, c,);

funcdef1(void, glIndexdv, const GLdouble*, c,);

funcdef1(void, glIndexfv, const GLfloat*, c,);

funcdef1(void, glIndexiv, const GLint*, c,);

funcdef1(void, glIndexsv, const GLshort*, c,);

funcdef1(void, glIndexubv, const GLubyte*, c,);

funcdef1(void, glClearIndex, GLfloat, c,);

funcdef2(void, glGetDoublev, GLenum, pname, GLdouble *, params,);

funcdef2(void, glGetFloatv, GLenum, pname, GLfloat *, params,);

funcdef2(void, glGetIntegerv, GLenum, pname, GLint *, params,);

funcdef3(void, glMaterialfv, GLenum, face, GLenum, pname, const GLfloat *,
	params,);

funcdef3(void, glMaterialiv, GLenum, face, GLenum, pname, const GLint *,
	params,);

funcdef2(void, glPixelTransferf, GLenum, pname, GLfloat, param,);

funcdef2(void, glPixelTransferi, GLenum, pname, GLint, param,);


// X11 functions

funcdef3(Bool, XCheckMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	return);

funcdef3(Bool, XCheckTypedEvent, Display *, dpy, int, event_type, XEvent *, xe,
	return);

funcdef4(Bool, XCheckTypedWindowEvent, Display *, dpy, Window, win,
	int, event_type, XEvent *, xe, return);

funcdef4(Bool, XCheckWindowEvent, Display *, dpy, Window, win, long,
	event_mask, XEvent *, xe, return);

funcdef1(int, XCloseDisplay, Display *, dpy, return);

funcdef4(int, XConfigureWindow, Display *, dpy, Window, win,
	unsigned int, value_mask, XWindowChanges *, values, return);

funcdef10(int, XCopyArea, Display *, dpy, Drawable, src, Drawable, dst, GC, gc,
	int, src_x, int, src_y, unsigned int, w, unsigned int, h, int, dest_x,
	int, dest_y, return);

funcdef9(Window, XCreateSimpleWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height, unsigned int,
	border_width, unsigned long, border, unsigned long, background, return);

funcdef12(Window, XCreateWindow, Display *, dpy, Window, parent, int, x, int,
	y, unsigned int, width, unsigned int, height, unsigned int, border_width,
	int, depth, unsigned int, c_class, Visual *, visual, unsigned long,
	value_mask, XSetWindowAttributes *, attributes, return);

funcdef2(int, XDestroySubwindows, Display *, dpy, Window, win, return);

funcdef2(int, XDestroyWindow, Display *, dpy, Window, win, return);

funcdef1(int, XFree, void *, data, return);

funcdef9(Status, XGetGeometry, Display *, display, Drawable, d, Window *,
	root, int *, x, int *, y, unsigned int *, width, unsigned int *, height,
	unsigned int *, border_width, unsigned int *, depth, return);

funcdef8(XImage *, XGetImage, Display *, display, Drawable, d, int, x, int, y,
	unsigned int, width, unsigned int, height, unsigned long, plane_mask, int,
	format, return);

funcdef2(char **, XListExtensions, Display *, dpy, int *, next, return);

funcdef3(int, XMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	return);

funcdef6(int, XMoveResizeWindow, Display *, dpy, Window, win, int, x, int, y,
	unsigned int, width, unsigned int, height, return);

funcdef2(int, XNextEvent, Display *, dpy, XEvent *, xe, return);

funcdef1(Display *, XOpenDisplay, _Xconst char*, name, return);

funcdef5(Bool, XQueryExtension, Display *, dpy, _Xconst char*, name, int *,
	major_opcode, int *, first_event, int *, first_error, return);

funcdef4(int, XResizeWindow, Display *, dpy, Window, win, unsigned int, width,
	unsigned int, height, return);

funcdef1(char *, XServerVendor, Display *, dpy, return);

funcdef4(int, XWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe, return);


// From dlfaker

typedef	void* (*_dlopenType)(const char *, int);
void *_vgl_dlopen(const char *, int);
symdef(dlopen);


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
