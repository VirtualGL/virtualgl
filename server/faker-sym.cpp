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

#define __LOCALSYM__
#include "faker-sym.h"
#include <dlfcn.h>
#include <string.h>
#include "fakerconfig.h"


static void *gldllhnd=NULL;
static void *loadGLSymbol(const char *);
static void *x11dllhnd=NULL;
static void *loadX11Symbol(const char *);
#ifdef FAKEXCB
static void *xcbdllhnd=NULL;
static void *loadXCBSymbol(const char *);
static void *xcbatomdllhnd=NULL;
static void *loadXCBAtomSymbol(const char *);
static void *xcbglxdllhnd=NULL;
static void *loadXCBGLXSymbol(const char *);
static void *xcbkeysymsdllhnd=NULL;
static void *loadXCBKeysymsSymbol(const char *);
static void *xcbx11dllhnd=NULL;
static void *loadXCBX11Symbol(const char *);
#endif


namespace vglfaker
{

void *loadSymbol(const char *name)
{
	if(!name)
	{
		vglout.print("[VGL] ERROR: Invalid argument in loadSymbol()\n");
		safeExit(1);
	}
	if(!strncmp(name, "gl", 2))
		return loadGLSymbol(name);
	#ifdef FAKEXCB
	else if(!strcmp(name, "XGetXCBConnection"))
		return loadXCBX11Symbol(name);
	#endif
	else if(!strncmp(name, "X", 1))
		return loadX11Symbol(name);
	#ifdef FAKEXCB
	else if(!strncmp(name, "xcb_intern_atom", 15))
		return loadXCBAtomSymbol(name);
	else if(!strncmp(name, "xcb_glx", 7))
		return loadXCBGLXSymbol(name);
	else if(!strncmp(name, "xcb_key", 7))
		return loadXCBKeysymsSymbol(name);
	else if(!strncmp(name, "xcb_", 4))
		return loadXCBSymbol(name);
	#endif
	else
	{
		vglout.print("[VGL] ERROR: don't know how to load symbol \"%s\"\n", name);
		return NULL;
	}
}

} // namespace


static void *loadGLSymbol(const char *name)
{
	char *err=NULL;

	if(!__glXGetProcAddress)
	{
		if(strlen(fconfig.gllib)>0)
		{
			dlerror();  // Clear error state
			void *dllhnd=_vgl_dlopen(fconfig.gllib, RTLD_LAZY);
			err=dlerror();
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.gllib);
				if(err) vglout.print("[VGL]    %s\n", err);
				return NULL;
			}
			gldllhnd=dllhnd;
		}
		else gldllhnd=RTLD_NEXT;

		dlerror();  // Clear error state
		__glXGetProcAddress=(_glXGetProcAddressType)dlsym(gldllhnd,
			"glXGetProcAddress");
		if(!__glXGetProcAddress)
			__glXGetProcAddress=(_glXGetProcAddressType)dlsym(gldllhnd,
				"glXGetProcAddressARB");
		err=dlerror();

		if(!__glXGetProcAddress)
		{
			vglout.print("[VGL] ERROR: Could not load GLX/OpenGL functions");
			if(strlen(fconfig.gllib)>0)
				vglout.print(" from %s", fconfig.gllib);
			vglout.print("\n");
			if(err) vglout.print("[VGL]    %s\n", err);
			return NULL;
		}
	}

	void *sym=NULL;
	if(!strcmp(name, "glXGetProcAddress") ||
		!strcmp(name, "glXGetProcAddressARB"))
		sym=(void *)__glXGetProcAddress;
	else
		sym=(void *)__glXGetProcAddress((const GLubyte *)name);
	if(!sym)
	{
		vglout.print("[VGL] ERROR: Could not load function \"%s\"", name);
		if(strlen(fconfig.gllib)>0)
			vglout.print(" from %s", fconfig.gllib);
	}
	return sym;
}


