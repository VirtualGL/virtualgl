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

#define NUMWINDOWS 2

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

void drawwindow(Display *dpy, Window win, int w, int h, GLXContext ctx)
{
	unsigned char *buf;  int pixsize=3;
	printf("Redrawing window 0x%.8x (%d, %d)\n", win, w, h);
/*	if((buf=(unsigned char *)malloc(w*h*pixsize))==NULL)
	{
		printf("ERROR: Could not allocate buffer\n");  exit(1);
	}
	int i, l, j, k, x=0, y=0, pitch=w*pixsize;
	for(i=0; i<h; i++)
	{
		l=h-i-1;
		for(j=0; j<w; j++)
		{
			for(k=0; k<pixsize; k++)
			{
				buf[i*pitch+j*pixsize+k]=((l+y)*(j+x)*(k+1))%256;
			}
		}
	}
*/
	if(!glXMakeCurrent(dpy, win, ctx))
	{
		printf("Could not make context current\n");//  free(buf);
		exit(1);
	}
	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT);
/*	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glDrawBuffer(GL_BACK);
	glDrawPixels(w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);*/
	glBegin(GL_TRIANGLES);
	glColor3f(0.5, 0.0, 0.75);
	glVertex2f(-0.5, -0.5);
	glColor3f(0.0, 0.75, 0.25);
	glVertex2f(0.5, -0.5);
	glColor3f(0.0, 0.25, 0.75);
	glVertex2f(0.0, 0.5);
	glEnd();
	glXSwapBuffers(dpy, win);
	if(check_errors("glDrawPixels")) {/*free(buf);*/  exit(1);}
//	free(buf);
}

int checkwindow(Display *dpy, Window win, int w, int h, GLXContext ctx)
{
	unsigned char *buf;
	XSync(dpy, False);
	XWindowAttributes xwa;  int pixsize=3;
	XGetWindowAttributes(dpy, win, &xwa);
	if((buf=(unsigned char *)malloc(w*h*pixsize))==NULL)
	{
		printf("ERROR: Could not allocate buffer\n");  exit(1);
	}
	if(!glXMakeCurrent(dpy, win, ctx))
	{
		printf("Could not make context current\n");  free(buf);
		exit(1);
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);
	if(check_errors("glReadPixels")) {free(buf);  exit(1);}
	int i, j, k, l, x=0, y=0, pitch=w*pixsize;
	int pc=0;
	for(i=0; i<h-1; i++)
	{
		l=h-i-1;
		for(j=0; j<w; j++)
		{
			for(k=0; k<pixsize; k++)
			{
				if(buf[i*pitch+j*pixsize+k]!=((l+y)*(j+x)*(k+1))%256 && k!=3)
				{
					pc++;
				}
			}
		}
	}
	printf("%.2f%% error on readback\n", (double)pc/(double)(w*h*pixsize)*100);
	glXSwapBuffers(dpy, win);
	free(buf);
	if(pc) return 0;
	return 1;
}

int main(int argc, char **argv)
{
	Display *dpy;  int i;
	Window win[NUMWINDOWS];
	int dpyw, dpyh;
	int glxattrib[]={GLX_DOUBLEBUFFER, GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None};
	XVisualInfo *v;  GLXContext ctx[NUMWINDOWS];
	XSetWindowAttributes swa;

	if(!(dpy=XOpenDisplay(0)))
	{
		printf("ERROR: Could not open display\n");  exit(1);
	}
	dpyw=DisplayWidth(dpy, DefaultScreen(dpy));
	dpyh=DisplayHeight(dpy, DefaultScreen(dpy));

	if((v=glXChooseVisual(dpy, DefaultScreen(dpy), glxattrib))==NULL)
	{
		printf("ERROR: Could not find a suitable 24-bit visual\n");  exit(1);
	}
	Window root=RootWindow(dpy, DefaultScreen(dpy));
	swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
	swa.border_pixel=0;
	swa.event_mask=ExposureMask | StructureNotifyMask | KeyPressMask | ButtonPressMask
		| ButtonMotionMask;

	for(i=0; i<NUMWINDOWS; i++)
	{
		if((win[i]=XCreateWindow(dpy, root, i*(dpyw/(NUMWINDOWS+1)),
			0, dpyw/(NUMWINDOWS+1), dpyh*3/4, 0, v->depth, InputOutput, v->visual,
			CWBorderPixel|CWColormap|CWEventMask, &swa))
			==0)
		{
			printf("ERROR: Could not create window %d\n", i+1);  exit(1);
		}
		char temps[80];
		sprintf(temps, "Window %d", i+1);
		XStoreName(dpy, win[i], temps);
		XMapWindow(dpy, win[i]);
		printf("Window %d = 0x%.8x (%d x %d)\n", i+1, win[i], dpyw/(NUMWINDOWS+1), dpyh*3/4);  fflush(stdout);
		if(i==0) {if((ctx[i]=glXCreateContext(dpy, v, 0, True))==NULL)
		{
			printf("ERROR: Could not establish GLX context\n");
			exit(1);
		}}
		else ctx[i]=ctx[0];
	}

	while(1)
	{
		XEvent e;
		if(XNextEvent(dpy, &e)) break;
		switch(e.type)
		{
			case ConfigureNotify:
			{
				for(i=0; i<NUMWINDOWS; i++) if(e.xconfigure.window==win[i]) break;
				if(i>=NUMWINDOWS) break;
				printf("CN on Window %d\n", i+1);
				drawwindow(e.xconfigure.display, e.xconfigure.window, e.xconfigure.width, e.xconfigure.height, ctx[i]);
//				checkwindow(e.xconfigure.display, e.xconfigure.window, e.xconfigure.width, e.xconfigure.height, ctx[i]);
				break;
			}
			case Expose:
			{
				for(i=0; i<NUMWINDOWS; i++) if(e.xexpose.window==win[i]) break;
				if(i>=NUMWINDOWS) break;
				printf("Expose on Window %d\n", i+1);
				XWindowAttributes xwa;
				XGetWindowAttributes(e.xexpose.display, e.xexpose.window, &xwa);
				drawwindow(e.xexpose.display, e.xexpose.window, xwa.width, xwa.height, ctx[i]);
//				checkwindow(e.xexpose.display, e.xexpose.window, e.xexpose.width, e.xexpose.height, ctx[i]);
				break;
			}
			case KeyPress:
				KeySym ks=XKeycodeToKeysym(dpy, e.xkey.keycode, 0);
				if(ks==XK_Escape) exit(0);
				break;
		}
	}

	return 0;
}
