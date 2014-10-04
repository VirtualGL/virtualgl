/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2014 D. R. Commander
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
#define GLX_GLXEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include "glx.h"
#include <GL/glu.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <dlfcn.h>
#include <unistd.h>
#include "Error.h"
#include "Thread.h"
#include "glext-vgl.h"

using namespace vglutil;

#ifndef GLX_RGBA_BIT
#define GLX_RGBA_BIT 0x00000001
#endif
#ifndef GLX_RGBA_TYPE
#define GLX_RGBA_TYPE 0x8014
#endif
#ifndef GLX_LARGEST_PBUFFER
#define GLX_LARGEST_PBUFFER 0x801C
#endif
#ifndef GLX_PBUFFER_WIDTH
#define GLX_PBUFFER_WIDTH 0x8041
#endif
#ifndef GLX_PBUFFER_HEIGHT
#define GLX_PBUFFER_HEIGHT 0x8040
#endif
#ifndef GLX_WIDTH
#define GLX_WIDTH 0x801D
#endif
#ifndef GLX_HEIGHT
#define GLX_HEIGHT 0x801E
#endif


#define CLEAR_BUFFER(buffer, r, g, b, a) {  \
	if(buffer>0) glDrawBuffer(buffer);  \
	glClearColor(r, g, b, a);  \
	glClear(GL_COLOR_BUFFER_BIT);  \
}

#define CLEAR_BUFFER_CI(buffer, i) {  \
	if(buffer>0) glDrawBuffer(buffer);  \
	glClearIndex(i);  \
	glClear(GL_COLOR_BUFFER_BIT);  \
}


#if 0
void clickToContinue(Display *dpy)
{
	XEvent e;
	printf("Click mouse in window to continue ...\n");
	while(1)
	{
		if(XNextEvent(dpy, &e)) break;
		if(e.type==ButtonPress) break;
	}
}
#endif

// Same as _throw but without the line number
#define _error(m) throw(Error(__FUNCTION__, m, 0))

#define _prerror1(m, a1) {  \
	char temps[256];  \
	snprintf(temps, 255, m, a1);  \
	throw(Error(__FUNCTION__, temps, 0));  \
}

#define _prerror2(m, a1, a2) {  \
	char temps[256];  \
	snprintf(temps, 255, m, a1, a2);  \
	throw(Error(__FUNCTION__, temps, 0));  \
}

#define _prerror3(m, a1, a2, a3) {  \
	char temps[256];  \
	snprintf(temps, 255, m, a1, a2, a3);  \
	throw(Error(__FUNCTION__, temps, 0));  \
}

#define _prerror5(m, a1, a2, a3, a4, a5) {  \
	char temps[256];  \
	snprintf(temps, 255, m, a1, a2, a3, a4, a5);  \
	throw(Error(__FUNCTION__, temps, 0));  \
}


unsigned int checkBufferColor(bool ci)
{
	int i, viewport[4], ps=ci? 1:3;  unsigned int ret=0;
	unsigned char *buf=NULL;

	try
	{
		viewport[0]=viewport[1]=viewport[2]=viewport[3]=0;
		glGetIntegerv(GL_VIEWPORT, viewport);
		if(viewport[2]<1 || viewport[3]<1) _throw("Invalid viewport dimensions");

		if((buf=(unsigned char *)malloc(viewport[2]*viewport[3]*ps))==NULL)
			_throw("Could not allocate buffer");

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, viewport[2], viewport[3], ci? GL_COLOR_INDEX:GL_RGB,
			GL_UNSIGNED_BYTE, buf);
		for(i=3; i<viewport[2]*viewport[3]*ps; i+=ps)
		{
			if(buf[i]!=buf[0]) _throw("Bogus data read back");
			if(!ci && (buf[i+1]!=buf[1] || buf[i+2]!=buf[2]))
				_throw("Bogus data read back");
		}
		ret=buf[0];
		if(!ci) ret=ret|(buf[1]<<8)|(buf[2]<<16);
		free(buf);
		return ret;
	}
	catch(...)
	{
		if(buf) free(buf);  throw;
	}
	return 0;
}


void checkWindowColor(Window win, unsigned int color, bool ci,
	bool right=false)
{
	char *env=NULL, temps[80];  int fakerColor;

	if(right)
		snprintf(temps, 79, "__VGL_AUTOTESTRCLR%x", (unsigned int)win);
	else
		snprintf(temps, 79, "__VGL_AUTOTESTCLR%x", (unsigned int)win);
	if((env=getenv(temps))==NULL)
		_error("Can't communicate w/ faker");
	if((fakerColor=atoi(env))<0 || (!ci && fakerColor>0xffffff)
		|| (ci && fakerColor>255))
		_error("Bogus data read back");
	if((unsigned int)fakerColor!=color)
	{
		if(right)
			_prerror2("R.buf is 0x%.6x, should be 0x%.6x", fakerColor, color)
		else
			_prerror2("Color is 0x%.6x, should be 0x%.6x", fakerColor, color)
	}
}


void checkFrame(Window win, int desiredReadbacks, int &lastFrame)
{
	char *env=NULL, temps[80];  int frame;

	snprintf(temps, 79, "__VGL_AUTOTESTFRAME%x", (unsigned int)win);
	if((env=getenv(temps))==NULL || (frame=atoi(env))<1)
		_error("Can't communicate w/ faker");
	if(frame-lastFrame!=desiredReadbacks && desiredReadbacks>=0)
		_prerror3("Expected %d readback%s, not %d", desiredReadbacks,
			desiredReadbacks==1? "":"s", frame-lastFrame);
	lastFrame=frame;
}


void checkCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx)
{
	if(glXGetCurrentDisplay()!=dpy)
		_throw("glXGetCurrentDisplay() returned incorrect value");
	if(glXGetCurrentDrawable()!=draw)
		_throw("glXGetCurrentDrawable() returned incorrect value");
	if(glXGetCurrentReadDrawable()!=read)
		_throw("glXGetCurrentReadDrawable() returned incorrect value");
	if(glXGetCurrentContext()!=ctx)
		_throw("glXGetCurrentContext() returned incorrect value");
}


void checkReadbackState(int oldReadBuf, Display *dpy, GLXDrawable draw,
	GLXDrawable read, GLXContext ctx)
{
	if(glXGetCurrentDisplay()!=dpy)
		_error("Current display changed");
	if(glXGetCurrentDrawable()!=draw || glXGetCurrentReadDrawable()!=read)
		_error("Current drawable changed");
	if(glXGetCurrentContext()!=ctx)
		_error("Context changed");
	int readBuf=-1;
	glGetIntegerv(GL_READ_BUFFER, &readBuf);
	if(readBuf!=oldReadBuf)
		_error("Read buffer changed");
}


// Check whether double buffering works properly
bool doubleBufferTest(bool ci)
{
	unsigned int bgColor=0, fgColor=0;
	int oldReadBuf=GL_BACK, oldDrawBuf=GL_BACK;

	glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);
	if(ci)
	{
		CLEAR_BUFFER_CI(GL_FRONT_AND_BACK, 0);
		CLEAR_BUFFER_CI(GL_BACK, 31);
		CLEAR_BUFFER_CI(GL_FRONT, 15);
	}
	else
	{
		CLEAR_BUFFER(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
		CLEAR_BUFFER(GL_BACK, 1., 1., 1., 0.);
		CLEAR_BUFFER(GL_FRONT, 1., 0., 1., 0.);
	}
	glReadBuffer(GL_BACK);
	bgColor=checkBufferColor(ci);
	glReadBuffer(GL_FRONT);
	fgColor=checkBufferColor(ci);
	glReadBuffer(oldReadBuf);
	glDrawBuffer(oldDrawBuf);
	if(!ci && bgColor==0xffffff && fgColor==0xff00ff) return true;
	if(ci && bgColor==31 && fgColor==15) return true;
	return false;
}


// Check whether stereo works properly
bool stereoTest(bool ci)
{
	unsigned int rightColor=0, leftColor=0;
	int oldReadBuf=GL_BACK, oldDrawBuf=GL_BACK;

	glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);
	if(ci)
	{
		CLEAR_BUFFER_CI(GL_FRONT_AND_BACK, 0);
		CLEAR_BUFFER_CI(GL_RIGHT, 31);
		CLEAR_BUFFER_CI(GL_LEFT, 15);
	}
	else
	{
		CLEAR_BUFFER(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
		CLEAR_BUFFER(GL_RIGHT, 1., 1., 1., 0.);
		CLEAR_BUFFER(GL_LEFT, 1., 0., 1., 0.);
	}
	glReadBuffer(GL_RIGHT);
	rightColor=checkBufferColor(ci);
	glReadBuffer(GL_LEFT);
	leftColor=checkBufferColor(ci);
	glReadBuffer(oldReadBuf);
	glDrawBuffer(oldDrawBuf);
	if(!ci && rightColor==0xffffff && leftColor==0xff00ff) return true;
	if(ci && rightColor==31 && leftColor==15) return true;
	return false;
}


// Check whether the faker properly handles gl*Index*() calls when an RGBA
// context is current.  This will fail with VirtualGL 2.4 beta1 and prior.
bool glIndexRGBAContextTest(void)
{
	unsigned int bgColor=0, fgColor=0;
	int oldReadBuf=GL_BACK, oldDrawBuf=GL_BACK;

	glGetIntegerv(GL_READ_BUFFER, &oldReadBuf);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuf);

	glDrawBuffer(GL_BACK);
	glClearColor(1, 1., 1., 0.);
	glClearIndex(128);
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawBuffer(GL_FRONT);
	glClearColor(1., 0., 1., 0.);
	glClearIndex(64);
	glClear(GL_COLOR_BUFFER_BIT);

	glReadBuffer(GL_BACK);
	bgColor=checkBufferColor(false);

	glReadBuffer(GL_FRONT);
	fgColor=checkBufferColor(false);

	glReadBuffer(oldReadBuf);
	glDrawBuffer(oldDrawBuf);

	if(bgColor==0xffffff && fgColor==0xff00ff) return true;
	return false;
}


typedef struct
{
	GLfloat r, g, b;
	unsigned int bits, index;
} Color;

