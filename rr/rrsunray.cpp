/* Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#include <dlfcn.h>
#include "rrsunray.h"
#include "rrmutex.h"
#include "rrerror.h"
#include "fakerconfig.h"
#include "rrlog.h"

static int RRSunRayConfigCallback(const char *name, int type, void *value)
{
	if(!name || !value || type<0) return -1;
	if(!stricmp(name, "VGL_CLIENT") && type==RRSUNRAY_CONFIGSTR)
		{*((char **)value)=(char *)fconfig.client;  return 0;}
	else if(!stricmp(name, "VGL_COMPRESS") && type==RRSUNRAY_CONFIGINT)
		{*((int *)value)=(int)fconfig.compress();  return 0;}
	else if(!stricmp(name, "VGL_INTERFRAME") && type==RRSUNRAY_CONFIGINT)
		{*((int *)value)=(int)fconfig.interframe;  return 0;}
	else if(!stricmp(name, "VGL_PROGRESSIVE") && type==RRSUNRAY_CONFIGINT)
		{*((int *)value)=(int)fconfig.progressive;  return 0;}
	else if(!stricmp(name, "VGL_SPOIL") && type==RRSUNRAY_CONFIGINT)
		{*((int *)value)=(int)fconfig.spoil;  return 0;}
	else if(!stricmp(name, "VGL_SUBSAMP") && type==RRSUNRAY_CONFIGINT)
		{*((int *)value)=(int)fconfig.subsamp;  return 0;}
	else if(!stricmp(name, "VGL_VERBOSE") && type==RRSUNRAY_CONFIGINT)
		{*((int *)value)=(int)fconfig.verbose;  return 0;}
	return -1;
}

static void *loadsym(void *dllhnd, const char *symbol, bool fatal)
{
	void *sym=NULL;  const char *err=NULL;
	sym=dlsym(dllhnd, (char *)symbol);
	if(!sym)
	{
		if(fatal)
		{
			err=dlerror();
			if(err) _throw(err);  else throw(rrerror(symbol, "Could not load symbol"));
		}
	}
	return sym;
}

#define lsym(s) { \
	_##s=(_##s##Type)loadsym(dllhnd, #s, fatal); \
	if(!_##s) { \
		if(fatal) _throw("Could not load symbol "#s); \
		else if(fconfig.verbose) \
			rrout.print("[VGL] Could not load Sun Ray plugin symbol "#s"\n"); \
		return 0; \
	} \
}

#define lsymopt(s) { \
	_##s=(_##s##Type)loadsym(dllhnd, #s, fatal); \
	if(!_##s && fconfig.verbose) \
	{ \
		rrout.print("[VGL] Could not load Sun Ray plugin symbol "#s"\n"); \
	} \
}

static rrcs sunraymutex;

extern "C" {

typedef int (*_RRSunRayQueryDisplayType)(Display *);
static _RRSunRayQueryDisplayType _RRSunRayQueryDisplay=NULL;

typedef void* (*_RRSunRayInitType)(Display *, Window);
static _RRSunRayInitType _RRSunRayInit=NULL;

typedef unsigned char* (*_RRSunRayGetFrameType)(void *, int, int, int *, int *,
	int *);
static _RRSunRayGetFrameType _RRSunRayGetFrame=NULL;

typedef int (*_RRSunRaySendFrameType)(void *, unsigned char *, int, int, int,
	int, int);
static _RRSunRaySendFrameType _RRSunRaySendFrame=NULL;

typedef const char* (*_RRSunRayGetErrorType)(void *);
static _RRSunRayGetErrorType _RRSunRayGetError=NULL;

typedef int (*_RRSunRayDestroyType)(void *);
static _RRSunRayDestroyType _RRSunRayDestroy=NULL;

typedef int (*_RRSunRayFrameReadyType)(void *);
static _RRSunRayFrameReadyType _RRSunRayFrameReady=NULL;

typedef void (*_RRSunRaySetConfigCallbackType)(_RRSunRayConfigCallbackType);
static _RRSunRaySetConfigCallbackType _RRSunRaySetConfigCallback=NULL;

static bool init=false;

// 1=success, 0=failure
static int loadsunraysymbols(bool fatal)
{
	void *dllhnd=NULL;  const char *err=NULL;
	rrcs::safelock l(sunraymutex);
	if(init) return 1;
	dlerror();  // Clear error state
	dllhnd=dlopen("librrsunray.so", RTLD_NOW);
	if(!dllhnd)
	{
		if(fatal)
		{
			err=dlerror();  if(err) _throw(err);
			else _throw("Could not find SunRay plugin");
		}
		else
		{
			if(fconfig.verbose)
			{
				err=dlerror();  if(err) rrout.print("[VGL] %s\n", err);
			}
			return 0;
		}
	}
	lsym(RRSunRayQueryDisplay)
	lsym(RRSunRayInit)
	lsym(RRSunRayGetFrame)
	lsym(RRSunRaySendFrame)
	lsym(RRSunRayGetError)
	lsymopt(RRSunRayDestroy)
	lsymopt(RRSunRayFrameReady)
	lsymopt(RRSunRaySetConfigCallback)
	if(_RRSunRaySetConfigCallback)
		_RRSunRaySetConfigCallback(RRSunRayConfigCallback);
	init=true;
	return 1;
}

int RRSunRayQueryDisplay(Display *display)
{
	if(!_RRSunRayQueryDisplay) {if(!loadsunraysymbols(false)) return 0;}
	return _RRSunRayQueryDisplay(display);
}

void *RRSunRayInit(Display *display, Window win)
{
	if(!_RRSunRayInit) {if(!loadsunraysymbols(true)) return NULL;}
	return _RRSunRayInit(display, win);
}

unsigned char *RRSunRayGetFrame(void *handle, int width, int height,
	int *pitch, int *format, int *bottomup)
{
	if(!_RRSunRayGetFrame) {if(!loadsunraysymbols(true)) return NULL;}
	return _RRSunRayGetFrame(handle, width, height, pitch, format, bottomup);
}

int RRSunRaySendFrame(void *handle, unsigned char *bitmap, int width,
	int height, int pitch, int format, int bottomup)
{
	if(!_RRSunRaySendFrame) {if(!loadsunraysymbols(true)) return -1;}
	return _RRSunRaySendFrame(handle, bitmap, width, height, pitch, format,
		bottomup);
}

const char *RRSunRayGetError(void *handle)
{
	if(!_RRSunRayGetError)
	{
		if(!loadsunraysymbols(true))
			return "SunRay plugin not properly initialized";
	}
	return _RRSunRayGetError(handle);
}

int RRSunRayDestroy(void* handle)
{
	if(!_RRSunRayDestroy) {if(!loadsunraysymbols(true)) return -1;}
	return _RRSunRayDestroy(handle);
}

int RRSunRayFrameReady(void *handle)
{
	if(!_RRSunRayFrameReady) return 1;
	return _RRSunRayFrameReady(handle);
}

}
