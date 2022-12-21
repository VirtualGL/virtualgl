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

#ifndef __FAKER_H__
#define __FAKER_H__

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include "Error.h"
#include "Log.h"
#include "Mutex.h"
#include "fakerconfig.h"
#include "vglutil.h"
#include "backend.h"


namespace faker
{
	#ifdef EGLBACKEND
	// Unfortunately, we have to create our own EGLDisplay structure because
	// there is no way to get multiple unique EGLDisplay handles for the same DRI
	// device.  VirtualGL assumes that EGLDisplay handles returned by the
	// underlying EGL implementation are pointers to opaque structures and thus
	// uses a pointer hash (EGLXDisplayHash) to figure out whether an EGLDisplay
	// handle passed by the 3D application was allocated by VirtualGL or by the
	// underlying EGL implementation.
	typedef struct
	{
		EGLDisplay edpy;
		Display *x11dpy;
		int screen;
		bool isDefault, isInit;
	} EGLXDisplay;
	#endif

	extern Display *dpy3D;
	extern bool deadYet;
	extern char *glExtensions;
	#ifdef EGLBACKEND
	extern EGLint eglMajor, eglMinor;
	#endif

	extern void init(void);
	extern Display *init3D(void);
	extern void safeExit(int);

	extern long getTraceLevel(void);
	extern void setTraceLevel(long level);
	extern long getFakerLevel(void);
	extern void setFakerLevel(long level);
	extern bool getGLXExcludeCurrent(void);
	extern void setGLXExcludeCurrent(bool excludeCurrent);
	#ifdef EGLBACKEND
	extern bool getEGLExcludeCurrent(void);
	extern void setEGLExcludeCurrent(bool eglExcludeCurrent);
	#endif
	extern bool getOGLExcludeCurrent(void);
	extern void setOGLExcludeCurrent(bool excludeCurrent);
	extern long getAutotestColor();
	extern void setAutotestColor(long color);
	extern long getAutotestRColor();
	extern void setAutotestRColor(long color);
	extern long getAutotestFrame();
	extern void setAutotestFrame(long color);
	extern Display *getAutotestDisplay();
	extern void setAutotestDisplay(Display *dpy);
	extern long getAutotestDrawable();
	extern void setAutotestDrawable(long d);
	extern bool getEGLXContextCurrent(void);
	extern void setEGLXContextCurrent(bool isEGLXContextCurrent);
	#ifdef EGLBACKEND
	extern long getEGLError(void);
	extern void setEGLError(long error);
	extern EGLXDisplay *getCurrentEGLXDisplay(void);
	extern void setCurrentEGLXDisplay(EGLXDisplay *display);
	#endif

	void *loadSymbol(const char *name, bool optional = false);
	void unloadSymbols(void);

	extern bool isDisplayStringExcluded(char *name);

	INLINE bool isDisplayExcluded(Display *dpy)
	{
		XEDataObject obj = { dpy };
		XExtData *extData;

		if(!dpy) return false;
		// The 3D X server may have its own extensions that conflict with ours.
		if(!fconfig.egl && dpy == dpy3D) return true;
		int minExtensionNumber =
			XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1;
		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj),
			minExtensionNumber);
		ERRIFNOT(extData);
		ERRIFNOT(extData->private_data);

		return *(bool *)extData->private_data;
	}

	extern "C" int deleteCS(XExtData *extData);
	extern "C" int deleteRBOContext(XExtData *extData);

	INLINE util::CriticalSection &getDisplayCS(Display *dpy)
	{
		XEDataObject obj = { dpy };
		XExtData *extData;

		// The 3D X server may have its own extensions that conflict with ours.
		if(!fconfig.egl && dpy == dpy3D)
			THROW("faker::getDisplayCS() called with 3D X server handle (this should never happen)");
		int minExtensionNumber =
			XFindOnExtensionList(XEHeadOfExtensionList(obj), 0) ? 0 : 1;
		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj),
			minExtensionNumber + 1);
		ERRIFNOT(extData);
		ERRIFNOT(extData->private_data);

		return *(util::CriticalSection *)extData->private_data;
	}

	extern void sendGLXError(Display *dpy, CARD16 minorCode, CARD8 errorCode,
		bool x11Error);
}

#define DPY3D  faker::init3D()
#define EDPY  ((EGLDisplay)faker::init3D())

#define IS_EXCLUDED(dpy) \
	(faker::deadYet || faker::getFakerLevel() > 0 \
		|| faker::isDisplayExcluded(dpy))

#define IS_FRONT(drawbuf) \
	(drawbuf == GL_FRONT || drawbuf == GL_FRONT_AND_BACK \
		|| drawbuf == GL_FRONT_LEFT || drawbuf == GL_FRONT_RIGHT \
		|| drawbuf == GL_LEFT || drawbuf == GL_RIGHT)

#define IS_RIGHT(drawbuf) \
	(drawbuf == GL_RIGHT || drawbuf == GL_FRONT_RIGHT \
		|| drawbuf == GL_BACK_RIGHT)

