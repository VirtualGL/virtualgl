/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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
	if(!sym && dllhnd==RTLD_NEXT) {dlerror();  sym=dlsym(RTLD_DEFAULT, (char *)symbol);}
	err=dlerror();	if(err) {if(!quiet) rrout.print("[VGL] ERROR: Could not load symbol %s:\n[VGL]    %s\n", symbol, err);}
	return sym;
}

#define lsym(s) __##s=(_##s##Type)loadsym(dllhnd, #s, 0);  if(!__##s) {  \
	rrout.print("[VGL] ERROR: Could not load symbol %s\n", #s);  __vgl_safeexit(1);}
#define lsymopt(s) __##s=(_##s##Type)loadsym(dllhnd, #s, 1);

static void *gldllhnd=NULL;
static void *x11dllhnd=NULL;

void __vgl_loadsymbols(void)
{
	void *dllhnd;
	dlerror();  // Clear error state

	if(fconfig.gllib)
	{
		dllhnd=dlopen(fconfig.gllib, RTLD_NOW);
		if(!dllhnd)
		{
			rrout.print("[VGL] ERROR: Could not open %s\n[VGL]    %s\n",
				(char *)fconfig.gllib, dlerror());
			__vgl_safeexit(1);
		}
		else gldllhnd=dllhnd;
	}
	else dllhnd=RTLD_NEXT;

	// GLX symbols
	lsym(glXChooseVisual)
	lsym(glXCopyContext)
	lsym(glXCreateContext)
	lsym(glXCreateGLXPixmap)
	lsym(glXDestroyContext)
	lsym(glXDestroyGLXPixmap)
	lsym(glXGetConfig)
	#ifdef USEGLP
	lsym(glXGetCurrentContext)
	#endif
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

	#ifdef USEGLP
	lsymopt(glPSwapBuffers)
	#endif

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

	// X11 symbols
	if(fconfig.x11lib)
	{
		dllhnd=dlopen(fconfig.x11lib, RTLD_NOW);
		if(!dllhnd)
		{
			rrout.print("[VGL] ERROR: Could not open %s\n[VGL]    %s\n",
				(char *)fconfig.x11lib, dlerror());
			__vgl_safeexit(1);
		}
		else x11dllhnd=dllhnd;
	}
	else dllhnd=RTLD_NEXT;

	lsym(XCheckMaskEvent);
	lsym(XCheckTypedEvent);
	lsym(XCheckTypedWindowEvent);
	lsym(XCheckWindowEvent);
	lsym(XConfigureWindow);
	lsym(XCopyArea);
	lsym(XCreateWindow);
	lsym(XCreateSimpleWindow);
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
}

void __vgl_unloadsymbols(void)
{
	if(gldllhnd) dlclose(gldllhnd);
	if(x11dllhnd) dlclose(x11dllhnd);
}
