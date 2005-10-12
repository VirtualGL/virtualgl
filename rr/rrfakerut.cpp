/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
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
#include <GL/glx.h>
#include <GL/glu.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <dlfcn.h>
#include <unistd.h>
#include "rrerror.h"
#include "rrthread.h"


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


int usingglp=0;


#define glClearBuffer(buffer, r, g, b, a) { \
	glDrawBuffer(buffer); \
	glClearColor(r, g, b, a); \
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

unsigned int checkbuffercolor(void)
{
	int i, vp[4];  unsigned int ret=0;
	unsigned char *buf=NULL;

	try
	{
		vp[0]=vp[1]=vp[2]=vp[3]=0;
		glGetIntegerv(GL_VIEWPORT, vp);
		if(vp[2]<1 || vp[3]<1) _throw("Invalid viewport dimensions");

		if((buf=(unsigned char *)malloc(vp[2]*vp[3]*3))==NULL)
			_throw("Could not allocate buffer");

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, vp[2], vp[3], GL_RGB, GL_UNSIGNED_BYTE, buf);
		for(i=3; i<vp[2]*vp[3]*3; i+=3)
		{
			if(buf[i]!=buf[0] || buf[i+1]!=buf[1] || buf[i+2]!=buf[2])
				_throw("Bogus data read back");
		}
		ret=buf[0]|(buf[1]<<8)|(buf[2]<<16);
		free(buf);
		return ret;
	}
	catch(...)
	{
		if(buf) free(buf);  throw;
	}
	return 0;
}


