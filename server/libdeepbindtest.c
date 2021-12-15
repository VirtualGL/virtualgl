/* Copyright (C)2017, 2019, 2021 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <GL/glx.h>
#ifdef EGLBACKEND
#include <EGL/egl.h>
#endif
#ifdef FAKEOPENCL
#include <CL/opencl.h>
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


#define _glXChooseVisual  glXChooseVisual
#define _glClear  glClear
#define _glClearColor  glClearColor
#define _glXCreateContext  glXCreateContext
#define _glXDestroyContext  glXDestroyContext
#define _glXMakeCurrent  glXMakeCurrent
#define _glXSwapBuffers  glXSwapBuffers
#ifdef EGLBACKEND
#define _eglChooseConfig  eglChooseConfig
#define _eglCreateContext  eglCreateContext
#define _eglCreateWindowSurface  eglCreateWindowSurface
#define _eglDestroyContext  eglDestroyContext
#define _eglDestroySurface  eglDestroySurface
#define _eglGetDisplay  eglGetDisplay
#define _eglGetError  eglGetError
#define _eglInitialize  eglInitialize
#define _eglMakeCurrent  eglMakeCurrent
#define _eglSwapBuffers  eglSwapBuffers
#define _eglTerminate  eglTerminate
#endif
#ifdef FAKEOPENCL
#define _clCreateContext  clCreateContext
#define _clGetContextInfo  clGetContextInfo
#define _clGetDeviceIDs  clGetDeviceIDs
#define _clGetDeviceInfo  clGetDeviceInfo
#define _clGetPlatformIDs  clGetPlatformIDs
#define _clGetPlatformInfo  clGetPlatformInfo
#define _clReleaseContext  clReleaseContext
#endif
#include "dlfakerut-test.c"
