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

#ifndef __FAKER_H__
#define __FAKER_H__

#include <X11/Xlib.h>
#include "Error.h"
#include "Log.h"
#include "Mutex.h"
#include "glx.h"
#include "fakerconfig.h"
#include "faker-sym.h"
#include "Timer.h"


namespace vglfaker
{
	extern Display *dpy3D;
	extern Display *init3D(void);
	extern void safeExit(int);
	extern bool deadYet;

	extern long getTraceLevel(void);
	extern void setTraceLevel(long level);
	extern long getFakerLevel(void);
	extern void setFakerLevel(long level);
	extern bool getExcludeCurrent(void);
	extern void setExcludeCurrent(bool excludeCurrent);
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

	extern bool isDisplayStringExcluded(char *name);

	INLINE bool isDisplayExcluded(Display *dpy)
	{
		XEDataObject obj = { dpy };
		XExtData *extData;

		// The 3D X server may have its own extensions that conflict with ours.
		if(dpy == dpy3D) return true;
		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 1);
		ERRIFNOT(extData);
		ERRIFNOT(extData->private_data);

		return *(bool *)extData->private_data;
	}

	extern "C" int deletePrivate(XExtData *extData);

	INLINE vglutil::CriticalSection &getDisplayCS(Display *dpy)
	{
		XEDataObject obj = { dpy };
		XExtData *extData;

		// The 3D X server may have its own extensions that conflict with ours.
		if(dpy == dpy3D)
			THROW("vglfaker::getDisplayCS() called with 3D X server handle (this should never happen)");
		extData = XFindOnExtensionList(XEHeadOfExtensionList(obj), 2);
		ERRIFNOT(extData);
		ERRIFNOT(extData->private_data);

		return *(vglutil::CriticalSection *)extData->private_data;
	}
}

#define DPY3D  vglfaker::init3D()

#define IS_EXCLUDED(dpy) \
	(vglfaker::deadYet || vglfaker::getFakerLevel() > 0 \
		|| vglfaker::isDisplayExcluded(dpy))

#define IS_FRONT(drawbuf) \
	(drawbuf == GL_FRONT || drawbuf == GL_FRONT_AND_BACK \
		|| drawbuf == GL_FRONT_LEFT || drawbuf == GL_FRONT_RIGHT \
		|| drawbuf == GL_LEFT || drawbuf == GL_RIGHT)

#define IS_RIGHT(drawbuf) \
	(drawbuf == GL_RIGHT || drawbuf == GL_FRONT_RIGHT \
		|| drawbuf == GL_BACK_RIGHT)


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


#define DIE(f, m) \
{ \
	if(!vglfaker::deadYet) \
		vglout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", f, m); \
	vglfaker::safeExit(1); \
}


#define TRY()  try {
#define CATCH()  } \
	catch(std::exception &e) { DIE(GET_METHOD(e), e.what()); }


// Tracing stuff

#define PRARGD(a)  vglout.print("%s=0x%.8lx(%s) ", #a, (unsigned long)a, \
		a ? DisplayString(a) : "NULL")

#define PRARGS(a)  vglout.print("%s=%s ", #a, a ? a : "NULL")

#define PRARGX(a)  vglout.print("%s=0x%.8lx ", #a, (unsigned long)a)

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
	for(int __an = 0; a[__an] != None; __an++) \
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
	for(int __an = 0; a[__an] != None; __an += 2) \
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
		if(vglfaker::getTraceLevel() > 0) \
		{ \
			vglout.print("\n[VGL 0x%.8x] ", pthread_self()); \
			for(int __i = 0; __i < vglfaker::getTraceLevel(); __i++) \
				vglout.print("  "); \
		} \
		else vglout.print("[VGL 0x%.8x] ", pthread_self()); \
		vglfaker::setTraceLevel(vglfaker::getTraceLevel() + 1); \
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
		vglfaker::setTraceLevel(vglfaker::getTraceLevel() - 1); \
		if(vglfaker::getTraceLevel() > 0) \
		{ \
			vglout.print("[VGL 0x%.8x] ", pthread_self()); \
			if(vglfaker::getTraceLevel() > 1) \
				for(int __i = 0; __i < vglfaker::getTraceLevel() - 1; __i++) \
					vglout.print("  "); \
		} \
	}

#endif  // __FAKER_H__
