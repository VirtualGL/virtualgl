// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2009, 2011, 2013-2016, 2019-2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#define __LOCALSYM__
#include "faker-sym.h"
#include <dlfcn.h>
#include <string.h>
#include "fakerconfig.h"


static void *gldllhnd = NULL;
static void *loadGLSymbol(const char *, bool);
#ifdef EGLBACKEND
static void *egldllhnd = NULL;
static void *loadEGLSymbol(const char *, bool);
#endif
#ifdef FAKEOPENCL
static void *ocldllhnd = NULL;
static void *loadOCLSymbol(const char *, bool);
#endif
static void *x11dllhnd = NULL;
static void *loadX11Symbol(const char *, bool);
#ifdef FAKEXCB
static void *xcbdllhnd = NULL;
static void *loadXCBSymbol(const char *, bool);
static void *xcbglxdllhnd = NULL;
static void *loadXCBGLXSymbol(const char *, bool);
static void *xcbkeysymsdllhnd = NULL;
static void *loadXCBKeysymsSymbol(const char *, bool);
static void *xcbx11dllhnd = NULL;
static void *loadXCBX11Symbol(const char *, bool);
#endif


// Attempt to load the glXGetProcAddress[ARB]() function.  This also checks
// whether dlsym() is returning our interposed version of
// glXGetProcAddress[ARB]() instead of the "real" function from libGL.  If so,
// then it's probably because another DSO in the process is interposing dlsym()
// (I'm looking at you, Steam.)
#define FIND_GLXGETPROCADDRESS(f) \
{ \
	__glXGetProcAddress = (_##f##Type)dlsym(gldllhnd, #f); \
	if(__glXGetProcAddress == f) \
	{ \
		vglout.print("[VGL] ERROR: VirtualGL attempted to load the real " #f " function\n"); \
		vglout.print("[VGL]   and got the fake one instead.  Something is terribly wrong.  Aborting\n"); \
		vglout.print("[VGL]   before chaos ensues.\n"); \
		faker::safeExit(1); \
	} \
}


namespace faker {

void *loadSymbol(const char *name, bool optional)
{
	if(!name)
	{
		vglout.print("[VGL] ERROR: Invalid argument in loadSymbol()\n");
		safeExit(1);
	}
	if(!strncmp(name, "gl", 2))
		return loadGLSymbol(name, optional);
	#ifdef EGLBACKEND
	else if(!strncmp(name, "egl", 3))
		return loadEGLSymbol(name, optional);
	#endif
	#ifdef FAKEOPENCL
	else if(!strncmp(name, "cl", 2))
		return loadOCLSymbol(name, optional);
	#endif
	#ifdef FAKEXCB
	else if(!strcmp(name, "XGetXCBConnection")
		|| !strcmp(name, "XSetEventQueueOwner"))
		return loadXCBX11Symbol(name, optional);
	#endif
	else if(!strncmp(name, "X", 1))
		return loadX11Symbol(name, optional);
	#ifdef FAKEXCB
	else if(!strncmp(name, "xcb_glx", 7))
		return loadXCBGLXSymbol(name, optional);
	else if(!strncmp(name, "xcb_key", 7))
		return loadXCBKeysymsSymbol(name, optional);
	else if(!strncmp(name, "xcb_", 4))
		return loadXCBSymbol(name, optional);
	#endif
	else
	{
		vglout.print("[VGL] ERROR: don't know how to load symbol \"%s\"\n",
			name);
		return NULL;
	}
}

}  // namespace


static void *loadGLSymbol(const char *name, bool optional)
{
	char *err = NULL;

	if(!__glXGetProcAddress)
	{
		if(strlen(fconfig.gllib) > 0)
		{
			dlerror();  // Clear error state
			void *dllhnd = _vgl_dlopen(fconfig.gllib, RTLD_LAZY);
			err = dlerror();
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.gllib);
				if(err) vglout.print("[VGL]    %s\n", err);
				return NULL;
			}
			gldllhnd = dllhnd;
		}
		else gldllhnd = RTLD_NEXT;

		dlerror();  // Clear error state
		FIND_GLXGETPROCADDRESS(glXGetProcAddress)
		if(!__glXGetProcAddress)
			FIND_GLXGETPROCADDRESS(glXGetProcAddressARB)
		err = dlerror();

		if(!__glXGetProcAddress)
		{
			vglout.print("[VGL] ERROR: Could not load GLX/OpenGL functions");
			if(strlen(fconfig.gllib) > 0)
				vglout.print(" from %s", fconfig.gllib);
			vglout.print("\n");
			if(err) vglout.print("[VGL]    %s\n", err);
			return NULL;
		}
	}

	void *sym = NULL;
	if(!strcmp(name, "glXGetProcAddress")
		|| !strcmp(name, "glXGetProcAddressARB"))
		sym = (void *)__glXGetProcAddress;
	else
	{
		// For whatever reason, on Solaris, if a function doesn't exist in libGL,
		// glXGetProcAddress() will return the address of VGL's interposed
		// version, which causes an infinite loop until the program blows its stack
		// and segfaults.  Thus, we use the old reliable dlsym() method by default.
		// On Linux and FreeBSD, we use glXGetProcAddress[ARB]() by default, to
		// work around issues with certain drivers, but because Steam's
		// gameoverlayrenderer.so interposer causes a similar problem to the
		// aforementioned issue on Solaris, we allow the GL symbol loading method
		// to be controlled at run time using an environment variable (VGL_DLSYM).
		if(fconfig.dlsymloader)
		{
			dlerror();  // Clear error state
			sym = dlsym(gldllhnd, (char *)name);
			err = dlerror();
		}
		else
		{
			sym = (void *)__glXGetProcAddress((const GLubyte *)name);
		}
	}

	if(!sym && (fconfig.verbose || !optional))
	{
		vglout.print("[VGL] %s: Could not load function \"%s\"",
			optional ? "WARNING" : "ERROR", name);
		if(strlen(fconfig.gllib) > 0)
			vglout.print(" from %s", fconfig.gllib);
		vglout.print("\n");
	}
	return sym;
}


