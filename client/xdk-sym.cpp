/* Copyright (C)2006 Sun Microsystems, Inc.
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
#include "rrmutex.h"
#include "xdk-sym.h"

static HMODULE hmod=0;
static rrcs globalmutex;

#define lsym(s) tryw32(__##s=(_##s##Type)GetProcAddress(hmod, #s));

void __vgl_loadsymbols(void)
{
	rrcs::safelock l(globalmutex);
	if(hmod) return;
	tryw32(hmod=LoadLibrary("hclglx.dll"));
	lsym(glXCreateContext)
	lsym(glXDestroyContext)
	lsym(glXGetCurrentContext)
	lsym(glXGetCurrentDisplayEXT)
	lsym(glXGetCurrentDrawable)
	lsym(glXMakeCurrent)
	lsym(glXSwapBuffers)
	lsym(glDrawBuffer)
	lsym(glDrawPixels)
	lsym(glFinish)
	lsym(glGetError)
	lsym(glGetIntegerv)
	lsym(glPixelStorei)
	lsym(glRasterPos2f)
	lsym(glViewport)
}

void __vgl_unloadsymbols(void)
{
	rrcs::safelock l(globalmutex);
	if(hmod) {FreeLibrary(hmod);  hmod=0;}
}