void checkwindowcolor(Window win, unsigned int color)
{
	char *e=NULL, temps[80];  int fakerclr;
	snprintf(temps, 79, "__VGL_AUTOTESTCLR%x", (unsigned int)win);
	if((e=getenv(temps))==NULL)
		_error("Can't communicate w/ faker");
	if((fakerclr=atoi(e))<0 || fakerclr>0xffffff)
		_error("Bogus data read back");
	if((unsigned int)fakerclr!=color)
		_prerror2("Color is 0x%.6x, should be 0x%.6x", fakerclr, color)
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
int dbtest(void)
{
	unsigned int bcolor=0, fcolor=0;
	int oldreadbuf=GL_BACK, olddrawbuf=GL_BACK;
	glGetIntegerv(GL_READ_BUFFER, &oldreadbuf);
	glGetIntegerv(GL_DRAW_BUFFER, &olddrawbuf);
	glClearBuffer(GL_FRONT_AND_BACK, 0., 0., 0., 0.);
	glClearBuffer(GL_BACK, 1., 1., 1., 0.);
	glClearBuffer(GL_FRONT, 1., 0., 1., 0.);
	glReadBuffer(GL_BACK);
	bcolor=checkbuffercolor();
	glReadBuffer(GL_FRONT);
	fcolor=checkbuffercolor();
	glReadBuffer(oldreadbuf);
	glDrawBuffer(olddrawbuf);
	if(bcolor==0xffffff && fcolor==0xff00ff) return true;
	else return false;
}


// This tests the faker's readback heuristics
int rbtest(void)
{
	Display *dpy=NULL;  Window win0=0, win1=0;
	int dpyw, dpyh, lastframe0=0, lastframe1=0, retval=1;
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None};
	int glxattrib13[]={GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE,
		GLX_WINDOW_BIT, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v0=NULL, *v1=NULL;  GLXFBConfig c=0, *configs=NULL;  int n=0;
	GLXContext ctx0=0, ctx1=0;
	XSetWindowAttributes swa;

	printf("Readback heuristics test\n\n");

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
		swa.colormap=XCreateColormap(dpy, root, v0->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=0;
		if((win0=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v0->depth,
			InputOutput, v0->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		if(!(v1=glXGetVisualFromFBConfig(dpy, c)))
			_throw("glXGetVisualFromFBConfig()");
		swa.colormap=XCreateColormap(dpy, root, v1->visual, AllocNone);
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
		int dbworking=dbtest();
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
			glClearBuffer(GL_BACK, 1., 0., 0., 0.);
			glClearBuffer(GL_FRONT, 0., 1., 0., 0.);
			glReadBuffer(GL_FRONT);
			// Intentionally leave a pending GL error (VirtualGL should clear the error state prior to readback)
			glReadPixels(0, 0, 0, 0, 0, 0, 0);
			glXSwapBuffers(dpy, win1);
			checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? 0x0000ff:0x00ff00);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		// Faker should readback front buffer on glFlush(), glFinish(), and
		// glXWaitGL()
		try
		{
			printf("glFlush() [f]:                 ");
			glClearBuffer(GL_FRONT, 1., 0., 1., 0.);
			glClearBuffer(GL_BACK, 0., 1., 1., 0.);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkframe(win1, 1, lastframe1);
			glDrawBuffer(GL_FRONT);  glFlush();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? 0xff00ff:0xffff00);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glFinish() [f]:                ");
			glClearBuffer(GL_FRONT, 1., 1., 0., 0.);
			glClearBuffer(GL_BACK, 0., 0., 1., 0.);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkframe(win1, 1, lastframe1);
			glDrawBuffer(GL_FRONT);  glFinish();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? 0x00ffff:0xff0000);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glXWaitGL() [f]:               ");
			glClearBuffer(GL_FRONT, 1., 0., 0., 0.);
			glClearBuffer(GL_BACK, 0., 1., 0., 0.);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkframe(win1, 1, lastframe1);
			glDrawBuffer(GL_FRONT);  glXWaitGL();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? 0x0000ff:0x00ff00);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glPopAttrib() [f]:             ");
			glDrawBuffer(GL_BACK);  glFinish();
			checkframe(win1, 1, lastframe1);
			glPushAttrib(GL_COLOR_BUFFER_BIT);
			glClearBuffer(GL_FRONT, 1., 1., 0., 0.);
			glPopAttrib();  // Back buffer should now be current again & dirty flag should be set
			glClearBuffer(GL_BACK, 0., 0., 1., 0.);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? 0x00ffff:0xff0000);
			printf("SUCCESS\n");
		}	
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glXMakeCurrent() [f]:          ");
			glClearBuffer(GL_FRONT, 0., 1., 1., 0.);
			glClearBuffer(GL_BACK, 1., 0., 1., 0.);
			glXMakeCurrent(dpy, win1, ctx0);  // readback should occur
			glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win0, win0, ctx0);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, dbworking? 0xffff00:0xff00ff);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glXMakeContextCurrent() [f]:   ");
			glClearBuffer(GL_FRONT, 0., 0., 1., 0.);
			glClearBuffer(GL_BACK, 1., 1., 0., 0.);
			glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win0, 1, lastframe0);
			checkwindowcolor(win0, dbworking? 0xff0000:0x00ffff);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		// Test for proper handling of GL_FRONT_AND_BACK
		try
		{
			printf("glXSwapBuffers() [f&b]:        ");
			glClearBuffer(GL_FRONT_AND_BACK, 1., 0., 0., 0.);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, win1);
			checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, 0x0000ff);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glFlush() [f&b]:               ");
			glClearBuffer(GL_FRONT_AND_BACK, 0., 1., 0., 0.);
			glReadBuffer(GL_BACK);
			glFlush();  glFlush();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 2, lastframe1);
			checkwindowcolor(win1, 0x00ff00);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glFinish() [f&b]:              ");
			glClearBuffer(GL_FRONT_AND_BACK, 0., 1., 1., 0.);
			glReadBuffer(GL_BACK);
			glFinish();  glFinish();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 2, lastframe1);
			checkwindowcolor(win1, 0xffff00);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glXWaitGL() [f&b]:             ");
			glClearBuffer(GL_FRONT_AND_BACK, 1., 0., 1., 0.);
			glReadBuffer(GL_BACK);
			glXWaitGL();  glXWaitGL();
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win1, 2, lastframe1);
			checkwindowcolor(win1, 0xff00ff);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glXMakeCurrent() [f&b]:        ");
			glClearBuffer(GL_FRONT_AND_BACK, 0., 0., 1., 0.);
			glXMakeCurrent(dpy, win0, ctx0);  // readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win0, win0, ctx0);
			checkframe(win1, 1, lastframe1);
			checkwindowcolor(win1, 0xff0000);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}

		try
		{
			printf("glXMakeContextCurrent() [f&b]: ");
			glClearBuffer(GL_FRONT_AND_BACK, 1., 1., 0., 0.);
			glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
			glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
			glDrawBuffer(GL_FRONT);  glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
			checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1);
			checkframe(win0, 1, lastframe0);
			checkwindowcolor(win0, 0x00ffff);
			printf("SUCCESS\n");
		}
		catch(rrerror &e) {printf("Failed! (%s)\n", e.getMessage());  retval=0;}
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
	if(ctemp!=vtemp) _prerror3("%s=%d in cfg & %d in X vis", #attrib, ctemp, vtemp); \
}