#define NC 6
static Color colors[NC]=
{
	{ 1., 0., 0., 0x0000ff, 5  },
	{ 0., 1., 0., 0x00ff00, 10 },
	{ 0., 0., 1., 0xff0000, 15 },
	{ 0., 1., 1., 0xffff00, 20 },
	{ 1., 0., 1., 0xff00ff, 25 },
	{ 1., 1., 0., 0x00ffff, 30 }
};

class TestColor
{
	public:

		TestColor(bool ci_, int index_) : index(index_%NC), ci(ci_)
		{
		}

		GLfloat& r(int offset=0) { return colors[(index+offset+NC)%NC].r; }
		GLfloat& g(int offset=0) { return colors[(index+offset+NC)%NC].g; }
		GLfloat& b(int offset=0) { return colors[(index+offset+NC)%NC].b; }
		void next(void) { index=(index+1)%NC; }

		unsigned int& bits(int offset=0)
		{
			if(ci) return colors[(index+offset+NC)%NC].index;
			else return colors[(index+offset+NC)%NC].bits;
		}

		void clear(int buffer)
		{
			if(ci) CLEAR_BUFFER_CI(buffer, bits())
			else CLEAR_BUFFER(buffer, r(), g(), b(), 0.)
			next();
		}

	private:

		int index;
		bool ci;
};


// This tests the faker's readback heuristics
int readbackTest(bool stereo, bool ci)
{
	TestColor clr(ci, 0), sclr(ci, 3);
	Display *dpy=NULL;  Window win0=0, win1=0;
	int dpyw, dpyh, lastFrame0=0, lastFrame1=0, retval=1;
	int rgbattrib[]={ GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None, None };
	int rgbattrib13[]={ GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None, None, None };
	int ciattrib[]={ GLX_DOUBLEBUFFER, GLX_BUFFER_SIZE, 8, None, None };
	int ciattrib13[]={ GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_BUFFER_SIZE, 8, None, None, None };
	int *glxattrib, *glxattrib13;
	XVisualInfo *vis0=NULL, *vis1=NULL;  GLXFBConfig config=0, *configs=NULL;
	int n=0;
	GLXContext ctx0=0, ctx1=0;
	XSetWindowAttributes swa;

	if(stereo)
	{
		rgbattrib[8]=rgbattrib13[12]=ciattrib[3]=ciattrib13[8]=GLX_STEREO;
		rgbattrib13[13]=ciattrib13[9]=1;
	}
	if(ci) { glxattrib=ciattrib;  glxattrib13=ciattrib13; }
	else { glxattrib=rgbattrib;  glxattrib13=rgbattrib13; }

	printf("Readback heuristics test ");
	if(stereo) printf("(Stereo ");
	else printf("(Mono ");
	if(ci) printf("Color Index)");
	else printf("RGB)");
	printf("\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((vis0=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");
		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib13, &n))
			==NULL || n==0)
			_throw("Could not find a suitable FB config");
		config=configs[0];
		XFree(configs);  configs=NULL;

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		if(ci)
		{
			swa.colormap=XCreateColormap(dpy, root, vis0->visual, AllocAll);
			XColor xc[32];  int i;
			if(vis0->colormap_size<32) _throw("Color map is not large enough");
			for(i=0; i<32; i++)
			{
				xc[i].red=(i<16? i*16:255)<<8;
				xc[i].green=(i<16? i*16:255-(i-16)*16)<<8;
				xc[i].blue=(i<16? 255:255-(i-16)*16)<<8;
				xc[i].flags = DoRed | DoGreen | DoBlue;
				xc[i].pixel=i;
			}
			XStoreColors(dpy, swa.colormap, xc, 32);
		}
		else swa.colormap=XCreateColormap(dpy, root, vis0->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=0;
		if((win0=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, vis0->depth,
			InputOutput, vis0->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		if(!(vis1=glXGetVisualFromFBConfig(dpy, config)))
			_throw("glXGetVisualFromFBConfig()");
		if(ci)
		{
			swa.colormap=XCreateColormap(dpy, root, vis1->visual, AllocAll);
			XColor xc[32];  int i;
			if(vis1->colormap_size<32) _throw("Color map is not large enough");
			for(i=0; i<32; i++)
			{
				xc[i].red=(i<16? i*16:255)<<8;
				xc[i].green=(i<16? i*16:255-(i-16)*16)<<8;
				xc[i].blue=(i<16? 255:255-(i-16)*16)<<8;
				xc[i].flags = DoRed | DoGreen | DoBlue;
				xc[i].pixel=i;
			}
			XStoreColors(dpy, swa.colormap, xc, 32);
		}
		else swa.colormap=XCreateColormap(dpy, root, vis1->visual, AllocNone);
		if((win1=XCreateWindow(dpy, root, dpyw/2, 0, dpyw/2, dpyh/2, 0,
			vis1->depth, InputOutput, vis1->visual,
			CWBorderPixel|CWColormap|CWEventMask, &swa))==0)
			_throw("Could not create window");
		XFree(vis1);  vis1=NULL;

		if((ctx0=glXCreateContext(dpy, vis0, 0, True))==NULL)
			_throw("Could not establish GLX context");
		XFree(vis0);  vis0=NULL;
		if((ctx1=glXCreateNewContext(dpy, config,
			ci? GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE, NULL, True))==NULL)
			_throw("Could not establish GLX context");

		if(!glXMakeCurrent(dpy, win1, ctx0))
			_throw("Could not make context current");
		checkCurrent(dpy, win1, win1, ctx0);
		if(stereo && !stereoTest(ci))
		{
			_throw("Stereo is not available or is not properly implemented");
		}
		if(!doubleBufferTest(ci))
			_throw("Double buffering appears to be broken");
		if(!ci && !glIndexRGBAContextTest())
			_throw("Faker does not properly handle gl*Index*() calls when RGBA context is current");
		glReadBuffer(GL_BACK);

		if(!glXMakeContextCurrent(dpy, win1, win0, ctx1))
			_throw("Could not make context current");
		checkCurrent(dpy, win1, win0, ctx1);

		XMapWindow(dpy, win0);
		XMapWindow(dpy, win1);

		// Faker should readback back buffer on a call to glXSwapBuffers()
		try
		{
			printf("glXSwapBuffers() [b]:          ");
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			glReadBuffer(GL_FRONT);
			// Intentionally leave a pending GL error (VirtualGL should clear the
			// error state prior to readback)
			char pixel[4];
			glReadPixels(0, 0, 1, 1, 0, GL_BYTE, pixel);
			glXSwapBuffers(dpy, win1);
			checkReadbackState(GL_FRONT, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win1, sclr.bits(-2), ci, true);
			// Make sure that glXSwapBuffers() actually swapped
			glDrawBuffer(GL_FRONT);
			glFinish();
			checkReadbackState(GL_FRONT, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			if(!stereo)
			{
				// NOTE: This doesn't work with stereo on nVidia hardware for some
				// reason.  One would expect that, after the call to glXSwapBuffers(),
				// GL_FRONT_LEFT would contain the contents of GL_BACK_LEFT,
				// GL_FRONT_RIGHT would contain the contents of GL_BACK_RIGHT,
				// and GL_BACK_* would be undefined, but what actually happens is that
				// the front buffers get swapped to the back buffers and the front
				// buffers become undefined.  This occurs irrespective of the presence
				// of VirtualGL, so it is believed to be a bug in nVidia's drivers.
				checkWindowColor(win1, clr.bits(-2), ci);
				if(stereo)
					checkWindowColor(win1, sclr.bits(-2), ci, true);
			}
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		// Faker should readback front buffer on glFlush(), glFinish(), and
		// glXWaitGL()
		try
		{
			printf("glFlush() [f]:                 ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkFrame(win1, 1, lastFrame1);
			glDrawBuffer(GL_FRONT);  glFlush();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win1, sclr.bits(-2), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glFinish() [f]:                ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkFrame(win1, 1, lastFrame1);
			glDrawBuffer(GL_FRONT);  glFinish();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win1, sclr.bits(-2), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glXWaitGL() [f]:               ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkFrame(win1, 1, lastFrame1);
			glDrawBuffer(GL_FRONT);  glXWaitGL();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win1, sclr.bits(-2), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glPopAttrib() [f]:             ");
			glDrawBuffer(GL_BACK);  glFinish();
			checkFrame(win1, 1, lastFrame1);
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			glPopAttrib();  // Back buffer should now be current again & dirty flag
			                // should be set
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win1, sclr.bits(-2), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeCurrent() [f]:          ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeCurrent(dpy, win1, ctx0);  // readback should occur
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkReadbackState(GL_BACK, dpy, win0, win0, ctx0);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win1, sclr.bits(-2), ci, true);
			// Now try swapping one window when another is current (this will fail
			// with VGL 2.3.3 and earlier)
			glXSwapBuffers(dpy, win1);
			checkFrame(win1, 1, lastFrame1);
			if(!stereo) // Also due to the nVidia bug
				checkWindowColor(win1, clr.bits(-1), ci);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeContextCurrent() [f]:   ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeContextCurrent(dpy, win0, win0, ctx1); // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1); // readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeContextCurrent(dpy, win1, win0, ctx1); // No readback should occur
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win0, 1, lastFrame0);
			checkWindowColor(win0, clr.bits(-2), ci);
			if(stereo)
				checkWindowColor(win0, sclr.bits(-2), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		// Test for proper handling of GL_FRONT_AND_BACK
		try
		{
			printf("glXSwapBuffers() [f&b]:        ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, win1);
			checkReadbackState(GL_FRONT, dpy, win1, win0, ctx1);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-1), ci);
			if(stereo) checkWindowColor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glFlush() [f&b]:               ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 2, lastFrame1);
			checkWindowColor(win1, clr.bits(-1), ci);
			if(stereo) checkWindowColor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glFinish() [f&b]:              ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 2, lastFrame1);
			checkWindowColor(win1, clr.bits(-1), ci);
			if(stereo) checkWindowColor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glXWaitGL() [f&b]:             ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win1, 2, lastFrame1);
			checkWindowColor(win1, clr.bits(-1), ci);
			if(stereo) checkWindowColor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeCurrent() [f&b]:        ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glXMakeCurrent(dpy, win0, ctx0);  // readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkReadbackState(GL_BACK, dpy, win0, win0, ctx0);
			checkFrame(win1, 1, lastFrame1);
			checkWindowColor(win1, clr.bits(-1), ci);
			if(stereo) checkWindowColor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glXMakeContextCurrent() [f&b]: ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glXMakeContextCurrent(dpy, win0, win0, ctx1); // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1); // readback should occur
			glDrawBuffer(GL_FRONT);
			glXMakeContextCurrent(dpy, win1, win0, ctx1); // No readback should occur
			checkReadbackState(GL_BACK, dpy, win1, win0, ctx1);
			checkFrame(win0, 1, lastFrame0);
			checkWindowColor(win0, clr.bits(-1), ci);
			if(stereo) checkWindowColor(win0, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx0 && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx0);  ctx0=0;
	}
	if(ctx1 && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx1);  ctx1=0;
	}
	if(win0) { XDestroyWindow(dpy, win0);  win0=0; }
	if(win1) { XDestroyWindow(dpy, win1);  win1=0; }
	if(vis0) { XFree(vis0);  vis0=NULL; }
	if(vis1) { XFree(vis1);  vis1=NULL; }
	if(configs) { XFree(configs);  configs=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


// This tests the faker's ability to handle the 2000 Flushes issue
int flushTest(void)
{
	TestColor clr(false, 0);
	Display *dpy=NULL;  Window win=0;
	int dpyw, dpyh, lastFrame=0, retval=1;
	int glxattrib[]={ GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis=NULL;
	GLXContext ctx=0;
	XSetWindowAttributes swa;

	putenv((char *)"VGL_SPOIL=1");
	printf("10000 flushes test:\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((vis=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=0;
		if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");

		if((ctx=glXCreateContext(dpy, vis, 0, True))==NULL)
			_throw("Could not establish GLX context");
		XFree(vis);  vis=NULL;
		if(!glXMakeCurrent(dpy, win, ctx))
			_throw("Could not make context current");
		checkCurrent(dpy, win, win, ctx);
		if(!doubleBufferTest(false))
			_throw("This test requires double buffering, which appears to be broken.");
		glReadBuffer(GL_FRONT);
		XMapWindow(dpy, win);

		clr.clear(GL_BACK);
		clr.clear(GL_FRONT);
		for(int i=0; i<10000; i++)
		{
			printf("%.4d\b\b\b\b", i);  glFlush();
		}
		checkFrame(win, -1, lastFrame);
		printf("Read back %d of 10000 frames\n", lastFrame);
		checkReadbackState(GL_FRONT, dpy, win, win, ctx);
		checkWindowColor(win, clr.bits(-1), 0);
		printf("SUCCESS\n");
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);
	putenv((char *)"VGL_SPOIL=0");

	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
	}
	if(win) { XDestroyWindow(dpy, win);  win=0; }
	if(vis) { XFree(vis);  vis=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


// This tests the faker's ability to do indexed rendering in the red channel of
// an RGBA off-screen drawable

#define DRAW_QUAD() {  \
	glBegin(GL_QUADS);  glVertex3f(-1., -1., 0.);  glVertex3f(-1., 1., 0.);  \
	glVertex3f(1., 1., 0.);  glVertex3f(1., -1., 0.);  glEnd();  \
}

#define CI_TEST(id, type) {  \
	unsigned int index=clr.bits();  glIndex##id((type)index);  \
	GLfloat indexf=-1.;  glGetFloatv(GL_CURRENT_INDEX, &indexf);  \
	if(index!=(unsigned int)indexf)  \
		_prerror2("Index should be %u, not %u", index, (unsigned int)indexf);  \
	GLdouble indexd=-1.;  glGetDoublev(GL_CURRENT_INDEX, &indexd);  \
	if(index!=(unsigned int)indexd)  \
		_prerror2("Index should be %u, not %u", index, (unsigned int)indexd);  \
	DRAW_QUAD();  \
	glXSwapBuffers(dpy, win);  \
	checkWindowColor(win, clr.bits(), true);  \
	clr.next();  \
	index=clr.bits();  \
	type indexv=(type)index;  glIndex##id##v(&indexv);  \
	GLint indexi=-1;  glGetIntegerv(GL_CURRENT_INDEX, &indexi);  \
	if(index!=(unsigned int)indexi)  \
		_prerror2("Index should be %u, not %u", index, (unsigned int)indexi);  \
	DRAW_QUAD();  \
	glXSwapBuffers(dpy, win);  \
	checkWindowColor(win, clr.bits(), true);  \
	clr.next();  \
}

#define CI_DRAW_TEST(type, glType) {  \
	type *ptr=(type *)bits;  \
	for(int i=0; i<width*height; i++) ptr[i]=(type)clr.bits();  \
	glDrawPixels(width, height, GL_COLOR_INDEX, glType, ptr);  \
	glXSwapBuffers(dpy, win);  \
	checkWindowColor(win, clr.bits(), true);  clr.next();  \
}

#define CI_READ_TEST(type, glType) {  \
	type *ptr=(type *)bits;  \
	memset(ptr, 0, sizeof(type)*width*height);  \
	glReadPixels(0, 0, width, height, GL_COLOR_INDEX, glType, ptr);  \
	for(int i=0; i<width*height; i++)  \
		if((unsigned int)ptr[i]!=clr.bits())  \
			_prerror2("Index should be %u, not %u", clr.bits(),  \
				(unsigned int)ptr[i]);  \
}

