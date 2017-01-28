/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2014, 2017 D. R. Commander
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


static int checkWindowColor(Window win, unsigned int color)
{
	char *e=NULL, temps[80];  int fakerclr;

	snprintf(temps, 79, "__VGL_AUTOTESTCLR%x", (unsigned int)win);
	if((e=getenv(temps))==NULL)
		_throw("Can't communicate w/ faker");
	if((fakerclr=atoi(e))<0 || fakerclr>0xffffff)
		_throw("Bogus data read back");
	if((unsigned int)fakerclr!=color)
	{
		fprintf(stderr, "Color is 0x%.6x, should be 0x%.6x", fakerclr, color);
		return 0;
	}
	return 1;

	bailout:
	return 0;
}


static int checkFrame(Window win, int desiredReadbacks, int *lastFrame)
{
	char *e=NULL, temps[80];  int frame;

	snprintf(temps, 79, "__VGL_AUTOTESTFRAME%x", (unsigned int)win);
	if((e=getenv(temps))==NULL || (frame=atoi(e))<1)
		_throw("Can't communicate w/ faker");
	if(frame-(*lastFrame)!=desiredReadbacks && desiredReadbacks>=0)
	{
		fprintf(stderr, "Expected %d readback%s, not %d", desiredReadbacks,
			desiredReadbacks==1?"":"s", frame-(*lastFrame));
		return 0;
	}
	*lastFrame=frame;
	return 1;

	bailout:
	return 0;
}


void test(const char *testName)
{
	Display *dpy=NULL;  Window win=0, root;
	int dpyw, dpyh, lastFrame=0;
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE,
		8, GLX_BLUE_SIZE, 8, None, None};
	XVisualInfo *v=NULL;  GLXContext ctx=0;
	XSetWindowAttributes swa;

	fprintf(stderr, "%s:\n", testName);

	if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
	dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
	dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

	if((v=_glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
		_throw("Could not find a suitable visual");

	root=RootWindow(dpy, DefaultScreen(dpy));
	swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
	swa.border_pixel=0;
	swa.event_mask=0;
	if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v->depth,
		InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
		&swa))==0)
		_throw("Could not create window");
	XMapWindow(dpy, win);

	if((ctx=_glXCreateContext(dpy, v, 0, True))==NULL)
		_throw("Could not establish GLX context");
	XFree(v);  v=NULL;
	if(!_glXMakeCurrent(dpy, win, ctx))
		_throw("Could not make context current");

	_glClearColor(1., 0., 0., 0.);
	_glClear(GL_COLOR_BUFFER_BIT);
	_glXSwapBuffers(dpy, win);
	if(!checkFrame(win, 1, &lastFrame)) goto bailout;
	if(!checkWindowColor(win, 0x0000ff)) goto bailout;
	printf("SUCCESS\n");

	bailout:
	if(ctx && dpy)
	{
		_glXMakeCurrent(dpy, 0, 0);  _glXDestroyContext(dpy, ctx);  ctx=0;
	}
	if(win && dpy) { XDestroyWindow(dpy, win);  win=0; }
	if(v) { XFree(v);  v=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
}
