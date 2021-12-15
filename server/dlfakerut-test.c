/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2014, 2017, 2019, 2021 D. R. Commander
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


#ifdef EGLBACKEND

#define CASE(c)  case c:  return #c

static const char *getEGLErrorString(void)
{
	switch(_eglGetError())
	{
		CASE(EGL_SUCCESS);
		CASE(EGL_NOT_INITIALIZED);
		CASE(EGL_BAD_ACCESS);
		CASE(EGL_BAD_ALLOC);
		CASE(EGL_BAD_ATTRIBUTE);
		CASE(EGL_BAD_CONFIG);
		CASE(EGL_BAD_CONTEXT);
		CASE(EGL_BAD_CURRENT_SURFACE);
		CASE(EGL_BAD_DISPLAY);
		CASE(EGL_BAD_MATCH);
		CASE(EGL_BAD_NATIVE_PIXMAP);
		CASE(EGL_BAD_NATIVE_WINDOW);
		CASE(EGL_BAD_PARAMETER);
		CASE(EGL_BAD_SURFACE);
		CASE(EGL_CONTEXT_LOST);
		default:  return "Unknown EGL error";
	}
}

#define THROW_EGL() \
{ \
	retval = -1; \
	fprintf(stderr, "%s ERROR at %s:%d\n", getEGLErrorString(), __FILE__, \
		__LINE__); \
	goto bailout; \
}

#endif


int test(const char *testName, int testOpenCL, int testEGLX);


static int checkWindowColor(Display *dpy, Window win, unsigned int color)
{
	int fakerColor, retval = 0;
	typedef unsigned int (*_vgl_getAutotestColorType)(Display *, Window, int);
	_vgl_getAutotestColorType _vgl_getAutotestColor;

	_vgl_getAutotestColor =
		(_vgl_getAutotestColorType)dlsym(RTLD_DEFAULT, "_vgl_getAutotestColor");
	if(!_vgl_getAutotestColor)
		THROW("Can't communicate w/ faker");
	fakerColor = _vgl_getAutotestColor(dpy, win, 0);
	if(fakerColor < 0 || fakerColor > 0xffffff)
		THROW("Bogus data read back");
	if((unsigned int)fakerColor != color)
	{
		fprintf(stderr, "Color is 0x%.6x, should be 0x%.6x\n", fakerColor, color);
		retval = -1;
	}

	bailout:
	return retval;
}


static int checkFrame(Display *dpy, Window win, int desiredReadbacks,
	int *lastFrame)
{
	int frame, retval = 0;
	typedef unsigned int (*_vgl_getAutotestFrameType)(Display *, Window);
	_vgl_getAutotestFrameType _vgl_getAutotestFrame;

	_vgl_getAutotestFrame =
		(_vgl_getAutotestFrameType)dlsym(RTLD_DEFAULT, "_vgl_getAutotestFrame");
	if(!_vgl_getAutotestFrame)
		THROW("Can't communicate w/ faker");
	frame = _vgl_getAutotestFrame(dpy, win);
	if(frame < 1)
		THROW("Can't communicate w/ faker");
	if(frame - (*lastFrame) != desiredReadbacks && desiredReadbacks >= 0)
	{
		fprintf(stderr, "Expected %d readback%s, not %d\n", desiredReadbacks,
			desiredReadbacks == 1 ? "" : "s", frame - (*lastFrame));
		retval = -1;
	}
	*lastFrame = frame;

	bailout:
	return retval;
}