void configvsvisual(Display *dpy, GLXFBConfig c, XVisualInfo *v)
{
	int ctemp, vtemp, r, g, b, bs;
	if(!dpy) _error("Invalid display handle");
	if(!c) _error("Invalid FB config");
	if(!v) _error("Invalid visual pointer");
	getcfgattrib(c, GLX_RENDER_TYPE, ctemp);
	getvisattrib(v, GLX_RGBA, vtemp);
	if((ctemp==GLX_RGBA_BIT)!=(vtemp==1))
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
	if(!usingglp)
	{
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
}


int cfgid(Display *dpy, GLXFBConfig c)
{
	int temp=0;
	if(!c || !dpy) _error("Invalid argument to cfgid()");
	getcfgattrib(c, GLX_FBCONFIG_ID, temp);
	return temp;
}


void queryctxtest(Display *dpy, XVisualInfo *v, GLXFBConfig c)
{
	GLXContext ctx=0;  int render_type, fbcid, temp;
	if(usingglp) return;
	try
	{
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


#ifdef sun
#define glXGetFBConfigFromVisual glXGetFBConfigFromVisualSGIX
#else
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
#endif


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
					printf("CFG ID 0x%.2x:  ", fbcid);
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
				printf("CFG ID 0x%.2x:  ", fbcid);
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

		if(!(v0=XGetVisualInfo(dpy, VisualNoMask, &vtemp, &n)) || n==0)
			_throw("No X Visuals found");
		printf("\n");

		for(int i=0; i<n; i++)
		{
			XVisualInfo *v2=NULL;
			try
			{
				printf("Vis ID 0x%.2x:  ", (int)v0[i].visualid);
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
	if(v && n) {for(i=0; i<n; i++) {if(v[i]) XFree(v[i]);}  free(v);  v=NULL;}
	if(v0) {XFree(v0);  v0=NULL;}
	if(configs) {XFree(configs);  configs=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


typedef struct _testcolor
{
	GLfloat r, g, b;
	unsigned int bits;
} testcolor;

#define NC 6
testcolor tc[NC]=
{
	{1., 0., 0., 0x0000ff},
	{0., 1., 0., 0x00ff00},
	{0., 0., 1., 0xff0000},
	{1., 1., 0., 0x00ffff},
	{1., 0., 1., 0xff00ff},
	{0., 1., 1., 0xffff00}
};


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
				checkwindowcolor(win, tc[clr].bits);
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


int mttest(void)
{
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v=NULL;
	Display *dpy=NULL;  Window win[NTHR];
	GLXContext ctx[NTHR];
	mttestthread *mt[NTHR];  Thread *t[NTHR];
	XSetWindowAttributes swa;
	int dpyw, dpyh, i, retval=1;
	for(i=0; i<NTHR; i++)
	{
		win[i]=0;  ctx[i]=0;  mt[i]=NULL;  t[i]=NULL;
	}

	printf("Multi-threaded rendering test\n\n");

	try
	{
		if(!XInitThreads()) _throw("XInitThreads() failed");
		#ifdef sun
		if(!glXInitThreadsSUN()) _throw("glXInitThreadsSUN() failed");
		#endif
		if(!(dpy=XOpenDisplay(0))) _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
			_throw("Could not find a suitable visual");
		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=StructureNotifyMask|ExposureMask;
		for(i=0; i<NTHR; i++)
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

		for(i=0; i<NTHR; i++)
		{
			mt[i]=new mttestthread(i, dpy, win[i], ctx[i]);
			t[i]=new Thread(mt[i]);
			if(!mt[i] || !t[i]) _prerror1("Could not create thread %d", i);
			t[i]->start();
		}
		printf("Phase 1\n");
		for(i=0; i<NTHR; i++)
		{
			int winx=(i%10)*100, winy=i/10*120;
			XMoveResizeWindow(dpy, win[i], winx, winy, 200, 200);
			mt[i]->resize(200, 200);
			if(i<5) usleep(0);
			XResizeWindow(dpy, win[i], 100, 100);
			mt[i]->resize(100, 100);
		}
		XSync(dpy, False);
		printf("Phase 2\n");
		for(i=0; i<NTHR; i++)
		{
			XWindowChanges xwc;
			xwc.width=xwc.height=200;
			XConfigureWindow(dpy, win[i], CWWidth|CWHeight, &xwc);
			mt[i]->resize(200, 200);
		}
		XSync(dpy, False);
		printf("Phase 3\n");
		for(i=0; i<NTHR; i++)
		{
			XResizeWindow(dpy, win[i], 100, 100);
			mt[i]->resize(100, 100);
		}
		XSync(dpy, False);
		deadyet=true;
		for(i=0; i<NTHR; i++) t[i]->stop();
		for(i=0; i<NTHR; i++)
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

	for(i=0; i<NTHR; i++) {if(t[i]) {delete t[i];  t[i]=NULL;}}
	for(i=0; i<NTHR; i++) {if(mt[i]) {delete mt[i];  mt[i]=NULL;}}
	if(v) {XFree(v);  v=NULL;}
	for(i=0; i<NTHR; i++)
	{
		if(dpy && ctx[i])
		{
			glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx[i]);  ctx[i]=0;
		}
	}
	for(i=0; i<NTHR; i++) {if(dpy && win[i]) {XDestroyWindow(dpy, win[i]);  win[i]=0;}}
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
	glReadBuffer(buf); \
	unsigned int bufcol=checkbuffercolor(); \
	if(bufcol!=(shouldbe)) \
		_prerror2(tag" is 0x%.6x, should be 0x%.6x", bufcol, (shouldbe)); \
}

// Test Pbuffer and Pixmap rendering
int pbtest(void)
{
	Display *dpy=NULL;  Window win=0;  Pixmap pm0=0, pm1=0;
	GLXPixmap glxpm0=0, glxpm1=0;  GLXPbuffer pb=0;  GLXWindow glxwin=0;
	int dpyw, dpyh, lastframe=0, retval=1;
	int glxattrib[]={GLX_DOUBLEBUFFER, 1, GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT|GLX_PBUFFER_BIT|GLX_WINDOW_BIT,
		GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v=NULL;  GLXFBConfig c=0, *configs=NULL;  int n=0;
	GLXContext ctx=0;
	XSetWindowAttributes swa;

	printf("Off-screen rendering test\n\n");

	try
	{
		if(!(dpy=XOpenDisplay(0)))  _throw("Could not open display");
		dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
		dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

		if((configs=glXChooseFBConfig(dpy, DefaultScreen(dpy), glxattrib, &n))
			==NULL || n==0) _throw("Could not find a suitable FB config");
		c=configs[0];
		int fbcid=cfgid(dpy, c);
		XFree(configs);  configs=NULL;
		if((v=glXGetVisualFromFBConfig(dpy, c))==NULL)
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
		|| (pm1=XCreatePixmap(dpy, win, dpyw/2, dpyh/2, v->depth))==0)
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

		if(!(ctx=glXCreateNewContext(dpy, c, GLX_RGBA_TYPE, NULL, True)))
			_throw("Could not create context");

		if(!glXMakeContextCurrent(dpy, glxwin, glxwin, ctx))
			_throw("Could not make context current");
		checkcurrent(dpy, glxwin, glxwin, ctx);
		int dbwin=dbtest();
		if(!dbwin)
		{
			printf("WARNING: Double buffering appears to be broken.\n");
			printf("         Testing in single buffered mode\n\n");
		}

		if(!glXMakeContextCurrent(dpy, pb, pb, ctx))
			_throw("Could not make context current");
		checkcurrent(dpy, pb, pb, ctx);
		int dbpb=dbtest();
		if(!dbpb)
		{
			printf("WARNING: Double buffered Pbuffers not available.\n");
			printf("         Testing in single buffered mode\n\n");
		}
		checkframe(win, -1, lastframe);

		try
		{
			printf("PBuffer->Window:  ");
			if(!(glXMakeContextCurrent(dpy, pb, pb, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, pb, pb, ctx);
			glClearBuffer(GL_BACK, 1., 0., 0., 0.);
			glClearBuffer(GL_FRONT, 0., 1., 0., 0.);
			verifybufcolor(GL_BACK, dbpb? 0x0000ff:0x00ff00, "PB");
			if(!(glXMakeContextCurrent(dpy, glxwin, pb, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxwin, pb, ctx);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw/2, dpyh/2, GL_COLOR);
			glReadBuffer(GL_FRONT);
			glXSwapBuffers(dpy, glxwin);
			checkframe(win, 1, lastframe);
			checkreadbackstate(GL_FRONT, dpy, glxwin, pb, ctx);
			checkwindowcolor(win, dbpb? 0x0000ff:0x00ff00);
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}

		try
		{
			printf("Window->Pbuffer:  ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxwin, glxwin, ctx);
			glClearBuffer(GL_BACK, 0., 1., 1., 0.);
			glClearBuffer(GL_FRONT, 1., 0., 1., 0.);
			verifybufcolor(GL_BACK, dbwin? 0xffff00:0xff00ff, "Win");
			if(!(glXMakeContextCurrent(dpy, pb, glxwin, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, pb, glxwin, ctx);
			checkframe(win, 1, lastframe);
			glReadBuffer(GL_BACK);  glDrawBuffer(GL_BACK);
			glCopyPixels(0, 0, dpyw/2, dpyh/2, GL_COLOR);
			glXSwapBuffers(dpy, pb);
			verifybufcolor(GL_BACK, dbwin? 0xffff00:0xff00ff, "PB");
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}

		try
		{
			printf("Pixmap->Window:   ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxpm0, glxpm0, ctx);
			glClearBuffer(GL_FRONT, 0., 0., 1., 0.);
			verifybufcolor(GL_FRONT, 0xff0000, "PM0");
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, win, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkreadbackstate(GL_BACK, dpy, glxpm0, glxpm0, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			checkframe(win, 1, lastframe);
			checkwindowcolor(win, 0xff0000);
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}

		try
		{
			printf("Window->Pixmap:   ");
			if(!(glXMakeContextCurrent(dpy, glxwin, glxwin, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxwin, glxwin, ctx);
			glClearBuffer(GL_FRONT, 0., 1., 0., 0.);
			glClearBuffer(GL_BACK, 1., 0., 0., 0.);
			verifybufcolor(GL_FRONT, dbwin? 0x00ff00:0x0000ff, "Win");
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
			verifybufcolor(GL_BACK, dbwin? 0x00ff00:0x0000ff, "PM1");
			verifybufcolor(GL_FRONT, dbwin? 0x00ff00:0x0000ff, "PM1");
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}

		try
		{
			printf("Pixmap->Pixmap:   ");
			if(!(glXMakeContextCurrent(dpy, glxpm0, glxpm0, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxpm0, glxpm0, ctx);
			glClearBuffer(GL_FRONT, 1., 0., 1., 0.);
			verifybufcolor(GL_FRONT, 0xff00ff, "PM0");
			if(!(glXMakeContextCurrent(dpy, glxpm1, glxpm1, ctx)))
				_error("Could not make context current");
			checkcurrent(dpy, glxpm1, glxpm1, ctx);
			glDrawBuffer(GL_BACK);  glReadBuffer(GL_BACK);
			XCopyArea(dpy, pm0, pm1, DefaultGC(dpy, DefaultScreen(dpy)), 0, 0,
				dpyw/2, dpyh/2, 0, 0);
			checkreadbackstate(GL_BACK, dpy, glxpm1, glxpm1, ctx);
			int temp=-1;  glGetIntegerv(GL_DRAW_BUFFER, &temp);
			if(temp!=GL_BACK) _error("Draw buffer changed");
			verifybufcolor(GL_BACK, 0xff00ff, "PM1");
			verifybufcolor(GL_FRONT, 0xff00ff, "PM1");
			printf("SUCCESS\n");
		}
		catch(rrerror &e)
		{
			printf("Failed! (%s)\n", e.getMessage());  retval=0;
		}
	}
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx && dpy) {glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
	if(pb && dpy) {glXDestroyPbuffer(dpy, pb);  pb=0;}
	if(glxpm1 && dpy) {glXDestroyGLXPixmap(dpy, glxpm1);  glxpm1=0;}
	if(glxpm0 && dpy) {glXDestroyGLXPixmap(dpy, glxpm0);  glxpm0=0;}
	if(pm1 && dpy) {XFreePixmap(dpy, pm1);  pm1=0;}
	if(pm0 && dpy) {XFreePixmap(dpy, pm0);  pm0=0;}
	if(glxwin && dpy) {glXDestroyWindow(dpy, glxwin);  glxwin=0;}
	if(win && dpy) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	if(configs) {XFree(configs);  configs=NULL;}
	if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
	return retval;
}


static int check_errors(const char * tag)
{
	int i;
	int ret;
	char * s;

	ret = 0;
	i = glGetError();
	if (i!= GL_NO_ERROR) ret = 1;
	while(i != GL_NO_ERROR)
	{
		s = (char *) gluErrorString(i);
		if (s)
			printf("ERROR: %s in %s \n", s, tag);
		else
			printf("OpenGL error #%d in %s\n", i, tag);
		i = glGetError();
	}
	return ret;
}


// Put the display hash through its paces
#define NDPY 15
int dpyhashtest(void)
{
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None};
	Display *_dpy[NDPY], *dpy=NULL;  int i, dpyw, dpyh, retval=1;
	XVisualInfo *v=NULL;  Window win=0;
	XSetWindowAttributes swa;  GLXContext ctx=0;
	for(i=0; i<NDPY; i++) _dpy[i]=NULL;

	printf("Display hash test:\n\n");

	try
	{
		for(i=0; i<NDPY; i++)
		{
			if((_dpy[i]=XOpenDisplay(0))==NULL)
				_throw("Could not open display");
		}
		for(i=0; i<NDPY; i++)
		{
			XCloseDisplay(_dpy[i]);  _dpy[i]=NULL;
		}
		for(i=0; i<5; i++)
		{
			if((_dpy[i]=XOpenDisplay(0))==NULL)
				_throw("Could not open display");
		}
		for(i=0; i<3; i++)
		{
			XCloseDisplay(_dpy[i]);  _dpy[i]=NULL;
		}

		for(i=3; i<5; i++)
		{
			dpy=_dpy[i];
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
			XMapWindow(dpy, win);
			if((ctx=glXCreateContext(dpy, v, 0, True))==NULL)
				_throw("Could not establish GLX context");
			XFree(v);  v=NULL;
			if(!glXMakeCurrent(dpy, win, ctx))
				_throw("Could not make context current");
			checkcurrent(dpy, win, win, ctx);
			glClearBuffer(GL_BACK, 1., 1., 1., 0.);
			glXSwapBuffers(dpy, win);
			checkwindowcolor(win, 0xffffff);
			glXMakeCurrent(dpy, 0, 0);
			glXDestroyContext(dpy, ctx);  ctx=0;
			XDestroyWindow(dpy, win);  win=0;
		}
		for(i=3; i<5; i++) {XCloseDisplay(_dpy[i]);  _dpy[i]=NULL;}

		printf("SUCCESS!\n");
	}	
	catch(rrerror &e)
	{
		printf("Failed! (%s)\n", e.getMessage());  retval=0;
	}
	if(ctx && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
	if(win && dpy) {XDestroyWindow(dpy, win);  win=0;}
	if(v) {XFree(v);  v=NULL;}
	for(i=0; i<NDPY; i++) {if(_dpy[i]) {XCloseDisplay(_dpy[i]);  _dpy[i]=NULL;}}
	return retval;
}


#ifdef GLX_ARB_get_proc_address

#define testprocsym(f) \
	if((sym=(void *)glXGetProcAddressARB((const GLubyte *)#f))==NULL) \
		_throw("glXGetProcAddressARB(\""#f"\") returned NULL"); \
	else if(sym!=(void *)f) \
		_throw("glXGetProcAddressARB(\""#f"\")!="#f);

int procaddrtest(void)
{
	int retval=1;  void *sym=NULL;

	printf("glXGetProcAddressARB test:\n\n");

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
	return retval;
}
#endif


int main(int argc, char **argv)
{
	int ret=0;
	if(putenv((char *)"RRAUTOTEST=1")==-1
	|| putenv((char *)"RRSPOIL=0")==-1)
	{
		printf("putenv() failed!\n");  return -1;
	}
	#ifdef USEGLP
	char *temp=NULL;
	if((temp=getenv("VGL_DISPLAY"))!=NULL && strlen(temp)>0)
	{
		for(int i=0; i<strlen(temp); i++)
			if(temp[i]!=' ' && temp[i]!='\t')
			{
				temp=&temp[i];  break;
			}
		if(temp[0]=='/' || !strncmp(temp, "GLP", 3)) usingglp=1;
	}
	#endif

	// Intentionally leave a pending dlerror()
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat");
	dlsym(RTLD_NEXT, "ifThisSymbolExistsI'llEatMyHat2");

	#ifdef GLX_ARB_get_proc_address
	if(!procaddrtest()) ret=-1;
	printf("\n");
	#endif
	if(!rbtest()) ret=-1;
	printf("\n");
	if(!vistest()) ret=-1;
	printf("\n");
	if(!mttest()) ret=-1;
	printf("\n");
	if(!pbtest()) ret=-1;
	printf("\n");
	if(!dpyhashtest()) ret=-1;
	printf("\n");
	return ret;
}
