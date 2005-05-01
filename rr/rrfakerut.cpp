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
#include "rrerror.h"


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


unsigned int checkbuffercolor(void)
{
	int i, vp[4];  unsigned int ret=0;
	unsigned char *buf=NULL;

	try
	{
		vp[0]=vp[1]=vp[2]=vp[3]=0;
		glGetIntegerv(GL_VIEWPORT, vp);
		if(vp[2]<1 || vp[3]<1) throw("Invalid viewport dimensions");

		if((buf=(unsigned char *)malloc(vp[2]*vp[3]*3))==NULL)
			throw("Could not allocate buffer");

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, vp[2], vp[3], GL_RGB, GL_UNSIGNED_BYTE, buf);
		for(i=3; i<vp[2]*vp[3]*3; i+=3)
		{
			if(buf[i]!=buf[0] || buf[i+1]!=buf[1] || buf[i+2]!=buf[2])
				throw("Bogus data read back from framebuffer");
		}
		ret=buf[2]|(buf[1]<<8)|(buf[0]<<16);
		free(buf);
		return ret;
	}
	catch(...)
	{
		if(buf) free(buf);  throw;
	}
	return 0;
}


int checkwindowcolor(unsigned int color)
{
	char *e=NULL;  int fakerclr;
	if((e=getenv("__VGL_AUTOTESTCLR"))==NULL)
	{
		printf("Failed! (Can't communicate w/ faker)\n");  return 0;
	}
	if((fakerclr=atoi(e))<0 || fakerclr>0xffffff)
	{
		printf("Failed! (Bogus data read back)\n");  return 0;
	}
	if((unsigned int)fakerclr!=color)
	{
		printf("Failed! (Color is 0x%.6x, should be 0x%.6x)\n", fakerclr, color);
		return 0;
	}
	return 1;
}


int checkframe(int desiredreadbacks)
{
	char *e=NULL;  int frame, ret=1;  static int lastframe=0;
	if((e=getenv("__VGL_AUTOTESTFRAME"))==NULL || (frame=atoi(e))<1)
	{
		printf("Failed! (Can't communicate w/ faker)\n");  return 0;
	}
	if(frame-lastframe!=desiredreadbacks)
	{
		printf("Failed! (Expected %d readback%s, not %d)\n", desiredreadbacks,
			desiredreadbacks==1?"":"s", frame-lastframe);  ret=0;
	}
	lastframe=frame;
	return ret;
}


void checkcurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
	if(glXGetCurrentDisplay()!=dpy) printf("glXGetCurrentDisplay() failed!\n");
	if(glXGetCurrentDrawable()!=draw) printf("glXGetCurrentDrawable() failed!\n");
	if(glXGetCurrentReadDrawable()!=read) printf("glXGetCurrentReadDrawable() failed!\n");
	if(glXGetCurrentContext()!=ctx) printf("glXGetCurrentContext() failed!\n");
}


