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

#include <unistd.h>
#include "Mutex.h"
#include "ConfigHash.h"
#include "ContextHash.h"
#include "DisplayHash.h"
#include "GLXDrawableHash.h"
#include "GlobalCriticalSection.h"
#include "PixmapHash.h"
#include "ReverseConfigHash.h"
#include "VisualHash.h"
#include "WindowHash.h"
#include "fakerconfig.h"
#include "threadlocal.h"
#include <dlfcn.h>


using namespace vglutil;
using namespace vglserver;


namespace vglfaker
{

Display *dpy3D=NULL;
bool deadYet=false;
int traceLevel=0;
#ifdef FAKEXCB
VGL_THREAD_LOCAL(FakerLevel, long, 0)
#endif
VGL_THREAD_LOCAL(ExcludeCurrent, bool, false)


static void cleanup(void)
{
	if(PixmapHash::isAlloc()) pmhash.kill();
	if(VisualHash::isAlloc()) vishash.kill();
	if(ConfigHash::isAlloc()) cfghash.kill();
	if(ReverseConfigHash::isAlloc()) rcfghash.kill();
	if(ContextHash::isAlloc()) ctxhash.kill();
	if(GLXDrawableHash::isAlloc()) glxdhash.kill();
	if(WindowHash::isAlloc()) winhash.kill();
	if(DisplayHash::isAlloc()) dpyhash.kill();
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

	if(init) return;
	GlobalCriticalSection::SafeLock l(globalMutex);
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


bool excludeDisplay(char *name)
{
	fconfig_reloadenv();

	char *dpyList=strdup(fconfig.excludeddpys);
	char *excluded=strtok(dpyList, " \t,");
	while(excluded)
	{
		if(!strcasecmp(name, excluded))
		{
			free(dpyList);  return true;
		}
		excluded=strtok(NULL, " \t,");
	}
	free(dpyList);
	return false;
}

}  // namespace


extern "C" {

// This is the "real" version of dlopen(), which is called by the interposed
// version of dlopen() in libdlfaker.  Can't recall why this is here and not
// in dlfaker, but it seems like there was a good reason.

void *_vgl_dlopen(const char *file, int mode)
{
	if(!__dlopen)
	{
		vglfaker::GlobalCriticalSection::SafeLock l(globalMutex);
		if(!__dlopen)
		{
			dlerror();  // Clear error state
			__dlopen=(_dlopenType)dlsym(RTLD_NEXT, "dlopen");
			char *err=dlerror();
			if(!__dlopen)
			{
				vglout.print("[VGL] ERROR: Could not load function \"dlopen\"\n");
				if(err) vglout.print("[VGL]    %s\n", err);
				vglfaker::safeExit(1);
			}
		}
	}
	return __dlopen(file, mode);
}

}
