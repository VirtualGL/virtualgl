/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2011 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <string.h>
#include <math.h>
#include "rrtimer.h"
#include "rrthread.h"
#include "rrmutex.h"
#include "fakerconfig.h"
#define __FAKERHASH_STATICDEF__
#include "faker-winhash.h"
#include "faker-ctxhash.h"
#include "faker-vishash.h"
#include "faker-cfghash.h"
#include "faker-rcfghash.h"
#include "faker-pmhash.h"
#include "faker-glxdhash.h"
#include "faker-sym.h"
#include "glxvisual.h"
#define __VGLCONFIGSTART_STATICDEF__
#include "vglconfigstart.h"
#include <sys/types.h>
#include <unistd.h>


Display *_localdpy=NULL;
static rrcs globalmutex;
static int __shutdown=0;


#define _localdisplayiscurrent() (_glXGetCurrentDisplay()==_localdpy)
#define _isremote(dpy) (_localdpy && dpy!=_localdpy)
#define _isfront(drawbuf) (drawbuf==GL_FRONT || drawbuf==GL_FRONT_AND_BACK \
	|| drawbuf==GL_FRONT_LEFT || drawbuf==GL_FRONT_RIGHT || drawbuf==GL_LEFT \
	|| drawbuf==GL_RIGHT)
#define _isright(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)


static inline int _drawingtofront(void)
{
	GLint drawbuf=GL_BACK;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isfront(drawbuf);
}


static inline int _drawingtoright(void)
{
	GLint drawbuf=GL_LEFT;
	_glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isright(drawbuf);
}


static inline int isdead(void)
{
	int retval=0;
	globalmutex.lock(false);
	retval=__shutdown;
	globalmutex.unlock(false);
	return retval;
}


static void __vgl_cleanup(void)
{
	if(pmhash::isalloc()) pmh.killhash();
	if(vishash::isalloc()) vish.killhash();
	if(cfghash::isalloc()) cfgh.killhash();
	if(rcfghash::isalloc()) rcfgh.killhash();
	if(ctxhash::isalloc()) ctxh.killhash();
	if(glxdhash::isalloc()) glxdh.killhash();
	if(winhash::isalloc()) winh.killhash();
	__vgl_unloadsymbols();
}


void __vgl_safeexit(int retcode)
{
	int shutdown;
	globalmutex.lock(false);
	shutdown=__shutdown;
	if(!__shutdown)
	{
		__shutdown=1;
		__vgl_cleanup();
		fconfig_deleteinstance();
	}
	globalmutex.unlock(false);
	if(!shutdown) exit(retcode);
	else pthread_exit(0);
}


class _globalcleanup
{
	public:
		~_globalcleanup()
		{
			globalmutex.lock(false);
			fconfig_deleteinstance();
			__shutdown=1;
			globalmutex.unlock(false);
		}
};
_globalcleanup gdt;


#define _die(f,m) {if(!isdead())  \
	rrout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", f, m);  \
	__vgl_safeexit(1);}


#define TRY() try {
#define CATCH() } catch(rrerror &e) {_die(e.getMethod(), e.getMessage());}


// Tracing stuff

static int __vgltracelevel=0;

#define prargd(a) rrout.print("%s=0x%.8lx(%s) ", #a, (unsigned long)a,  \
	a? DisplayString(a):"NULL")
#define prargs(a) rrout.print("%s=%s ", #a, a?a:"NULL")
#define prargx(a) rrout.print("%s=0x%.8lx ", #a, (unsigned long)a)
#define prargi(a) rrout.print("%s=%d ", #a, a)
#define prargf(a) rrout.print("%s=%f ", #a, (double)a)
#define prargv(a) rrout.print("%s=0x%.8lx(0x%.2lx) ", #a, (unsigned long)a,  \
	a? a->visualid:0)
#define prargc(a) rrout.print("%s=0x%.8lx(0x%.2x) ", #a, (unsigned long)a,  \
	a? _FBCID(a):0)