int checkreadbackstate(int oldreadbuf, Display *dpy, GLXDrawable draw,
	GLXDrawable read, GLXContext ctx)
{
	if(glXGetCurrentDisplay()!=dpy)
	{
		printf("Failed! (Current display changed)\n");
		return 0;
	}
	if(glXGetCurrentDrawable()!=draw || glXGetCurrentReadDrawable()!=read)
	{
		printf("Failed! (Current drawable changed\n");
		return 0;
	}
	if(glXGetCurrentContext()!=ctx)
	{
		printf("Failed! (Context changed)\n");
		return 0;
	}
	int readbuf=-1;
	glGetIntegerv(GL_READ_BUFFER, &readbuf);
	if(readbuf!=oldreadbuf)
	{
		printf("Failed! (Read buffer changed)\n");
		return 0;
	}
	return 1;
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
	int dpyw, dpyh;
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
			==NULL) _throw("Could not find a suitable FB config");
		c=configs[0];
		XFree(configs);  configs=NULL;

		Window root=RootWindow(dpy, DefaultScreen(dpy));
		swa.colormap=XCreateColormap(dpy, root, v0->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=KeyPressMask | ButtonPressMask;
		if((win0=XCreateWindow(dpy, root, 0, 0, dpyw/2, dpyh/2, 0, v0->depth,
			InputOutput, v0->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XMapWindow(dpy, win0);
		if(!(v1=glXGetVisualFromFBConfig(dpy, c)))
			_throw("glXGetVisualFromFBConfig() failed");
		swa.colormap=XCreateColormap(dpy, root, v1->visual, AllocNone);
		if((win1=XCreateWindow(dpy, root, dpyw/2, 0, dpyw/2, dpyh/2, 0, v1->depth,
			InputOutput, v1->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa))==0)
			_throw("Could not create window");
		XFree(v1);  v1=NULL;
		XMapWindow(dpy, win1);

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

		// Faker should readback back buffer on a call to glXSwapBuffers()
		printf("glXSwapBuffers() [b]:          ");
		glClearBuffer(GL_BACK, 1., 0., 0., 0.);
		glClearBuffer(GL_FRONT, 0., 1., 0., 0.);
		glReadBuffer(GL_FRONT);
		glXSwapBuffers(dpy, win1);
		if(checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1) && checkframe(1)
		&& checkwindowcolor(dbworking? 0x0000ff:0x00ff00)) printf("SUCCESS\n");

		// Faker should readback front buffer on glFlush(), glFinish(), and
		// glXWaitGL()
		printf("glFlush() [f]:                 ");
		glClearBuffer(GL_FRONT, 1., 0., 1., 0.);
		glClearBuffer(GL_BACK, 0., 1., 1., 0.);
		glReadBuffer(GL_BACK);
		glFlush();  glFlush();
		if(checkframe(1))
		{
			glDrawBuffer(GL_FRONT);  glFlush();
			if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(1)
			&& checkwindowcolor(dbworking? 0xff00ff:0xffff00)) printf("SUCCESS\n");
		}

		printf("glFinish() [f]:                ");
		glClearBuffer(GL_FRONT, 1., 1., 0., 0.);
		glClearBuffer(GL_BACK, 0., 0., 1., 0.);
		glReadBuffer(GL_BACK);
		glFinish();  glFinish();
		if(checkframe(1))
		{
			glDrawBuffer(GL_FRONT);  glFinish();
			if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(1)
			&& checkwindowcolor(dbworking? 0x00ffff:0xff0000)) printf("SUCCESS\n");
		}

		printf("glXWaitGL() [f]:               ");
		glClearBuffer(GL_FRONT, 1., 0., 0., 0.);
		glClearBuffer(GL_BACK, 0., 1., 0., 0.);
		glReadBuffer(GL_BACK);
		glXWaitGL();  glXWaitGL();
		if(checkframe(1))
		{
			glDrawBuffer(GL_FRONT);  glXWaitGL();
			if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(1)
			&& checkwindowcolor(dbworking? 0x0000ff:0x00ff00)) printf("SUCCESS\n");
		}

		printf("glXMakeCurrent() [f]:          ");
		glClearBuffer(GL_FRONT, 0., 1., 1., 0.);
		glClearBuffer(GL_BACK, 1., 0., 1., 0.);
		glXMakeCurrent(dpy, win1, ctx0);  // readback should occur
		glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
		glDrawBuffer(GL_FRONT);  glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
		if(checkreadbackstate(GL_BACK, dpy, win0, win0, ctx0) && checkframe(1)
		&& checkwindowcolor(dbworking? 0xffff00:0xff00ff)) printf("SUCCESS\n");

		printf("glXMakeContextCurrent() [f]:   ");
		glClearBuffer(GL_FRONT, 0., 0., 1., 0.);
		glClearBuffer(GL_BACK, 1., 1., 0., 0.);
		glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
		glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
		glDrawBuffer(GL_FRONT);  glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
		if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(1)
		&& checkwindowcolor(dbworking? 0xff0000:0x00ffff)) printf("SUCCESS\n");

		// Test for proper handling of GL_FRONT_AND_BACK
		printf("glXSwapBuffers() [f&b]:        ");
		glClearBuffer(GL_FRONT_AND_BACK, 1., 0., 0., 0.);
		glReadBuffer(GL_FRONT);
		glXSwapBuffers(dpy, win1);
		if(checkreadbackstate(GL_FRONT, dpy, win1, win0, ctx1) && checkframe(1)
		&& checkwindowcolor(0x0000ff)) printf("SUCCESS\n");

		printf("glFlush() [f&b]:               ");
		glClearBuffer(GL_FRONT_AND_BACK, 0., 1., 0., 0.);
		glReadBuffer(GL_BACK);
		glFlush();  glFlush();
		if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(2)
		&& checkwindowcolor(0x00ff00)) printf("SUCCESS\n");

		printf("glFinish() [f&b]:              ");
		glClearBuffer(GL_FRONT_AND_BACK, 0., 1., 1., 0.);
		glReadBuffer(GL_BACK);
		glFinish();  glFinish();
		if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(2)
		&& checkwindowcolor(0xffff00)) printf("SUCCESS\n");

		printf("glXWaitGL() [f&b]:             ");
		glClearBuffer(GL_FRONT_AND_BACK, 1., 0., 1., 0.);
		glReadBuffer(GL_BACK);
		glXWaitGL();  glXWaitGL();
		if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(2)
		&& checkwindowcolor(0xff00ff)) printf("SUCCESS\n");

		printf("glXMakeCurrent() [f&b]:        ");
		glClearBuffer(GL_FRONT_AND_BACK, 0., 0., 1., 0.);
		glXMakeCurrent(dpy, win1, ctx0);  // readback should occur
		glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
		glDrawBuffer(GL_FRONT);  glXMakeCurrent(dpy, win0, ctx0);  // No readback should occur
		if(checkreadbackstate(GL_BACK, dpy, win0, win0, ctx0) && checkframe(1)
		&& checkwindowcolor(0xff0000)) printf("SUCCESS\n");

		printf("glXMakeContextCurrent() [f&b]: ");
		glClearBuffer(GL_FRONT_AND_BACK, 1., 1., 0., 0.);
		glXMakeContextCurrent(dpy, win0, win0, ctx1);  // No readback should occur
		glXMakeContextCurrent(dpy, win1, win0, ctx1);  // readback should occur
		glDrawBuffer(GL_FRONT);  glXMakeContextCurrent(dpy, win1, win0, ctx1);  // No readback should occur
		if(checkreadbackstate(GL_BACK, dpy, win1, win0, ctx1) && checkframe(1)
		&& checkwindowcolor(0x00ffff)) printf("SUCCESS\n");

		if(ctx0 && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx0);  ctx0=0;}
		if(ctx1 && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx1);  ctx1=0;}
		if(win0) {XDestroyWindow(dpy, win0);  win0=0;}
		if(win1) {XDestroyWindow(dpy, win1);  win1=0;}
		if(v0) {XFree(v0);  v0=NULL;}
		if(v1) {XFree(v1);  v1=NULL;}
		if(configs) {XFree(configs);  configs=NULL;}
		if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
		return 1;
	}
	catch(...)
	{
		if(ctx0 && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx0);  ctx0=0;}
		if(ctx1 && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx1);  ctx1=0;}
		if(win0) {XDestroyWindow(dpy, win0);  win0=0;}
		if(win1) {XDestroyWindow(dpy, win1);  win1=0;}
		if(v0) {XFree(v0);  v0=NULL;}
		if(v1) {XFree(v1);  v1=NULL;}
		if(configs) {XFree(configs);  configs=NULL;}
		if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
		throw;
	}
	return 0;
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


int main(int argc, char **argv)
{
	if(putenv((char *)"VGL_AUTOTEST=1")==-1
	|| putenv((char *)"VGL_SPOIL=0")==-1)
	{
		printf("putenv() failed!\n");  return -1;
	}

	try
	{
		if(!rbtest()) printf("Readback test failed!\n");
	}
	catch(rrerror &e)
	{
		printf("%s--\n%s\n", e.getMethod(), e.getMessage());
		return -1;
	}
}