static void *loadX11Symbol(const char *name)
{
	char *err=NULL;

	if(!x11dllhnd)
	{
		if(strlen(fconfig.x11lib)>0)
		{
			dlerror();  // Clear error state
			void *dllhnd=_vgl_dlopen(fconfig.x11lib, RTLD_LAZY);
			err=dlerror();
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.x11lib);
				if(err) vglout.print("[VGL]    %s\n", err);
				return NULL;
			}
			x11dllhnd=dllhnd;
		}
		else x11dllhnd=RTLD_NEXT;
	}

	dlerror();  // Clear error state
	void *sym=dlsym(x11dllhnd, (char *)name);
	err=dlerror();

	if(!sym)
	{
		vglout.print("[VGL] ERROR: Could not load function \"%s\"", name);
		if(strlen(fconfig.x11lib)>0)
			vglout.print(" from %s", fconfig.x11lib);
		vglout.print("\n");
		if(err) vglout.print("[VGL]    %s\n", err);
	}
	return sym;
}


#ifdef FAKEXCB

#define LOAD_XCB_SYMBOL(ID, id, libid, minrev, maxrev)  \
static void *load##ID##Symbol(const char *name)  \
{  \
	char *err=NULL;  \
  \
	if(!id##dllhnd)  \
	{  \
		if(strlen(fconfig.id##lib)>0)  \
		{  \
			dlerror();  \
			void *dllhnd=_vgl_dlopen(fconfig.id##lib, RTLD_LAZY);  \
			err=dlerror();  \
			if(!dllhnd)  \
			{  \
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.id##lib);  \
				if(err) vglout.print("[VGL]    %s\n", err);  \
				return NULL;  \
			}  \
			id##dllhnd=dllhnd;  \
		}  \
		else  \
		{  \
			void *dllhnd=NULL;  \
			for(int i=minrev; i<=maxrev; i++)  \
			{  \
				char libName[MAXSTR];  \
				snprintf(libName, MAXSTR, "lib%s.so.%d", #libid, i);  \
				dlerror();  \
				dllhnd=_vgl_dlopen(libName, RTLD_LAZY);  \
				err=dlerror();  \
				if(dllhnd) break;  \
			}  \
			if(!dllhnd)  \
			{  \
				vglout.print("[VGL] ERROR: Could not open lib%s\n", #libid);  \
				if(err) vglout.print("[VGL]    %s\n", err);  \
				return NULL;  \
			}  \
			id##dllhnd=dllhnd;  \
		}  \
	}  \
  \
	dlerror();  \
	void *sym=dlsym(id##dllhnd, (char *)name);  \
	err=dlerror();  \
  \
	if(!sym)  \
	{  \
		vglout.print("[VGL] ERROR: Could not load symbol \"%s\"", name);  \
		if(strlen(fconfig.id##lib)>0)  \
			vglout.print(" from %s", fconfig.id##lib);  \
		vglout.print("\n");  \
		if(err) vglout.print("[VGL]    %s\n", err);  \
	}  \
	return sym;  \
}

LOAD_XCB_SYMBOL(XCB, xcb, xcb, 1, 1)
LOAD_XCB_SYMBOL(XCBAtom, xcbatom, xcb-atom, 0, 1)
LOAD_XCB_SYMBOL(XCBGLX, xcbglx, xcb-glx, 0, 0);
LOAD_XCB_SYMBOL(XCBKeysyms, xcbkeysyms, xcb-keysyms, 0, 1)
LOAD_XCB_SYMBOL(XCBX11, xcbx11, X11-xcb, 1, 1)

#endif


namespace vglfaker {

void unloadSymbols(void)
{
	if(gldllhnd && gldllhnd!=RTLD_NEXT) dlclose(gldllhnd);
	if(x11dllhnd && x11dllhnd!=RTLD_NEXT) dlclose(x11dllhnd);
	#ifdef FAKEXCB
	if(xcbdllhnd) dlclose(xcbdllhnd);
	if(xcbatomdllhnd) dlclose(xcbatomdllhnd);
	if(xcbglxdllhnd) dlclose(xcbglxdllhnd);
	if(xcbkeysymsdllhnd) dlclose(xcbkeysymsdllhnd);
	if(xcbx11dllhnd) dlclose(xcbx11dllhnd);
	#endif
}

}
