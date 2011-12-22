/* Copyright (C)2009-2011 D. R. Commander
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

#include "transplugin.h"
#include "fakerconfig.h"
#include <dlfcn.h>

#undef _throw
#define _throw(m) throw(rrerror("transport plugin", m, -1))

static void *loadsym(void *dllhnd, const char *symbol)
{
	void *sym=NULL;  const char *err=NULL;
	sym=dlsym(dllhnd, (char *)symbol);
	if(!sym)
	{
		err=dlerror();
		if(err) _throw(err);
		else _throw("Could not load symbol");
	}
	return sym;
}

transplugin::transplugin(Display *dpy, Window win, char *name)
{
	if(!name || strlen(name)<1) _throw("Transport name is empty or NULL!");
	const char *err=NULL;
	rrcs::safelock l(mutex);
	dlerror();  // Clear error state
	char filename[MAXSTR];
	snprintf(filename, MAXSTR-1, "libvgltrans_%s.so", name);
	dllhnd=dlopen(filename, RTLD_NOW);
	if(!dllhnd)
	{
		err=dlerror();
		if(err) _throw(err);
		else _throw("Could not open transport plugin");
	}
	_RRTransInit=(_RRTransInitType)loadsym(dllhnd, "RRTransInit");
	_RRTransConnect=(_RRTransConnectType)loadsym(dllhnd, "RRTransConnect");
	_RRTransGetFrame=(_RRTransGetFrameType)loadsym(dllhnd, "RRTransGetFrame");
	_RRTransReady=(_RRTransReadyType)loadsym(dllhnd, "RRTransReady");
	_RRTransSynchronize=(_RRTransSynchronizeType)loadsym(dllhnd, "RRTransSynchronize");
	_RRTransSendFrame=(_RRTransSendFrameType)loadsym(dllhnd, "RRTransSendFrame");
	_RRTransDestroy=(_RRTransDestroyType)loadsym(dllhnd, "RRTransDestroy");
	_RRTransGetError=(_RRTransGetErrorType)loadsym(dllhnd, "RRTransGetError");
	if(!(handle=_RRTransInit(dpy, win, &fconfig))) _throw(_RRTransGetError());
}

transplugin::~transplugin(void)
{
	rrcs::safelock l(mutex);
	destroy();
	if(dllhnd) dlclose(dllhnd);
}

void transplugin::connect(char *name, int port)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransConnect(handle, name, port);
	if(ret<0) _throw(_RRTransGetError());
}

void transplugin::destroy(void)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransDestroy(handle);
	if(ret<0) _throw(_RRTransGetError());
}

int transplugin::ready(void)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransReady(handle);
	if(ret<0) _throw(_RRTransGetError());
	return ret;
}

void transplugin::synchronize(void)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransSynchronize(handle);
	if(ret<0) _throw(_RRTransGetError());
}

RRFrame *transplugin::getframe(int width, int height, int format, bool stereo)
{
	rrcs::safelock l(mutex);
	RRFrame *ret=_RRTransGetFrame(handle, width, height, format, stereo);
	if(!ret) _throw(_RRTransGetError());
	return ret;
}

void transplugin::sendframe(RRFrame *frame, bool sync)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransSendFrame(handle, frame, sync);
	if(ret<0) _throw(_RRTransGetError());
}