#ifdef EGLBACKEND

static void *loadEGLSymbol(const char *name, bool optional)
{
	char *err = NULL;

	if(!__eglGetProcAddress)
	{
		if(strlen(fconfig.egllib) > 0)
		{
			dlerror();  // Clear error state
			void *dllhnd = _vgl_dlopen(fconfig.egllib, RTLD_LAZY);
			err = dlerror();
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.egllib);
				if(err) vglout.print("[VGL]    %s\n", err);
				return NULL;
			}
			egldllhnd = dllhnd;
		}
		else egldllhnd = RTLD_NEXT;

		dlerror();  // Clear error state
		__eglGetProcAddress =
			(_eglGetProcAddressType)dlsym(egldllhnd, "eglGetProcAddress");
		err = dlerror();

		if(!__eglGetProcAddress)
		{
			vglout.print("[VGL] ERROR: Could not load EGL functions");
			if(strlen(fconfig.egllib) > 0)
				vglout.print(" from %s", fconfig.egllib);
			vglout.print("\n");
			if(err) vglout.print("[VGL]    %s\n", err);
			return NULL;
		}
	}

	void *sym = NULL;
	if(!strcmp(name, "eglGetProcAddress"))
		sym = (void *)__eglGetProcAddress;
	else
	{
		if(fconfig.dlsymloader)
		{
			dlerror();  // Clear error state
			sym = dlsym(egldllhnd, (char *)name);
			err = dlerror();
		}
		else
		{
			sym = (void *)__eglGetProcAddress(name);
		}
	}

	if(!sym && (fconfig.verbose || !optional))
	{
		vglout.print("[VGL] %s: Could not load function \"%s\"",
			optional ? "WARNING" : "ERROR", name);
		if(strlen(fconfig.egllib) > 0)
			vglout.print(" from %s", fconfig.egllib);
		vglout.print("\n");
	}
	return sym;
}

#endif


#ifdef FAKEOPENCL

static void *loadOCLSymbol(const char *name, bool optional)
{
	char *err = NULL;

	if(!ocldllhnd)
	{
		if(strlen(fconfig.ocllib) > 0)
		{
			dlerror();  // Clear error state
			void *dllhnd = _vgl_dlopen(fconfig.ocllib, RTLD_LAZY);
			err = dlerror();
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.ocllib);
				if(err) vglout.print("[VGL]    %s\n", err);
				return NULL;
			}
			ocldllhnd = dllhnd;
		}
		else ocldllhnd = RTLD_NEXT;
	}

	dlerror();  // Clear error state
	void *sym = dlsym(ocldllhnd, (char *)name);
	err = dlerror();

	if(!sym && (fconfig.verbose || !optional))
	{
		vglout.print("[VGL] %s: Could not load function \"%s\"",
			optional ? "WARNING" : "ERROR", name);
		if(strlen(fconfig.ocllib) > 0)
			vglout.print(" from %s", fconfig.ocllib);
		vglout.print("\n");
		if(err) vglout.print("[VGL]    %s\n", err);
	}
	return sym;
}

#endif


