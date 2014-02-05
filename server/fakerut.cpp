/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2013 D. R. Commander
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
#include "rrerror.h"
#include "rrthread.h"
#include "glext-vgl.h"

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


#define glClearBuffer(buffer, r, g, b, a) { \
	if(buffer>0) glDrawBuffer(buffer); \
	glClearColor(r, g, b, a); \
	glClear(GL_COLOR_BUFFER_BIT);}

#define glClearBufferci(buffer, i) { \
	if(buffer>0) glDrawBuffer(buffer); \
	glClearIndex(i); \
	glClear(GL_COLOR_BUFFER_BIT);}


#if 0
void clicktocontinue(Display *dpy)
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
#define _error(m) throw(rrerror(__FUNCTION__, m, 0))

#define _prerror1(m, a1) { \
	char temps[256]; \
	snprintf(temps, 255, m, a1); \
	throw(rrerror(__FUNCTION__, temps, 0));  \
}

#define _prerror2(m, a1, a2) { \
	char temps[256]; \
	snprintf(temps, 255, m, a1, a2); \
	throw(rrerror(__FUNCTION__, temps, 0));  \
}

#define _prerror3(m, a1, a2, a3) { \
	char temps[256]; \
	snprintf(temps, 255, m, a1, a2, a3); \
	throw(rrerror(__FUNCTION__, temps, 0));  \
}

#define _prerror5(m, a1, a2, a3, a4, a5) { \
	char temps[256]; \
	snprintf(temps, 255, m, a1, a2, a3, a4, a5); \
	throw(rrerror(__FUNCTION__, temps, 0));  \
}

unsigned int checkbuffercolor(bool ci)
{
	int i, vp[4], ps=ci? 1:3;  unsigned int ret=0;
	unsigned char *buf=NULL;

	try
	{
		vp[0]=vp[1]=vp[2]=vp[3]=0;
		glGetIntegerv(GL_VIEWPORT, vp);
		if(vp[2]<1 || vp[3]<1) _throw("Invalid viewport dimensions");

		if((buf=(unsigned char *)malloc(vp[2]*vp[3]*ps))==NULL)
			_throw("Could not allocate buffer");

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, vp[2], vp[3], ci? GL_COLOR_INDEX:GL_RGB, GL_UNSIGNED_BYTE, buf);
		for(i=3; i<vp[2]*vp[3]*ps; i+=ps)
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


void checkwindowcolor(Window win, unsigned int color, bool ci,
	bool right=false)
{
	char *e=NULL, temps[80];  int fakerclr;
	if(right)
		snprintf(temps, 79, "__VGL_AUTOTESTRCLR%x", (unsigned int)win);
	else
		snprintf(temps, 79, "__VGL_AUTOTESTCLR%x", (unsigned int)win);
	if((e=getenv(temps))==NULL)
		_error("Can't communicate w/ faker");
	if((fakerclr=atoi(e))<0 || (!ci && fakerclr>0xffffff)
		|| (ci && fakerclr>255))
		_error("Bogus data read back");
	if((unsigned int)fakerclr!=color)
	{
		if(right)
			_prerror2("R.buf is 0x%.6x, should be 0x%.6x", fakerclr, color)
		else
			_prerror2("Color is 0x%.6x, should be 0x%.6x", fakerclr, color)
	}
}


void checkframe(Window win, int desiredreadbacks, int &lastframe)
{
	char *e=NULL, temps[80];  int frame;
	snprintf(temps, 79, "__VGL_AUTOTESTFRAME%x", (unsigned int)win);
	if((e=getenv(temps))==NULL || (frame=atoi(e))<1)
		_error("Can't communicate w/ faker");
	if(frame-lastframe!=desiredreadbacks && desiredreadbacks>=0)
		_prerror3("Expected %d readback%s, not %d", desiredreadbacks,
			desiredreadbacks==1?"":"s", frame-lastframe);
	lastframe=frame;
}


void checkcurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
	if(glXGetCurrentDisplay()!=dpy) _throw("glXGetCurrentDisplay() returned incorrect value");
	if(glXGetCurrentDrawable()!=draw) _throw("glXGetCurrentDrawable() returned incorrect value");
	if(glXGetCurrentReadDrawable()!=read) _throw("glXGetCurrentReadDrawable() returned incorrect value");
	if(glXGetCurrentContext()!=ctx) _throw("glXGetCurrentContext() returned incorrect value");
}


void checkreadbackstate(int oldreadbuf, Display *dpy, GLXDrawable draw,
	GLXDrawable read, GLXContext ctx)
{
	if(glXGetCurrentDisplay()!=dpy)
		_error("Current display changed");
	if(glXGetCurrentDrawable()!=draw || glXGetCurrentReadDrawable()!=read)
		_error("Current drawable changed");
	if(glXGetCurrentContext()!=ctx)
		_error("Context changed");
	int readbuf=-1;
	glGetIntegerv(GL_READ_BUFFER, &readbuf);
	if(readbuf!=oldreadbuf)
		_error("Read buffer changed");
}


// Check whether double buffering works properly
int dbtest(bool ci)
{
	unsigned int bcolor=0, fcolor=0;
	int oldreadbuf=GL_BACK, olddrawbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &oldreadbuf);
	glGetIntegerv(GL_DRAW_BUFFER, &olddrawbuf);
	if(ci)
	{
		glClearBufferci(GL_FRONT_AND_BACK, 0);
		glClearBufferci(GL_BACK, 31);
		glClearBufferci(GL_FRONT, 15);
	}
	else
	{
		glClearBuffer(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
		glClearBuffer(GL_BACK, 1., 1., 1., 0.);
		glClearBuffer(GL_FRONT, 1., 0., 1., 0.);
	}
	glReadBuffer(GL_BACK);
	bcolor=checkbuffercolor(ci);
	glReadBuffer(GL_FRONT);
	fcolor=checkbuffercolor(ci);
	glReadBuffer(oldreadbuf);
	glDrawBuffer(olddrawbuf);
	if(!ci && bcolor==0xffffff && fcolor==0xff00ff) return true;
	if(ci && bcolor==31 && fcolor==15) return true;
	return false;
}


// Check whether stereo works properly
int stereotest(bool ci)
{
	unsigned int rcolor=0, lcolor=0;
	int oldreadbuf=GL_BACK, olddrawbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &oldreadbuf);
	glGetIntegerv(GL_DRAW_BUFFER, &olddrawbuf);
	if(ci)
	{
		glClearBufferci(GL_FRONT_AND_BACK, 0);
		glClearBufferci(GL_RIGHT, 31);
		glClearBufferci(GL_LEFT, 15);
	}
	else
	{
		glClearBuffer(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
		glClearBuffer(GL_RIGHT, 1., 1., 1., 0.);
		glClearBuffer(GL_LEFT, 1., 0., 1., 0.);
	}
	glReadBuffer(GL_RIGHT);
	rcolor=checkbuffercolor(ci);
	glReadBuffer(GL_LEFT);
	lcolor=checkbuffercolor(ci);
	glReadBuffer(oldreadbuf);
	glDrawBuffer(olddrawbuf);
	if(!ci && rcolor==0xffffff && lcolor==0xff00ff) return true;
	if(ci && rcolor==31 && lcolor==15) return true;
	return false;
}


struct _testcolor
{
	GLfloat r, g, b;
	unsigned int bits, ndx;
};

#define NC 6
static struct _testcolor tc[NC]=
{
	{1., 0., 0., 0x0000ff, 5},
	{0., 1., 0., 0x00ff00, 10},
	{0., 0., 1., 0xff0000, 15},
	{0., 1., 1., 0xffff00, 20},
	{1., 0., 1., 0xff00ff, 25},
	{1., 1., 0., 0x00ffff, 30}
};

class testcolor
{
	public:

		testcolor(bool ci, int index) : _index(index%NC), _ci(ci)
		{
		}

		GLfloat& r(int rel=0) {return tc[(_index+rel+NC)%NC].r;}
		GLfloat& g(int rel=0) {return tc[(_index+rel+NC)%NC].g;}
		GLfloat& b(int rel=0) {return tc[(_index+rel+NC)%NC].b;}
		void next(void) {_index=(_index+1)%NC;}

		unsigned int& bits(int rel=0)
		{
			if(_ci) return tc[(_index+rel+NC)%NC].ndx;
			else return tc[(_index+rel+NC)%NC].bits;
		}

		void clear(int buffer)
		{
			if(_ci) glClearBufferci(buffer, bits())
			else glClearBuffer(buffer, r(), g(), b(), 0.)
			next();
		}

