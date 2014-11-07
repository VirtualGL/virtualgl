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

#define __LOCALSYM__
#include "faker-sym.h"
#include <dlfcn.h>
#include <string.h>
#include "fakerconfig.h"


static void *loadSym(void *dllhnd, const char *symbol, int quiet)
{
	void *sym;  const char *err;
	dlerror();  // Clear error state
	sym=dlsym(dllhnd, (char *)symbol);
	err=dlerror();
	if(err && !quiet) vglout.print("[VGL] %s\n", err);
	return sym;
}


#define LSYM(s) __##s=(_##s##Type)loadSym(dllhnd, #s, !fconfig.verbose);  \
	if(!__##s) return -1;

#define LSYMOPT(s) __##s=(_##s##Type)loadSym(dllhnd, #s, 1);

static void *gldllhnd=NULL;
static void *x11dllhnd=NULL;
static int loadGLSymbols(void *);
static int loadX11Symbols(void *);
#ifdef FAKEXCB
static int loadXCBSymbols(void *);
#endif


namespace vglfaker
{

void loadDLSymbols(void)
{
	dlerror();  // Clear error state
	__dlopen=(_dlopenType)loadSym(RTLD_NEXT, "dlopen", 0);
	if(!__dlopen)
	{
		vglout.print("[VGL] ERROR: Could not load symbol dlopen\n");
		safeExit(1);
	}
}


void loadSymbols(void)
{
	void *dllhnd;

	if(strlen(fconfig.gllib)>0)
	{
		dllhnd=_vgl_dlopen(fconfig.gllib, RTLD_NOW);
		if(!dllhnd)
		{
			vglout.print("[VGL] ERROR: Could not open %s\n[VGL]    %s\n",
				fconfig.gllib, dlerror());
			safeExit(1);
		}
		else gldllhnd=dllhnd;
	}
	else dllhnd=RTLD_NEXT;
	if(loadGLSymbols(dllhnd)<0)
	{
		if(dllhnd==RTLD_NEXT)
		{
			if(fconfig.verbose)
			{
				vglout.print("[VGL] WARNING: Could not load GLX/OpenGL symbols using RTLD_NEXT.  Attempting\n");
				vglout.print("[VGL]    to load GLX/OpenGL symbols directly from libGL.so.1.\n");
			}
			dllhnd=_vgl_dlopen("libGL.so.1", RTLD_NOW);
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open libGL.so.1\n[VGL]    %s\n",
					dlerror());
				safeExit(1);
			}
			if(loadGLSymbols(dllhnd)<0)
			{
				vglout.print("[VGL] ERROR: Could not load GLX/OpenGL symbols from libGL.so.1.\n");
				safeExit(1);
			}
			gldllhnd=dllhnd;
		}
		else
		{
			if(strlen(fconfig.gllib)>0)
				vglout.print("[VGL] ERROR: Could not load GLX/OpenGL symbols from %s.\n",
					fconfig.gllib);
			safeExit(1);
		}
	}

	if(strlen(fconfig.x11lib)>0)
	{
		dllhnd=_vgl_dlopen(fconfig.x11lib, RTLD_NOW);
		if(!dllhnd)
		{
			vglout.print("[VGL] ERROR: Could not open %s\n[VGL]    %s\n",
				fconfig.x11lib, dlerror());
			safeExit(1);
		}
		else x11dllhnd=dllhnd;
	}
	else dllhnd=RTLD_NEXT;
	if(loadX11Symbols(dllhnd)<0)
	{
		if(dllhnd==RTLD_NEXT)
		{
			if(fconfig.verbose)
			{
				vglout.print("[VGL] WARNING: Could not load X11 symbols using RTLD_NEXT.  Attempting\n");
				vglout.print("[VGL]    to load X11 symbols directly from libX11.\n");
			}
			dllhnd=_vgl_dlopen("libX11.so.4", RTLD_NOW);
			if(!dllhnd) dllhnd=_vgl_dlopen("libX11.so.5", RTLD_NOW);
			if(!dllhnd) dllhnd=_vgl_dlopen("libX11.so.6", RTLD_NOW);
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open libX11\n[VGL]    %s\n",
					dlerror());
				safeExit(1);
			}
			if(loadX11Symbols(dllhnd)<0)
			{
				vglout.print("[VGL] ERROR: Could not load X11 symbols from libX11.\n");
				safeExit(1);
			}
			x11dllhnd=dllhnd;
		}
		else
		{
			if(strlen(fconfig.x11lib)>0)
				vglout.print("[VGL] ERROR: Could not load X11 symbols from %s.\n",
					fconfig.x11lib);
			safeExit(1);
		}
	}

	#ifdef FAKEXCB
	if(loadXCBSymbols(RTLD_NEXT)<0)
	{
		vglout.print("[VGL] ERROR: Could not load XCB symbols from libxcb.\n");
		safeExit(1);
	}
	#endif
}

} // namespace


