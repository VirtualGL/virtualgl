/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2014-2015, 2017 D. R. Commander
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

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


#define _throw(m)  { fprintf(stderr, "ERROR: %s\n", m);  goto bailout; }


typedef XVisualInfo *(*_glXChooseVisualType)(Display *, int, int *);
_glXChooseVisualType _glXChooseVisual = NULL;

typedef GLXContext (*_glXCreateContextType)(Display *, XVisualInfo *,
	GLXContext, Bool);
_glXCreateContextType _glXCreateContext = NULL;

typedef GLXContext (*_glXDestroyContextType)(Display *, GLXContext);
_glXDestroyContextType _glXDestroyContext = NULL;

typedef void (*(*_glXGetProcAddressARBType)(const GLubyte *))(void);
_glXGetProcAddressARBType _glXGetProcAddressARB = NULL;

typedef Bool (*_glXMakeCurrentType)(Display *, GLXDrawable, GLXContext);
_glXMakeCurrentType _glXMakeCurrent = NULL;

typedef void (*_glXSwapBuffersType)(Display *, GLXDrawable);
_glXSwapBuffersType _glXSwapBuffers = NULL;

typedef void (*_glClearType)(GLbitfield);
_glClearType _glClear = NULL;

typedef void (*_glClearColorType)(GLclampf, GLclampf, GLclampf, GLclampf);
_glClearColorType _glClearColor = NULL;

void *gldllhnd = NULL;

#define LSYM(s) \
	dlerror(); \
	_##s = (_##s##Type)dlsym(gldllhnd, #s); \
	err = dlerror(); \
	if(err) _throw(err) \
	else if(!_##s) _throw("Could not load symbol " #s)

void loadSymbols1(char *prefix)
{
	const char *err = NULL;
	if(prefix)
	{
		char temps[256];
		snprintf(temps, 255, "%s/libGL.so", prefix);
		gldllhnd = dlopen(temps, RTLD_NOW);
	}
	else gldllhnd = dlopen("libGL.so", RTLD_NOW);
	err = dlerror();
	if(err) _throw(err)
	else if(!gldllhnd) _throw("Could not open libGL")

	LSYM(glXChooseVisual);
	LSYM(glXCreateContext);
	LSYM(glXDestroyContext);
	LSYM(glXMakeCurrent);
	LSYM(glXSwapBuffers);
	LSYM(glClear);
	LSYM(glClearColor);
	return;

	bailout:
	exit(1);
}

void unloadSymbols1(void)
{
	if(gldllhnd) dlclose(gldllhnd);
}


#define LSYM2(s) \
	_##s = (_##s##Type)_glXGetProcAddressARB((const GLubyte *)#s); \
	if(!_##s) _throw("Could not load symbol " #s)

void loadSymbols2(void)
{
	const char *err = NULL;

	LSYM(glXGetProcAddressARB);
	LSYM2(glXChooseVisual);
	LSYM2(glXCreateContext);
	LSYM2(glXDestroyContext);
	LSYM2(glXMakeCurrent);
	LSYM2(glXSwapBuffers);
	LSYM2(glClear);
	LSYM2(glClearColor);
	return;

	bailout:
	exit(1);
}


/* Test whether libvglfaker's version of dlopen() is discriminating enough.
   This will fail on VGL 2.1.2 and prior */

typedef void (*_myTestFunctionType)(void);
_myTestFunctionType _myTestFunction = NULL;

void nameMatchTest(void)
{
	const char *err = NULL;

	fprintf(stderr, "dlopen() name matching test:\n");
	gldllhnd = dlopen("libGLdlfakerut.so", RTLD_NOW);
	err = dlerror();
	if(err) _throw(err)
	else if(!gldllhnd) _throw("Could not open libGLdlfakerut")

	LSYM(myTestFunction);
	_myTestFunction();
	dlclose(gldllhnd);
	gldllhnd = NULL;
	return;

	bailout:
	exit(1);
}


#include "dlfakerut-test.c"


#ifdef RTLD_DEEPBIND
/* Test whether libdlfaker.so properly circumvents RTLD_DEEPBIND */

typedef void (*_testType)(const char *);
_testType _test = NULL;

void deepBindTest(void)
{
	const char *err = NULL;

	gldllhnd = dlopen("libdeepbindtest.so", RTLD_NOW | RTLD_DEEPBIND);
	err = dlerror();
	if(err) _throw(err)
	else if(!gldllhnd) _throw("Could not open libdlfakerut")

	LSYM(test);
	_test("RTLD_DEEPBIND test");
	dlclose(gldllhnd);
	gldllhnd = NULL;
	return;

	bailout:
	exit(1);
}
#endif


int main(int argc, char **argv)
{
	char *env, *prefix = NULL;

	if(argc > 2 && !strcasecmp(argv[1], "--prefix"))
	{
		prefix = argv[2];
		fprintf(stderr, "prefix = %s\n", prefix);
	}

	if(putenv((char *)"VGL_AUTOTEST=1") == -1
		|| putenv((char *)"VGL_SPOIL=0") == -1)
		_throw("putenv() failed!\n");

	env = getenv("LD_PRELOAD");
	fprintf(stderr, "LD_PRELOAD = %s\n", env ? env : "(NULL)");
	#ifdef sun
	env = getenv("LD_PRELOAD_32");
	fprintf(stderr, "LD_PRELOAD_32 = %s\n", env ? env : "(NULL)");
	env = getenv("LD_PRELOAD_64");
	fprintf(stderr, "LD_PRELOAD_64 = %s\n", env ? env : "(NULL)");
	#endif

	fprintf(stderr, "\n");
	nameMatchTest();

	loadSymbols1(prefix);
	test("dlopen() test");

	loadSymbols2();
	test("glXGetProcAddressARB() test");

	unloadSymbols1();

	#ifdef RTLD_DEEPBIND
	deepBindTest();
	#endif

	bailout:
	return 0;
}