	private:

		int _index;
		bool _ci;
};


// This tests the faker's readback heuristics
int rbtest(bool stereo, bool ci)
{
	testcolor clr(ci, 0), sclr(ci, 3);
	Display *dpy=NULL;  Window win0=0, win1=0;
	int dpyw, dpyh, lastframe0=0, lastframe1=0, retval=1;
	int rgbattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE,
		8, GLX_BLUE_SIZE, 8, None, None};
	int rgbattrib13[]={GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None, None, None};
	int ciattrib[]={GLX_DOUBLEBUFFER, GLX_BUFFER_SIZE, 8, None, None};
	int ciattrib13[]={GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_BUFFER_SIZE, 8, None, None, None};
	int *glxattrib, *glxattrib13;
	XVisualInfo *v0=NULL, *v1=NULL;  GLXFBConfig c=0, *configs=NULL;  int n=0;
	GLXContext ctx0=0, ctx1=0;
	XSetWindowAttributes swa;

	if(stereo)
	{
		rgbattrib[8]=rgbattrib13[12]=ciattrib[3]=ciattrib13[8]=GLX_STEREO;
		rgbattrib13[13]=ciattrib13[9]=1;
	}
	if(ci) {glxattrib=ciattrib;  glxattrib13=ciattrib13;}
	else {glxattrib=rgbattrib;  glxattrib13=rgbattrib13;}

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

		if((v0=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");
		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib13, &n))
			==NULL || n==0) _throw("Could not find a suitable FB config");
		c=configs[0];
		XFree(configs);  configs=NULL;

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		if(ci)
		{
			swa.colormap=XCreateColormap(dpy, root, v0->visual, AllocAll);
			XColor xc[32];  int i;
			if(v0->colormap_size<32) _throw("Color map is not large enough");
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
		else swa.colormap=XCreateColormap(dpy, root, v0->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=0;
		if((win0=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v0->depth,
			InputOutput, v0->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		if(!(v1=glXGetVisualFromFBConfig(dpy, c)))
			_throw("glXGetVisualFromFBConfig()");
		if(ci)
		{
			swa.colormap=XCreateColormap(dpy, root, v1->visual, AllocAll);
			XColor xc[32];  int i;
			if(v1->colormap_size<32) _throw("Color map is not large enough");
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
		else swa.colormap=XCreateColormap(dpy, root, v1->visual, AllocNone);
		if((win1=XCreateWindow(dpy, root, dpyw/2, 0, dpyw/2, dpyh/2, 0, v1->depth,
			InputOutput, v1->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XFree(v1);  v1=NULL;

		if((ctx0=glXCreateContext(dpy, v0, 0, True))==NULL)
			_throw("Could not establish GLX context");
		XFree(v0);  v0=NULL;
		if((ctx1=glXCreateNewContext(dpy, c, GLX_RGBA_TYPE, NULL, True))==NULL)
			_throw("Could not establish GLX context");

		if(!glXMakeCurrent(dpy, win1, ctx0))
			_throw("Could not make context current");
		checkcurrent(dpy, win1, win1, ctx0);
		if(stereo && !stereotest(ci))
		{
			_throw("Stereo is not available or is not properly implemented");
		}
		int dbworking=dbtest(ci);
		if(!dbworking)
		{
			printf("WARNING: Double buffering appears to be broken.\n");
			printf("         Testing in single buffered mode\n\n");
		}
		glReadBuffer(GL_BACK);

		if(!glXMakeContextCurrent(dpy, win1, win0, ctx1))
			_throw("Could not make context current");
		checkcurrent(dpy, win1, win0, ctx1);

		XMapWindow(dpy, win0);
		XMapWindow(dpy, win1);

		// Faker should readback back buffer on a call to glXSwapBuffers()
		try
		{
			printf("glXSwapBuffers() [b]:          ");
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			glReadBuffer(GL_FRONT);
			// Intentionally leave a pending GL error (VirtualGL should clear the error state prior to readback)
			char pixel[4];
			glReadPixels(0, 0, 1, 1, 0, GL_BYTE, pixel);
			glXSwapBuffers(dpy, win1);
			checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			// Make sure that glXSwapBuffers() actually swapped
			glDrawBuffer(GL_FRONT);
			glFinish();
			checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
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
			checkframe(win1, 1, lastframe1);
			glDrawBuffer(GL_FRONT);  glFlush();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glFinish() [f]:                ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkframe(win1, 1, lastframe1);
			glDrawBuffer(GL_FRONT);  glFinish();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glXWaitGL() [f]:               ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkframe(win1, 1, lastframe1);
			glDrawBuffer(GL_FRONT);  glXWaitGL();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glPopAttrib() [f]:             ");
			glDrawBuffer(GL_BACK);  glFinish();
			checkframe(win1, 1, lastframe1);
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			glPopAttrib();  // Back buffer should now be current again & dirty flag should be set
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}	
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glXMakeCurrent() [f]:          ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeCurrent(dpy, win1, ctx0);  // readback should occur
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win0, win0, ctx0);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win1, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glXMakeContextCurrent() [f]:   ");
			clr.clear(GL_FRONT);  if(stereo) sclr.clear(GL_FRONT_RIGHT);
			clr.clear(GL_BACK);  if(stereo) sclr.clear(GL_BACK_RIGHT);
			glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win0, 1, lastframe0);
			checkwindowcolor(win0, dbworking? clr.bits(-2):clr.bits(-1), ci);
			if(stereo)
				checkwindowcolor(win0, dbworking? sclr.bits(-2):sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		// Test for proper handling of GL_FRONT_AND_BACK
		try
		{
			printf("glXSwapBuffers() [f&b]:        ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, win1);
			checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, clr.bits(-1), ci);
			if(stereo) checkwindowcolor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glFlush() [f&b]:               ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 2, lastframe1);
			checkwindowcolor(win1, clr.bits(-1), ci);
			if(stereo) checkwindowcolor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glFinish() [f&b]:              ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 2, lastframe1);
			checkwindowcolor(win1, clr.bits(-1), ci);
			if(stereo) checkwindowcolor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glXWaitGL() [f&b]:             ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 2, lastframe1);
			checkwindowcolor(win1, clr.bits(-1), ci);
			if(stereo) checkwindowcolor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glXMakeCurrent() [f&b]:        ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glXMakeCurrent(dpy, win0, ctx0);  // readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win0, win0, ctx0);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, clr.bits(-1), ci);
			if(stereo) checkwindowcolor(win1, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glXMakeContextCurrent() [f&b]: ");
			clr.clear(GL_FRONT_AND_BACK);  if(stereo) sclr.clear(GL_RIGHT);
			glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win0, 1, lastframe0);
			checkwindowcolor(win0, clr.bits(-1), ci);
			if(stereo) checkwindowcolor(win0, sclr.bits(-1), ci, true);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx0 && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx0);  ctx0=0;}
	if(ctx1 && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx1);  ctx1=0;}
	if(win0) {XDestroyWindow(dpy, win0);  win0=0;}
	if(win1) {XDestroyWindow(dpy, win1);  win1=0;}
	if(v0) {XFree(v0);  v0=NULL;}
	if(v1) {XFree(v1);  v1=NULL;}
	if(configs) {XFree(configs);  configs=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


// This tests the faker's ability to handle the 2000 Flushes issue
int flushtest(void)
{
	testcolor clr(false, 0);
	Display *dpy=NULL;  Window win=0;
	int dpyw, dpyh, lastframe=0, retval=1;
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE,
		8, GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v=NULL;
	GLXContext ctx=0;
	XSetWindowAttributes swa;

	putenv((char *)"VGL_SPOIL=1");
	printf("10000 flushes test:\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=0;
		if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v->depth,
			InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");

		if((ctx=glXCreateContext(dpy, v, 0, True))==NULL)
			_throw("Could not establish GLX context");
		XFree(v);  v=NULL;
		if(!glXMakeCurrent(dpy, win, ctx))
			_throw("Could not make context current");
		checkcurrent(dpy, win, win, ctx);
		int dbworking=dbtest(false);
		if(!dbworking)
			_throw("This test requires double buffering, which appears to be broken.");
		glReadBuffer(GL_FRONT);
		XMapWindow(dpy, win);

		clr.clear(GL_BACK);
		clr.clear(GL_FRONT);
		for(int i=0; i<10000; i++)
		{
			printf("%.4d\b\b\b\b", i);  glFlush();
		}
		checkframe(win, -1, lastframe);
		printf("Read back %d of 10000 frames\n", lastframe);
		checkreadbackstate(GL_FRONT, dpy, win, win, ctx);
		checkwindowcolor(win, clr.bits(-1), 0);
		printf("SUCCESS\n");
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);
	putenv((char *)"VGL_SPOIL=0");
	if(ctx && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
	if(win) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


// This tests the faker's ability to do indexed rendering in the red channel of
// an RGBA off-screen drawable

#define drawquad() {  \
	glBegin(GL_QUADS);  glVertex3f(-1., -1., 0.);  glVertex3f(-1., 1., 0.);  \
	glVertex3f(1., 1., 0.);  glVertex3f(1., -1., 0.);  glEnd();}  \

#define _citest(id, type) {  \
	unsigned int c=clr.bits();  glIndex##id((type)c);  \
	GLfloat f=-1.;  glGetFloatv(GL_CURRENT_INDEX, &f);  \
	if(c!=(unsigned int)f)  \
		_prerror2("Index should be %u, not %u", c, (unsigned int)f);  \
	GLdouble d=-1.;  glGetDoublev(GL_CURRENT_INDEX, &d);  \
	if(c!=(unsigned int)d)  \
		_prerror2("Index should be %u, not %u", c, (unsigned int)d);  \
	drawquad();  \
	glXSwapBuffers(dpy, win);  \
	checkwindowcolor(win, clr.bits(), true);  \
	clr.next();  \
	c=clr.bits();  \
	type cv=(type)c;  glIndex##id##v(&cv);  \
	GLint i=-1;  glGetIntegerv(GL_CURRENT_INDEX, &i);  \
	if(c!=(unsigned int)i)  \
		_prerror2("Index should be %u, not %u", c, (unsigned int)i);  \
	drawquad();  \
	glXSwapBuffers(dpy, win);  \
	checkwindowcolor(win, clr.bits(), true);  \
	clr.next();}

#include "rrtimer.h"

#define _cidrawtest(type, gltype) {  \
	type *ptr=(type *)bits;  \
	for(int i=0; i<w*h; i++) ptr[i]=(type)clr.bits();  \
	glDrawPixels(w, h, GL_COLOR_INDEX, gltype, ptr);  \
	glXSwapBuffers(dpy, win);  \
	checkwindowcolor(win, clr.bits(), true);  clr.next();}

#define _cireadtest(type, gltype) {  \
	type *ptr=(type *)bits;  \
	memset(ptr, 0, sizeof(type)*w*h);  \
	glReadPixels(0, 0, w, h, GL_COLOR_INDEX, gltype, ptr);  \
	for(int i=0; i<w*h; i++)  \
		if((unsigned int)ptr[i]!=clr.bits())  \
			_prerror2("Index should be %u, not %u", clr.bits(), (unsigned int)ptr[i]);}

#define _cidtranstest(type, gltype) {  \
	type *ptr=(type *)bits;  \
	glPushAttrib(GL_PIXEL_MODE_BIT);  \
	glPixelTransferi(GL_INDEX_SHIFT, 2);  \
	glPixelTransferi(GL_INDEX_OFFSET, -3);  \
	double tempd;  \
	glGetDoublev(GL_INDEX_SHIFT, &tempd);  \
	if(tempd!=2.) _prerror1("glGetDoublev(GL_INDEX_SHIFT)=%f", tempd);  \
	glGetDoublev(GL_INDEX_OFFSET, &tempd);  \
	if(tempd!=-3.) _prerror1("glGetDoublev(GL_INDEX_OFFSET)=%f", tempd);  \
	for(int i=0; i<w*h; i++) ptr[i]=(type)4;  \
	glDrawPixels(w, h, GL_COLOR_INDEX, gltype, ptr);  \
	glPopAttrib();  \
	glXSwapBuffers(dpy, win);  \
	checkwindowcolor(win, 13, true);  \
	glPushAttrib(GL_PIXEL_MODE_BIT);  \
	glPixelTransferf(GL_INDEX_SHIFT, -2.);  \
	glPixelTransferf(GL_INDEX_OFFSET, 1.);  \
	int temp=0;  \
	glGetIntegerv(GL_INDEX_SHIFT, &temp);  \
	if(temp!=-2) _prerror1("glGetIntegerv(GL_INDEX_SHIFT)=%d", temp);  \
	glGetIntegerv(GL_INDEX_OFFSET, &temp);  \
	if(temp!=1) _prerror1("glGetIntegerv(GL_INDEX_OFFSET)=%d", temp);  \
	for(int i=0; i<w*h; i++) ptr[i]=(type)8;  \
	glDrawPixels(w, h, GL_COLOR_INDEX, gltype, ptr);  \
	glPopAttrib();  \
	glXSwapBuffers(dpy, win);  \
	checkwindowcolor(win, 3, true);}

