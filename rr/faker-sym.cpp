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

#define __LOCALSYM__
#include "faker-sym.h"
#include <dlfcn.h>

#include "fakerconfig.h"
extern FakerConfig fconfig;

void *loadsym(void *dllhnd, const char *symbol, int quiet)
{
	void *sym;  const char *err;
	sym=dlsym(dllhnd, (char *)symbol);
	err=dlerror();	if(err) {if(!quiet) fprintf(stderr, "%s\n", err);  return NULL;}
	return sym;
}

#define lsym(s) __##s=(_##s##Type)loadsym(dllhnd, #s, 0);  if(!__##s) {  \
	fprintf(stderr, "Could not load symbol %s\n", #s);  safeexit(1);}
#define lsymopt(s) __##s=(_##s##Type)loadsym(dllhnd, #s, 1);

void loadsymbols(void)
{
	void *dllhnd;
	dlerror();  // Clear error state

	if(fconfig.gllib)
	{
		dllhnd=dlopen(fconfig.gllib, RTLD_NOW);
		if(!dllhnd)
		{
			fprintf(stderr, "Could not open %s\n%s\n", fconfig.gllib, dlerror());
			safeexit(1);
		}
	}
	else dllhnd=RTLD_NEXT;

	// GLX symbols
	lsym(glXChooseVisual)
	lsym(glXCopyContext)
	lsym(glXCreateContext)
	lsym(glXCreateGLXPixmap)
	lsym(glXDestroyContext)
	lsym(glXDestroyGLXPixmap)
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

	lsymopt(glXGetFBConfigAttribSGIX)
	lsymopt(glXChooseFBConfigSGIX)

	lsymopt(glXCreateGLXPbufferSGIX)
	lsymopt(glXDestroyGLXPbufferSGIX)
	lsymopt(glXQueryGLXPbufferSGIX)
	lsymopt(glXSelectEventSGIX)
	lsymopt(glXGetSelectedEventSGIX)

	#ifdef sun
	lsymopt(glXVideoResizeSUN)
	lsymopt(glXGetVideoResizeSUN)
	lsymopt(glXDisableXineramaSUN)
	#endif

	// GL symbols
	lsym(glFinish)
	lsym(glFlush)
	lsym(glViewport)

	if(dllhnd!=RTLD_NEXT) dlclose(dllhnd);

	// X11 symbols
	if(fconfig.x11lib)
	{
		dllhnd=dlopen(fconfig.x11lib, RTLD_NOW);
		if(!dllhnd)
		{
			fprintf(stderr, "Could not open %s\n%s\n", fconfig.x11lib, dlerror());
			safeexit(1);
		}
	}
	else dllhnd=RTLD_NEXT;

	lsym(XCheckMaskEvent);
	lsym(XCheckTypedEvent);
	lsym(XCheckTypedWindowEvent);
	lsym(XCheckWindowEvent);
	lsym(XCloseDisplay);
	lsym(XConfigureWindow);
	lsym(XCopyArea);
	lsym(XCreateWindow);
	lsym(XCreateSimpleWindow);
	lsym(XDestroyWindow);
	lsym(XMaskEvent);
	lsym(XMoveResizeWindow);
	lsym(XNextEvent);
	lsym(XOpenDisplay);
	lsym(XResizeWindow);
	lsym(XWindowEvent);

	if(dllhnd!=RTLD_NEXT) dlclose(dllhnd);
}
