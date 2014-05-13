/* Copyright (C)2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2014 D. R. Commander
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


#define _throw(m) { fprintf(stderr, "ERROR: %s\n", m);  goto bailout; }


int checkWindowColor(Window win, unsigned int color)
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


int checkFrame(Window win, int desiredReadbacks, int *lastFrame)
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


typedef XVisualInfo* (*_glXChooseVisualType)(Display *, int, int *);
_glXChooseVisualType _glXChooseVisual=NULL;

typedef GLXContext (*_glXCreateContextType)(Display *, XVisualInfo *,
	GLXContext, Bool);
_glXCreateContextType _glXCreateContext=NULL;

typedef GLXContext (*_glXDestroyContextType)(Display *, GLXContext);
_glXDestroyContextType _glXDestroyContext=NULL;

typedef void (*(*_glXGetProcAddressARBType)(const GLubyte*))(void);
_glXGetProcAddressARBType _glXGetProcAddressARB=NULL;

typedef Bool (*_glXMakeCurrentType)(Display *, GLXDrawable, GLXContext);
_glXMakeCurrentType _glXMakeCurrent=NULL;

typedef void (*_glXSwapBuffersType)(Display *, GLXDrawable);
_glXSwapBuffersType _glXSwapBuffers=NULL;

typedef void (*_glClearType)(GLbitfield);
_glClearType _glClear=NULL;

typedef void (*_glClearColorType)(GLclampf, GLclampf, GLclampf, GLclampf);
_glClearColorType _glClearColor=NULL;

void *gldllhnd=NULL;

#define LSYM(s)  \
	dlerror();  \
	_##s=(_##s##Type)dlsym(gldllhnd, #s);  \
	err=dlerror();  \
	if(err) _throw(err)  \
	else if(!_##s) _throw("Could not load symbol "#s)

void loadSymbols1(char *prefix)
{
	const char *err=NULL;
	if(prefix)
	{
		char temps[256];
		snprintf(temps, 255, "%s/libGL.so", prefix);
		gldllhnd=dlopen(temps, RTLD_NOW);
	}
	else gldllhnd=dlopen("libGL.so", RTLD_NOW);
	err=dlerror();
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


#define LSYM2(s)  \
	_##s=(_##s##Type)_glXGetProcAddressARB((const GLubyte *)#s);  \
	if(!_##s) _throw("Could not load symbol "#s)

void loadSymbols2(void)
{
	const char *err=NULL;

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


/* Test whether librrfaker's version of dlopen() is discriminating enough.
   This will fail on VGL 2.1.2 and prior */

typedef void (*_myTestFunctionType)(void);
_myTestFunctionType _myTestFunction=NULL;

void nameMatchTest(void)
{
	const char *err=NULL;

	fprintf(stderr, "dlopen() name matching test:\n");
	gldllhnd=dlopen("libGLdlfakerut.so", RTLD_NOW);
	err=dlerror();
	if(err) _throw(err)
	else if(!gldllhnd) _throw("Could not open libGLdlfakerut")

	LSYM(myTestFunction);
	_myTestFunction();
	dlclose(gldllhnd);
	gldllhnd=NULL;
	return;

	bailout:
	exit(1);
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


int main(int argc, char **argv)
{
	char *env, *prefix=NULL;

	if(argc>2 && !strcasecmp(argv[1], "--prefix"))
	{
		prefix=argv[2];
		fprintf(stderr, "prefix = %s\n", prefix);
	}

	if(putenv((char *)"VGL_AUTOTEST=1")==-1
		|| putenv((char *)"VGL_SPOIL=0")==-1)
		_throw("putenv() failed!\n");

	env=getenv("LD_PRELOAD");
	fprintf(stderr, "LD_PRELOAD = %s\n", env? env:"(NULL)");
	#ifdef sun
	env=getenv("LD_PRELOAD_32");
	fprintf(stderr, "LD_PRELOAD_32 = %s\n", env? env:"(NULL)");
	env=getenv("LD_PRELOAD_64");
	fprintf(stderr, "LD_PRELOAD_64 = %s\n", env? env:"(NULL)");
	#endif

	fprintf(stderr, "\n");
	nameMatchTest();

	loadSymbols1(prefix);
	test("dlopen() test");

	loadSymbols2();
	test("glXGetProcAddressARB() test");

	unloadSymbols1();

	bailout:
	return 0;
}