int test(const char *testName, int testOpenCL, int testEGLX)
{
	Display *dpy = NULL;  Window win = 0, root;
	int dpyw, dpyh, lastFrame = 0, retval = 0;
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None, None };
	XVisualInfo *v = NULL;  GLXContext ctx = 0;
	XSetWindowAttributes swa;
	#ifdef EGLBACKEND
	EGLDisplay edpy = 0;  EGLContext ectx = 0;  EGLSurface surface = 0;
	EGLint cfgAttribs[] = { EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
		EGL_NONE };
	EGLConfig config = 0;  EGLint nc = 0;
	#endif
	#ifdef FAKEOPENCL
	cl_uint nPlatforms = 0, pi, nDevices = 0, di;
	cl_context oclctx = 0;
	cl_platform_id *platforms = NULL;
	cl_device_id *devices = NULL;
	cl_int oclerr;
	#endif

	fprintf(stderr, "%s:\n", testName);

	if(!(dpy = XOpenDisplay(0))) THROW("Could not open display");
	dpyw = DisplayWidth(dpy, DefaultScreen(dpy));
	dpyh = DisplayHeight(dpy, DefaultScreen(dpy));

	if((v = _glXChooseVisual(dpy, DefaultScreen(dpy), glxattribs)) == NULL)
		THROW("Could not find a suitable visual");

	root = RootWindow(dpy, DefaultScreen(dpy));
	swa.colormap = XCreateColormap(dpy, root, v->visual, AllocNone);
	swa.border_pixel = 0;
	swa.event_mask = 0;
	if((win = XCreateWindow(dpy, root, 0, 0, dpyw / 2, dpyh / 2, 0, v->depth,
		InputOutput, v->visual, CWBorderPixel | CWColormap | CWEventMask,
		&swa)) == 0)
		THROW("Could not create window");
	XMapWindow(dpy, win);

	if((ctx = _glXCreateContext(dpy, v, 0, True)) == NULL)
		THROW("Could not establish GLX context");
	XFree(v);  v = NULL;
	if(!_glXMakeCurrent(dpy, win, ctx))
		THROW("Could not make context current");

	_glClearColor(1., 0., 0., 0.);
	_glClear(GL_COLOR_BUFFER_BIT);
	_glXSwapBuffers(dpy, win);
	TRY(checkFrame(dpy, win, 1, &lastFrame));
	TRY(checkWindowColor(dpy, win, 0x0000ff));

	#ifdef FAKEOPENCL

	if(testOpenCL)
	{
		if((oclerr = _clGetPlatformIDs(0, NULL, &nPlatforms)) != CL_SUCCESS
			&& oclerr != CL_PLATFORM_NOT_FOUND_KHR)
			THROW("Could not get OpenCL platforms");
		if(nPlatforms)
		{
			if((platforms =
				(cl_platform_id *)malloc(sizeof(cl_platform_id) * nPlatforms)) == NULL)
				THROW("Memory allocation error");
			if(_clGetPlatformIDs(nPlatforms, platforms, NULL) != CL_SUCCESS)
				THROW("Could not get OpenCL platforms");

			for(pi = 0; pi < nPlatforms; pi++)
			{
				char name[256];

				if(_clGetPlatformInfo(platforms[pi], CL_PLATFORM_NAME, 255, name,
					NULL) != CL_SUCCESS)
					THROW("Could not get OpenCL platform info");
				fprintf(stderr, "OpenCL platform:  %s\n", name);

				if((oclerr = _clGetDeviceIDs(platforms[pi], CL_DEVICE_TYPE_GPU, 0,
					NULL, &nDevices)) != CL_SUCCESS && oclerr != CL_DEVICE_NOT_FOUND)
					THROW("Could not get OpenCL devices");
				if(nDevices)
				{
					if((devices =
						(cl_device_id *)malloc(sizeof(cl_device_id) * nDevices)) == NULL)
							THROW("Memory allocation error");
					if(_clGetDeviceIDs(platforms[pi], CL_DEVICE_TYPE_GPU, nDevices,
						devices, NULL) != CL_SUCCESS)
						THROW("Could not get OpenCL devices");

					for(di = 0; di < nDevices; di++)
					{
						cl_context_properties properties[] = { CL_GL_CONTEXT_KHR, 0,
							CL_GLX_DISPLAY_KHR, 0, CL_CONTEXT_PLATFORM, 0, 0 };

						if(_clGetDeviceInfo(devices[di], CL_DEVICE_NAME, 255, name,
							NULL) != CL_SUCCESS)
							THROW("Could not get OpenCL device info");
						fprintf(stderr, "  Device:  %s\n", name);

						properties[1] = (cl_context_properties)ctx;
						properties[3] = (cl_context_properties)dpy;
						properties[5] = (cl_context_properties)platforms[pi];
						if((oclctx = _clCreateContext(properties, 1, &devices[di], NULL,
							NULL, NULL)) == NULL)
							THROW("Could not create OpenCL context");
						properties[3] = 0;
						if(_clGetContextInfo(oclctx, CL_CONTEXT_PROPERTIES,
							sizeof(cl_context_properties) * 7, properties,
							NULL) != CL_SUCCESS)
							THROW("Could not get OpenCL context properties");
						if(!properties[3] || properties[3] == (cl_context_properties)dpy)
							THROW("clCreateContext() not correctly interposed");

						_clReleaseContext(oclctx);  oclctx = 0;
					}

					free(devices);  devices = NULL;
				}
				else fprintf(stderr, "  NO GPU DEVICES\n");
			}

			free(platforms);  platforms = NULL;
		}
		else fprintf(stderr, "No OpenCL platforms\n");
	}

	#endif

	#ifdef EGLBACKEND

	if(testEGLX)
	{
		_glXMakeCurrent(dpy, 0, 0);  _glXDestroyContext(dpy, ctx);  ctx = 0;
		lastFrame = 0;

		if(!(edpy = _eglGetDisplay(dpy)))
			THROW_EGL();
		if(!_eglInitialize(edpy, NULL, NULL))
			THROW_EGL();
		if(!_eglChooseConfig(edpy, cfgAttribs, &config, 1, &nc))
			THROW_EGL();
		if(!(surface = _eglCreateWindowSurface(edpy, config, win, NULL)))
			THROW_EGL();
		if(!(ectx = _eglCreateContext(edpy, config, NULL, NULL)))
			THROW_EGL();
		if(!_eglMakeCurrent(edpy, surface, surface, ectx))
			THROW_EGL();

		_glClearColor(0., 0., 1., 0.);
		_glClear(GL_COLOR_BUFFER_BIT);
		if(!_eglSwapBuffers(edpy, surface))
			THROW_EGL();
		TRY(checkFrame(dpy, win, 1, &lastFrame));
		TRY(checkWindowColor(dpy, win, 0xff0000));
	}

	#endif

	fprintf(stderr, "SUCCESS\n");

	bailout:
	#ifdef FAKEOPENCL
	if(oclctx) _clReleaseContext(oclctx);
	free(devices);
	free(platforms);
	#endif
	if(ctx && dpy)
	{
		_glXMakeCurrent(dpy, 0, 0);  _glXDestroyContext(dpy, ctx);
	}
	#ifdef EGLBACKEND
	if(edpy)
	{
		if(ectx)
		{
			_eglMakeCurrent(edpy, 0, 0, 0);  _eglDestroyContext(edpy, ectx);
		}
		if(surface)
		{
			_eglDestroySurface(edpy, surface);
		}
		_eglTerminate(edpy);
	}
	#endif
	if(win && dpy) XDestroyWindow(dpy, win);
	if(v) XFree(v);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}