#define CI_DRAW_SHIFT_TEST(type, glType) {  \
	type *ptr=(type *)bits;  \
	glPushAttrib(GL_PIXEL_MODE_BIT);  \
	glPixelTransferi(GL_INDEX_SHIFT, 2);  \
	glPixelTransferi(GL_INDEX_OFFSET, -3);  \
	double tempd;  \
	glGetDoublev(GL_INDEX_SHIFT, &tempd);  \
	if(tempd!=2.) _prerror1("glGetDoublev(GL_INDEX_SHIFT)=%f", tempd);  \
	glGetDoublev(GL_INDEX_OFFSET, &tempd);  \
	if(tempd!=-3.) _prerror1("glGetDoublev(GL_INDEX_OFFSET)=%f", tempd);  \
	for(int i=0; i<width*height; i++) ptr[i]=(type)4;  \
	glDrawPixels(width, height, GL_COLOR_INDEX, glType, ptr);  \
	glPopAttrib();  \
	glXSwapBuffers(dpy, win);  \
	checkWindowColor(win, 13, true);  \
	glPushAttrib(GL_PIXEL_MODE_BIT);  \
	glPixelTransferf(GL_INDEX_SHIFT, -2.);  \
	glPixelTransferf(GL_INDEX_OFFSET, 1.);  \
	int temp=0;  \
	glGetIntegerv(GL_INDEX_SHIFT, &temp);  \
	if(temp!=-2) _prerror1("glGetIntegerv(GL_INDEX_SHIFT)=%d", temp);  \
	glGetIntegerv(GL_INDEX_OFFSET, &temp);  \
	if(temp!=1) _prerror1("glGetIntegerv(GL_INDEX_OFFSET)=%d", temp);  \
	for(int i=0; i<width*height; i++) ptr[i]=(type)8;  \
	glDrawPixels(width, height, GL_COLOR_INDEX, glType, ptr);  \
	glPopAttrib();  \
	glXSwapBuffers(dpy, win);  \
	checkWindowColor(win, 3, true);  \
}

#define CI_READ_SHIFT_TEST(type, glType) {  \
	memset(bits, 4, width*height);  \
	glDrawPixels(width, height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bits);  \
	glXSwapBuffers(dpy, win);  \
	type *ptr=(type *)bits;  \
	glPushAttrib(GL_PIXEL_MODE_BIT);  \
	glPixelTransferi(GL_INDEX_SHIFT, 2);  \
	glPixelTransferi(GL_INDEX_OFFSET, -3);  \
	int temp=0;  \
	glGetIntegerv(GL_INDEX_SHIFT, &temp);  \
	if(temp!=2) _prerror1("glGetIntegerv(GL_INDEX_SHIFT)=%d", temp);  \
	glGetIntegerv(GL_INDEX_OFFSET, &temp);  \
	if(temp!=-3) _prerror1("glGetIntegerv(GL_INDEX_OFFSET)=%d", temp);  \
	glReadPixels(0, 0, width, height, GL_COLOR_INDEX, glType, ptr);  \
	glPopAttrib();  \
	for(int i=0; i<width*height; i++)  \
		if((unsigned int)ptr[i]!=13)  \
			_prerror1("Index should be 13, not %u", (unsigned int)ptr[i]);  \
	glPushAttrib(GL_PIXEL_MODE_BIT);  \
	glPixelTransferf(GL_INDEX_SHIFT, -1.);  \
	glPixelTransferf(GL_INDEX_OFFSET, 1.);  \
	float tempf;  \
	glGetFloatv(GL_INDEX_SHIFT, &tempf);  \
	if(tempf!=-1.) _prerror1("glGetFloatv(GL_INDEX_SHIFT)=%f", tempf);  \
	glGetFloatv(GL_INDEX_OFFSET, &tempf);  \
	if(tempf!=1.) _prerror1("glGetFloatv(GL_INDEX_OFFSET)=%f", tempf);  \
	glReadPixels(0, 0, width, height, GL_COLOR_INDEX, glType, ptr);  \
	glPopAttrib();  \
	for(int i=0; i<width*height; i++)  \
		if((unsigned int)ptr[i]!=3)  \
			_prerror1("Index should be 3, not %u", (unsigned int)ptr[i]);  \
}

