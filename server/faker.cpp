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

#include <unistd.h>
#include "Mutex.h"
#include "ConfigHash.h"
#include "ContextHash.h"
#include "GLXDrawableHash.h"
#include "PixmapHash.h"
#include "ReverseConfigHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "fakerconfig.h"


using namespace vglutil;
using namespace vglserver;


namespace vglfaker
{

Display *dpy3D=NULL;
CriticalSection globalMutex;
bool deadYet=false;
int traceLevel=0;
#ifdef FAKEXCB
bool fakeXCB=false;
__thread int fakerLevel=0;
#endif


static void cleanup(void)
{
	if(PixmapHash::isAlloc()) pmhash.kill();
	if(VisualHash::isAlloc()) vishash.kill();
	if(ConfigHash::isAlloc()) cfghash.kill();
	if(ReverseConfigHash::isAlloc()) rcfghash.kill();
	if(ContextHash::isAlloc()) ctxhash.kill();
	if(GLXDrawableHash::isAlloc()) glxdhash.kill();
	if(WindowHash::isAlloc()) winhash.kill();
	unloadSymbols();
}


void safeExit(int retcode)
{
	bool shutdown;

	globalMutex.lock(false);
	shutdown=deadYet;
	if(!deadYet)
	{
		deadYet=true;
		cleanup();
		fconfig_deleteinstance();
	}
	globalMutex.unlock(false);
	if(!shutdown) exit(retcode);
	else pthread_exit(0);
}


class GlobalCleanup
{
	public:

		~GlobalCleanup()
		{
			globalMutex.lock(false);
			fconfig_deleteinstance();
			deadYet=true;
			globalMutex.unlock(false);
		}
};
GlobalCleanup globalCleanup;


// Used when VGL_TRAPX11=1

int xhandler(Display *dpy, XErrorEvent *xe)
{
	char temps[256];

	temps[0]=0;
	XGetErrorText(dpy, xe->error_code, temps, 255);
	vglout.PRINT("[VGL] WARNING: X11 error trapped\n[VGL]    Error:  %s\n[VGL]    XID:    0x%.8x\n",
		temps, xe->resourceid);
	return 0;
}


// Called from XOpenDisplay(), unless a GLX function is called first

void init(void)
{
	static int init=0;

	CriticalSection::SafeLock l(globalMutex);
	if(init) return;
	init=1;

	fconfig_reloadenv();
	if(strlen(fconfig.log)>0) vglout.logTo(fconfig.log);

	if(fconfig.verbose)
		vglout.println("[VGL] %s v%s %d-bit (Build %s)",
			__APPNAME, __VERSION, (int)sizeof(size_t)*8, __BUILD);

	if(getenv("VGL_DEBUG"))
	{
		vglout.print("[VGL] Attach debugger to process %d ...\n", getpid());
		fgetc(stdin);
	}
	if(fconfig.trapx11) XSetErrorHandler(xhandler);

	#ifdef FAKEXCB
	char *env=NULL;
	if((env=getenv("VGL_FAKEXCB"))!=NULL && strlen(env)>0
		&& !strncmp(env, "1", 1))
		vglfaker::fakeXCB=true;
	#endif

	loadSymbols();
	if(!dpy3D)
	{
		if(fconfig.verbose)
			vglout.println("[VGL] Opening connection to 3D X server %s",
				strlen(fconfig.localdpystring)>0? fconfig.localdpystring:"(default)");
		if((dpy3D=_XOpenDisplay(fconfig.localdpystring))==NULL)
		{
			vglout.print("[VGL] ERROR: Could not open display %s.\n",
			fconfig.localdpystring);
			safeExit(1);
		}
	}
}

}  // namespace


extern "C" {

// This is the "real" version of dlopen(), which is called by the interposed
// version of dlopen() in libdlfaker.  Can't recall why this is here and not
// in dlfaker, but it seems like there was a good reason.

void *_vgl_dlopen(const char *file, int mode)
{
	vglfaker::globalMutex.lock(false);
	if(!__dlopen) vglfaker::loadDLSymbols();
	vglfaker::globalMutex.unlock(false);
	CHECKSYM(dlopen);
	return __dlopen(file, mode);
}

}
