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

#define __LOCALSYM__
#include "faker-sym.h"
#include <dlfcn.h>

#include "fakerconfig.h"


static void *loadsym(void *dllhnd, const char *symbol, int quiet)
{
	void *sym;  const char *err;
	dlerror();  // Clear error state
	sym=dlsym(dllhnd, (char *)symbol);
	err=dlerror();
	if(err && !quiet) rrout.print("[VGL] %s\n", err);
	return sym;
}


#define lsym(s) __##s=(_##s##Type)loadsym(dllhnd, #s, !fconfig.verbose);  \
	if(!__##s) return -1;

#define lsymopt(s) __##s=(_##s##Type)loadsym(dllhnd, #s, 1);

static void *gldllhnd=NULL;
static void *x11dllhnd=NULL;
static int __vgl_loadglsymbols(void *);
static int __vgl_loadx11symbols(void *);


void __vgl_loaddlsymbols(void)
{
	dlerror();  // Clear error state
	__dlopen=(_dlopenType)loadsym(RTLD_NEXT, "dlopen", 0);
	if(!__dlopen)
	{
		rrout.print("[VGL] ERROR: Could not load symbol dlopen\n");
		__vgl_safeexit(1);
	}
}


void __vgl_loadsymbols(void)
{
	void *dllhnd;

	if(strlen(fconfig.gllib)>0)
	{
		dllhnd=_vgl_dlopen(fconfig.gllib, RTLD_NOW);
		if(!dllhnd)
		{
			rrout.print("[VGL] ERROR: Could not open %s\n[VGL]    %s\n",
				fconfig.gllib, dlerror());
			__vgl_safeexit(1);
		}
		else gldllhnd=dllhnd;
	}
	else dllhnd=RTLD_NEXT;
	if(__vgl_loadglsymbols(dllhnd)<0)
	{
		if(dllhnd==RTLD_NEXT)
		{
			if(fconfig.verbose)
			{
				rrout.print("[VGL] WARNING: Could not load GLX/OpenGL symbols using RTLD_NEXT.  Attempting\n");
				rrout.print("[VGL]    to load GLX/OpenGL symbols directly from libGL.so.1.\n");
			}
			dllhnd=_vgl_dlopen("libGL.so.1", RTLD_NOW);
			if(!dllhnd)
			{
				rrout.print("[VGL] ERROR: Could not open libGL.so.1\n[VGL]    %s\n",
					dlerror());
				__vgl_safeexit(1);
			}
			if(__vgl_loadglsymbols(dllhnd)<0)
			{
				rrout.print("[VGL] ERROR: Could not load GLX/OpenGL symbols from libGL.so.1.\n");
				__vgl_safeexit(1);
			}
			gldllhnd=dllhnd;
		}
		else
		{
			if(strlen(fconfig.gllib)>0)
				rrout.print("[VGL] ERROR: Could not load GLX/OpenGL symbols from %s.\n",
					fconfig.gllib);
			__vgl_safeexit(1);
		}
	}

	if(strlen(fconfig.x11lib)>0)
	{
		dllhnd=_vgl_dlopen(fconfig.x11lib, RTLD_NOW);
		if(!dllhnd)
		{
			rrout.print("[VGL] ERROR: Could not open %s\n[VGL]    %s\n",
				fconfig.x11lib, dlerror());
			__vgl_safeexit(1);
		}
		else x11dllhnd=dllhnd;
	}
	else dllhnd=RTLD_NEXT;
	if(__vgl_loadx11symbols(dllhnd)<0)
	{
		if(dllhnd==RTLD_NEXT)
		{
			if(fconfig.verbose)
			{
				rrout.print("[VGL] WARNING: Could not load X11 symbols using RTLD_NEXT.  Attempting\n");
				rrout.print("[VGL]    to load X11 symbols directly from libX11.\n");
			}
			dllhnd=_vgl_dlopen("libX11.so.4", RTLD_NOW);
			if(!dllhnd) dllhnd=_vgl_dlopen("libX11.so.5", RTLD_NOW);
			if(!dllhnd) dllhnd=_vgl_dlopen("libX11.so.6", RTLD_NOW);
			if(!dllhnd)
			{
				rrout.print("[VGL] ERROR: Could not open libX11\n[VGL]    %s\n",
					dlerror());
				__vgl_safeexit(1);
			}
			if(__vgl_loadx11symbols(dllhnd)<0)
			{
				rrout.print("[VGL] ERROR: Could not load X11 symbols from libX11.\n");
				__vgl_safeexit(1);
			}
			x11dllhnd=dllhnd;
		}
		else
		{
			if(strlen(fconfig.x11lib)>0)
				rrout.print("[VGL] ERROR: Could not load X11 symbols from %s.\n",
					fconfig.x11lib);
			__vgl_safeexit(1);
		}
	}
}


static int __vgl_loadglsymbols(void *dllhnd)
{
	dlerror();  // Clear error state

	// GLX symbols
	lsym(glXChooseVisual)
	lsym(glXCopyContext)
	lsym(glXCreateContext)
	lsym(glXCreateGLXPixmap)
	lsym(glXDestroyContext)
	lsym(glXDestroyGLXPixmap)
	lsym(glXGetConfig)
	lsym(glXGetCurrentDrawable)
	lsym(glXIsDirect)
	lsym(glXMakeCurrent);
	lsym(glXQueryExtension)
	lsym(glXQueryVersion)
	lsym(glXSwapBuffers)
	lsym(glXUseXFont)
	lsym(glXWaitGL)

	lsym(glXGetClientString)
	lsym(glXQueryServerString)
	lsym(glXQueryExtensionsString)

	lsym(glXChooseFBConfig)
	lsym(glXCreateNewContext)
	lsym(glXCreatePbuffer)
	lsym(glXCreatePixmap)
	lsym(glXCreateWindow)
	lsym(glXDestroyPbuffer)
	lsym(glXDestroyPixmap)
	lsym(glXDestroyWindow)
	lsym(glXGetCurrentDisplay)
	lsym(glXGetCurrentReadDrawable)
	lsym(glXGetFBConfigAttrib)
	lsym(glXGetFBConfigs)
	lsym(glXGetSelectedEvent)
	lsym(glXGetVisualFromFBConfig)
	lsym(glXMakeContextCurrent);
	lsym(glXQueryContext)
	lsym(glXQueryDrawable)
	lsym(glXSelectEvent)

	// Optional extensions.  We fake these if they exist
	lsymopt(glXFreeContextEXT)
	lsymopt(glXImportContextEXT)
	lsymopt(glXQueryContextInfoEXT)

	lsymopt(glXJoinSwapGroupNV)
	lsymopt(glXBindSwapBarrierNV)
	lsymopt(glXQuerySwapGroupNV)
	lsymopt(glXQueryMaxSwapGroupsNV)
	lsymopt(glXQueryFrameCountNV)
	lsymopt(glXResetFrameCountNV)

	lsymopt(glXGetProcAddressARB)
	lsymopt(glXGetProcAddress)

	lsymopt(glXCreateContextAttribsARB)

	// GL symbols
	lsym(glFinish)
	lsym(glFlush)
	lsym(glViewport)
	lsym(glDrawBuffer)
	lsym(glPopAttrib)
	lsym(glReadPixels)
	lsym(glDrawPixels)
	#ifdef SUNOGL
	lsym(glBegin)
	#else
	lsym(glIndexd)
	lsym(glIndexf)
	lsym(glIndexi)
	lsym(glIndexs)
	lsym(glIndexub)
	lsym(glIndexdv)
	lsym(glIndexfv)
	lsym(glIndexiv)
	lsym(glIndexsv)
	lsym(glIndexubv)
	#endif
	lsym(glClearIndex)
	lsym(glGetDoublev)
	lsym(glGetFloatv)
	lsym(glGetIntegerv)
	lsym(glMaterialfv)
	lsym(glMaterialiv)
	lsym(glPixelTransferf)
	lsym(glPixelTransferi)
	return 0;
}


static int __vgl_loadx11symbols(void *dllhnd)
{
	dlerror();  // Clear error state

	lsym(XCheckMaskEvent);
	lsym(XCheckTypedEvent);
	lsym(XCheckTypedWindowEvent);
	lsym(XCheckWindowEvent);
	lsym(XCloseDisplay);
	lsym(XConfigureWindow);
	lsym(XCopyArea);
	lsym(XCreateWindow);
	lsym(XCreateSimpleWindow);
	lsym(XDestroySubwindows);
	lsym(XDestroyWindow);
	lsym(XFree);
	lsym(XGetGeometry);
	lsym(XListExtensions);
	lsym(XMaskEvent);
	lsym(XMoveResizeWindow);
	lsym(XNextEvent);
	lsym(XOpenDisplay);
	lsym(XQueryExtension);
	lsym(XResizeWindow);
	lsym(XServerVendor);
	lsym(XWindowEvent);
	return 0;
}


void __vgl_unloadsymbols(void)
{
	if(gldllhnd) dlclose(gldllhnd);
	if(x11dllhnd) dlclose(x11dllhnd);
}
