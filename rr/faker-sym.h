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

#ifndef __FAKER_SYM_H__
#define __FAKER_SYM_H__

#include <stdio.h>
#include "glx.h"
#ifdef USEGLP
#include <GL/glp.h>
#endif

extern void safeexit(int);

#define checksym(s) {if(!__##s) { \
	fprintf(stderr, #s" symbol not loaded\n");  fflush(stderr);  safeexit(1);}}

#ifdef __LOCALSYM__
#define symdef(f) _##f##Type __##f=NULL
#else
#define symdef(f) extern _##f##Type __##f
#endif

#define funcdef0(rettype, f, ret) \
	typedef rettype (*_##f##Type)(void); \
	symdef(f); \
	static inline rettype _##f(void) {checksym(f);  ret __##f();}

#define funcdef1(rettype, f, at1, a1, ret) \
	typedef rettype (*_##f##Type)(at1); \
	symdef(f); \
	static inline rettype _##f(at1 a1) {checksym(f);  ret __##f(a1);}

#define funcdef2(rettype, f, at1, a1, at2, a2, ret) \
	typedef rettype (*_##f##Type)(at1, at2); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2) {checksym(f);  ret __##f(a1, a2);}

#define funcdef3(rettype, f, at1, a1, at2, a2, at3, a3, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3) {checksym(f);  ret __##f(a1, a2, a3);}

#define funcdef4(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4) {checksym(f);  ret __##f(a1, a2, a3, a4);}

#define funcdef5(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5) {checksym(f);  ret __##f(a1, a2, a3, a4, a5);}

#define funcdef6(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6) {checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6);}

#define funcdef7(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, at7, a7, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, at7 a7) {checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7);}

#define funcdef9(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, at7, a7, at8, a8, at9, a9, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, at7 a7, at8 a8, at9 a9) {checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9);}

#define funcdef10(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, at10); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, at7 a7, at8 a8, at9 a9, at10 a10) {checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);}

#define funcdef12(rettype, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, at11, a11, at12, a12, ret) \
	typedef rettype (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, at10, at11, at12); \
	symdef(f); \
	static inline rettype _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, at7 a7, at8 a8, at9 a9, at10 a10, at11 a11, at12 a12) {checksym(f);  ret __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);}


#ifdef __cplusplus
extern "C" {
#endif


// GLX 1.0 functions

funcdef3(XVisualInfo*, glXChooseVisual, Display *, dpy, int, screen,
	int *, attrib_list, return);

funcdef4(void, glXCopyContext, Display *, dpy, GLXContext, src, GLXContext, dst,
	unsigned long, mask,);

funcdef4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct, return);

funcdef3(GLXPixmap, glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
	Pixmap, pixmap, return);

funcdef2(void, glXDestroyContext, Display *, dpy, GLXContext, ctx,);

funcdef2(void, glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pix,);

funcdef0(GLXContext, glXGetCurrentContext, return);

funcdef0(GLXDrawable, glXGetCurrentDrawable, return);

funcdef2(Bool, glXIsDirect, Display *, dpy, GLXContext, ctx, return);

funcdef3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx, return);

funcdef3(Bool, glXQueryExtension, Display *, dpy, int *, error_base,
	int *, event_base, return);

funcdef3(Bool, glXQueryVersion, Display *, dpy, int *, major, int *, minor, return);

funcdef2(void, glXSwapBuffers, Display *, dpy, GLXDrawable, drawable,);

funcdef4(void, glXUseXFont, Font, font, int, first, int, count, int, list_base,);


// GLX 1.1 functions

funcdef2(const char *, glXGetClientString, Display *, dpy, int, name, return);

funcdef3(const char *, glXQueryServerString, Display *, dpy, int, screen,
	int, name, return);

funcdef2(const char *, glXQueryExtensionsString, Display *, dpy, int, screen, return);


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


// SGIX_fbconfig

funcdef4(int, glXGetFBConfigAttribSGIX, Display *, dpy, GLXFBConfigSGIX, config,
	int, attribute, int *, value_return, return);

funcdef4(GLXFBConfigSGIX *, glXChooseFBConfigSGIX, Display *, dpy, int, screen,
	const int *, attrib_list, int *, nelements, return);


// SGIX_pbuffer

funcdef5(GLXPbuffer, glXCreateGLXPbufferSGIX, Display *, dpy,
	GLXFBConfig, config, unsigned int, width, unsigned int, height,
	const int *, attrib_list, return);

funcdef2(void, glXDestroyGLXPbufferSGIX, Display *, dpy, GLXPbuffer, pbuf,);

funcdef4(void, glXQueryGLXPbufferSGIX, Display *, dpy, GLXPbuffer, pbuf,
	int, attribute, unsigned int *, value,);

funcdef3(void, glXSelectEventSGIX, Display *, dpy, GLXDrawable, drawable,
	unsigned long, mask,);

funcdef3(void, glXGetSelectedEventSGIX, Display *, dpy, GLXDrawable, drawable,
	unsigned long *, mask,);


// SUN-specific

#ifdef sun
funcdef3(int, glXVideoResizeSUN, Display *, display, GLXDrawable, window,
	float, factor, return);

funcdef3(int, glXGetVideoResizeSUN, Display *,display, GLXDrawable, window,
	float *, factor, return);

funcdef1(int, glXDisableXineramaSUN, Display *, dpy, return);
#endif


// GL functions

funcdef0(void, glFinish,);

funcdef0(void, glFlush,);

funcdef4(void, glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height,);


// X11 functions

funcdef3(Bool, XCheckMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe, return);

funcdef3(Bool, XCheckTypedEvent, Display *, dpy, int, event_type, XEvent *, xe, return);

funcdef4(Bool, XCheckTypedWindowEvent, Display *, dpy, Window, win,
	int, event_type, XEvent *, xe, return);

funcdef4(Bool, XCheckWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe, return);

funcdef1(int, XCloseDisplay, Display *, dpy, return);

funcdef4(int, XConfigureWindow, Display *, dpy, Window, win,
	unsigned int, value_mask, XWindowChanges *, values, return);

funcdef10(int, XCopyArea, Display *, dpy, Drawable, src, Drawable, dst, GC, gc,
	int, src_x, int, src_y, unsigned int, w, unsigned int, h, int, dest_x,
	int, dest_y, return);

funcdef9(Window, XCreateSimpleWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height, unsigned int, border_width,
	unsigned long, border, unsigned long, background, return);

funcdef12(Window, XCreateWindow, Display *, dpy, Window, parent, int, x, int, y,
	unsigned int, width, unsigned int, height, unsigned int, border_width,
	int, depth, unsigned int, c_class, Visual *, visual, unsigned long, value_mask,
	XSetWindowAttributes *, attributes, return);

funcdef2(int, XDestroyWindow, Display *, dpy, Window, win, return);

funcdef3(int, XMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe, return);

funcdef6(int, XMoveResizeWindow, Display *, dpy, Window, win, int, x, int, y,
	unsigned int, width, unsigned int, height, return);

funcdef2(int, XNextEvent, Display *, dpy, XEvent *, xe, return);

funcdef1(Display *, XOpenDisplay, _Xconst char*, name, return);

funcdef4(int, XResizeWindow, Display *, dpy, Window, win, unsigned int, width,
	unsigned int, height, return);

funcdef4(int, XWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe, return);


#ifdef __cplusplus
}
#endif

void loadsymbols(void);

#endif