#define _cirtranstest(type, gltype) {  \
	memset(bits, 4, w*h);  \
	glDrawPixels(w, h, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bits);  \
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
	glReadPixels(0, 0, w, h, GL_COLOR_INDEX, gltype, ptr);  \
	glPopAttrib();  \
	for(int i=0; i<w*h; i++)  \
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
	glReadPixels(0, 0, w, h, GL_COLOR_INDEX, gltype, ptr);  \
	glPopAttrib();  \
	for(int i=0; i<w*h; i++)  \
		if((unsigned int)ptr[i]!=3)  \
			_prerror1("Index should be 3, not %u", (unsigned int)ptr[i]);}

int citest(void)
{
	testcolor clr(true, 0);
	Display *dpy=NULL;  Window win=0;
	int dpyw, dpyh, retval=1;
	int ciattrib[]={GLX_DOUBLEBUFFER, GLX_BUFFER_SIZE, 8, None, None};
	XVisualInfo *v=NULL;  GLXContext ctx=0;
	XSetWindowAttributes swa;
	unsigned char *bits=NULL;

	printf("Color Index rendering test ");
	printf("\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));
		int w=dpyw/2, h=dpyh/2;

		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), ciattrib))==NULL)
			_throw("Could not find a suitable visual");

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v->visual, AllocAll);
		XColor xc[32];  int i;
		if(v->colormap_size<32) _throw("Color map is not large enough");
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
		if((win=XCreateWindow(dpy, root, 0, 0, w, h, 0, v->depth,
			InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");

		if((ctx=glXCreateContext(dpy, v, 0, True))==NULL)
			_throw("Could not establish GLX context");
		XFree(v);  v=NULL;
		if(!glXMakeCurrent(dpy, win, ctx))
			_throw("Could not make context current");
		checkcurrent(dpy, win, win, ctx);

		glDrawBuffer(GL_BACK);
		XMapWindow(dpy, win);

		try
		{
			// There must be fifty ways to change your index
			printf("glIndex*()                 : ");
			_citest(d, GLdouble);
			_citest(f, GLfloat);
			_citest(i, GLint);
			_citest(s, GLshort);
			_citest(ub, GLubyte);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		fflush(stdout);

		try
		{
			printf("glDrawPixels()             : ");
			bits=new unsigned char[w*h*4];
			if(!bits) _throw("Could not allocate buffer");
			_cidrawtest(unsigned char, GL_UNSIGNED_BYTE);
			_cidrawtest(char, GL_BYTE);
			_cidrawtest(unsigned short, GL_UNSIGNED_SHORT);
			_cidrawtest(short, GL_SHORT);
			_cidrawtest(unsigned int, GL_UNSIGNED_INT);
			_cidrawtest(int, GL_INT);
			_cidrawtest(float, GL_FLOAT);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		if(bits) {delete [] bits;  bits=NULL;}
		fflush(stdout);

		try
		{
			printf("glBitmap()                 : ");
			bits=new unsigned char[(w*h)/8+1];
			if(!bits) _throw("Could not allocate buffer");
			memset(bits, 0xAA, (w*h)/8+1);
			glIndexi((GLint)clr.bits());
			glRasterPos2f(-1., -1.);
			GLint i=-1;  glGetIntegerv(GL_CURRENT_RASTER_INDEX, &i);
			if((unsigned int)i!=clr.bits())
				_prerror2("Index should be %u, not %u", clr.bits(), (unsigned int)i);
			GLfloat f=-1;  glGetFloatv(GL_CURRENT_RASTER_INDEX, &f);
			if((unsigned int)f!=clr.bits())
				_prerror2("Index should be %u, not %u", clr.bits(), (unsigned int)f);
			GLdouble d=-1;  glGetDoublev(GL_CURRENT_RASTER_INDEX, &d);
			if((unsigned int)d!=clr.bits())
				_prerror2("Index should be %u, not %u", clr.bits(), (unsigned int)d);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBitmap(w, h, 0, 0, 0, 0, bits);
			memset(bits, 0x55, (w*h)/8+1);
			glBitmap(w, h, 0, 0, 0, 0, bits);
			glXSwapBuffers(dpy, win);
			checkwindowcolor(win, clr.bits(), true);  clr.next();
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		if(bits) {delete [] bits;  bits=NULL;}
		fflush(stdout);

		try
		{
			printf("glReadPixels()             : ");
			bits=new unsigned char[w*h*4];
			if(!bits) _throw("Could not allocate buffer");
			for(int i=0; i<w*h; i++) bits[i]=clr.bits();
			glDrawPixels(w, h, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, bits);
			glXSwapBuffers(dpy, win);
			checkwindowcolor(win, clr.bits(), true);
			glReadBuffer(GL_FRONT);
			_cireadtest(char, GL_BYTE);
			_cireadtest(unsigned short, GL_UNSIGNED_SHORT);
			_cireadtest(short, GL_SHORT);
			_cireadtest(unsigned int, GL_UNSIGNED_INT);
			_cireadtest(int, GL_INT);
			_cireadtest(float, GL_FLOAT);
			clr.next();
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		if(bits) {delete [] bits;  bits=NULL;}
		fflush(stdout);

		try
		{
			printf("glDrawPixels() (shift)     : ");
			bits=new unsigned char[w*h*4];
			if(!bits) _throw("Could not allocate buffer");
			glPushAttrib(GL_PIXEL_MODE_BIT);
			_cidtranstest(unsigned char, GL_UNSIGNED_BYTE);
			_cidtranstest(char, GL_BYTE);
			_cidtranstest(unsigned short, GL_UNSIGNED_SHORT);
			_cidtranstest(short, GL_SHORT);
			_cidtranstest(unsigned int, GL_UNSIGNED_INT);
			_cidtranstest(int, GL_INT);
			_cidtranstest(float, GL_FLOAT);
			glPopAttrib();
			clr.next();
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		if(bits) {delete [] bits;  bits=NULL;}
		fflush(stdout);

		try
		{
			printf("glReadPixels() (shift)     : ");
			bits=new unsigned char[w*h*4];
			if(!bits) _throw("Could not allocate buffer");
			glPushAttrib(GL_PIXEL_MODE_BIT);
			glReadBuffer(GL_FRONT);
			_cirtranstest(unsigned char, GL_UNSIGNED_BYTE);
			_cirtranstest(char, GL_BYTE);
			_cirtranstest(unsigned short, GL_UNSIGNED_SHORT);
			_cirtranstest(short, GL_SHORT);
			_cirtranstest(unsigned int, GL_UNSIGNED_INT);
			_cirtranstest(int, GL_INT);
			_cirtranstest(float, GL_FLOAT);
			glPopAttrib();
			clr.next();
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
		if(bits) {delete [] bits;  bits=NULL;}
		fflush(stdout);

	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(bits) delete [] bits;
	if(ctx && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
	if(win) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


int cfgid(Display *dpy, GLXFBConfig c);


#define getcfgattrib(c, attrib, ctemp) { \
	ctemp=-10; \
	glXGetFBConfigAttrib(dpy, c, attrib, &ctemp); \
	if(ctemp==-10) _error(#attrib" cfg attrib not supported"); \
}

#define getvisattrib(v, attrib, vtemp) { \
	vtemp=-20; \
	glXGetConfig(dpy, v, attrib, &vtemp); \
	if(vtemp==-20) _error(#attrib" vis attrib not supported"); \
}

#define compareattrib(c, v, attrib, ctemp) { \
	getcfgattrib(c, attrib, ctemp); \
	getvisattrib(v, attrib, vtemp); \
	if(ctemp!=vtemp) _prerror5("%s=%d in C%.2x & %d in V%.2x", #attrib, ctemp, cfgid(dpy, c), vtemp, v?(unsigned int)v->visualid:0); \
}


void configvsvisual(Display *dpy, GLXFBConfig c, XVisualInfo *v)
{
	int ctemp, vtemp, r, g, b, bs;
	if(!dpy) _error("Invalid display handle");
	if(!c) _error("Invalid FB config");
	if(!v) _error("Invalid visual pointer");
	getcfgattrib(c, GLX_VISUAL_ID, ctemp);
	if(ctemp!=(int)v->visualid)
		_error("Visual ID mismatch");
	getcfgattrib(c, GLX_RENDER_TYPE, ctemp);
	getvisattrib(v, GLX_RGBA, vtemp);
	if((ctemp==GLX_RGBA_BIT && vtemp!=1) ||
		(ctemp==GLX_COLOR_INDEX_BIT && vtemp!=0))
		_error("GLX_RGBA mismatch w/ X visual");
	compareattrib(c, v, GLX_BUFFER_SIZE, bs);
	compareattrib(c, v, GLX_LEVEL, ctemp);
	compareattrib(c, v, GLX_DOUBLEBUFFER, ctemp);
	compareattrib(c, v, GLX_STEREO, ctemp);
	compareattrib(c, v, GLX_AUX_BUFFERS, ctemp);
	compareattrib(c, v, GLX_RED_SIZE, r);
	compareattrib(c, v, GLX_GREEN_SIZE, g);
	compareattrib(c, v, GLX_BLUE_SIZE, b);
	compareattrib(c, v, GLX_ALPHA_SIZE, ctemp);
	compareattrib(c, v, GLX_DEPTH_SIZE, ctemp);
	compareattrib(c, v, GLX_STENCIL_SIZE, ctemp);
	compareattrib(c, v, GLX_ACCUM_RED_SIZE, ctemp);
	compareattrib(c, v, GLX_ACCUM_GREEN_SIZE, ctemp);
	compareattrib(c, v, GLX_ACCUM_BLUE_SIZE, ctemp);
	compareattrib(c, v, GLX_ACCUM_ALPHA_SIZE, ctemp);
	#ifdef GLX_SAMPLE_BUFFERS_ARB
	compareattrib(c, v, GLX_SAMPLE_BUFFERS_ARB, ctemp);
	#endif
	#ifdef GLX_SAMPLES_ARB
	compareattrib(c, v, GLX_SAMPLES_ARB, ctemp);
	#endif
	#ifdef GLX_X_VISUAL_TYPE_EXT
	compareattrib(c, v, GLX_X_VISUAL_TYPE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_TYPE_EXT
	compareattrib(c, v, GLX_TRANSPARENT_TYPE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_INDEX_VALUE_EXT
	compareattrib(c, v, GLX_TRANSPARENT_INDEX_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_RED_VALUE_EXT
	compareattrib(c, v, GLX_TRANSPARENT_RED_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_GREEN_VALUE_EXT
	compareattrib(c, v, GLX_TRANSPARENT_GREEN_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_BLUE_VALUE_EXT
	compareattrib(c, v, GLX_TRANSPARENT_BLUE_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_TRANSPARENT_ALPHA_VALUE_EXT
	compareattrib(c, v, GLX_TRANSPARENT_ALPHA_VALUE_EXT, ctemp);
	#endif
	#ifdef GLX_VIDEO_RESIZE_SUN
	compareattrib(c, v, GLX_VIDEO_RESIZE_SUN, ctemp);
	#endif
	#ifdef GLX_VIDEO_REFRESH_TIME_SUN
	compareattrib(c, v, GLX_VIDEO_REFRESH_TIME_SUN, ctemp);
	#endif
	#ifdef GLX_GAMMA_VALUE_SUN
	compareattrib(c, v, GLX_GAMMA_VALUE_SUN, ctemp);
	#endif
}


int cfgid(Display *dpy, GLXFBConfig c)
{
	int temp=0;
	if(!c) _error("config==NULL in cfgid()");
	if(!dpy) _error("display==NULL in cfgid()");
	getcfgattrib(c, GLX_FBCONFIG_ID, temp);
	return temp;
}


void queryctxtest(Display *dpy, XVisualInfo *v, GLXFBConfig c)
{
	GLXContext ctx=0;  int render_type, fbcid, temp;
	try
	{
		int visual_caveat;
		getcfgattrib(c, GLX_CONFIG_CAVEAT, visual_caveat);
		if(visual_caveat==GLX_NON_CONFORMANT_CONFIG) return;
		getcfgattrib(c, GLX_RENDER_TYPE, render_type);
		render_type=(render_type==GLX_COLOR_INDEX_BIT)? GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE;
		fbcid=cfgid(dpy, c);

		if(!(ctx=glXCreateNewContext(dpy, c, render_type, NULL, True)))
			_error("glXCreateNewContext");
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &temp);
		if(temp!=fbcid) _error("glXQueryContext FB cfg ID");
		#ifndef sun
		getcfgattrib(c, GLX_RENDER_TYPE, render_type);
		render_type=(render_type==GLX_COLOR_INDEX_BIT)? GLX_COLOR_INDEX_TYPE:GLX_RGBA_TYPE;
		#endif
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_RENDER_TYPE, &temp);
		if(temp!=render_type) _error("glXQueryContext render type");
		glXDestroyContext(dpy, ctx);  ctx=0;

		if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
			_error("glXCreateNewContext");
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_FBCONFIG_ID, &temp);
		if(temp!=fbcid) _error("glXQueryContext FB cfg ID");
		temp=-20;
		glXQueryContext(dpy, ctx, GLX_RENDER_TYPE, &temp);
		if(temp!=render_type) _error("glXQueryContext render type");
		glXDestroyContext(dpy, ctx);  ctx=0;
	}
	catch(...)
	{
		if(ctx) {glXDestroyContext(dpy, ctx);  ctx=0;}
		throw;
	}
}


GLXFBConfig glXGetFBConfigFromVisual(Display *dpy, XVisualInfo *vis)
{
	GLXContext ctx=0;  int temp, fbcid=0, n=0;  GLXFBConfig *configs=NULL, c=0;
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
			if(temp==fbcid) {c=configs[i];  break;}
		}
		XFree(configs);  configs=NULL;
		if(!c) _error("Cannot map visual to FB config");
		return c;
	}
	catch(...)
	{
		if(ctx) {glXDestroyContext(dpy, ctx);  ctx=0;}
		if(configs) {XFree(configs);  configs=NULL;}
		throw;
	}
	return 0;
}


// This tests the faker's client/server visual matching heuristics
int vistest(void)
{
	Display *dpy=NULL;
	XVisualInfo **v=NULL, *v0=NULL, vtemp;
	GLXFBConfig c=0, *configs=NULL;  int n=0, i, retval=1;

	printf("Visual matching heuristics test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0))) _throw("Could not open display");

		// This will fail with VGL 2.2.x and earlier
		if(!(configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), NULL, &n))
			|| n==0)
			_throw("No FB configs found");
		XFree(configs);  configs=NULL;

		try
		{
			printf("RGBA:   ");

			// Iterate through RGBA attributes
			int rgbattrib[]={GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
				GLX_ALPHA_SIZE, 0, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
				GLX_STENCIL_SIZE, 0, GLX_ACCUM_RED_SIZE, 0, GLX_ACCUM_GREEN_SIZE, 0,
				GLX_ACCUM_BLUE_SIZE, 0, GLX_ACCUM_ALPHA_SIZE, 0, GLX_SAMPLE_BUFFERS, 0,
				GLX_SAMPLES, 1, GLX_RGBA, None, None, None};
			int rgbattrib13[]={GLX_DOUBLEBUFFER, 1, GLX_STEREO, 1, GLX_RED_SIZE, 8,
				GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 0, GLX_DEPTH_SIZE, 0,
				GLX_AUX_BUFFERS, 0, GLX_STENCIL_SIZE, 0, GLX_ACCUM_RED_SIZE, 0,
				GLX_ACCUM_GREEN_SIZE, 0, GLX_ACCUM_BLUE_SIZE, 0, GLX_ACCUM_ALPHA_SIZE, 0,
				GLX_SAMPLE_BUFFERS, 0, GLX_SAMPLES, 1, None};

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
										if(alpha) {rgbattrib13[25]=rgbattrib[21]=accum;}
										for(int samples=0; samples<=16; samples==0? samples=1:samples*=2)
										{
											rgbattrib13[29]=rgbattrib[25]=samples;
											rgbattrib13[27]=rgbattrib[23]=samples? 1:0;

											if((!(configs=glXChooseFBConfig(dpy, DefaultScreen(dpy),
												rgbattrib13, &n)) || n==0) && !stereo && !samples && !aux && !accum)
												_throw("No FB configs found");
											if(!(v0=glXChooseVisual(dpy, DefaultScreen(dpy), rgbattrib))
												&& !stereo && !samples && !aux && !accum)
												_throw("Could not find visual");
											if(v0 && configs)
											{
												configvsvisual(dpy, configs[0], v0);
												XFree(v0);  XFree(configs);
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
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("INDEX:  ");

			// Iterate through color index attributes
			int ciattrib[]={GLX_BUFFER_SIZE, 8, GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0,
				GLX_STENCIL_SIZE, 0, GLX_TRANSPARENT_TYPE, GLX_NONE, GLX_LEVEL, 0,
				None, None, None};
			int ciattrib13[]={GLX_DOUBLEBUFFER, 1, GLX_STEREO, 1, GLX_BUFFER_SIZE, 8,
				GLX_DEPTH_SIZE, 0, GLX_AUX_BUFFERS, 0, GLX_STENCIL_SIZE, 0, GLX_RENDER_TYPE,
				GLX_COLOR_INDEX_BIT, GLX_TRANSPARENT_TYPE, GLX_NONE, GLX_LEVEL, 0, None};

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
									ciattrib13[15]=ciattrib[9]=trans? GLX_TRANSPARENT_INDEX:GLX_NONE;

									if((!(configs=glXChooseFBConfig(dpy, DefaultScreen(dpy),
										ciattrib13, &n)) || n==0) && !stereo && !aux && !trans)
										_throw("No FB configs found");
									if(!(v0=glXChooseVisual(dpy, DefaultScreen(dpy), ciattrib))
										&& !stereo && !aux && !trans)
										_throw("Could not find visual");
									if(v0 && configs)
									{
										configvsvisual(dpy, configs[0], v0);
										XFree(v0);  XFree(configs);
									}
								}
							}
						}
					}
				}
			}
			printf("SUCCESS!\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		printf("\n");
		if(!(configs=glXGetFBConfigs(dpy, DefaultScreen(dpy), &n)) || n==0)
			_throw("No FB configs found");

		int fbcid=0;
		if(!(v=(XVisualInfo **)malloc(sizeof(XVisualInfo *)*n)))
			_throw("Memory allocation error");
		memset(v, 0, sizeof(XVisualInfo *)*n);

		for(i=0; i<n; i++)
		{
			if(!configs[i]) continue;
			try
			{
				fbcid=cfgid(dpy, configs[i]);
				if(!(v[i]=glXGetVisualFromFBConfig(dpy, configs[i])))
				{
					printf("CFG 0x%.2x:  ", fbcid);
					_error("No matching X visual for CFG");
				}
			}
			catch(rrerror &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
		}

		for(i=0; i<n; i++)
		{
			XVisualInfo *v1=NULL;
			if(!configs[i]) continue;
			try
			{
				fbcid=cfgid(dpy, configs[i]);
				printf("CFG 0x%.2x:  ", fbcid);
				if(!(v1=glXGetVisualFromFBConfig(dpy, configs[i])))
					_error("No matching X visual for CFG");

				configvsvisual(dpy, configs[i], v[i]);
				configvsvisual(dpy, configs[i], v1);
				queryctxtest(dpy, v[i], configs[i]);
				queryctxtest(dpy, v1, configs[i]);
				
				c=glXGetFBConfigFromVisual(dpy, v[i]);
				if(!c || cfgid(dpy, c)!=fbcid) _error("glXGetFBConfigFromVisual");
				c=glXGetFBConfigFromVisual(dpy, v1);
				if(!c || cfgid(dpy, c)!=fbcid) _error("glXGetFBConfigFromVisual");

				printf("SUCCESS!\n");
			}
			catch(rrerror &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
			if(v1) {XFree(v1);  v1=NULL;}
		}

		XFree(configs);  configs=NULL;
		for(i=0; i<n; i++) {if(v[i]) XFree(v[i]);}  free(v);  v=NULL;  n=0;
		fflush(stdout);

		if(!(v0=XGetVisualInfo(dpy, VisualNoMask, &vtemp, &n)) || n==0)
			_throw("No X Visuals found");
		printf("\n");

		for(int i=0; i<n; i++)
		{
			XVisualInfo *v2=NULL;
			try
			{
				int level=0;
				glXGetConfig(dpy, &v0[i], GLX_LEVEL, &level);
				if(level) continue;
				printf("Vis 0x%.2x:  ", (int)v0[i].visualid);
				if(!(c=glXGetFBConfigFromVisual(dpy, &v0[i])))
					_error("No matching CFG for X Visual");
				configvsvisual(dpy, c, &v0[i]);
				v2=glXGetVisualFromFBConfig(dpy, c);
				configvsvisual(dpy, c, v2);

				printf("SUCCESS!\n");
			}
			catch(rrerror &e)
			{
				printf("Failed! (%s)\n", e.getMessage());  retval=0;
			}
			if(v2) {XFree(v2);  v2=NULL;}
		}
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);

	if(v && n) {for(i=0; i<n; i++) {if(v[i]) XFree(v[i]);}  free(v);  v=NULL;}
	if(v0) {XFree(v0);  v0=NULL;}
	if(configs) {XFree(configs);  configs=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


#define NTHR 30
bool deadyet=false;

class mttestthread : public Runnable
{
	public:

		mttestthread(int _myrank, Display *_dpy, Window _win, GLXContext _ctx)
		: myrank(_myrank), dpy(_dpy), win(_win), ctx(_ctx), doresize(false)
		{
		}

		void run(void)
		{
			int clr=myrank%NC, lastframe=0;
			if(!(glXMakeCurrent(dpy, win, ctx)))
				_error("Could not make context current");
			while(!deadyet)
			{
				if(doresize)
				{
					glViewport(0, 0, w, h);
					doresize=false;
				}
				glClearColor(tc[clr].r, tc[clr].g, tc[clr].b, 0.);
				glClear(GL_COLOR_BUFFER_BIT);
				glReadBuffer(GL_FRONT);
				glXSwapBuffers(dpy, win);
				checkreadbackstate(GL_FRONT, dpy, win, win, ctx);
				checkframe(win, 1, lastframe);
				checkwindowcolor(win, tc[clr].bits, false);
				clr=(clr+1)%NC;
			}
		}

		void resize(int _w, int _h)
		{
			w=_w;  h=_h;
			doresize=true;
		}

	private:

		int myrank;
		Display *dpy;
		Window win;
		GLXContext ctx;
		bool doresize;
		int w, h;
};


int mttest(int nthr)
{
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v=NULL;
	Display *dpy=NULL;  Window win[NTHR];
	GLXContext ctx[NTHR];
	mttestthread *mt[NTHR];  Thread *t[NTHR];
	XSetWindowAttributes swa;
	int dpyw, dpyh, i, retval=1;
	if(nthr==0) return 1;
	for(i=0; i<nthr; i++)
	{
		win[i]=0;  ctx[i]=0;  mt[i]=NULL;  t[i]=NULL;
	}

	printf("Multi-threaded rendering test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0))) _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");
		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=StructureNotifyMask|ExposureMask;
		for(i=0; i<nthr; i++)
		{
			int winx=(i%10)*100, winy=(i/10)*120;
			if((win[i]=XCreateWindow(dpy, root, winx, winy, 100, 100, 0, v->depth,
				InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
				&swa))==0)
				_throw("Could not create window");
			XMapWindow(dpy, win[i]);
			if(!(ctx[i]=glXCreateContext(dpy, v, NULL, True)))
				_throw("Could not establish GLX context");
			XMoveResizeWindow(dpy, win[i], winx, winy, 100, 100);
		}
		XSync(dpy, False);
		XFree(v);  v=NULL;

		for(i=0; i<nthr; i++)
		{
			mt[i]=new mttestthread(i, dpy, win[i], ctx[i]);
			t[i]=new Thread(mt[i]);
			if(!mt[i] || !t[i]) _prerror1("Could not create thread %d", i);
			t[i]->start();
		}
		printf("Phase 1\n");
		for(i=0; i<nthr; i++)
		{
			int winx=(i%10)*100, winy=i/10*120;
			XMoveResizeWindow(dpy, win[i], winx, winy, 200, 200);
			mt[i]->resize(200, 200);
			if(i<5) usleep(0);
			XResizeWindow(dpy, win[i], 100, 100);
			mt[i]->resize(100, 100);
		}
		XSync(dpy, False);
		fflush(stdout);

		printf("Phase 2\n");
		for(i=0; i<nthr; i++)
		{
			XWindowChanges xwc;
			xwc.width=xwc.height=200;
			XConfigureWindow(dpy, win[i], CWWidth|CWHeight, &xwc);
			mt[i]->resize(200, 200);
		}
		XSync(dpy, False);
		fflush(stdout);

		printf("Phase 3\n");
		for(i=0; i<nthr; i++)
		{
			XResizeWindow(dpy, win[i], 100, 100);
			mt[i]->resize(100, 100);
		}
		XSync(dpy, False);
		deadyet=true;
		for(i=0; i<nthr; i++) t[i]->stop();
		for(i=0; i<nthr; i++)
		{
			try
			{
				t[i]->checkerror();
			}
			catch(rrerror &e)
			{
				printf("Thread %d failed! (%s)\n", i, e.getMessage());  retval=0;
			}
		}
		if(retval==1) printf("SUCCESS!\n");
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);

	for(i=0; i<nthr; i++) {if(t[i]) {delete t[i];  t[i]=NULL;}}
	for(i=0; i<nthr; i++) {if(mt[i]) {delete mt[i];  mt[i]=NULL;}}
	if(v) {XFree(v);  v=NULL;}
	for(i=0; i<nthr; i++)
	{
		if(dpy && ctx[i])
		{
			glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx[i]);  ctx[i]=0;
		}
	}
	for(i=0; i<nthr; i++) {if(dpy && win[i]) {XDestroyWindow(dpy, win[i]);  win[i]=0;}}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


#define comparedrawattrib(dpy, draw, value, attrib) { \
	if(value>=0) { \
		unsigned int temp=0xffffffff; \
		glXQueryDrawable(dpy, draw, attrib, &temp); \
		if(temp==0xffffffff) _throw(#attrib" attribute not supported"); \
		if(temp!=(unsigned int)value) _prerror3("%s=%d (should be %d)", #attrib, temp, value); \
	} \
}

void checkdrawable(Display *dpy, GLXDrawable draw, int width, int height,
	int preserved_contents, int largest_pbuffer, int fbconfig_id)
{
	if(!dpy || !draw) _throw("Invalid argument to checkdrawable()");
	comparedrawattrib(dpy, draw, width, GLX_WIDTH);
	comparedrawattrib(dpy, draw, height, GLX_HEIGHT);
	comparedrawattrib(dpy, draw, preserved_contents, GLX_PRESERVED_CONTENTS);
	comparedrawattrib(dpy, draw, largest_pbuffer, GLX_LARGEST_PBUFFER);
	comparedrawattrib(dpy, draw, fbconfig_id, GLX_FBCONFIG_ID);
}

#define verifybufcolor(buf, shouldbe, tag) {\
	if(buf>0) glReadBuffer(buf); \
	unsigned int bufcol=checkbuffercolor(false); \
	if(bufcol!=(shouldbe)) \
		_prerror2(tag" is 0x%.6x, should be 0x%.6x", bufcol, (shouldbe)); \
}

// Test off-screen rendering
int pbtest(void)
{
	Display *dpy=NULL;  Window win=0;  Pixmap pm0=0, pm1=0, pm2=0;
	GLXPixmap glxpm0=0, glxpm1=0;  GLXPbuffer pb=0;  GLXWindow glxwin=0;
	int dpyw, dpyh, lastframe=0, retval=1;
	int glxattrib[]={GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v=NULL;  GLXFBConfig c=0, *configs=NULL;  int n=0;
	GLXContext ctx=0;
	XSetWindowAttributes swa;
	XFontStruct *fontinfo=NULL;  int minchar, maxchar;
	int fontlistbase=0;
	GLuint fb=0, rb=0;
	testcolor clr(false, 0);

	printf("Off-screen rendering test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if(!(fontinfo=XLoadQueryFont(dpy, "fixed")))
			_throw("Could not load X font");
		minchar=fontinfo->min_char_or_byte2;
		maxchar=fontinfo->max_char_or_byte2;
		fontlistbase=glGenLists(maxchar+1);

		if((configs=glXChooseFBConfigSGIX(dpy, DefaultScreen(dpy), glxattrib, &n))
			==NULL || n==0) _throw("Could not find a suitable FB config");
		c=configs[0];
		int fbcid=cfgid(dpy, c);
		XFree(configs);  configs=NULL;
		if((v=glXGetVisualFromFBConfigSGIX(dpy, c))==NULL)
			_throw("Could not find matching visual for FB config");

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
		swa.border_pixel=0;
		swa.background_pixel=0;
		swa.event_mask = 0;
		if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v->depth,
			InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XMapWindow(dpy, win);
		if((glxwin=glXCreateWindow(dpy, c, win, NULL))==0)
			_throw("Could not create GLX window");
		checkdrawable(dpy, glxwin, dpyw/2, dpyh/2, -1, -1, fbcid);

		if((pm0=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, v->depth))==0
		|| (pm1=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, v->depth))==0
		|| (pm2=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, v->depth))==0)
			_throw("Could not create pixmap");
		if((glxpm0=glXCreateGLXPixmap(dpy, v, pm0))==0
		|| (glxpm1=glXCreatePixmap(dpy, c, pm1, NULL))==0)
			_throw("Could not create GLX pixmap");
		checkdrawable(dpy, glxpm0, dpyw/2, dpyh/2, -1, -1, fbcid);
		checkdrawable(dpy, glxpm1, dpyw/2, dpyh/2, -1, -1, fbcid);

		int pbattribs[]={GLX_PBUFFER_WIDTH, dpyw/2, GLX_PBUFFER_HEIGHT, dpyh/2,
			GLX_PRESERVED_CONTENTS, True, GLX_LARGEST_PBUFFER, False, None};
		if((pb=glXCreatePbuffer(dpy, c, pbattribs))==0)
			_throw("Could not create Pbuffer");
		checkdrawable(dpy, pb, dpyw/2, dpyh/2, 1, 0, fbcid);
		unsigned int tempw=0, temph=0;
		glXQueryGLXPbufferSGIX(dpy, pb, GLX_WIDTH_SGIX, &tempw);
		glXQueryGLXPbufferSGIX(dpy, pb, GLX_HEIGHT_SGIX, &temph);
		if(tempw!=(unsigned int)dpyw/2 || temph!=(unsigned int)dpyh/2)
			_throw("glXQueryGLXPbufferSGIX() failed");

		if(!(ctx=glXCreateContextWithConfigSGIX(dpy, c, GLX_RGBA_TYPE, NULL, True)))
			_throw("Could not create context");

		if(!glXMakeContextCurrent(dpy, glxwin, glxwin, ctx))
			_throw("Could not make context current");
		checkcurrent(dpy, glxwin, glxwin, ctx);
		int dbwin=dbtest(false);
		if(!dbwin)
		{
			printf("WARNING: Double buffering appears to be broken.\n");
			printf("         Testing in single buffered mode\n\n");
		}

		if(!glXMakeContextCurrent(dpy, pb, pb, ctx))
			_throw("Could not make context current");
		checkcurrent(dpy, pb, pb, ctx);
		int dbpb=dbtest(false);
		if(!dbpb)
		{
			printf("WARNING: Double buffered off-screen rendering not available.\n");
			printf("         Testing in single buffered mode\n\n");
		}
		checkframe(win, -1, lastframe);

		try
		{
			printf("PBuffer->Window:                ");
			if(!(glXMakeContextCurrent(dpy, pb, pb, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, pb, pb, ctx);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			verifybufcolor(GL_BACK, dbpb? clr.bits(-2):clr.bits(-1), "PB");
			if(!(glXMakeContextCurrent(dpy, glxwin, pb, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxwin, pb, ctx);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw/2, dpyh/2, GL_COLOR);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, glxwin);
			checkframe(win, 1, lastframe);
			checkreadbackstate(GL_FRONT, dpy, glxwin, pb, ctx);
			checkwindowcolor(win, dbpb? clr.bits(-2):clr.bits(-1), false);
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("Window->Pbuffer:                ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontinfo->fid, minchar, maxchar-minchar+1,
				fontlistbase+minchar);
			checkcurrent(dpy, glxwin, glxwin, ctx);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			verifybufcolor(GL_BACK, dbwin? clr.bits(-2):clr.bits(-1), "Win");
			if(!(glXMakeContextCurrent(dpy, pb, glxwin, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontinfo->fid, minchar, maxchar-minchar+1,
				fontlistbase+minchar);
			checkcurrent(dpy, pb, glxwin, ctx);
			checkframe(win, 1, lastframe);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw/2, dpyh/2, GL_COLOR);
			glXSwapBuffers(dpy, pb);
			verifybufcolor(GL_BACK, dbwin? clr.bits(-2):clr.bits(-1), "PB");
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("FBO->Window:                    ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxwin, glxwin, ctx);
			clr.clear(GL_BACK);
			clr.clear(GL_FRONT);
			verifybufcolor(GL_BACK, dbwin? clr.bits(-2):clr.bits(-1), "Win");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxwin, glxwin, ctx);
			glDrawBuffer(GL_BACK);
			glGenFramebuffersEXT(1, &fb);
			glGenRenderbuffersEXT(1, &rb);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA, dpyw/2, dpyh/2);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rb);
			clr.clear(0);
			verifybufcolor(0, clr.bits(-1), "FBO");
			glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
			glFramebufferRenderbufferEXT(GL_DRAW_FRAMEBUFFER_EXT,
				GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, 0);
			glDrawBuffer(GL_BACK);
			glXSwapBuffers(dpy, glxwin);
			checkframe(win, 1, lastframe);
			checkreadbackstate(GL_COLOR_ATTACHMENT0_EXT, dpy, glxwin, glxwin, ctx);
			checkwindowcolor(win, dbwin? clr.bits(-3):clr.bits(-2), false);
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_RENDERBUFFER_EXT, 0);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		if(rb) {glDeleteRenderbuffersEXT(1, &rb);  rb=0;}
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		if(fb) {glDeleteFramebuffersEXT(1, &fb);  fb=0;}

		try
		{
			printf("Pixmap->Window:                 ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontinfo->fid, minchar, maxchar-minchar+1,
				fontlistbase+minchar);
			checkcurrent(dpy, glxpm0, glxpm0, ctx);
			clr.clear(GL_FRONT);
			verifybufcolor(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, win, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkreadbackstate(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkframe(win, 1, lastframe);
			checkwindowcolor(win, clr.bits(-1), false);
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("Window->Pixmap:                 ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontinfo->fid, minchar, maxchar-minchar+1,
				fontlistbase+minchar);
			checkcurrent(dpy, glxwin, glxwin, ctx);
			clr.clear(GL_FRONT);
			clr.clear(GL_BACK);
			verifybufcolor(GL_FRONT, dbwin? clr.bits(-2):clr.bits(-1), "Win");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxpm1, glxpm1, ctx);
			checkframe(win, 1, lastframe);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, win, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkreadbackstate(GL_BACK, dpy, glxpm1, glxpm1, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkframe(win, 0, lastframe);
			verifybufcolor(GL_BACK, dbwin? clr.bits(-2):clr.bits(-1), "PM1");
			verifybufcolor(GL_FRONT, dbwin? clr.bits(-2):clr.bits(-1), "PM1");
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("Pixmap->Pixmap:                 ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			glXUseXFont(fontinfo->fid, minchar, maxchar-minchar+1,
				fontlistbase+minchar);
			checkcurrent(dpy, glxpm0, glxpm0, ctx);
			clr.clear(GL_FRONT);
			verifybufcolor(GL_FRONT, clr.bits(-1), "PM0");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxpm1, glxpm1, ctx);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkreadbackstate(GL_BACK, dpy, glxpm1, glxpm1, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			verifybufcolor(GL_BACK, clr.bits(-1), "PM1");
			verifybufcolor(GL_FRONT, clr.bits(-1), "PM1");
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

		try
		{
			printf("GLX Pixmap->2D Pixmap:          ");
			lastframe=0;
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxpm0, glxpm0, ctx);

			clr.clear(GL_FRONT);
			verifybufcolor(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm2, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkreadbackstate(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkframe(pm0, 1, lastframe);
			checkwindowcolor(pm0, clr.bits(-1), false);

			clr.clear(GL_FRONT);
			clr.clear(GL_BACK);
			verifybufcolor(GL_FRONT, clr.bits(-2), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XImage *xi=XGetImage(dpy, pm0, 0, 0, dpyw/2, dpyh/2, AllPlanes, ZPixmap);
			if(xi) XDestroyImage(xi);
			checkreadbackstate(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkframe(pm0, 1, lastframe);
			checkwindowcolor(pm0, clr.bits(-2), false);

			printf("SUCCESS\n");
		}
		catch(rrerror &e)
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
			checkcurrent(dpy, glxpm0, glxpm0, ctx);
			clr.clear(GL_FRONT);
			verifybufcolor(GL_FRONT, clr.bits(-1), "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			if(!glXMakeContextCurrent(dpy, 0, 0, 0))
				_error("Could not make context current");
			glXDestroyPixmap(dpy, glxpm0);  glxpm0=0;
			XCopyArea(dpy, pm0, pm2, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkframe(pm0, 1, lastframe);
			checkwindowcolor(pm0, clr.bits(-1), false);
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);

	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_RENDERBUFFER_EXT, 0);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	if(rb) {glDeleteRenderbuffersEXT(1, &rb);  rb=0;}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	if(fb) {glDeleteFramebuffersEXT(1, &fb);  fb=0;}
	if(ctx && dpy) {glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
	if(pb && dpy) {glXDestroyPbuffer(dpy, pb);  pb=0;}
	if(glxpm1 && dpy) {glXDestroyGLXPixmap(dpy, glxpm1);  glxpm1=0;}
	if(glxpm0 && dpy) {glXDestroyGLXPixmap(dpy, glxpm0);  glxpm0=0;}
	if(pm2 && dpy) {XFreePixmap(dpy, pm2);  pm2=0;}
	if(pm1 && dpy) {XFreePixmap(dpy, pm1);  pm1=0;}
	if(pm0 && dpy) {XFreePixmap(dpy, pm0);  pm0=0;}
	if(glxwin && dpy) {glXDestroyWindow(dpy, glxwin);  glxwin=0;}
	if(win && dpy) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	if(configs) {XFree(configs);  configs=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


// Test whether glXMakeCurrent() can handle mismatches between the FB config
// of the context and the off-screen drawable

int ctxmismatchtest(void)
{
	Display *dpy=NULL;  Window win=0;
	int dpyw, dpyh, retval=1;
	int glxattrib1[]={GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None};
	int glxattrib2[]={GLX_DOUBLEBUFFER, 0, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
		None};
	XVisualInfo *v=NULL;  GLXFBConfig c1=0, c2=0, *configs=NULL;  int n=0;
	GLXContext ctx1=0, ctx2=0;
	XSetWindowAttributes swa;

	printf("Context FB config mismatch test:\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib1, &n))
			==NULL || n==0) _throw("Could not find a suitable FB config");
		c1=configs[0];
		if(!(v=glXGetVisualFromFBConfig(dpy, c1)))
			_throw("glXGetVisualFromFBConfig()");
		XFree(configs);  configs=NULL;

		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib2, &n))
			==NULL || n==0) _throw("Could not find a suitable FB config");
		c2=configs[0];
		XFree(configs);  configs=NULL;

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
		swa.border_pixel=0;
		swa.background_pixel=0;
		swa.event_mask = 0;
		if((win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v->depth,
			InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XMapWindow(dpy, win);

		try
		{
			if(!(ctx1=glXCreateNewContext(dpy, c1, GLX_RGBA_TYPE, NULL, True)))
				_throw("Could not create context");
			if(!(ctx2=glXCreateNewContext(dpy, c2, GLX_RGBA_TYPE, NULL, True)))
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
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx1 && dpy) {glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx1);  ctx1=0;}
	if(ctx2 && dpy) {glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx2);  ctx2=0;}
	if(win && dpy) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	if(configs) {XFree(configs);  configs=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


// Test whether VirtualGL properly handles explicit and implicit destruction of
// subwindows

int subwintest(void)
{
	Display *dpy=NULL;  Window win=0, win1=0, win2=0;
	testcolor clr(false, 0);
	int dpyw, dpyh, retval=1, lastframe=0;
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE,
		8, GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v=NULL;
	GLXContext ctx=0;
	XSetWindowAttributes swa;

	printf("Subwindow destruction test:\n\n");

	try
	{
		try
		{
			for(int i=0; i<20; i++)
			{
				if(!dpy && !(dpy=XOpenDisplay(0))) _throw("Could not open display");
				dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
				dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

				if((v=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))
					==NULL) _throw("Could not find a suitable visual");

				Window root=RootWindow(dpy, DefaultScreen(dpy));
				swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
				swa.border_pixel=0;
				swa.background_pixel=0;
				swa.event_mask = 0;
				if(!win && (win=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0,
					v->depth, InputOutput, v->visual,
					CWBorderPixel|CWColormap|CWEventMask, &swa))==0)
					_throw("Could not create window");
				if((win1=XCreateWindow(dpy, win, 0, 0, dpyw/2, dpyh/2, 0, v->depth,
					InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
					&swa))==0)
					_throw("Could not create subwindow");
				if((win2=XCreateWindow(dpy, win1, 0, 0, dpyw/2, dpyh/2, 0, v->depth,
					InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
					&swa))==0)
					_throw("Could not create subwindow");
				XMapSubwindows(dpy, win);
				XMapWindow(dpy, win);

				lastframe=0;
				if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
					_throw("Could not create context");
				XFree(v);  v=NULL;
				if(!(glXMakeCurrent(dpy, win2, ctx)))
					_error("Could not make context current");
				clr.clear(GL_BACK);
				glXSwapBuffers(dpy, win2);
				checkframe(win2, 1, lastframe);
				checkwindowcolor(win2, clr.bits(-1), false);
				glXMakeCurrent(dpy, 0, 0);
				glXDestroyContext(dpy, ctx);  ctx=0;

				if(i%3==0) {XCloseDisplay(dpy);  dpy=NULL;  win=0;}
				else if(i%3==1) {XDestroyWindow(dpy, win);  win=0;}
				else XDestroySubwindows(dpy, win);
			}
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
		fflush(stdout);
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
	if(win && dpy) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


int querytest(void)
{
	Display *dpy=NULL;  int retval=1;
	int dummy1=-1, dummy2=-1, dummy3=-1;

	printf("Extension query test:\n\n");

	try
	{
		int major=-1, minor=-1;
		if((dpy=XOpenDisplay(0))==NULL) _throw("Could not open display");
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
		printf("  Version=%s\n", glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VERSION));
		printf("  Vendor=%s\n", glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VENDOR));
		printf("  Extensions=%s\n", glXQueryServerString(dpy, DefaultScreen(dpy), GLX_EXTENSIONS));
		if(major<1 || minor<3)
			_throw("glXQueryVersion() reports version < 1.3");
		printf("glXQueryExtensionsString():\n%s\n",
			glXQueryExtensionsString(dpy, DefaultScreen(dpy)));
		printf("SUCCESS!\n");
	}	
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


#define testprocsym(f) \
	if((sym=(void *)glXGetProcAddressARB((const GLubyte *)#f))==NULL) \
		_throw("glXGetProcAddressARB(\""#f"\") returned NULL"); \
	else if(sym!=(void *)f) \
		_throw("glXGetProcAddressARB(\""#f"\")!="#f);

int procaddrtest(void)
{
	int retval=1;  void *sym=NULL;

	printf("glXGetProcAddress test:\n\n");

	try
	{
		testprocsym(glXChooseVisual)
		testprocsym(glXCreateContext)
		testprocsym(glXMakeCurrent)
		testprocsym(glXChooseFBConfig)
		testprocsym(glXCreateNewContext)
		testprocsym(glXMakeContextCurrent)
		printf("SUCCESS!\n");
	}	
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	fflush(stdout);
	return retval;
}


int main(int argc, char **argv)
{
	int ret=0;  int nthr=NTHR;  int doci=1;

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
			int temp=atoi(argv[++i]);  if(temp>=0 && temp<=NTHR) nthr=temp;
		}
		if(!strcasecmp(argv[i], "-noci")) doci=0;
	}

	// Intentionally leave a pending dlerror()
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat");
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat2");

	if(!XInitThreads()) _throw("XInitThreads() failed");
	if(!querytest()) ret=-1;
	printf("\n");
	if(!procaddrtest()) ret=-1;
	printf("\n");
	if(!rbtest(false, false)) ret=-1;
	printf("\n");
	rbtest(true, false);
	printf("\n");
	ctxmismatchtest();
	printf("\n");
	flushtest();
	printf("\n");
	if(doci)
	{
		rbtest(false, true);
		printf("\n");
		citest();
		printf("\n");
	}
	if(!vistest()) ret=-1;
	printf("\n");
	if(!mttest(nthr)) ret=-1;
	printf("\n");
	if(!pbtest()) ret=-1;
	printf("\n");
	if(!subwintest()) ret=-1;
	printf("\n");
	return ret;
}