#define MAX_ATTRIBS  256


static INLINE int DrawingToFront(void)
{
	GLint drawbuf = GL_BACK;
	backend::getIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return IS_FRONT(drawbuf);
}


static INLINE int DrawingToRight(void)
{
	GLint drawbuf = GL_LEFT;
	backend::getIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return IS_RIGHT(drawbuf);
}


#define DIE(f, m) \
{ \
	if(!faker::deadYet) \
		vglout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", f, m); \
	faker::safeExit(1); \
}


#define TRY()  try {
#define CATCH()  } \
	catch(std::exception &e) { DIE(GET_METHOD(e), e.what()); }


// Clear any pending OpenGL errors.
#define TRY_GL() \
{ \
	int e; \
	do { e = _glGetError(); } while(e != GL_NO_ERROR); \
}

// Catch any OpenGL errors that occurred since TRY_GL() was invoked.
#define CATCH_GL(m) \
{ \
	int e;  bool doThrow = false; \
	do \
	{ \
		e = _glGetError(); \
		if(e != GL_NO_ERROR) \
		{ \
			vglout.print("[VGL] ERROR: OpenGL error 0x%.4x\n", e); \
			doThrow = true; \
		} \
	} while(e != GL_NO_ERROR); \
	if(doThrow) THROW(m); \
}


// Tracing stuff

#define PRARGD(a)  vglout.print("%s=0x%.8lx(%s) ", #a, (unsigned long)a, \
		a ? DisplayString(a) : "NULL")

#define PRARGS(a)  vglout.print("%s=%s ", #a, a ? a : "NULL")

#define PRARGX(a)  vglout.print("%s=0x%.8lx ", #a, a)

#define PRARGIX(a)  vglout.print("%s=%d(0x%.lx) ", #a, (unsigned long)a, \
	(unsigned long)a)

#define PRARGI(a)  vglout.print("%s=%d ", #a, a)

#define PRARGF(a)  vglout.print("%s=%f ", #a, (double)a)

#define PRARGV(a) \
	vglout.print("%s=0x%.8lx(0x%.2lx) ", #a, (unsigned long)a, \
		a ? (a)->visualid : 0)

#define PRARGC(a) \
	vglout.print("%s=0x%.8lx(0x%.2x) ", #a, (unsigned long)a, a ? FBCID(a) : 0)

#define PRARGAL11(a)  if(a) \
{ \
	vglout.print(#a "=["); \
	for(int __an = 0; a[__an] != None && __an < MAX_ATTRIBS; __an++) \
	{ \
		vglout.print("0x%.4x", a[__an]); \
		if(a[__an] != GLX_USE_GL && a[__an] != GLX_DOUBLEBUFFER \
			&& a[__an] != GLX_STEREO && a[__an] != GLX_RGBA) \
			vglout.print("=0x%.4x", a[++__an]); \
		vglout.print(" "); \
	} \
	vglout.print("] "); \
}

#define PRARGAL13(a)  if(a != NULL) \
{ \
	vglout.print(#a "=["); \
	for(int __an = 0; a[__an] != None && __an < MAX_ATTRIBS; __an += 2) \
	{ \
		vglout.print("0x%.4x=0x%.4x ", a[__an], a[__an + 1]); \
	} \
	vglout.print("] "); \
}

#ifdef FAKEXCB
#define PRARGERR(a) \
{ \
	vglout.print("(%s)->response_type=%d ", #a, (a)->response_type); \
	vglout.print("(%s)->error_code=%d ", #a, (a)->error_code); \
}
#endif

#define OPENTRACE(f) \
	double vglTraceTime = 0.; \
	if(fconfig.trace) \
	{ \
		if(faker::getTraceLevel() > 0) \
		{ \
			vglout.print("\n[VGL 0x%.8x] ", pthread_self()); \
			for(int __i = 0; __i < faker::getTraceLevel(); __i++) \
				vglout.print("  "); \
		} \
		else vglout.print("[VGL 0x%.8x] ", pthread_self()); \
		faker::setTraceLevel(faker::getTraceLevel() + 1); \
		vglout.print("%s (", #f); \

#define STARTTRACE() \
		vglTraceTime = GetTime(); \
	}

#define STOPTRACE() \
	if(fconfig.trace) \
	{ \
		vglTraceTime = GetTime() - vglTraceTime;

#define CLOSETRACE() \
		vglout.PRINT(") %f ms\n", vglTraceTime * 1000.); \
		faker::setTraceLevel(faker::getTraceLevel() - 1); \
		if(faker::getTraceLevel() > 0) \
		{ \
			vglout.print("[VGL 0x%.8x] ", pthread_self()); \
			if(faker::getTraceLevel() > 1) \
				for(int __i = 0; __i < faker::getTraceLevel() - 1; __i++) \
					vglout.print("  "); \
		} \
	}

#endif  // __FAKER_H__