int ciTest(void)
{
	TestColor clr(true, 0);
	Display *dpy=NULL;  Window win=0;
	int dpyw, dpyh, retval=1;
	int ciattrib[]={ GLX_DOUBLEBUFFER, GLX_BUFFER_SIZE, 8, None, None };
	XVisualInfo *vis=NULL;  GLXContext ctx=0;
	XSetWindowAttributes swa;
	unsigned char *bits=NULL;

	printf("Color Index rendering test ");
	printf("\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));
		int width=dpyw/2, height=dpyh/2;

		if((vis=glXChooseVisual(dpy, DefaultScreen(dpy), ciattrib))==NULL)
			_throw("Could not find a suitable visual");

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, vis->visual, AllocAll);
		XColor xc[32];  int i;
		if(vis->colormap_size<32) _throw("Color map is not large enough");
		for(i=0; i<32; i++)
		{
			xc[i].red=(i<16? i*16:255)<<8;
			xc[i].green=(i<16? i*16:255-(i-16)*16)<<8;
			xc[i].blue=(i<16? 255:255-(i-16)*16)<<8;
			xc[i].flags = DoRed | DoGreen | DoBlue;
			xc[i].pixel=i;
		}
		XStoreColors(dpy, swa.colormap, xc, 32);
		swa.border_pixel=0;
		swa.event_mask=0;
		if((win=XCreateWindow(dpy, root, 0, 0, width, height, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");

		if((ctx=glXCreateContext(dpy, vis, 0, True))==NULL)
			_throw("Could not establish GLX context");
		XFree(vis);  vis=NULL;
		if(!glXMakeCurrent(dpy, win, ctx))
			_throw("Could not make context current");
		checkCurrent(dpy, win, win, ctx);

		glDrawBuffer(GL_BACK);
		XMapWindow(dpy, win);

		try
		{
			// There must be fifty ways to change your index
			printf("glIndex*()                 : ");
			CI_TEST(d, GLdouble);
			CI_TEST(f, GLfloat);
			CI_TEST(i, GLint);
			CI_TEST(s, GLshort);
			CI_TEST(ub, GLubyte);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("glDrawPixels()             : ");
			bits=new unsigned char[width*height*4];
			if(!bits) _throw("Could not allocate buffer");
			CI_DRAW_TEST(unsigned char, GL_UNSIGNED_BYTE);
			CI_DRAW_TEST(char, GL_BYTE);
			CI_DRAW_TEST(unsigned short, GL_UNSIGNED_SHORT);
			CI_DRAW_TEST(short, GL_SHORT);
			CI_DRAW_TEST(unsigned int, GL_UNSIGNED_INT);
			CI_DRAW_TEST(int, GL_INT);
			CI_DRAW_TEST(float, GL_FLOAT);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		if(bits) { delete [] bits;  bits=NULL; }
		fflush(stdout);

		try
		{
			printf("glBitmap()                 : ");
			bits=new unsigned char[(width*height)/8+1];
			if(!bits) _throw("Could not allocate buffer");
			memset(bits, 0xAA, (width*height)/8+1);
			glIndexi((GLint)clr.bits());
			glRasterPos2f(-1., -1.);
			GLint indexi=-1;  glGetIntegerv(GL_CURRENT_RASTER_INDEX, &indexi);
			if((unsigned int)indexi!=clr.bits())
				_prerror2("Index should be %u, not %u", clr.bits(),
					(unsigned int)indexi);
			GLfloat indexf=-1;  glGetFloatv(GL_CURRENT_RASTER_INDEX, &indexf);
			if((unsigned int)indexf!=clr.bits())
				_prerror2("Index should be %u, not %u", clr.bits(),
					(unsigned int)indexf);
			GLdouble indexd=-1;  glGetDoublev(GL_CURRENT_RASTER_INDEX, &indexd);
			if((unsigned int)indexd!=clr.bits())
				_prerror2("Index should be %u, not %u", clr.bits(),
					(unsigned int)indexd);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBitmap(width, height, 0, 0, 0, 0, bits);
			memset(bits, 0x55, (width*height)/8+1);
			glBitmap(width, height, 0, 0, 0, 0, bits);
			glXSwapBuffers(dpy, win);
			checkWindowColor(win, clr.bits(), true);  clr.next();
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		if(bits) { delete [] bits;  bits=NULL; }
		fflush(stdout);

		try
		{
			printf("glReadPixels()             : ");
			bits=new unsigned char[width*height*4];
			if(!bits) _throw("Could not allocate buffer");
			for(int i=0; i<width*height; i++) bits[i]=clr.bits();
			glDrawPixels(width, height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bits);
			glXSwapBuffers(dpy, win);
			checkWindowColor(win, clr.bits(), true);
			glReadBuffer(GL_FRONT);
			CI_READ_TEST(char, GL_BYTE);
			CI_READ_TEST(unsigned short, GL_UNSIGNED_SHORT);
			CI_READ_TEST(short, GL_SHORT);
			CI_READ_TEST(unsigned int, GL_UNSIGNED_INT);
			CI_READ_TEST(int, GL_INT);
			CI_READ_TEST(float, GL_FLOAT);
			clr.next();
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		if(bits) { delete [] bits;  bits=NULL; }
		fflush(stdout);

		try
		{
			printf("glDrawPixels() (shift)     : ");
			bits=new unsigned char[width*height*4];
			if(!bits) _throw("Could not allocate buffer");
			glPushAttrib(GL_PIXEL_MODE_BIT);
			CI_DRAW_SHIFT_TEST(unsigned char, GL_UNSIGNED_BYTE);
			CI_DRAW_SHIFT_TEST(char, GL_BYTE);
			CI_DRAW_SHIFT_TEST(unsigned short, GL_UNSIGNED_SHORT);
			CI_DRAW_SHIFT_TEST(short, GL_SHORT);
			CI_DRAW_SHIFT_TEST(unsigned int, GL_UNSIGNED_INT);
			CI_DRAW_SHIFT_TEST(int, GL_INT);
			CI_DRAW_SHIFT_TEST(float, GL_FLOAT);
			glPopAttrib();
			clr.next();
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		if(bits) { delete [] bits;  bits=NULL; }
		fflush(stdout);

		try
		{
			printf("glReadPixels() (shift)     : ");
			bits=new unsigned char[width*height*4];
			if(!bits) _throw("Could not allocate buffer");
			glPushAttrib(GL_PIXEL_MODE_BIT);
			glReadBuffer(GL_FRONT);
			CI_READ_SHIFT_TEST(unsigned char, GL_UNSIGNED_BYTE);
			CI_READ_SHIFT_TEST(char, GL_BYTE);
			CI_READ_SHIFT_TEST(unsigned short, GL_UNSIGNED_SHORT);
			CI_READ_SHIFT_TEST(short, GL_SHORT);
			CI_READ_SHIFT_TEST(unsigned int, GL_UNSIGNED_INT);
			CI_READ_SHIFT_TEST(int, GL_INT);
			CI_READ_SHIFT_TEST(float, GL_FLOAT);
			glPopAttrib();
			clr.next();
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		if(bits) { delete [] bits;  bits=NULL; }
		fflush(stdout);

	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(bits) delete [] bits;
	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
	}
	if(win) { XDestroyWindow(dpy, win);  win=0; }
	if(vis) { XFree(vis);  vis=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


int cfgid(Display *dpy, GLXFBConfig config);


#define GET_CFG_ATTRIB(config, attrib, ctemp) {  \
	ctemp=-10;  \
	glXGetFBConfigAttrib(dpy, config, attrib, &ctemp);  \
	if(ctemp==-10) _error(#attrib" cfg attrib not supported");  \
}

#define GET_VIS_ATTRIB(vis, attrib, vtemp) {  \
	vtemp=-20;  \
	glXGetConfig(dpy, vis, attrib, &vtemp);  \
	if(vtemp==-20) _error(#attrib" vis attrib not supported");  \
}

#define COMPARE_ATTRIB(config, vis, attrib, ctemp) {  \
	GET_CFG_ATTRIB(config, attrib, ctemp);  \
	GET_VIS_ATTRIB(vis, attrib, vtemp);  \
	if(ctemp!=vtemp)  \
		_prerror5("%s=%d in C%.2x & %d in V%.2x", #attrib, ctemp,  \
			cfgid(dpy, config), vtemp, vis? (unsigned int)vis->visualid:0);  \
}


void configVsVisual(Display *dpy, GLXFBConfig config, XVisualInfo *vis)
{
	int ctemp, vtemp, r, g, b, bs;
	if(!dpy) _error("Invalid display handle");
	if(!config) _error("Invalid FB config");
	if(!vis) _error("Invalid visual pointer");
	GET_CFG_ATTRIB(config, GLX_VISUAL_ID, ctemp);
	if(ctemp!=(int)vis->visualid)
		_error("Visual ID mismatch");
	GET_CFG_ATTRIB(config, GLX_RENDER_TYPE, ctemp);
	GET_VIS_ATTRIB(vis, GLX_RGBA, vtemp);
	if((ctemp==GLX_RGBA_BIT && vtemp!=1) ||
		(ctemp==GLX_COLOR_INDEX_BIT && vtemp!=0))
		_error("GLX_RGBA mismatch w/ X visual");
	COMPARE_ATTRIB(config, vis, GLX_BUFFER_SIZE, bs);
	COMPARE_ATTRIB(config, vis, GLX_LEVEL, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_DOUBLEBUFFER, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_STEREO, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_AUX_BUFFERS, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_RED_SIZE, r);
	COMPARE_ATTRIB(config, vis, GLX_GREEN_SIZE, g);
	COMPARE_ATTRIB(config, vis, GLX_BLUE_SIZE, b);
	COMPARE_ATTRIB(config, vis, GLX_ALPHA_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_DEPTH_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_STENCIL_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_RED_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_GREEN_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_BLUE_SIZE, ctemp);
	COMPARE_ATTRIB(config, vis, GLX_ACCUM_ALPHA_SIZE, ctemp);
	#ifdef GLX_SAMPLE_BUFFERS_ARB
	COMPARE_ATTRIB(config, vis, GLX_SAMPLE_BUFFERS_ARB, ctemp);
	#endif
	#ifdef GLX_SAMPLES_ARB
	COMPARE_ATTRIB(config, vis, GLX_SAMPLES_ARB, ctemp);
	#endif
	#ifdef GLX_X_VISUAL_TYPE_EXT
	COMPARE_ATTRIB(config, vis, GLX_X_VISUAL_TYPE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_TYPE_EXT
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_TYPE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_INDEX_VALUE_EXT
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_INDEX_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_RED_VALUE_EXT
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_RED_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_GREEN_VALUE_EXT
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_GREEN_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_BLUE_VALUE_EXT
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_BLUE_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_ALPHA_VALUE_EXT
	COMPARE_ATTRIB(config, vis, GLX_TRANSPARENT_ALPHA_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_VIDEO_RESIZE_SUN
	COMPARE_ATTRIB(config, vis, GLX_VIDEO_RESIZE_SUN, ctemp);
	#endif
	#ifdef GLX_VIDEO_REFRESH_TIME_SUN
	COMPARE_ATTRIB(config, vis, GLX_VIDEO_REFRESH_TIME_SUN, ctemp);
	#endif
	#ifdef GLX_GAMMA_VALUE_SUN
	COMPARE_ATTRIB(config, vis, GLX_GAMMA_VALUE_SUN, ctemp);
	#endif
}


int cfgid(Display *dpy, GLXFBConfig config)
{
	int temp=0;
	if(!config) _error("config==NULL in cfgid()");
	if(!dpy) _error("display==NULL in cfgid()");
	GET_CFG_ATTRIB(config, GLX_FBCONFIG_ID, temp);
	return temp;
}


void queryContextTest(Display *dpy, XVisualInfo *vis, GLXFBConfig config)
{
	GLXContext ctx=0;  int renderType, fbcid, temp;
	try
	{
		int visual_caveat;
		GET_CFG_ATTRIB(config, GLX_CONFIG_CAVEAT, visual_caveat);
		if(visual_caveat==GLX_NON_CONFORMANT_CONFIG) return;
		GET_CFG_ATTRIB(config, GLX_RENDER_TYPE, renderType);
		renderType=(renderType==GLX_COLOR_INDEX_BIT)?
			GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE;
		fbcid=cfgid(dpy, config);

		if(!(ctx=glXCreateNewContext(dpy, config, renderType, NULL, True)))
			_error("glXCreateNewContext");
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &temp);
		if(temp!=fbcid) _error("glXQueryContext FB cfg ID");
		GET_CFG_ATTRIB(config, GLX_RENDER_TYPE, renderType);
		renderType=(renderType==GLX_COLOR_INDEX_BIT)?
			GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE;
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_RENDER_TYPE, &temp);
		if(temp!=renderType) _error("glXQueryContext render type");
		glXDestroyContext(dpy, ctx);  ctx=0;

		if(!(ctx=glXCreateContext(dpy, vis, NULL, True)))
			_error("glXCreateNewContext");
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &temp);
		if(temp!=fbcid) _error("glXQueryContext FB cfg ID");
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_RENDER_TYPE, &temp);
		if(temp!=renderType) _error("glXQueryContext render type");
		glXDestroyContext(dpy, ctx);  ctx=0;
	}
	catch(...)
	{
		if(ctx) { glXDestroyContext(dpy, ctx);  ctx=0; }
		throw;
	}
}


GLXFBConfig getFBConfigFromVisual(Display *dpy, XVisualInfo *vis)
{
	GLXContext ctx=0;  int temp, fbcid=0, n=0;
	GLXFBConfig *configs=NULL, config=0;
	try
	{
		if(!(ctx=glXCreateContext(dpy, vis, NULL, True)))
			_error("glXCreateNewContext");
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &fbcid);
		glXDestroyContext(dpy, ctx);  ctx=0;
		if(!(configs=glXGetFBConfigs(dpy, DefaultScreen(dpy), &n)) || n==0)
			_error("Cannot map visual to FB config");
		for(int i=0; i<n; i++)
		{
			temp=cfgid(dpy, configs[i]);
			if(temp==fbcid) { config=configs[i];  break; }
		}
		XFree(configs);  configs=NULL;
		if(!config) _error("Cannot map visual to FB config");
		return config;
	}
	catch(...)
	{
		if(ctx) { glXDestroyContext(dpy, ctx);  ctx=0; }
		if(configs) { XFree(configs);  configs=NULL; }
		throw;
	}
	return 0;
}


// This tests the faker's client/server visual matching heuristics
int visTest(bool ci)
{
	Display *dpy=NULL;
	XVisualInfo **visuals=NULL, *vis0=NULL, vtemp;
	GLXFBConfig config=0, *configs=NULL;  int n=0, i, retval=1;

	printf("Visual matching heuristics test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0))) _throw("Could not open display");

		// This will fail with VGL 2.2.x and earlier
		if(!(configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), NULL, &n))
			|| n==0)
			_throw("No FB configs found");
		XFree(configs);  configs=NULL;

		if(!ci)
		{
			try
			{
				printf("RGBA:   ");

				// Iterate through RGBA attributes
				int rgbattrib[]={ GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
					GLX_ALPHA_SIZE, 0, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
					GLX_STENCIL_SIZE, 0, GLX_ACCUM_RED_SIZE, 0, GLX_ACCUM_GREEN_SIZE, 0,
					GLX_ACCUM_BLUE_SIZE, 0, GLX_ACCUM_ALPHA_SIZE, 0,
					GLX_SAMPLE_BUFFERS, 0, GLX_SAMPLES, 1, GLX_RGBA, None, None, None };
				int rgbattrib13[]={ GLX_DOUBLEBUFFER, 1, GLX_STEREO, 1,
					GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
					GLX_ALPHA_SIZE, 0, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
					GLX_STENCIL_SIZE, 0, GLX_ACCUM_RED_SIZE, 0, GLX_ACCUM_GREEN_SIZE, 0,
					GLX_ACCUM_BLUE_SIZE, 0, GLX_ACCUM_ALPHA_SIZE, 0,
					GLX_SAMPLE_BUFFERS, 0, GLX_SAMPLES, 1, None };

				for(int db=0; db<=1; db++)
				{
					rgbattrib13[1]=db;
					rgbattrib[27]=db? GLX_DOUBLEBUFFER:0;
					for(int stereo=0; stereo<=1; stereo++)
					{
						rgbattrib13[3]=stereo;
						rgbattrib[db? 28:27]=stereo? GLX_STEREO:0;
						for(int alpha=0; alpha<=1; alpha++)
						{
							rgbattrib13[11]=rgbattrib[7]=alpha;
							for(int depth=0; depth<=1; depth++)
							{
								rgbattrib13[13]=rgbattrib[9]=depth;
								for(int aux=0; aux<=1; aux++)
								{
									rgbattrib13[15]=rgbattrib[11]=aux;
									for(int stencil=0; stencil<=1; stencil++)
									{
										rgbattrib13[17]=rgbattrib[13]=stencil;
										for(int accum=0; accum<=1; accum++)
										{
											rgbattrib13[19]=rgbattrib13[21]=rgbattrib13[23]=accum;
											rgbattrib[15]=rgbattrib[17]=rgbattrib[19]=accum;
											if(alpha) { rgbattrib13[25]=rgbattrib[21]=accum; }
											for(int samples=0; samples<=16;
												samples==0? samples=1:samples*=2)
											{
												rgbattrib13[29]=rgbattrib[25]=samples;
												rgbattrib13[27]=rgbattrib[23]=samples? 1:0;

												if((!(configs=glXChooseFBConfig(dpy,
													DefaultScreen(dpy), rgbattrib13, &n)) || n==0)
													&& !stereo && !samples && !aux && !accum)
													_throw("No FB configs found");
												if(!(vis0=glXChooseVisual(dpy, DefaultScreen(dpy),
													rgbattrib)) && !stereo && !samples && !aux && !accum)
													_throw("Could not find visual");
												if(vis0 && configs)
												{
													configVsVisual(dpy, configs[0], vis0);
													XFree(vis0);  XFree(configs);
												}
											}
										}
									}
								}
							}
						}
					}
				}
				printf("SUCCESS!\n");
			}
			catch(Error &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
			fflush(stdout);
		}
		else
		{
			try
			{
				printf("INDEX:  ");

				// Iterate through color index attributes
				int ciattrib[]={ GLX_BUFFER_SIZE, 8, GLX_DEPTH_SIZE, 0,
					GLX_AUX_BUFFERS, 0, GLX_STENCIL_SIZE, 0,
					GLX_TRANSPARENT_TYPE, GLX_NONE, GLX_LEVEL, 0, None, None, None };
				int ciattrib13[]={ GLX_DOUBLEBUFFER, 1, GLX_STEREO, 1,
					GLX_BUFFER_SIZE, 8, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
					GLX_STENCIL_SIZE, 0, GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT,
					GLX_TRANSPARENT_TYPE, GLX_NONE, GLX_LEVEL, 0, None };

				for(int db=0; db<=1; db++)
				{
					ciattrib13[1]=db;
					ciattrib[12]=db? GLX_DOUBLEBUFFER:0;
					for(int stereo=0; stereo<=1; stereo++)
					{
						ciattrib13[3]=stereo;
						ciattrib[db? 13:12]=stereo? GLX_STEREO:0;
						for(int depth=0; depth<=1; depth++)
						{
							ciattrib13[7]=ciattrib[3]=depth;
							for(int aux=0; aux<=1; aux++)
							{
								ciattrib13[9]=ciattrib[5]=aux;
								for(int stencil=0; stencil<=1; stencil++)
								{
									ciattrib13[11]=ciattrib[7]=stencil;
									for(int trans=0; trans<=1; trans++)
									{
										ciattrib13[15]=ciattrib[9]=trans?
											GLX_TRANSPARENT_INDEX:GLX_NONE;

										if((!(configs=glXChooseFBConfig(dpy, DefaultScreen(dpy),
											ciattrib13, &n)) || n==0) && !stereo && !aux && !trans)
											_throw("No FB configs found");
										if(!(vis0=glXChooseVisual(dpy, DefaultScreen(dpy),
											ciattrib)) && !stereo && !aux && !trans)
											_throw("Could not find visual");
										if(vis0 && configs)
										{
											configVsVisual(dpy, configs[0], vis0);
											XFree(vis0);  XFree(configs);
										}
									}
								}
							}
						}
					}
				}
				printf("SUCCESS!\n");
			}
			catch(Error &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
			fflush(stdout);
		}

		printf("\n");
		if(!(configs=glXGetFBConfigs(dpy, DefaultScreen(dpy), &n)) || n==0)
			_throw("No FB configs found");

		int fbcid=0;
		if(!(visuals=(XVisualInfo **)malloc(sizeof(XVisualInfo *)*n)))
			_throw("Memory allocation error");
		memset(visuals, 0, sizeof(XVisualInfo *)*n);

		for(i=0; i<n; i++)
		{
			if(!configs[i]) continue;
			try
			{
				int renderType;
				fbcid=cfgid(dpy, configs[i]);
				GET_CFG_ATTRIB(configs[i], GLX_RENDER_TYPE, renderType);
				if((ci && renderType!=GLX_COLOR_INDEX_BIT)
					|| (!ci && renderType==GLX_COLOR_INDEX_BIT))
					continue;
				if(!(visuals[i]=glXGetVisualFromFBConfig(dpy, configs[i])))
				{
					printf("CFG 0x%.2x:  ", fbcid);
					_error("No matching X visual for CFG");
				}
			}
			catch(Error &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
		}

		for(i=0; i<n; i++)
		{
			XVisualInfo *vis1=NULL;
			if(!configs[i]) continue;
			try
			{
				int renderType;
				fbcid=cfgid(dpy, configs[i]);
				GET_CFG_ATTRIB(configs[i], GLX_RENDER_TYPE, renderType);
				if((ci && renderType!=GLX_COLOR_INDEX_BIT)
					|| (!ci && renderType==GLX_COLOR_INDEX_BIT))
					continue;
				printf("CFG 0x%.2x:  ", fbcid);
				if(!(vis1=glXGetVisualFromFBConfig(dpy, configs[i])))
					_error("No matching X visual for CFG");

				configVsVisual(dpy, configs[i], visuals[i]);
				configVsVisual(dpy, configs[i], vis1);
				queryContextTest(dpy, visuals[i], configs[i]);
				queryContextTest(dpy, vis1, configs[i]);

				config=getFBConfigFromVisual(dpy, visuals[i]);
				if(!config || cfgid(dpy, config)!=fbcid)
					_error("getFBConfigFromVisual");
				config=getFBConfigFromVisual(dpy, vis1);
				if(!config || cfgid(dpy, config)!=fbcid)
					_error("getFBConfigFromVisual");

				printf("SUCCESS!\n");
			}
			catch(Error &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
			if(vis1) { XFree(vis1);  vis1=NULL; }
		}

		XFree(configs);  configs=NULL;
		for(i=0; i<n; i++) { if(visuals[i]) XFree(visuals[i]); }
		free(visuals);  visuals=NULL;  n=0;
		fflush(stdout);

		if(!(vis0=XGetVisualInfo(dpy, VisualNoMask, &vtemp, &n)) || n==0)
			_throw("No X Visuals found");
		printf("\n");

		for(int i=0; i<n; i++)
		{
			XVisualInfo *vis2=NULL;
			try
			{
				int level=0;
				glXGetConfig(dpy, &vis0[i], GLX_LEVEL, &level);
				if(level) continue;
				printf("Vis 0x%.2x:  ", (int)vis0[i].visualid);
				if(!(config=getFBConfigFromVisual(dpy, &vis0[i])))
					_error("No matching CFG for X Visual");
				configVsVisual(dpy, config, &vis0[i]);
				vis2=glXGetVisualFromFBConfig(dpy, config);
				configVsVisual(dpy, config, vis2);

				printf("SUCCESS!\n");
			}
			catch(Error &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
			if(vis2) { XFree(vis2);  vis2=NULL; }
		}
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);

	if(visuals && n)
	{
		for(i=0; i<n; i++) { if(visuals[i]) XFree(visuals[i]); }
		free(visuals);  visuals=NULL;
	}
	if(vis0) { XFree(vis0);  vis0=NULL; }
	if(configs) { XFree(configs);  configs=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


#define NTHREADS 30
bool deadYet=false;

class TestThread : public Runnable
{
	public:

		TestThread(int myRank_, Display *dpy_, Window win_, GLXContext ctx_)
			: myRank(myRank_), dpy(dpy_), win(win_), ctx(ctx_), doResize(false)
		{
		}

		void run(void)
		{
			int clr=myRank%NC, lastFrame=0;
			if(!(glXMakeCurrent(dpy, win, ctx)))
				_error("Could not make context current");
			while(!deadYet)
			{
				if(doResize)
				{
					glViewport(0, 0, width, height);
					doResize=false;
				}
				glClearColor(colors[clr].r, colors[clr].g, colors[clr].b, 0.);
				glClear(GL_COLOR_BUFFER_BIT);
				glReadBuffer(GL_FRONT);
				glXSwapBuffers(dpy, win);
				checkReadbackState(GL_FRONT, dpy, win, win, ctx);
				checkFrame(win, 1, lastFrame);
				checkWindowColor(win, colors[clr].bits, false);
				clr=(clr+1)%NC;
			}
		}

		void resize(int width_, int height_)
		{
			width=width_;  height=height_;
			doResize=true;
		}

	private:

		int myRank;
		Display *dpy;
		Window win;
		GLXContext ctx;
		bool doResize;
		int width, height;
};


int multiThreadTest(int nThreads)
{
	int glxattrib[]={ GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis=NULL;
	Display *dpy=NULL;  Window windows[NTHREADS];
	GLXContext contexts[NTHREADS];
	TestThread *testThreads[NTHREADS];  Thread *threads[NTHREADS];
	XSetWindowAttributes swa;
	int i, retval=1;

	if(nThreads==0) return 1;
	for(i=0; i<nThreads; i++)
	{
		windows[i]=0;  contexts[i]=0;  testThreads[i]=NULL;  threads[i]=NULL;
	}

	printf("Multi-threaded rendering test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0))) _throw("Could not open display");

		if((vis=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");
		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=StructureNotifyMask|ExposureMask;
		for(i=0; i<nThreads; i++)
		{
			int winX=(i%10)*100, winY=(i/10)*120;
			if((windows[i]=XCreateWindow(dpy, root, winX, winY, 100, 100, 0,
				vis->depth, InputOutput, vis->visual,
				CWBorderPixel|CWColormap|CWEventMask, &swa))==0)
				_throw("Could not create window");
			XMapWindow(dpy, windows[i]);
			if(!(contexts[i]=glXCreateContext(dpy, vis, NULL, True)))
				_throw("Could not establish GLX context");
			XMoveResizeWindow(dpy, windows[i], winX, winY, 100, 100);
		}
		XSync(dpy, False);
		XFree(vis);  vis=NULL;

		for(i=0; i<nThreads; i++)
		{
			testThreads[i]=new TestThread(i, dpy, windows[i], contexts[i]);
			threads[i]=new Thread(testThreads[i]);
			if(!testThreads[i] || !threads[i])
				_prerror1("Could not create thread %d", i);
			threads[i]->start();
		}
		printf("Phase 1\n");
		for(i=0; i<nThreads; i++)
		{
			int winX=(i%10)*100, winY=i/10*120;
			XMoveResizeWindow(dpy, windows[i], winX, winY, 200, 200);
			testThreads[i]->resize(200, 200);
			if(i<5) usleep(0);
			XResizeWindow(dpy, windows[i], 100, 100);
			testThreads[i]->resize(100, 100);
		}
		XSync(dpy, False);
		fflush(stdout);

		printf("Phase 2\n");
		for(i=0; i<nThreads; i++)
		{
			XWindowChanges xwc;
			xwc.width=xwc.height=200;
			XConfigureWindow(dpy, windows[i], CWWidth|CWHeight, &xwc);
			testThreads[i]->resize(200, 200);
		}
		XSync(dpy, False);
		fflush(stdout);

		printf("Phase 3\n");
		for(i=0; i<nThreads; i++)
		{
			XResizeWindow(dpy, windows[i], 100, 100);
			testThreads[i]->resize(100, 100);
		}
		XSync(dpy, False);
		deadYet=true;
		for(i=0; i<nThreads; i++) threads[i]->stop();
		for(i=0; i<nThreads; i++)
		{
			try
			{
				threads[i]->checkError();
			}
			catch(Error &e)
			{
				printf("Thread %d failed! (%s)\n", i, e.getMessage());  retval=0;
			}
		}
		if(retval==1) printf("SUCCESS!\n");
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);

	for(i=0; i<nThreads; i++)
	{
		if(threads[i]) { delete threads[i];  threads[i]=NULL; }
	}
	for(i=0; i<nThreads; i++)
	{
		if(testThreads[i]) { delete testThreads[i];  testThreads[i]=NULL; }
	}
	if(vis) { XFree(vis);  vis=NULL; }
	for(i=0; i<nThreads; i++)
	{
		if(dpy && contexts[i])
		{
			glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, contexts[i]);
			contexts[i]=0;
		}
	}
	for(i=0; i<nThreads; i++)
	{
		if(dpy && windows[i]) { XDestroyWindow(dpy, windows[i]);  windows[i]=0; }
	}
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


#define COMPARE_DRAW_ATTRIB(dpy, draw, value, attrib) {  \
	if(value>=0) {  \
		unsigned int temp=0xffffffff;  \
		glXQueryDrawable(dpy, draw, attrib, &temp);  \
		if(temp==0xffffffff)  \
			_throw(#attrib" attribute not supported");  \
		if(temp!=(unsigned int)value)  \
			_prerror3("%s=%d (should be %d)", #attrib, temp, value);  \
	}  \
}

void checkDrawable(Display *dpy, GLXDrawable draw, int width, int height,
	int preservedContents, int largestPbuffer, int fbcid)
{
	if(!dpy || !draw) _throw("Invalid argument to checkdrawable()");
	COMPARE_DRAW_ATTRIB(dpy, draw, width, GLX_WIDTH);
	COMPARE_DRAW_ATTRIB(dpy, draw, height, GLX_HEIGHT);
	COMPARE_DRAW_ATTRIB(dpy, draw, preservedContents, GLX_PRESERVED_CONTENTS);
	COMPARE_DRAW_ATTRIB(dpy, draw, largestPbuffer, GLX_LARGEST_PBUFFER);
	COMPARE_DRAW_ATTRIB(dpy, draw, fbcid, GLX_FBCONFIG_ID);
}

#define VERIFY_BUF_COLOR(buf, colorShouldBe, tag) {  \
	if(buf>0) glReadBuffer(buf);  \
	unsigned int color=checkBufferColor(false);  \
	if(color!=(colorShouldBe))  \
		_prerror2(tag" is 0x%.6x, should be 0x%.6x", color, (colorShouldBe));  \
}

// Test off-screen rendering
int offScreenTest(void)
{
	Display *dpy=NULL;  Window win=0;  Pixmap pm0=0, pm1=0, pm2=0;
	GLXPixmap glxpm0=0, glxpm1=0;  GLXPbuffer pb=0;  GLXWindow glxwin=0;
	int dpyw, dpyh, lastFrame=0, retval=1;
	int glxattrib[]={ GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis=NULL;  GLXFBConfig config=0, *configs=NULL;  int n=0;
	GLXContext ctx=0;
	XSetWindowAttributes swa;
	XFontStruct *fontInfo=NULL;  int minChar, maxChar;
	int fontListBase=0;
	GLuint fbo=0, rbo=0;
	TestColor clr(false, 0);

	printf("Off-screen rendering test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if(!(fontInfo=XLoadQueryFont(dpy, "fixed")))
			_throw("Could not load X font");
		minChar=fontInfo->min_char_or_byte2;
		maxChar=fontInfo->max_char_or_byte2;
		fontListBase=glGenLists(maxChar+1);

		if((configs=glXChooseFBConfigSGIX(dpy, DefaultScreen(dpy), glxattrib, &n))
			==NULL || n==0) _throw("Could not find a suitable FB config");
		config=configs[0];
		int fbcid=cfgid(dpy, config);
		XFree(configs);  configs=NULL;
		if((vis=glXGetVisualFromFBConfigSGIX(dpy, config))==NULL)
			_throw("Could not find matching visual for FB config");

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel=0;
		swa.background_pixel=0;
		swa.event_mask = 0;
		if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XMapWindow(dpy, win);
		if((glxwin=glXCreateWindow(dpy, config, win, NULL))==0)
			_throw("Could not create GLX window");
		checkDrawable(dpy, glxwin, dpyw/2, dpyh/2, -1, -1, fbcid);

		if((pm0=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, vis->depth))==0
			|| (pm1=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, vis->depth))==0
			|| (pm2=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, vis->depth))==0)
			_throw("Could not create pixmap");
		if((glxpm0=glXCreateGLXPixmap(dpy, vis, pm0))==0
			|| (glxpm1=glXCreatePixmap(dpy, config, pm1, NULL))==0)
			_throw("Could not create GLX pixmap");
		checkDrawable(dpy, glxpm0, dpyw/2, dpyh/2, -1, -1, fbcid);
		checkDrawable(dpy, glxpm1, dpyw/2, dpyh/2, -1, -1, fbcid);

		int pbattribs[]={ GLX_PBUFFER_WIDTH, dpyw/2, GLX_PBUFFER_HEIGHT, dpyh/2,
			GLX_PRESERVED_CONTENTS, True, GLX_LARGEST_PBUFFER, False, None };
		if((pb=glXCreatePbuffer(dpy, config, pbattribs))==0)
			_throw("Could not create Pbuffer");
		checkDrawable(dpy, pb, dpyw/2, dpyh/2, 1, 0, fbcid);
		unsigned int tempw=0, temph=0;
		glXQueryGLXPbufferSGIX(dpy, pb, GLX_WIDTH_SGIX, &tempw);
		glXQueryGLXPbufferSGIX(dpy, pb, GLX_HEIGHT_SGIX, &temph);
		if(tempw!=(unsigned int)dpyw/2 || temph!=(unsigned int)dpyh/2)
			_throw("glXQueryGLXPbufferSGIX() failed");

		if(!(ctx=glXCreateContextWithConfigSGIX(dpy, config, GLX_RGBA_TYPE, NULL,
			True)))
			_throw("Could not create context");

		if(!glXMakeContextCurrent(dpy, glxwin, glxwin, ctx))
			_throw("Could not make context current");
		checkCurrent(dpy, glxwin, glxwin, ctx);
		if(!doubleBufferTest(false))
			_throw("Double buffering appears to be broken");

		if(!glXMakeContextCurrent(dpy, pb, pb, ctx))
			_throw("Could not make context current");
		checkCurrent(dpy, pb, pb, ctx);
		if(!doubleBufferTest(false))
			_throw("Double-buffered off-screen rendering not available");
		checkFrame(win, -1, lastFrame);

		try
		{
			printf("Pbuffer->Window:                ");
			if(!(glXMakeContextCurrent(dpy, pb, pb, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, pb, pb, ctx);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "PB");
			if(!(glXMakeContextCurrent(dpy, glxwin, pb, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxwin, pb, ctx);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw/2, dpyh/2, GL_COLOR);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, glxwin);
			checkFrame(win, 1, lastFrame);
			checkReadbackState(GL_FRONT, dpy, glxwin, pb, ctx);
			checkWindowColor(win, clr.bits(-2), false);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("Window->Pbuffer:                ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontInfo->fid, minChar, maxChar-minChar+1,
				fontListBase+minChar);
			checkCurrent(dpy, glxwin, glxwin, ctx);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "Win");
			if(!(glXMakeContextCurrent(dpy, pb, glxwin, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontInfo->fid, minChar, maxChar-minChar+1,
				fontListBase+minChar);
			checkCurrent(dpy, pb, glxwin, ctx);
			checkFrame(win, 1, lastFrame);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw/2, dpyh/2, GL_COLOR);
			glXSwapBuffers(dpy, pb);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "PB");
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("FBO->Window:                    ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxwin, glxwin, ctx);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "Win");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxwin, glxwin, ctx);
			glDrawBuffer(GL_BACK);
			glGenFramebuffersEXT(1, &fbo);
			glGenRenderbuffersEXT(1, &rbo);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbo);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, dpyw/2, dpyh/2);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rbo);
			clr.clear(0);
			VERIFY_BUF_COLOR(0, clr.bits(-1), "FBO");
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
			glFramebufferRenderbufferEXT(GL_DRAW_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, 0);
			glDrawBuffer(GL_BACK);
			glXSwapBuffers(dpy, glxwin);
			checkFrame(win, 1, lastFrame);
			checkReadbackState(GL_COLOR_ATTACHMENT0_EXT, dpy, glxwin, glxwin, ctx);
			checkWindowColor(win, clr.bits(-3), false);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_RENDERBUFFER_EXT, 0);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		if(rbo) { glDeleteRenderbuffersEXT(1, &rbo);  rbo=0; }
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		if(fbo) { glDeleteFramebuffersEXT(1, &fbo);  fbo=0; }

		try
		{
			printf("Pixmap->Window:                 ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontInfo->fid, minChar, maxChar-minChar+1,
				fontListBase+minChar);
			checkCurrent(dpy, glxpm0, glxpm0, ctx);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, win, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkReadbackState(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkFrame(win, 1, lastFrame);
			checkWindowColor(win, clr.bits(-1), false);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("Window->Pixmap:                 ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontInfo->fid, minChar, maxChar-minChar+1,
				fontListBase+minChar);
			checkCurrent(dpy, glxwin, glxwin, ctx);
			clr.clear(GL_FRONT);
			clr.clear(GL_BACK);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-2), "Win");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxpm1, glxpm1, ctx);
			checkFrame(win, 1, lastFrame);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, win, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkReadbackState(GL_BACK, dpy, glxpm1, glxpm1, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkFrame(win, 0, lastFrame);
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-2), "PM1");
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-2), "PM1");
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("Pixmap->Pixmap:                 ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontInfo->fid, minChar, maxChar-minChar+1,
				fontListBase+minChar);
			checkCurrent(dpy, glxpm0, glxpm0, ctx);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxpm1, glxpm1, ctx);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkReadbackState(GL_BACK, dpy, glxpm1, glxpm1, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			VERIFY_BUF_COLOR(GL_BACK, clr.bits(-1), "PM1");
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM1");
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("GLX Pixmap->2D Pixmap:          ");
			lastFrame=0;
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxpm0, glxpm0, ctx);

			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm2, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkReadbackState(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkFrame(pm0, 1, lastFrame);
			checkWindowColor(pm0, clr.bits(-1), false);

			clr.clear(GL_FRONT);
			clr.clear(GL_BACK);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-2), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XImage *xi=XGetImage(dpy, pm0, 0, 0, dpyw/2, dpyh/2, AllPlanes, ZPixmap);
			if(xi) XDestroyImage(xi);
			checkReadbackState(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkFrame(pm0, 1, lastFrame);
			checkWindowColor(pm0, clr.bits(-2), false);

			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			// Same as above, but with a deleted GLX pixmap
			printf("Deleted GLX Pixmap->2D Pixmap:  ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			checkCurrent(dpy, glxpm0, glxpm0, ctx);
			clr.clear(GL_FRONT);
			VERIFY_BUF_COLOR(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			if(!glXMakeContextCurrent(dpy, 0, 0, 0))
				_error("Could not make context current");
			glXDestroyPixmap(dpy, glxpm0);  glxpm0=0;
			XCopyArea(dpy, pm0, pm2, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkFrame(pm0, 1, lastFrame);
			checkWindowColor(pm0, clr.bits(-1), false);
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_RENDERBUFFER_EXT, 0);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	if(rbo) { glDeleteRenderbuffersEXT(1, &rbo);  rbo=0; }
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if(fbo) { glDeleteFramebuffersEXT(1, &fbo);  fbo=0; }
	if(ctx && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
	}
	if(pb && dpy) { glXDestroyPbuffer(dpy, pb);  pb=0; }
	if(glxpm1 && dpy) { glXDestroyGLXPixmap(dpy, glxpm1);  glxpm1=0; }
	if(glxpm0 && dpy) { glXDestroyGLXPixmap(dpy, glxpm0);  glxpm0=0; }
	if(pm2 && dpy) { XFreePixmap(dpy, pm2);  pm2=0; }
	if(pm1 && dpy) { XFreePixmap(dpy, pm1);  pm1=0; }
	if(pm0 && dpy) { XFreePixmap(dpy, pm0);  pm0=0; }
	if(glxwin && dpy) { glXDestroyWindow(dpy, glxwin);  glxwin=0; }
	if(win && dpy) { XDestroyWindow(dpy, win);  win=0; }
	if(vis) { XFree(vis);  vis=NULL; }
	if(configs) { XFree(configs);  configs=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


// Test whether glXMakeCurrent() can handle mismatches between the FB config
// of the context and the off-screen drawable

int contextMismatchTest(void)
{
	Display *dpy=NULL;  Window win=0;
	int dpyw, dpyh, retval=1;
	int glxattrib1[]={ GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	int glxattrib2[]={ GLX_DOUBLEBUFFER, 0, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
		None };
	XVisualInfo *vis=NULL;
	GLXFBConfig config1=0, config2=0, *configs=NULL;  int n=0;
	GLXContext ctx1=0, ctx2=0;
	XSetWindowAttributes swa;

	printf("Context FB config mismatch test:\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib1,
			&n))==NULL || n==0)
			_throw("Could not find a suitable FB config");
		config1=configs[0];
		if(!(vis=glXGetVisualFromFBConfig(dpy, config1)))
			_throw("glXGetVisualFromFBConfig()");
		XFree(configs);  configs=NULL;

		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib2,
			&n))==NULL || n==0)
			_throw("Could not find a suitable FB config");
		config2=configs[0];
		XFree(configs);  configs=NULL;

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, vis->visual, AllocNone);
		swa.border_pixel=0;
		swa.background_pixel=0;
		swa.event_mask = 0;
		if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, vis->depth,
			InputOutput, vis->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XMapWindow(dpy, win);

		try
		{
			if(!(ctx1=glXCreateNewContext(dpy, config1, GLX_RGBA_TYPE, NULL, True)))
				_throw("Could not create context");
			if(!(ctx2=glXCreateNewContext(dpy, config2, GLX_RGBA_TYPE, NULL, True)))
				_throw("Could not create context");

			if(!(glXMakeCurrent(dpy, win, ctx1)))
				_error("Could not make context current");
			if(!(glXMakeCurrent(dpy, win, ctx2)))
				_error("Could not make context current");

			if(!(glXMakeContextCurrent(dpy, win, win, ctx1)))
				_error("Could not make context current");
			if(!(glXMakeContextCurrent(dpy, win, win, ctx2)))
				_error("Could not make context current");

			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx1 && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx1);
		ctx1=0;
	}
	if(ctx2 && dpy)
	{
		glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx2);
		ctx2=0;
	}
	if(win && dpy) { XDestroyWindow(dpy, win);  win=0; }
	if(vis) { XFree(vis);  vis=NULL; }
	if(configs) { XFree(configs);  configs=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


// Test whether VirtualGL properly handles explicit and implicit destruction of
// subwindows

int subWinTest(void)
{
	Display *dpy=NULL;  Window win=0, win1=0, win2=0;
	TestColor clr(false, 0);
	int dpyw, dpyh, retval=1, lastFrame=0;
	int glxattrib[]={ GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
	XVisualInfo *vis=NULL;
	GLXContext ctx=0;
	XSetWindowAttributes swa;

	printf("Subwindow destruction test:\n\n");

	try
	{
		try
		{
			for(int i=0; i<20; i++)
			{
				if(!dpy && !(dpy=XOpenDisplay(0)))
					_throw("Could not open display");
				dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
				dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

				if((vis=glXChooseVisual(dpy, DefaultScreen(dpy),
					glxattrib))==NULL)
					_throw("Could not find a suitable visual");

				Window root=RootWindow(dpy, DefaultScreen(dpy));
				swa.colormap=XCreateColormap(dpy, root, vis->visual, AllocNone);
				swa.border_pixel=0;
				swa.background_pixel=0;
				swa.event_mask = 0;
				if(!win && (win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0,
					vis->depth, InputOutput, vis->visual,
					CWBorderPixel|CWColormap|CWEventMask, &swa))==0)
					_throw("Could not create window");
				if((win1=XCreateWindow(dpy, win, 0, 0, dpyw/2, dpyh/2, 0, vis->depth,
					InputOutput, vis->visual, CWBorderPixel|CWColormap|CWEventMask,
					&swa))==0)
					_throw("Could not create subwindow");
				if((win2=XCreateWindow(dpy, win1, 0, 0, dpyw/2, dpyh/2, 0, vis->depth,
					InputOutput, vis->visual, CWBorderPixel|CWColormap|CWEventMask,
					&swa))==0)
					_throw("Could not create subwindow");
				XMapSubwindows(dpy, win);
				XMapWindow(dpy, win);

				lastFrame=0;
				if(!(ctx=glXCreateContext(dpy, vis, NULL, True)))
					_throw("Could not create context");
				XFree(vis);  vis=NULL;
				if(!(glXMakeCurrent(dpy, win2, ctx)))
					_error("Could not make context current");
				clr.clear(GL_BACK);
				glXSwapBuffers(dpy, win2);
				checkFrame(win2, 1, lastFrame);
				checkWindowColor(win2, clr.bits(-1), false);
				glXMakeCurrent(dpy, 0, 0);
				glXDestroyContext(dpy, ctx);  ctx=0;

				if(i%3==0) { XCloseDisplay(dpy);  dpy=NULL;  win=0; }
				else if(i%3==1) { XDestroyWindow(dpy, win);  win=0; }
				else XDestroySubwindows(dpy, win);
			}
			printf("SUCCESS\n");
		}
		catch(Error &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx && dpy)
	{
		glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;
	}
	if(win && dpy) { XDestroyWindow(dpy, win);  win=0; }
	if(vis) { XFree(vis);  vis=NULL; }
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


int extensionQueryTest(void)
{
	Display *dpy=NULL;  int retval=1;
	int dummy1=-1, dummy2=-1, dummy3=-1;

	printf("Extension query test:\n\n");

	try
	{
		int major=-1, minor=-1;
		if((dpy=XOpenDisplay(0))==NULL)
			_throw("Could not open display");
		if(!XQueryExtension(dpy, "GLX", &dummy1, &dummy2, &dummy3)
			|| dummy1<0 || dummy2<0 || dummy3<0)
			_throw("GLX Extension not reported as present");
		char *vendor=XServerVendor(dpy);
		if(!vendor || strcmp(vendor, "Spacely Sprockets, Inc."))
			_throw("XServerVendor()");
		glXQueryVersion(dpy, &major, &minor);
		printf("glXQueryVersion():  %d.%d\n", major, minor);
		printf("glXGetClientString():\n");
		printf("  Version=%s\n", glXGetClientString(dpy, GLX_VERSION));
		printf("  Vendor=%s\n", glXGetClientString(dpy, GLX_VENDOR));
		printf("  Extensions=%s\n", glXGetClientString(dpy, GLX_EXTENSIONS));
		printf("glXQueryServerString():\n");
		printf("  Version=%s\n",
			glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VERSION));
		printf("  Vendor=%s\n",
			glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VENDOR));
		printf("  Extensions=%s\n",
			glXQueryServerString(dpy, DefaultScreen(dpy), GLX_EXTENSIONS));
		if(major<1 || minor<3)
			_throw("glXQueryVersion() reports version < 1.3");
		printf("glXQueryExtensionsString():\n%s\n",
			glXQueryExtensionsString(dpy, DefaultScreen(dpy)));
		printf("SUCCESS!\n");
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);
	if(dpy) { XCloseDisplay(dpy);  dpy=NULL; }
	return retval;
}


#define TEST_PROC_SYM(f)  \
	if((sym=(void *)glXGetProcAddressARB((const GLubyte *)#f))==NULL)  \
		_throw("glXGetProcAddressARB(\""#f"\") returned NULL");  \
	else if(sym!=(void *)f)  \
		_throw("glXGetProcAddressARB(\""#f"\")!="#f);

int procAddrTest(void)
{
	int retval=1;  void *sym=NULL;

	printf("glXGetProcAddress test:\n\n");

	try
	{
		TEST_PROC_SYM(glXChooseVisual)
		TEST_PROC_SYM(glXCreateContext)
		TEST_PROC_SYM(glXMakeCurrent)
		TEST_PROC_SYM(glXChooseFBConfig)
		TEST_PROC_SYM(glXCreateNewContext)
		TEST_PROC_SYM(glXMakeContextCurrent)
		printf("SUCCESS!\n");
	}
	catch(Error &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);
	return retval;
}


int main(int argc, char **argv)
{
	int ret=0;  int nThreads=NTHREADS;  bool doCI=false, doStereo=true;

	if(putenv((char *)"VGL_AUTOTEST=1")==-1
		|| putenv((char *)"VGL_SPOIL=0")==-1
		|| putenv((char *)"VGL_XVENDOR=Spacely Sprockets, Inc.")==-1)
	{
		printf("putenv() failed!\n");  return -1;
	}

	if(argc>1) for(int i=1; i<argc; i++)
	{
		if(!strcasecmp(argv[i], "-n") && i<argc-1)
		{
			int temp=atoi(argv[++i]);
			if(temp>=0 && temp<=NTHREADS) nThreads=temp;
		}
		if(!strcasecmp(argv[i], "-ci")) doCI=true;
		if(!strcasecmp(argv[i], "-nostereo")) doStereo=false;
	}

	// Intentionally leave a pending dlerror()
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat");
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat2");

	if(!XInitThreads())
		_throw("XInitThreads() failed");
	if(doCI)
	{
		if(!readbackTest(false, true)) ret=-1;
		printf("\n");
		if(!ciTest()) ret=-1;
		printf("\n");
		if(!visTest(doCI)) ret=-1;
		printf("\n");
		return ret;
	}
	if(!extensionQueryTest()) ret=-1;
	printf("\n");
	if(!procAddrTest()) ret=-1;
	printf("\n");
	if(!readbackTest(false, false)) ret=-1;
	printf("\n");
	if(doStereo)
	{
		if(!readbackTest(true, false)) ret=-1;
		printf("\n");
	}
	if(!contextMismatchTest()) ret=-1;
	printf("\n");
	if(!flushTest()) ret=-1;
	printf("\n");
	if(!visTest(doCI)) ret=-1;
	printf("\n");
	if(!multiThreadTest(nThreads)) ret=-1;
	printf("\n");
	if(!offScreenTest()) ret=-1;
	printf("\n");
	if(!subWinTest()) ret=-1;
	printf("\n");

	return ret;
}
