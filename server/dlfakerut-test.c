/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2014, 2017, 2019 D. R. Commander
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
		fprintf(stderr, "Color is 0x%.6x, should be 0x%.6x", fakerColor, color);
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
		fprintf(stderr, "Expected %d readback%s, not %d", desiredReadbacks,
			desiredReadbacks == 1 ? "" : "s", frame - (*lastFrame));
		retval = -1;
	}
	*lastFrame = frame;

	bailout:
	return retval;
}


int test(const char *testName)
{
	Display *dpy = NULL;  Window win = 0, root;
	int dpyw, dpyh, lastFrame = 0, retval = 0;
	int glxattribs[] = { GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None, None };
	XVisualInfo *v = NULL;  GLXContext ctx = 0;
	XSetWindowAttributes swa;

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
	printf("SUCCESS\n");

	bailout:
	if(ctx && dpy)
	{
		_glXMakeCurrent(dpy, 0, 0);  _glXDestroyContext(dpy, ctx);  ctx = 0;
	}
	if(win && dpy) { XDestroyWindow(dpy, win);  win = 0; }
	if(v) { XFree(v);  v = NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy = NULL; }
	return retval;
}
