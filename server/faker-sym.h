// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011, 2013-2016, 2018-2022 D. R. Commander
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
#include <GL/glx.h>
#ifdef EGLBACKEND
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#endif
#ifdef FAKEOPENCL
#include <CL/opencl.h>
#endif
#include "Log.h"
#include "GlobalCriticalSection.h"
#include "vglinline.h"
#ifdef FAKEXCB
extern "C" {
	#include <xcb/xcb.h>
	#include <xcb/xcbext.h>
	#include <xcb/xcb_keysyms.h>
	#include <xcb/glx.h>
	#include <X11/Xlib-xcb.h>
}
#endif
#include "faker.h"
#include <X11/XKBlib.h>
#ifdef USEHELGRIND
	#include <valgrind/helgrind.h>
#else
	#define ANNOTATE_BENIGN_RACE_SIZED(a, b, c) {}
#endif


#define CHECKSYM_NONFATAL(s) \
{ \
	ANNOTATE_BENIGN_RACE_SIZED(&__##s, sizeof(_##s##Type), ); \
	if(!__##s) \
	{ \
		faker::init(); \
		faker::GlobalCriticalSection::SafeLock l(globalMutex); \
		if(!__##s) __##s = (_##s##Type)faker::loadSymbol(#s, true); \
	} \
}

#define CHECKSYM(s, fake_s) \
{ \
	ANNOTATE_BENIGN_RACE_SIZED(&__##s, sizeof(_##s##Type), ); \
	if(!__##s) \
	{ \
		faker::init(); \
		faker::GlobalCriticalSection::SafeLock l(globalMutex); \
		if(!__##s) __##s = (_##s##Type)faker::loadSymbol(#s); \
	} \
	if(!__##s) faker::safeExit(1); \
	if(__##s == fake_s) \
	{ \
		vglout.print("[VGL] ERROR: VirtualGL attempted to load the real\n"); \
		vglout.print("[VGL]   " #s " function and got the fake one instead.\n"); \
		vglout.print("[VGL]   Something is terribly wrong.  Aborting before chaos ensues.\n"); \
		faker::safeExit(1); \
	} \
}

#ifdef __LOCALSYM__
#define SYMDEF(f)  _##f##Type __##f = NULL
#else
#define SYMDEF(f)  extern _##f##Type __##f
#endif


#define DISABLE_FAKER()  faker::setFakerLevel(faker::getFakerLevel() + 1);
#define ENABLE_FAKER()  faker::setFakerLevel(faker::getFakerLevel() - 1);


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

#define VFUNCDEF10(f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, at6, a6, \
	at7, a7, at8, a8, at9, a9, at10, a10, fake_f) \
	typedef void (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10); \
	SYMDEF(f); \
	static INLINE void _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10) \
	{ \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		__##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); \
		ENABLE_FAKER(); \
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

#define FUNCDEF13(RetType, f, at1, a1, at2, a2, at3, a3, at4, a4, at5, a5, \
	at6, a6, at7, a7, at8, a8, at9, a9, at10, a10, at11, a11, at12, a12, \
	at13, a13, fake_f) \
	typedef RetType (*_##f##Type)(at1, at2, at3, at4, at5, at6, at7, at8, at9, \
		at10, at11, at12, at13); \
	SYMDEF(f); \
	static INLINE RetType _##f(at1 a1, at2 a2, at3 a3, at4 a4, at5 a5, at6 a6, \
		at7 a7, at8 a8, at9 a9, at10 a10, at11 a11, at12 a12, at13 a13) \
	{ \
		RetType retval; \
		CHECKSYM(f, fake_f); \
		DISABLE_FAKER(); \
		retval = __##f(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13); \
		ENABLE_FAKER(); \
		return retval; \
	}


#ifdef __cplusplus
extern "C" {
#endif


// GLX 1.0 functions

FUNCDEF3(XVisualInfo *, glXChooseVisual, Display *, dpy, int, screen,
	int *, attrib_list, glXChooseVisual)

VFUNCDEF4(glXCopyContext, Display *, dpy, GLXContext, src, GLXContext, dst,
	unsigned long, mask, glXCopyContext)

FUNCDEF4(GLXContext, glXCreateContext, Display *, dpy, XVisualInfo *, vis,
	GLXContext, share_list, Bool, direct, glXCreateContext)

FUNCDEF3(GLXPixmap, glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
	Pixmap, pixmap, glXCreateGLXPixmap)

VFUNCDEF2(glXDestroyContext, Display *, dpy, GLXContext, ctx,
	glXDestroyContext)

VFUNCDEF2(glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pix,
	glXDestroyGLXPixmap)

FUNCDEF4(int, glXGetConfig, Display *, dpy, XVisualInfo *, vis, int, attrib,
	int *, value, glXGetConfig)

FUNCDEF0(GLXContext, glXGetCurrentContext, glXGetCurrentContext)

FUNCDEF0(GLXDrawable, glXGetCurrentDrawable, glXGetCurrentDrawable)

FUNCDEF2(Bool, glXIsDirect, Display *, dpy, GLXContext, ctx, glXIsDirect)

FUNCDEF3(Bool, glXMakeCurrent, Display *, dpy, GLXDrawable, drawable,
	GLXContext, ctx, glXMakeCurrent)

FUNCDEF3(Bool, glXQueryExtension, Display *, dpy, int *, error_base,
	int *, event_base, glXQueryExtension)

FUNCDEF3(Bool, glXQueryVersion, Display *, dpy, int *, major, int *, minor,
	glXQueryVersion)

VFUNCDEF2(glXSwapBuffers, Display *, dpy, GLXDrawable, drawable,
	glXSwapBuffers)

VFUNCDEF4(glXUseXFont, Font, font, int, first, int, count, int, list_base,
	glXUseXFont)

VFUNCDEF0(glXWaitGL, glXWaitGL)


// GLX 1.1 functions

FUNCDEF2(const char *, glXGetClientString, Display *, dpy, int, name,
	glXGetClientString)

FUNCDEF3(const char *, glXQueryServerString, Display *, dpy, int, screen, int,
	name, glXQueryServerString)

FUNCDEF2(const char *, glXQueryExtensionsString, Display *, dpy, int, screen,
	glXQueryExtensionsString)


// GLX 1.2 functions

FUNCDEF0(Display *, glXGetCurrentDisplay, glXGetCurrentDisplay)


// GLX 1.3 functions

FUNCDEF4(GLXFBConfig *, glXChooseFBConfig, Display *, dpy, int, screen,
	const int *, attrib_list, int *, nelements, glXChooseFBConfig)

FUNCDEF5(GLXContext, glXCreateNewContext, Display *, dpy, GLXFBConfig, config,
	int, render_type, GLXContext, share_list, Bool, direct, glXCreateNewContext)

FUNCDEF3(GLXPbuffer, glXCreatePbuffer, Display *, dpy, GLXFBConfig, config,
	const int *, attrib_list, glXCreatePbuffer)

FUNCDEF4(GLXPixmap, glXCreatePixmap, Display *, dpy, GLXFBConfig, config,
	Pixmap, pixmap, const int *, attrib_list, glXCreatePixmap)

FUNCDEF4(GLXWindow, glXCreateWindow, Display *, dpy, GLXFBConfig, config,
	Window, win, const int *, attrib_list, glXCreateWindow)

VFUNCDEF2(glXDestroyPbuffer, Display *, dpy, GLXPbuffer, pbuf,
	glXDestroyPbuffer)

VFUNCDEF2(glXDestroyPixmap, Display *, dpy, GLXPixmap, pixmap,
	glXDestroyPixmap)

VFUNCDEF2(glXDestroyWindow, Display *, dpy, GLXWindow, win, glXDestroyWindow)

FUNCDEF0(GLXDrawable, glXGetCurrentReadDrawable, glXGetCurrentReadDrawable)

FUNCDEF4(int, glXGetFBConfigAttrib, Display *, dpy, GLXFBConfig, config,
	int, attribute, int *, value, glXGetFBConfigAttrib)

FUNCDEF3(GLXFBConfig *, glXGetFBConfigs, Display *, dpy, int, screen,
	int *, nelements, glXGetFBConfigs)

VFUNCDEF3(glXGetSelectedEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long *, event_mask, glXGetSelectedEvent)

FUNCDEF2(XVisualInfo *, glXGetVisualFromFBConfig, Display *, dpy,
	GLXFBConfig, config, glXGetVisualFromFBConfig)

FUNCDEF4(Bool, glXMakeContextCurrent, Display *, display, GLXDrawable, draw,
	GLXDrawable, read, GLXContext, ctx, glXMakeContextCurrent)

FUNCDEF4(int, glXQueryContext, Display *, dpy, GLXContext, ctx, int, attribute,
	int *, value, glXQueryContext)

VFUNCDEF4(glXQueryDrawable, Display *, dpy, GLXDrawable, draw, int, attribute,
	unsigned int *, value, glXQueryDrawable)

VFUNCDEF3(glXSelectEvent, Display *, dpy, GLXDrawable, draw,
	unsigned long, event_mask, glXSelectEvent)


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
	glXCreateContextAttribsARB)


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
	glXFreeContextEXT)

FUNCDEF2(GLXContext, glXImportContextEXT, Display *, dpy,
	GLXContextID, contextID, glXImportContextEXT)

FUNCDEF4(int, glXQueryContextInfoEXT, Display *, dpy, GLXContext, ctx,
	int, attribute, int *, value, glXQueryContextInfoEXT)


// GLX_EXT_swap_control

VFUNCDEF3(glXSwapIntervalEXT, Display *, dpy, GLXDrawable, drawable, int,
	interval, glXSwapIntervalEXT)


// GLX_EXT_texture_from_pixmap

VFUNCDEF4(glXBindTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, const int *, attrib_list, glXBindTexImageEXT)

VFUNCDEF3(glXReleaseTexImageEXT, Display *, dpy, GLXDrawable, drawable,
	int, buffer, glXReleaseTexImageEXT)


// GLX_SGI_swap_control

FUNCDEF1(int, glXSwapIntervalSGI, int, interval, glXSwapIntervalSGI)


// GLX_SGIX_fbconfig

FUNCDEF2(GLXFBConfigSGIX, glXGetFBConfigFromVisualSGIX, Display *, dpy,
	XVisualInfo *, vis, glXGetFBConfigFromVisualSGIX)


#ifdef EGLBACKEND


// EGL 1.0 functions

FUNCDEF5(EGLBoolean, eglChooseConfig, EGLDisplay, display,
	const EGLint *, attrib_list, EGLConfig *, configs, EGLint, config_size,
	EGLint *, num_config, eglChooseConfig)

FUNCDEF3(EGLBoolean, eglCopyBuffers, EGLDisplay, display, EGLSurface, surface,
	NativePixmapType, native_pixmap, eglCopyBuffers)

FUNCDEF4(EGLContext, eglCreateContext, EGLDisplay, display, EGLConfig, config,
	EGLContext, share_context, const EGLint *, attrib_list, eglCreateContext)

FUNCDEF3(EGLSurface, eglCreatePbufferSurface, EGLDisplay, display,
	EGLConfig, config, const EGLint *, attrib_list, eglCreatePbufferSurface)

FUNCDEF4(EGLSurface, eglCreatePixmapSurface, EGLDisplay, display,
	EGLConfig, config, EGLNativePixmapType, native_pixmap,
	const EGLint *, attrib_list, eglCreatePixmapSurface)

FUNCDEF4(EGLSurface, eglCreateWindowSurface, EGLDisplay, display,
	EGLConfig, config, NativeWindowType, native_window,
	const EGLint *, attrib_list, eglCreateWindowSurface)

FUNCDEF2(EGLBoolean, eglDestroySurface, EGLDisplay, display,
	EGLSurface, surface, eglDestroySurface)

FUNCDEF4(EGLBoolean, eglGetConfigAttrib, EGLDisplay, display,
	EGLConfig, config, EGLint, attribute, EGLint *, value, eglGetConfigAttrib)

FUNCDEF4(EGLBoolean, eglGetConfigs, EGLDisplay, display, EGLConfig *, configs,
	EGLint, config_size, EGLint *, num_config, eglGetConfigs)

FUNCDEF0(EGLDisplay, eglGetCurrentDisplay, eglGetCurrentDisplay)

FUNCDEF1(EGLSurface, eglGetCurrentSurface, EGLint, readdraw,
	eglGetCurrentSurface)

FUNCDEF1(EGLDisplay, eglGetDisplay, EGLNativeDisplayType, native_display,
	eglGetDisplay)

FUNCDEF0(EGLint, eglGetError, eglGetError)

typedef void (*(*_eglGetProcAddressType)(const char *))(void);
SYMDEF(eglGetProcAddress);
static INLINE void (*_eglGetProcAddress(const char *procName))(void)
{
	CHECKSYM(eglGetProcAddress, NULL);
	return __eglGetProcAddress(procName);
}

FUNCDEF3(EGLBoolean, eglInitialize, EGLDisplay, display, EGLint *, major,
	EGLint *, minor, eglInitialize)

FUNCDEF4(EGLBoolean, eglMakeCurrent, EGLDisplay, display, EGLSurface, draw,
	EGLSurface, read, EGLContext, context, eglMakeCurrent)

FUNCDEF4(EGLBoolean, eglQueryContext, EGLDisplay, display, EGLContext, context,
	EGLint, attribute, EGLint *, value, eglQueryContext)

FUNCDEF2(const char *, eglQueryString, EGLDisplay, display, EGLint, name,
	eglQueryString)

FUNCDEF4(EGLBoolean, eglQuerySurface, EGLDisplay, display, EGLSurface, surface,
	EGLint, attribute, EGLint *, value, eglQuerySurface)

FUNCDEF2(EGLBoolean, eglSwapBuffers, EGLDisplay, display, EGLSurface, surface,
	eglSwapBuffers)

FUNCDEF1(EGLBoolean, eglTerminate, EGLDisplay, display, eglTerminate)


// EGL 1.1 functions

FUNCDEF3(EGLBoolean, eglBindTexImage, EGLDisplay, display, EGLSurface, surface,
	EGLint, buffer, eglBindTexImage)

FUNCDEF3(EGLBoolean, eglReleaseTexImage, EGLDisplay, display,
	EGLSurface, surface, EGLint, buffer, eglReleaseTexImage)

FUNCDEF4(EGLBoolean, eglSurfaceAttrib, EGLDisplay, display,
	EGLSurface, surface, EGLint, attribute, EGLint, value, eglSurfaceAttrib)

FUNCDEF2(EGLBoolean, eglSwapInterval, EGLDisplay, display, EGLint, interval,
	eglSwapInterval)


// EGL 1.2 functions

FUNCDEF5(EGLSurface, eglCreatePbufferFromClientBuffer, EGLDisplay, display,
	EGLenum, buftype, EGLClientBuffer, buffer, EGLConfig, config,
	const EGLint *, attrib_list, eglCreatePbufferFromClientBuffer)


// EGL 1.5 functions

FUNCDEF4(EGLint, eglClientWaitSync, EGLDisplay, display, EGLSync, sync,
	EGLint, flags, EGLTime, timeout, eglClientWaitSync)

FUNCDEF5(EGLImage, eglCreateImage, EGLDisplay, display, EGLContext, context,
	EGLenum, target, EGLClientBuffer, buffer, const EGLAttrib *, attrib_list,
	eglCreateImage)

FUNCDEF4(EGLSurface, eglCreatePlatformPixmapSurface, EGLDisplay, display,
	EGLConfig, config, void *, native_pixmap, const EGLAttrib *, attrib_list,
	eglCreatePlatformPixmapSurface)

FUNCDEF3(EGLSync, eglCreateSync, EGLDisplay, display, EGLenum, type,
	const EGLAttrib *, attrib_list, eglCreateSync)

FUNCDEF2(EGLBoolean, eglDestroyImage, EGLDisplay, display, EGLImage, image,
	eglDestroyImage)

FUNCDEF2(EGLBoolean, eglDestroySync, EGLDisplay, display, EGLSync, sync,
	eglDestroySync)

FUNCDEF3(EGLDisplay, eglGetPlatformDisplay, EGLenum, platform,
	void *, native_display, const EGLAttrib *, attrib_list,
	eglGetPlatformDisplay)

FUNCDEF4(EGLBoolean, eglGetSyncAttrib, EGLDisplay, display, EGLSync, sync,
	EGLint, attribute, EGLAttrib *, value, eglGetSyncAttrib)

FUNCDEF3(EGLBoolean, eglWaitSync, EGLDisplay, display, EGLSync, sync,
	EGLint, flags, eglWaitSync)


// EGL_EXT_device_query

FUNCDEF3(EGLBoolean, eglQueryDisplayAttribEXT, EGLDisplay, display,
	EGLint, attribute, EGLAttrib *, value, eglQueryDisplayAttribEXT)


// EGL_EXT_platform_base

FUNCDEF4(EGLSurface, eglCreatePlatformPixmapSurfaceEXT, EGLDisplay, display,
	EGLConfig, config, void *, native_pixmap, const EGLint *, attrib_list,
	eglCreatePlatformPixmapSurfaceEXT)

FUNCDEF3(EGLDisplay, eglGetPlatformDisplayEXT, EGLenum, platform,
	void *, native_display, const EGLint *, attrib_list,
	eglGetPlatformDisplayEXT)


// EGL_KHR_cl_event2

FUNCDEF3(EGLSyncKHR, eglCreateSync64KHR, EGLDisplay, display, EGLenum, type,
	const EGLAttribKHR *, attrib_list, eglCreateSync64KHR)


// EGL_KHR_fence_sync

FUNCDEF4(EGLint, eglClientWaitSyncKHR, EGLDisplay, display, EGLSyncKHR, sync,
	EGLint, flags, EGLTimeKHR, timeout, eglClientWaitSyncKHR)

FUNCDEF3(EGLSyncKHR, eglCreateSyncKHR, EGLDisplay, display, EGLenum, type,
	const EGLint *, attrib_list, eglCreateSyncKHR)

FUNCDEF2(EGLBoolean, eglDestroySyncKHR, EGLDisplay, display, EGLSyncKHR, sync,
	eglDestroySyncKHR)

FUNCDEF4(EGLBoolean, eglGetSyncAttribKHR, EGLDisplay, display,
	EGLSyncKHR, sync, EGLint, attribute, EGLint *, value, eglGetSyncAttribKHR)


// EGL_KHR_image

FUNCDEF5(EGLImageKHR, eglCreateImageKHR, EGLDisplay, display,
	EGLContext, context, EGLenum, target, EGLClientBuffer, buffer,
	const EGLint *, attrib_list, eglCreateImageKHR)

FUNCDEF2(EGLBoolean, eglDestroyImageKHR, EGLDisplay, display,
	EGLImageKHR, image, eglDestroyImageKHR)


// EGL_KHR_reusable_sync

FUNCDEF3(EGLBoolean, eglSignalSyncKHR, EGLDisplay, display, EGLSyncKHR, sync,
	EGLenum, mode, eglSignalSyncKHR)


// EGL_KHR_wait_sync

FUNCDEF3(EGLint, eglWaitSyncKHR, EGLDisplay, display, EGLSyncKHR, sync,
	EGLint, flags, eglWaitSyncKHR)


#endif


// GL functions

VFUNCDEF2(glBindFramebuffer, GLenum, target, GLuint, framebuffer,
	glBindFramebuffer)

VFUNCDEF2(glBindFramebufferEXT, GLenum, target, GLuint, framebuffer,
	glBindFramebufferEXT)

VFUNCDEF2(glDeleteFramebuffers, GLsizei, n, const GLuint *, framebuffers,
	glDeleteFramebuffers)

VFUNCDEF0(glFinish, glFinish)

VFUNCDEF0(glFlush, glFlush)

VFUNCDEF1(glDrawBuffer, GLenum, drawbuf, glDrawBuffer)

VFUNCDEF2(glDrawBuffers, GLsizei, n, const GLenum *, bufs, glDrawBuffers)

VFUNCDEF2(glFramebufferDrawBufferEXT, GLuint, framebuffer, GLenum, mode,
	glFramebufferDrawBufferEXT)

VFUNCDEF3(glFramebufferDrawBuffersEXT, GLuint, framebuffer, GLsizei, n,
	const GLenum *, bufs, glFramebufferDrawBuffersEXT)

VFUNCDEF2(glFramebufferReadBufferEXT, GLuint, framebuffer, GLenum, mode,
	glFramebufferReadBufferEXT)

VFUNCDEF2(glGetBooleanv, GLenum, pname, GLboolean *, data, glGetBooleanv)

VFUNCDEF2(glGetDoublev, GLenum, pname, GLdouble *, data, glGetDoublev)

VFUNCDEF2(glGetFloatv, GLenum, pname, GLfloat *, data, glGetFloatv)

VFUNCDEF4(glGetFramebufferAttachmentParameteriv, GLenum, target,
	GLenum, attachment, GLenum, pname, GLint *, params,
	glGetFramebufferAttachmentParameteriv)

VFUNCDEF3(glGetFramebufferParameteriv, GLenum, target, GLenum, pname,
	GLint *, params, glGetFramebufferParameteriv)

VFUNCDEF2(glGetIntegerv, GLenum, pname, GLint *, params, glGetIntegerv)

VFUNCDEF2(glGetInteger64v, GLenum, pname, GLint64 *, data, glGetInteger64v)

VFUNCDEF3(glGetNamedFramebufferParameteriv, GLuint, framebuffer,
	GLenum, pname, GLint *, param, glGetNamedFramebufferParameteriv)

FUNCDEF1(const GLubyte *, glGetString, GLenum, name, glGetString)

FUNCDEF2(const GLubyte *, glGetStringi, GLenum, name, GLuint, index,
	glGetStringi)

VFUNCDEF2(glNamedFramebufferDrawBuffer, GLuint, framebuffer, GLenum, buf,
	glNamedFramebufferDrawBuffer)

VFUNCDEF3(glNamedFramebufferDrawBuffers, GLuint, framebuffer, GLsizei, n,
	const GLenum *, bufs, glNamedFramebufferDrawBuffers)

VFUNCDEF2(glNamedFramebufferReadBuffer, GLuint, framebuffer, GLenum, mode,
	glNamedFramebufferReadBuffer)

VFUNCDEF0(glPopAttrib, glPopAttrib)

VFUNCDEF1(glReadBuffer, GLenum, mode, glReadBuffer)

VFUNCDEF7(glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	GLenum, format, GLenum, type, GLvoid *, pixels, glReadPixels)

VFUNCDEF4(glViewport, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	glViewport)


#ifdef FAKEOPENCL

// OpenCL functions

typedef void (*pfn_notifyType)(const char *, const void *, size_t, void *);
FUNCDEF6(cl_context, clCreateContext,
	const cl_context_properties *, properties, cl_uint, num_devices,
	const cl_device_id *, devices, pfn_notifyType, pfn_notify, void *, user_data,
	cl_int *, errcode_ret, clCreateContext)

#endif


// X11 functions

FUNCDEF3(Bool, XCheckMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	XCheckMaskEvent)

FUNCDEF3(Bool, XCheckTypedEvent, Display *, dpy, int, event_type, XEvent *, xe,
	XCheckTypedEvent)

FUNCDEF4(Bool, XCheckTypedWindowEvent, Display *, dpy, Window, win,
	int, event_type, XEvent *, xe, XCheckTypedWindowEvent)

FUNCDEF4(Bool, XCheckWindowEvent, Display *, dpy, Window, win, long,
	event_mask, XEvent *, xe, XCheckWindowEvent)

FUNCDEF1(int, XCloseDisplay, Display *, dpy, XCloseDisplay)

FUNCDEF4(int, XConfigureWindow, Display *, dpy, Window, win,
	unsigned int, value_mask, XWindowChanges *, values, XConfigureWindow)

FUNCDEF10(int, XCopyArea, Display *, dpy, Drawable, src, Drawable, dst, GC, gc,
	int, src_x, int, src_y, unsigned int, w, unsigned int, h, int, dest_x,
	int, dest_y, XCopyArea)

FUNCDEF9(Window, XCreateSimpleWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height, unsigned int,
	border_width, unsigned long, border, unsigned long, background,
	XCreateSimpleWindow)

FUNCDEF12(Window, XCreateWindow, Display *, dpy, Window, parent, int, x,
	int, y, unsigned int, width, unsigned int, height,
	unsigned int, border_width, int, depth, unsigned int, c_class,
	Visual *, visual, unsigned long, value_mask,
	XSetWindowAttributes *, attributes, XCreateWindow)

FUNCDEF2(int, XDestroySubwindows, Display *, dpy, Window, win,
	XDestroySubwindows)

FUNCDEF2(int, XDestroyWindow, Display *, dpy, Window, win, XDestroyWindow)

FUNCDEF1(int, XFree, void *, data, XFree)

FUNCDEF9(Status, XGetGeometry, Display *, display, Drawable, d, Window *, root,
	int *, x, int *, y, unsigned int *, width, unsigned int *, height,
	unsigned int *, border_width, unsigned int *, depth, XGetGeometry)

FUNCDEF8(XImage *, XGetImage, Display *, display, Drawable, d, int, x, int, y,
	unsigned int, width, unsigned int, height, unsigned long, plane_mask,
	int, format, XGetImage)

FUNCDEF2(char **, XListExtensions, Display *, dpy, int *, next,
	XListExtensions)

FUNCDEF3(int, XMaskEvent, Display *, dpy, long, event_mask, XEvent *, xe,
	XMaskEvent)

FUNCDEF6(int, XMoveResizeWindow, Display *, dpy, Window, win, int, x, int, y,
	unsigned int, width, unsigned int, height, XMoveResizeWindow)

FUNCDEF2(int, XNextEvent, Display *, dpy, XEvent *, xe, XNextEvent)

FUNCDEF1(Display *, XOpenDisplay, _Xconst char *, name, XOpenDisplay)

#ifdef LIBX11_18
FUNCDEF6(Display *, XkbOpenDisplay, _Xconst char *, display_name,
	int *, event_rtrn, int *, error_rtrn, int *, major_in_out,
	int *, minor_in_out, int *, reason_rtrn, XkbOpenDisplay)
#else
FUNCDEF6(Display *, XkbOpenDisplay, char *, display_name, int *, event_rtrn,
	int *, error_rtrn, int *, major_in_out, int *, minor_in_out,
	int *, reason_rtrn, XkbOpenDisplay)
#endif

FUNCDEF5(Bool, XQueryExtension, Display *, dpy, _Xconst char *, name,
	int *, major_opcode, int *, first_event, int *, first_error,
	XQueryExtension)

FUNCDEF4(int, XResizeWindow, Display *, dpy, Window, win, unsigned int, width,
	unsigned int, height, XResizeWindow)

FUNCDEF1(char *, XServerVendor, Display *, dpy, XServerVendor)

FUNCDEF4(int, XWindowEvent, Display *, dpy, Window, win, long, event_mask,
	XEvent *, xe, XWindowEvent)


// From dlfaker

typedef void *(*_dlopenType)(const char *, int);
void *_vgl_dlopen(const char *, int);
SYMDEF(dlopen);


#ifdef FAKEXCB

// XCB functions

FUNCDEF13(xcb_void_cookie_t, xcb_create_window, xcb_connection_t *, conn,
	uint8_t, depth, xcb_window_t, wid, xcb_window_t, parent, int16_t, x,
	int16_t, y, uint16_t, width, uint16_t, height, uint16_t, border_width,
	uint16_t, _class, xcb_visualid_t, visual, uint32_t, value_mask,
	const void *, value_list, xcb_create_window)

FUNCDEF13(xcb_void_cookie_t, xcb_create_window_aux, xcb_connection_t *, conn,
	uint8_t, depth, xcb_window_t, wid, xcb_window_t, parent, int16_t, x,
	int16_t, y, uint16_t, width, uint16_t, height, uint16_t, border_width,
	uint16_t, _class, xcb_visualid_t, visual, uint32_t, value_mask,
	const xcb_create_window_value_list_t *, value_list, xcb_create_window_aux)

FUNCDEF13(xcb_void_cookie_t, xcb_create_window_aux_checked,
	xcb_connection_t *, conn, uint8_t, depth, xcb_window_t, wid,
	xcb_window_t, parent, int16_t, x, int16_t, y, uint16_t, width,
	uint16_t, height, uint16_t, border_width, uint16_t, _class,
	xcb_visualid_t, visual, uint32_t, value_mask,
	const xcb_create_window_value_list_t *, value_list,
	xcb_create_window_aux_checked)

FUNCDEF13(xcb_void_cookie_t, xcb_create_window_checked,
	xcb_connection_t *, conn, uint8_t, depth, xcb_window_t, wid,
	xcb_window_t, parent, int16_t, x, int16_t, y, uint16_t, width,
	uint16_t, height, uint16_t, border_width, uint16_t, _class,
	xcb_visualid_t, visual, uint32_t, value_mask, const void *, value_list,
	xcb_create_window_checked)

FUNCDEF2(xcb_void_cookie_t, xcb_destroy_subwindows, xcb_connection_t *, conn,
	xcb_window_t, window, xcb_destroy_subwindows)

FUNCDEF2(xcb_void_cookie_t, xcb_destroy_subwindows_checked,
	xcb_connection_t *, conn, xcb_window_t, window,
	xcb_destroy_subwindows_checked)

FUNCDEF2(xcb_void_cookie_t, xcb_destroy_window, xcb_connection_t *, conn,
	xcb_window_t, window, xcb_destroy_window)

FUNCDEF2(xcb_void_cookie_t, xcb_destroy_window_checked,
	xcb_connection_t *, conn, xcb_window_t, window, xcb_destroy_window_checked)

FUNCDEF2(const xcb_query_extension_reply_t *, xcb_get_extension_data,
	xcb_connection_t *, conn, xcb_extension_t *, ext, xcb_get_extension_data)

FUNCDEF3(xcb_glx_query_version_cookie_t, xcb_glx_query_version,
	xcb_connection_t *, conn, uint32_t, major_version, uint32_t, minor_version,
	xcb_glx_query_version)

FUNCDEF3(xcb_glx_query_version_reply_t *, xcb_glx_query_version_reply,
	xcb_connection_t *, conn, xcb_glx_query_version_cookie_t, cookie,
	xcb_generic_error_t **, error, xcb_glx_query_version_reply)

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_event, xcb_connection_t *, conn,
	xcb_poll_for_event)

FUNCDEF1(xcb_generic_event_t *, xcb_poll_for_queued_event, xcb_connection_t *,
	conn, xcb_poll_for_queued_event)

FUNCDEF1(xcb_generic_event_t *, xcb_wait_for_event, xcb_connection_t *, conn,
	xcb_wait_for_event)

#endif


// Functions used by the faker (but not interposed.)  We load the GLX/OpenGL
// functions dynamically to prevent the 3D application from overriding them, as
// well as to ensure that, with 'vglrun -nodl', libGL is not loaded into the
// process until the 3D application actually uses it.

VFUNCDEF2(glBindBuffer, GLenum, target, GLuint, buffer, NULL)

VFUNCDEF2(glBindRenderbuffer, GLenum, target, GLuint, renderbuffer, NULL)

VFUNCDEF7(glBitmap, GLsizei, width, GLsizei, height, GLfloat, xorig,
	GLfloat, yorig, GLfloat, xmove, GLfloat, ymove, const GLubyte *, bitmap,
	NULL)

VFUNCDEF10(glBlitFramebuffer, GLint, srcX0, GLint, srcY0, GLint, srcX1,
	GLint, srcY1, GLint, dstX0, GLint, dstY0, GLint, dstX1, GLint, dstY1,
	GLbitfield, mask, GLenum, filter, NULL)

VFUNCDEF4(glBufferData, GLenum, target, GLsizeiptr, size, const GLvoid *, data,
	GLenum, usage, NULL)

FUNCDEF1(GLenum, glCheckFramebufferStatus, GLenum, target, NULL)

VFUNCDEF1(glClear, GLbitfield, mask, NULL)

VFUNCDEF4(glClearColor, GLclampf, red, GLclampf, green, GLclampf, blue,
	GLclampf, alpha, NULL)

VFUNCDEF5(glCopyPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height,
	GLenum, type, NULL)

VFUNCDEF2(glDeleteRenderbuffers, GLsizei, n, const GLuint *, renderbuffers,
	NULL)

VFUNCDEF0(glEndList, NULL)

VFUNCDEF4(glFramebufferRenderbuffer, GLenum, target, GLenum, attachment,
	GLenum, renderbuffertarget, GLuint, renderbuffer, NULL)

VFUNCDEF2(glGenBuffers, GLsizei, n, GLuint *, buffers, NULL)

VFUNCDEF2(glGenFramebuffers, GLsizei, n, GLuint *, ids, NULL)

VFUNCDEF2(glGenRenderbuffers, GLsizei, n, GLuint *, renderbuffers, NULL)

VFUNCDEF3(glGetBufferParameteriv, GLenum, target, GLenum, value, GLint *, data,
	NULL)

FUNCDEF0(GLenum, glGetError, NULL)

VFUNCDEF0(glLoadIdentity, NULL)

FUNCDEF2(void *, glMapBuffer, GLenum, target, GLenum, access, NULL)

VFUNCDEF1(glMatrixMode, GLenum, mode, NULL)

VFUNCDEF2(glNewList, GLuint, list, GLenum, mode, NULL)

VFUNCDEF6(glOrtho, GLdouble, left, GLdouble, right, GLdouble, bottom,
	GLdouble, top, GLdouble, near_val, GLdouble, far_val, NULL)

VFUNCDEF2(glPixelStorei, GLenum, pname, GLint, param, NULL)

VFUNCDEF0(glPopMatrix, NULL)

VFUNCDEF0(glPushMatrix, NULL)

VFUNCDEF2(glRasterPos2i, GLint, x, GLint, y, NULL)

VFUNCDEF4(glRenderbufferStorage, GLenum, target, GLenum, internalformat,
	GLsizei, width, GLsizei, height, NULL)

VFUNCDEF5(glRenderbufferStorageMultisample, GLenum, target, GLsizei, samples,
	GLenum, internalformat, GLsizei, width, GLsizei, height, NULL)

FUNCDEF1(GLboolean, glUnmapBuffer, GLenum, target, NULL)

// EGL functions used by the faker (but not interposed.)

#ifdef EGLBACKEND

FUNCDEF1(EGLBoolean, eglBindAPI, EGLenum, api, NULL)

FUNCDEF2(EGLBoolean, eglDestroyContext, EGLDisplay, display,
	EGLContext, context, NULL)

FUNCDEF0(EGLContext, eglGetCurrentContext, NULL)

FUNCDEF3(EGLBoolean, eglQueryDevicesEXT, EGLint, max_devices, EGLDeviceEXT *,
	devices, EGLint *, num_devices, NULL)

FUNCDEF2(const char *, eglQueryDeviceStringEXT, EGLDeviceEXT, device, EGLint,
	name, NULL)

FUNCDEF0(EGLenum, eglQueryAPI, NULL)

#endif

// We load all XCB functions dynamically, so that the same VirtualGL binary
// can be used to support systems with and without XCB libraries.

#ifdef FAKEXCB

FUNCDEF1(xcb_connection_t *, XGetXCBConnection, Display *, dpy, NULL)

VFUNCDEF2(XSetEventQueueOwner, Display *, dpy, enum XEventQueueOwner, owner,
	XSetEventQueueOwner)

typedef xcb_extension_t *_xcb_glx_idType;
SYMDEF(xcb_glx_id);
static INLINE xcb_extension_t *_xcb_glx_id(void)
{
	CHECKSYM(xcb_glx_id, NULL);
	return __xcb_glx_id;
}

FUNCDEF4(xcb_intern_atom_cookie_t, xcb_intern_atom, xcb_connection_t *, conn,
	uint8_t, only_if_exists, uint16_t, name_len, const char *, name, NULL)

FUNCDEF3(xcb_intern_atom_reply_t *, xcb_intern_atom_reply, xcb_connection_t *,
	conn, xcb_intern_atom_cookie_t, cookie, xcb_generic_error_t **, e, NULL)

FUNCDEF1(xcb_key_symbols_t *, xcb_key_symbols_alloc, xcb_connection_t *, c,
	NULL)

VFUNCDEF1(xcb_key_symbols_free, xcb_key_symbols_t *, syms, NULL)

FUNCDEF3(xcb_keysym_t, xcb_key_symbols_get_keysym, xcb_key_symbols_t *, syms,
	xcb_keycode_t, keycode, int, col, NULL)

#endif

#ifdef __cplusplus
}
#endif


#endif  // __FAKER_SYM_H__