static int loadGLSymbols(void *dllhnd)
{
	dlerror();  // Clear error state

	// GLX symbols
	LSYM(glXChooseVisual)
	LSYM(glXCopyContext)
	LSYM(glXCreateContext)
	LSYM(glXCreateGLXPixmap)
	LSYM(glXDestroyContext)
	LSYM(glXDestroyGLXPixmap)
	LSYM(glXGetConfig)
	LSYM(glXGetCurrentDrawable)
	LSYM(glXIsDirect)
	LSYM(glXMakeCurrent);
	LSYM(glXQueryExtension)
	LSYM(glXQueryVersion)
	LSYM(glXSwapBuffers)
	LSYM(glXUseXFont)
	LSYM(glXWaitGL)

	LSYM(glXGetClientString)
	LSYM(glXQueryServerString)
	LSYM(glXQueryExtensionsString)

	LSYM(glXChooseFBConfig)
	LSYM(glXCreateNewContext)
	LSYM(glXCreatePbuffer)
	LSYM(glXCreatePixmap)
	LSYM(glXCreateWindow)
	LSYM(glXDestroyPbuffer)
	LSYM(glXDestroyPixmap)
	LSYM(glXDestroyWindow)
	LSYM(glXGetCurrentDisplay)
	LSYM(glXGetCurrentReadDrawable)
	LSYM(glXGetFBConfigAttrib)
	LSYM(glXGetFBConfigs)
	LSYM(glXGetSelectedEvent)
	LSYM(glXGetVisualFromFBConfig)
	LSYM(glXMakeContextCurrent);
	LSYM(glXQueryContext)
	LSYM(glXQueryDrawable)
	LSYM(glXSelectEvent)

	// Optional extensions.  We fake these if they exist
	LSYMOPT(glXFreeContextEXT)
	LSYMOPT(glXImportContextEXT)
	LSYMOPT(glXQueryContextInfoEXT)

	LSYMOPT(glXJoinSwapGroupNV)
	LSYMOPT(glXBindSwapBarrierNV)
	LSYMOPT(glXQuerySwapGroupNV)
	LSYMOPT(glXQueryMaxSwapGroupsNV)
	LSYMOPT(glXQueryFrameCountNV)
	LSYMOPT(glXResetFrameCountNV)

	LSYMOPT(glXGetProcAddressARB)
	LSYMOPT(glXGetProcAddress)

	LSYMOPT(glXCreateContextAttribsARB)

	LSYMOPT(glXBindTexImageEXT)
	LSYMOPT(glXReleaseTexImageEXT)
	// For some reason, the ATI implementation of libGL on Ubuntu (and maybe
	// other platforms as well) does not export glXBindTexImageEXT and
	// glXReleaseTexImageEXT, but those symbols can be obtained through
	// glXGetProcAddressARB().
	if(!__glXBindTexImageEXT && __glXGetProcAddressARB)
		__glXBindTexImageEXT=
			(_glXBindTexImageEXTType)__glXGetProcAddressARB((const GLubyte *)"glXBindTexImageEXT");
	if(!__glXReleaseTexImageEXT && __glXGetProcAddressARB)
		__glXReleaseTexImageEXT=
			(_glXReleaseTexImageEXTType)__glXGetProcAddressARB((const GLubyte *)"glXReleaseTexImageEXT");

	LSYMOPT(glXSwapIntervalEXT)
	LSYMOPT(glXSwapIntervalSGI)

	// GL symbols
	LSYM(glFinish)
	LSYM(glFlush)
	LSYM(glViewport)
	LSYM(glDrawBuffer)
	LSYM(glPopAttrib)
	LSYM(glReadPixels)
	LSYM(glDrawPixels)
	LSYM(glIndexd)
	LSYM(glIndexf)
	LSYM(glIndexi)
	LSYM(glIndexs)
	LSYM(glIndexub)
	LSYM(glIndexdv)
	LSYM(glIndexfv)
	LSYM(glIndexiv)
	LSYM(glIndexsv)
	LSYM(glIndexubv)
	LSYM(glClearIndex)
	LSYM(glGetDoublev)
	LSYM(glGetFloatv)
	LSYM(glGetIntegerv)
	LSYM(glMaterialfv)
	LSYM(glMaterialiv)
	LSYM(glPixelTransferf)
	LSYM(glPixelTransferi)
	return 0;
}


static int loadX11Symbols(void *dllhnd)
{
	dlerror();  // Clear error state

	LSYM(XCheckMaskEvent);
	LSYM(XCheckTypedEvent);
	LSYM(XCheckTypedWindowEvent);
	LSYM(XCheckWindowEvent);
	LSYM(XCloseDisplay);
	LSYM(XConfigureWindow);
	LSYM(XCopyArea);
	LSYM(XCreateWindow);
	LSYM(XCreateSimpleWindow);
	LSYM(XDestroySubwindows);
	LSYM(XDestroyWindow);
	LSYM(XFree);
	LSYM(XGetGeometry);
	LSYM(XGetImage);
	LSYM(XListExtensions);
	LSYM(XMaskEvent);
	LSYM(XMoveResizeWindow);
	LSYM(XNextEvent);
	LSYM(XOpenDisplay);
	LSYM(XQueryExtension);
	LSYM(XResizeWindow);
	LSYM(XServerVendor);
	LSYM(XWindowEvent);
	return 0;
}


#ifdef FAKEXCB

static int loadXCBSymbols(void *dllhnd)
{
	dlerror();  // Clear error state

	LSYM(xcb_get_extension_data);
	LSYM(xcb_glx_query_version);
	LSYM(xcb_glx_query_version_reply);
	LSYM(xcb_poll_for_event);
	LSYMOPT(xcb_poll_for_queued_event);
	LSYM(xcb_wait_for_event);
	return 0;
}

#endif


namespace vglfaker {

void unloadSymbols(void)
{
	if(gldllhnd) dlclose(gldllhnd);
	if(x11dllhnd) dlclose(x11dllhnd);
}

}