#define prargal11(a) if(a) {  \
	rrout.print(#a"=[");  \
	for(int __an=0; a[__an]!=None; __an++) {  \
		rrout.print("0x%.4x", a[__an]);  \
		if(a[__an]!=GLX_USE_GL && a[__an]!=GLX_DOUBLEBUFFER  \
			&& a[__an]!=GLX_STEREO && a[__an]!=GLX_RGBA)  \
			rrout.print("=0x%.4x", a[++__an]);  \
		rrout.print(" ");  \
	}  rrout.print("] ");}
#define prargal13(a) if(a) {  \
	rrout.print(#a"=[");  \
	for(int __an=0; a[__an]!=None; __an+=2) {  \
		rrout.print("0x%.4x=0x%.4x ", a[__an], a[__an+1]);  \
	}  rrout.print("] ");}

#define opentrace(f)  \
	double __vgltracetime=0.;  \
	if(fconfig.trace) {  \
		if(__vgltracelevel>0) {  \
			rrout.print("\n[VGL] ");  \
			for(int __i=0; __i<__vgltracelevel; __i++) rrout.print("  ");  \
		}  \
		else rrout.print("[VGL] ");  \
		__vgltracelevel++;  \
		rrout.print("%s (", #f);  \

#define starttrace()  \
		__vgltracetime=rrtime();  \
	}

#define stoptrace()  \
	if(fconfig.trace) {  \
		__vgltracetime=rrtime()-__vgltracetime;

#define closetrace()  \
		rrout.PRINT(") %f ms\n", __vgltracetime*1000.);  \
		__vgltracelevel--;  \
		if(__vgltracelevel>0) {  \
			rrout.print("[VGL] ");  \
			if(__vgltracelevel>1)  \
				for(int __i=0; __i<__vgltracelevel-1; __i++) rrout.print("  ");  \
    }  \
	}


// Used when VGL_TRAPX11=1

int xhandler(Display *dpy, XErrorEvent *xe)
{
	char temps[256];
	temps[0]=0;
	XGetErrorText(dpy, xe->error_code, temps, 255);
	rrout.PRINT("[VGL] WARNING: X11 error trapped\n[VGL]    Error:  %s\n[VGL]    XID:    0x%.8x\n",
		temps, xe->resourceid);
	return 0;
}


// Called from XOpenDisplay(), unless a GLX function is called first

void __vgl_fakerinit(void)
{
	static int init=0;

	rrcs::safelock l(globalmutex);
	if(init) return;
	init=1;

	fconfig_reloadenv();
	if(strlen(fconfig.log)>0) rrout.logto(fconfig.log);

	if(fconfig.verbose)
		rrout.println("[VGL] %s v%s %d-bit (Build %s)",
			__APPNAME, __VERSION, (int)sizeof(size_t)*8, __BUILD);

	if(getenv("VGL_DEBUG"))
	{
		rrout.print("[VGL] Attach debugger to process %d ...\n", getpid());
		fgetc(stdin);
	}
	if(fconfig.trapx11) XSetErrorHandler(xhandler);

	__vgl_loadsymbols();
	if(!_localdpy)
	{
		if(fconfig.verbose) rrout.println("[VGL] Opening local display %s",
			strlen(fconfig.localdpystring)>0? fconfig.localdpystring:"(default)");
		if((_localdpy=_XOpenDisplay(fconfig.localdpystring))==NULL)
		{
			rrout.print("[VGL] ERROR: Could not open display %s.\n",
				fconfig.localdpystring);
			__vgl_safeexit(1);
		}
	}
}


extern "C" {

// This is the "real" version of dlopen(), which is called by the interposed
// version of dlopen() in libdlfaker.  Can't recall why this is here and not
// in dlfaker, but it seems like there was a good reason.

void *_vgl_dlopen(const char *file, int mode)
{
	globalmutex.lock(false);
	if(!__dlopen) __vgl_loaddlsymbols();
	globalmutex.unlock(false);
	checksym(dlopen);
	return __dlopen(file, mode);
}

}

#include "faker-x11.cpp"
#include "faker-gl.cpp"
#include "faker-glx.cpp"