static void *loadX11Symbol(const char *name, bool optional)
{
	char *err = NULL;

	if(!x11dllhnd)
	{
		if(strlen(fconfig.x11lib) > 0)
		{
			dlerror();  // Clear error state
			void *dllhnd = _vgl_dlopen(fconfig.x11lib, RTLD_LAZY);
			err = dlerror();
			if(!dllhnd)
			{
				vglout.print("[VGL] ERROR: Could not open %s\n", fconfig.x11lib);
				if(err) vglout.print("[VGL]    %s\n", err);
				return NULL;
			}
			x11dllhnd = dllhnd;
		}
		else x11dllhnd = RTLD_NEXT;
	}

	dlerror();  // Clear error state
	void *sym = dlsym(x11dllhnd, (char *)name);
	err = dlerror();

	if(!sym && (fconfig.verbose || !optional))
	{
		vglout.print("[VGL] %s: Could not load function \"%s\"",
			optional ? "WARNING" : "ERROR", name);
		if(strlen(fconfig.x11lib) > 0)
			vglout.print(" from %s", fconfig.x11lib);
		vglout.print("\n");
		if(err) vglout.print("[VGL]    %s\n", err);
	}
	return sym;
}


#ifdef FAKEXCB

#define LOAD_XCB_SYMBOL(ID, id, libid, minrev, maxrev) \
static void *load##ID##Symbol(const char *name, bool optional) \
{ \
	char *err = NULL; \
	\
	if(!id##dllhnd) \
	{ \
		if(strlen(fconfig.id##lib) > 0) \
		{ \
			dlerror(); \
			void *dllhnd = _vgl_dlopen(fconfig.id##lib, RTLD_LAZY); \
			err = dlerror(); \
			if(!dllhnd) \
			{ \
				if(fconfig.verbose || !optional) \
				{ \
					vglout.print("[VGL] %s: Could not open %s\n", \
						optional ? "WARNING" : "ERROR", fconfig.id##lib); \
					if(err) vglout.print("[VGL]    %s\n", err); \
				} \
				return NULL; \
			} \
			id##dllhnd = dllhnd; \
		} \
		else \
		{ \
			void *dllhnd = NULL; \
			for(int i = minrev; i <= maxrev; i++) \
			{ \
				char libName[MAXSTR]; \
				snprintf(libName, MAXSTR, "lib%s.so.%d", #libid, i); \
				dlerror(); \
				dllhnd = _vgl_dlopen(libName, RTLD_LAZY); \
				err = dlerror(); \
				if(dllhnd) break; \
			} \
			if(!dllhnd) \
			{ \
				if(fconfig.verbose || !optional) \
				{ \
					vglout.print("[VGL] %s: Could not open lib%s\n", \
						optional ? "WARNING" : "ERROR", #libid); \
					if(err) vglout.print("[VGL]    %s\n", err); \
				} \
				return NULL; \
			} \
			id##dllhnd = dllhnd; \
		} \
	} \
	\
	dlerror(); \
	void *sym = dlsym(id##dllhnd, (char *)name); \
	err = dlerror(); \
	\
	if(!sym && (fconfig.verbose || !optional)) \
	{ \
		vglout.print("[VGL] %s: Could not load symbol \"%s\"", \
			optional ? "WARNING" : "ERROR", name); \
		if(strlen(fconfig.id##lib) > 0) \
			vglout.print(" from %s", fconfig.id##lib); \
		vglout.print("\n"); \
		if(err) vglout.print("[VGL]    %s\n", err); \
	} \
	return sym; \
}

LOAD_XCB_SYMBOL(XCB, xcb, xcb, 1, 1)
LOAD_XCB_SYMBOL(XCBGLX, xcbglx, xcb-glx, 0, 0)
LOAD_XCB_SYMBOL(XCBKeysyms, xcbkeysyms, xcb-keysyms, 0, 1)
LOAD_XCB_SYMBOL(XCBX11, xcbx11, X11-xcb, 1, 1)

#endif


namespace faker {

void unloadSymbols(void)
{
	if(gldllhnd && gldllhnd != RTLD_NEXT) dlclose(gldllhnd);
	#ifdef EGLBACKEND
	if(egldllhnd && egldllhnd != RTLD_NEXT) dlclose(egldllhnd);
	#endif
	#ifdef FAKEOPENCL
	if(ocldllhnd && ocldllhnd != RTLD_NEXT) dlclose(ocldllhnd);
	#endif
	if(x11dllhnd && x11dllhnd != RTLD_NEXT) dlclose(x11dllhnd);
	#ifdef FAKEXCB
	if(xcbdllhnd) dlclose(xcbdllhnd);
	if(xcbglxdllhnd) dlclose(xcbglxdllhnd);
	if(xcbkeysymsdllhnd) dlclose(xcbkeysymsdllhnd);
	if(xcbx11dllhnd) dlclose(xcbx11dllhnd);
	#endif
}

}
