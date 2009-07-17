/* Copyright (C)2009 D. R. Commander
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

#include "rrplugin.h"
#include "fakerconfig.h"
#include <dlfcn.h>

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

rrplugin::rrplugin(char *name)
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
	_RRTransFrameReady=(_RRTransFrameReadyType)loadsym(dllhnd, "RRTransFrameReady");
	_RRTransSendFrame=(_RRTransSendFrameType)loadsym(dllhnd, "RRTransSendFrame");
	_RRTransDestroy=(_RRTransDestroyType)loadsym(dllhnd, "RRTransDestroy");
	_RRTransGetError=(_RRTransGetErrorType)loadsym(dllhnd, "RRTransGetError");
	if(!(handle=_RRTransInit(&fconfig))) _throw(_RRTransGetError());
}

rrplugin::~rrplugin(void)
{
	rrcs::safelock l(mutex);
	destroy();
	if(dllhnd) dlclose(dllhnd);
}

void rrplugin::connect(char *name, int port)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransConnect(handle, name, port);
	if(ret<0) _throw(_RRTransGetError());
}

void rrplugin::destroy(void)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransDestroy(handle);
	if(ret<0) _throw(_RRTransGetError());
}

int rrplugin::frameready(void)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransFrameReady(handle);
	if(ret<0) _throw(_RRTransGetError());
	return ret;
}

RRFrame *rrplugin::getframe(int width, int height, bool stereo, bool spoil)
{
	rrcs::safelock l(mutex);
	RRFrame *ret=_RRTransGetFrame(handle, width, height, stereo, spoil);
	if(!ret) _throw(_RRTransGetError());
	return ret;
}

void rrplugin::sendframe(RRFrame *frame)
{
	rrcs::safelock l(mutex);
	int ret=_RRTransSendFrame(handle, frame);
	if(ret<0) _throw(_RRTransGetError());
}
