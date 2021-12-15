/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2014-2015, 2017, 2019-2021 D. R. Commander
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
#ifdef EGLBACKEND
#include <EGL/egl.h>
#endif
#ifdef FAKEOPENCL
#include <CL/opencl.h>
#endif
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(__has_feature)
	#if __has_feature(address_sanitizer)
		#define USING_ASAN  1
	#endif
#endif
#if !defined(USING_ASAN) && defined(__SANITIZE_ADDRESS__)
	#define USING_ASAN  1
#endif

#define THROW(m) \
{ \
	retval = -1;  fprintf(stderr, "ERROR: %s\n", m);  goto bailout; \
}

#define TRY(f) \
{ \
	if((f) < 0) \
	{ \
		retval = -1;  goto bailout; \
	} \
}


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

#ifdef EGLBACKEND

typedef EGLBoolean (*_eglChooseConfigType)(EGLDisplay, const EGLint *,
	EGLConfig *, EGLint, EGLint *);
_eglChooseConfigType _eglChooseConfig = NULL;

typedef EGLContext (*_eglCreateContextType)(EGLDisplay, EGLConfig,
	EGLContext, const EGLint *);
_eglCreateContextType _eglCreateContext = NULL;

typedef EGLSurface (*_eglCreateWindowSurfaceType)(EGLDisplay, EGLConfig,
	EGLNativeWindowType, const EGLint *);
_eglCreateWindowSurfaceType _eglCreateWindowSurface = NULL;

typedef EGLBoolean (*_eglDestroyContextType)(EGLDisplay, EGLContext);
_eglDestroyContextType _eglDestroyContext = NULL;

typedef EGLBoolean (*_eglDestroySurfaceType)(EGLDisplay, EGLSurface);
_eglDestroySurfaceType _eglDestroySurface = NULL;

typedef EGLDisplay (*_eglGetDisplayType)(EGLNativeDisplayType);
_eglGetDisplayType _eglGetDisplay = NULL;

typedef EGLint (*_eglGetErrorType)(void);
_eglGetErrorType _eglGetError = NULL;

typedef __eglMustCastToProperFunctionPointerType
	(*_eglGetProcAddressType)(const char *);
_eglGetProcAddressType _eglGetProcAddress = NULL;

typedef EGLBoolean (*_eglInitializeType)(EGLDisplay, EGLint *, EGLint *);
_eglInitializeType _eglInitialize = NULL;

typedef EGLBoolean (*_eglMakeCurrentType)(EGLDisplay, EGLSurface, EGLSurface,
	EGLContext);
_eglMakeCurrentType _eglMakeCurrent = NULL;

typedef EGLBoolean (*_eglSwapBuffersType)(EGLDisplay, EGLSurface);
_eglSwapBuffersType _eglSwapBuffers = NULL;

typedef EGLBoolean (*_eglTerminateType)(EGLDisplay);
_eglTerminateType _eglTerminate = NULL;

#endif

typedef void (*_glClearType)(GLbitfield);
_glClearType _glClear = NULL;

typedef void (*_glClearColorType)(GLclampf, GLclampf, GLclampf, GLclampf);
_glClearColorType _glClearColor = NULL;

#ifdef FAKEOPENCL

typedef cl_context (*_clCreateContextType)(cl_context_properties *, cl_uint,
	const cl_device_id *, void (*)(const char *, const void *, size_t, void *),
	void *, cl_int *);
_clCreateContextType _clCreateContext = NULL;

typedef cl_int (*_clGetContextInfoType)(cl_context, cl_context_info, size_t,
	void *, size_t *);
_clGetContextInfoType _clGetContextInfo = NULL;

typedef cl_int (*_clGetDeviceIDsType)(cl_platform_id, cl_device_type, cl_uint,
	cl_device_id *, cl_uint *);
_clGetDeviceIDsType _clGetDeviceIDs = NULL;

typedef cl_int (*_clGetDeviceInfoType)(cl_device_id, cl_device_info, size_t,
	void *, size_t *);
_clGetDeviceInfoType _clGetDeviceInfo = NULL;

typedef cl_int (*_clGetPlatformIDsType)(cl_uint, cl_platform_id *, cl_uint *);
_clGetPlatformIDsType _clGetPlatformIDs = NULL;

typedef cl_int (*_clGetPlatformInfoType)(cl_platform_id, cl_platform_info,
	size_t, void *, size_t *);
_clGetPlatformInfoType _clGetPlatformInfo = NULL;

typedef cl_int (*_clReleaseContextType)(cl_context);
_clReleaseContextType _clReleaseContext = NULL;

#endif

void *glxdllhnd = NULL;
#ifdef EGLBACKEND
void *egldllhnd = NULL;
#endif
void *gldllhnd = NULL;
#ifdef FAKEOPENCL
void *ocldllhnd = NULL;
#endif
int fakeOpenCL = 0, eglx = 0;
const char *libGLX = "libGL.so", *libOpenGL = "libGL.so";
#ifdef EGLBACKEND
const char *libEGL = "libEGL.so";
#endif

#define LSYM(dllhnd, s) \
	dlerror(); \
	_##s = (_##s##Type)dlsym(dllhnd, #s); \
	err = dlerror(); \
	if(err) THROW(err) \
	else if(!_##s) THROW("Could not load symbol " #s)

static int loadSymbols1(char *prefix)
{
	const char *err = NULL;
	int retval = 0;

	if(prefix)
	{
		char temps[256];
		snprintf(temps, 255, "%s/%s", prefix, libGLX);
		glxdllhnd = dlopen(temps, RTLD_NOW);
	}
	else glxdllhnd = dlopen(libGLX, RTLD_NOW);
	err = dlerror();
	if(err) THROW(err)
	else if(!glxdllhnd) THROW("Could not open GLX library")

	LSYM(glxdllhnd, glXChooseVisual);
	LSYM(glxdllhnd, glXCreateContext);
	LSYM(glxdllhnd, glXDestroyContext);
	LSYM(glxdllhnd, glXMakeCurrent);
	LSYM(glxdllhnd, glXSwapBuffers);

	#ifdef EGLBACKEND

	if(eglx)
	{
		if(prefix)
		{
			char temps[256];
			snprintf(temps, 255, "%s/%s", prefix, libEGL);
			egldllhnd = dlopen(temps, RTLD_NOW);
		}
		else egldllhnd = dlopen(libEGL, RTLD_NOW);
		err = dlerror();
		if(err) THROW(err)
		else if(!egldllhnd) THROW("Could not open EGL library")

		LSYM(egldllhnd, eglChooseConfig);
		LSYM(egldllhnd, eglCreateContext);
		LSYM(egldllhnd, eglCreateWindowSurface);
		LSYM(egldllhnd, eglDestroyContext);
		LSYM(egldllhnd, eglDestroySurface);
		LSYM(egldllhnd, eglGetDisplay);
		LSYM(egldllhnd, eglGetError);
		LSYM(egldllhnd, eglInitialize);
		LSYM(egldllhnd, eglMakeCurrent);
		LSYM(egldllhnd, eglSwapBuffers);
		LSYM(egldllhnd, eglTerminate);
	}

	#endif

	if(prefix)
	{
		char temps[256];
		snprintf(temps, 255, "%s/%s", prefix, libOpenGL);
		gldllhnd = dlopen(temps, RTLD_NOW);
	}
	else gldllhnd = dlopen(libOpenGL, RTLD_NOW);
	err = dlerror();
	if(err) THROW(err)
	else if(!gldllhnd) THROW("Could not open OpenGL library")

	LSYM(gldllhnd, glClear);
	LSYM(gldllhnd, glClearColor);

	#ifdef FAKEOPENCL

	if(fakeOpenCL)
	{
		if(prefix)
		{
			char temps[256];
			snprintf(temps, 255, "%s/libOpenCL.so", prefix);
			ocldllhnd = dlopen(temps, RTLD_NOW);
		}
		else ocldllhnd = dlopen("libOpenCL.so", RTLD_NOW);
		err = dlerror();
		if(err) THROW(err)
		else if(!ocldllhnd) THROW("Could not open libOpenCL")

		LSYM(ocldllhnd, clCreateContext);
		LSYM(ocldllhnd, clGetContextInfo);
		LSYM(ocldllhnd, clGetDeviceIDs);
		LSYM(ocldllhnd, clGetDeviceInfo);
		LSYM(ocldllhnd, clGetPlatformIDs);
		LSYM(ocldllhnd, clGetPlatformInfo);
		LSYM(ocldllhnd, clReleaseContext);
	}

	#endif

	bailout:
	return retval;
}

static void unloadSymbols1(void)
{
	if(glxdllhnd) dlclose(glxdllhnd);
	#ifdef EGLBACKEND
	if(egldllhnd) dlclose(egldllhnd);
	#endif
	if(gldllhnd) dlclose(gldllhnd);
	#ifdef FAKEOPENCL
	if(ocldllhnd) dlclose(ocldllhnd);
	#endif
}


#define LSYMGLX(s) \
	_##s = (_##s##Type)_glXGetProcAddressARB((const GLubyte *)#s); \
	if(!_##s) THROW("Could not load symbol " #s)

#ifdef EGLBACKEND
#define LSYMEGL(s) \
	_##s = (_##s##Type)_eglGetProcAddress(#s); \
	if(!_##s) THROW("Could not load symbol " #s)
#endif

static int loadSymbols2(void)
{
	const char *err = NULL;
	int retval = 0;

	LSYM(glxdllhnd, glXGetProcAddressARB);
	LSYMGLX(glXChooseVisual);
	LSYMGLX(glXCreateContext);
	LSYMGLX(glXDestroyContext);
	LSYMGLX(glXMakeCurrent);
	LSYMGLX(glXSwapBuffers);
	LSYMGLX(glClear);
	LSYMGLX(glClearColor);

	#ifdef EGLBACKEND

	if(eglx)
	{
		LSYM(egldllhnd, eglGetProcAddress);
		LSYMEGL(eglChooseConfig);
		LSYMEGL(eglCreateContext);
		LSYMEGL(eglCreateWindowSurface);
		LSYMEGL(eglDestroyContext);
		LSYMEGL(eglDestroySurface);
		LSYMEGL(eglGetDisplay);
		LSYMEGL(eglGetError);
		LSYMEGL(eglInitialize);
		LSYMEGL(eglMakeCurrent);
		LSYMEGL(eglSwapBuffers);
		LSYMEGL(eglTerminate);
	}

	#endif

	bailout:
	return retval;
}


/* Test whether libvglfaker's version of dlopen() is discriminating enough.
   This will fail on VGL 2.1.2 and prior */

typedef void (*_myTestFunctionType)(void);
_myTestFunctionType _myTestFunction = NULL;

static int nameMatchTest(void)
{
	const char *err = NULL;
	int retval = 0;

	fprintf(stderr, "dlopen() name matching test:\n");
	gldllhnd = dlopen("libGLdlfakerut.so", RTLD_NOW);
	err = dlerror();
	if(err) THROW(err)
	else if(!gldllhnd) THROW("Could not open libGLdlfakerut")

	LSYM(gldllhnd, myTestFunction);
	_myTestFunction();
	dlclose(gldllhnd);
	gldllhnd = NULL;

	bailout:
	return retval;
}


#include "dlfakerut-test.c"


#if defined(RTLD_DEEPBIND) && !defined(USING_ASAN)
/* Test whether libdlfaker.so properly circumvents RTLD_DEEPBIND */

typedef void (*_testType)(const char *, int, int);
_testType _test = NULL;

static int deepBindTest(void)
{
	const char *err = NULL;
	int retval = 0;

	gldllhnd = dlopen("libdeepbindtest.so", RTLD_NOW | RTLD_DEEPBIND);
	err = dlerror();
	if(err) THROW(err)
	else if(!gldllhnd) THROW("Could not open libdlfakerut")

	LSYM(gldllhnd, test);
	_test("RTLD_DEEPBIND test", fakeOpenCL, eglx);
	dlclose(gldllhnd);
	gldllhnd = NULL;

	bailout:
	return retval;
}
#endif


int main(int argc, char **argv)
{
	char *env, *prefix = NULL;
	int i, retval = 0;

	if(argc > 1)
	{
		for(i = 1; i < argc; i++)
		{
			if(!strcasecmp(argv[i], "--prefix") && i < argc - 1)
				prefix = argv[++i];
			else if(!strcasecmp(argv[i], "--glvnd"))
			{
				libGLX = "libGLX.so";
				libOpenGL = "libOpenGL.so";
			}
			#ifdef EGLBACKEND
			else if(!strcasecmp(argv[i], "--eglx"))
				eglx = 1;
			#endif
		}
	}

	fprintf(stderr, "GLX library = %s%s%s\n", prefix ? prefix : "",
		prefix ? "/" : "", libGLX);
	#ifdef EGLBACKEND
	if(eglx)
		fprintf(stderr, "EGL library = %s%s%s\n", prefix ? prefix : "",
			prefix ? "/" : "", libEGL);
	#endif
	fprintf(stderr, "OpenGL library = %s%s%s\n", prefix ? prefix : "",
		prefix ? "/" : "", libOpenGL);

	if(putenv((char *)"VGL_AUTOTEST=1") == -1
		|| putenv((char *)"VGL_SPOIL=0") == -1)
		THROW("putenv() failed!\n");

	env = getenv("LD_PRELOAD");
	fprintf(stderr, "LD_PRELOAD = %s\n", env ? env : "(NULL)");
	#ifdef sun
	env = getenv("LD_PRELOAD_32");
	fprintf(stderr, "LD_PRELOAD_32 = %s\n", env ? env : "(NULL)");
	env = getenv("LD_PRELOAD_64");
	fprintf(stderr, "LD_PRELOAD_64 = %s\n", env ? env : "(NULL)");
	#endif
	#ifdef FAKEOPENCL
	if((env = getenv("VGL_FAKEOPENCL")) != NULL && strlen(env) > 0
		&& !strncmp(env, "1", 1)) fakeOpenCL = 1;
	#endif

	fprintf(stderr, "\n");
	TRY(nameMatchTest());

	TRY(loadSymbols1(prefix));
	fprintf(stderr, "\n");
	TRY(test("dlopen() test", fakeOpenCL, eglx));

	TRY(loadSymbols2());
	fprintf(stderr, "\n");
	TRY(test("glXGetProcAddressARB()/eglGetProcAddress() test", fakeOpenCL,
		eglx));

	unloadSymbols1();

	#if defined(RTLD_DEEPBIND) && !defined(USING_ASAN)
	fprintf(stderr, "\n");
	TRY(deepBindTest());
	#endif

	bailout:
	return retval;
}
