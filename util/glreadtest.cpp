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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "rrutil.h"
#include "rrtimer.h"
#include "rrerror.h"
#include <errno.h>
#include "../rr/glx.h"
#include <GL/glu.h>
#include "x11err.h"

#define BMPPAD(pitch) ((pitch+(sizeof(int)-1))&(~(sizeof(int)-1)))

//////////////////////////////////////////////////////////////////////
// Structs and globals
//////////////////////////////////////////////////////////////////////

typedef struct _pixelformat
{
	unsigned long rmask, gmask, bmask, amask;
	int pixelsize;
	int glformat;
	int bgr;
	const char *name;
} pixelformat;

#if defined(GL_BGRA_EXT) && defined(GL_BGR_EXT)
 #define FORMATS 4
#elif defined(GL_BGRA_EXT) || defined(GL_BGR_EXT)
 #define FORMATS 3
#else
 #define FORMATS 2
#endif
pixelformat pix[FORMATS]={
	#ifdef GL_BGRA_EXT
	{0xFF0000, 0xFF00, 0xFF, 0xFF000000, 4, GL_BGRA_EXT, 1, "BGRA"},
	#endif
	#ifdef GL_BGR_EXT
	{0xFF0000, 0xFF00, 0xFF, 0, 3, GL_BGR_EXT, 1, "BGR"},
	#endif
	{0xFF, 0xFF00, 0xFF0000, 0xFF000000, 4, GL_RGBA, 0, "RGBA"},
	{0xFF, 0xFF00, 0xFF0000, 0, 3, GL_RGB, 0, "RGB"},
};

#define bench_name		"GLreadtest"

#define WIDTH             701
#define HEIGHT            701
#define N                 5

Display *dpy=NULL;  Window win=0;
rrtimer timer;
int usewindow=0;

//////////////////////////////////////////////////////////////////////
// Error handling
//////////////////////////////////////////////////////////////////////

int xhandler(Display *dpy, XErrorEvent *xe)
{
	fprintf(stderr, "X11 Error: %s\n", x11error(xe->error_code));
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Pbuffer setup
//////////////////////////////////////////////////////////////////////

void pbufferinit(Display *dpy, Window win)
{
	int fbattribs[]={GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, None};
	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0, None};
	GLXFBConfig *fbconfigs;  int nelements;
	GLXPbuffer pbuffer=0;
	GLXContext ctx=0;

	try {

	errifnot(win);  errifnot(dpy);
	if(usewindow) fbattribs[9]=GLX_WINDOW_BIT;
	fbconfigs=glXChooseFBConfig(dpy, DefaultScreen(dpy), fbattribs, &nelements);
	if(!nelements || !fbconfigs) _throw("Could not obtain Visual");
	pbattribs[1]=WIDTH;  pbattribs[3]=HEIGHT;
	if(!(ctx=glXCreateNewContext(dpy, fbconfigs[0], GLX_RGBA_TYPE, NULL, True)))
		_throw("Could not create GLX context");
	if(usewindow)
		glXMakeContextCurrent(dpy, win, win, ctx);
	else
	{
		if(!(pbuffer=glXCreatePbuffer(dpy, fbconfigs[0], pbattribs)))
			_throw("Could not create Pbuffer");
		glXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx);
	}

	} catch(...)
	{
		if(pbuffer) glXDestroyPbuffer(dpy, pbuffer);
		if(ctx) {glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx);}
		throw;
	}
}

//////////////////////////////////////////////////////////////////////
// Useful functions
//////////////////////////////////////////////////////////////////////

static int check_errors(const char * tag)
{
	int i, ret;  char *s;
	ret=0;
	i=glGetError();
	if(i!=GL_NO_ERROR) ret=1;
	while(i!=GL_NO_ERROR)
	{
		s=(char *)gluErrorString(i);
		if(s) fprintf(stderr, "ERROR: %s in %s \n", s, tag);
		else fprintf(stderr, "OpenGL error #%d in %s\n", i, tag);
		i=glGetError();
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////
// Buffer initialization and checking
//////////////////////////////////////////////////////////////////////
void initbuf(int x, int y, int w, int h, int pixsize, unsigned char *buf, int bassackwards, int pad)
{
	int i, j, k, l, pitch;
	if(pad) pitch=BMPPAD(w*pixsize);
	else pitch=w*pixsize;
	for(i=0; i<h; i++)
	{
		l=bassackwards?h-i-1:i;
		for(j=0; j<w; j++)
		{
			for(k=0; k<pixsize; k++)
			{
				buf[i*pitch+j*pixsize+k]=((l+y)*(j+x)*(k+1))%256;
			}
		}
	}
}

int cmpbuf(int x, int y, int w, int h, int ps, unsigned char *buf, int bassackwards, int byteswap, int pad)
{
	int i, j, k, l, pitch;
	if(pad) pitch=BMPPAD(w*ps);
	else pitch=w*ps;
	for(i=0; i<h; i++)
	{
		l=bassackwards?h-i-1:i;
		for(j=0; j<w; j++)
		{
			for(k=0; k<ps; k++)
			{
				if(!littleendian() && byteswap)
				{
					if(buf[i*pitch+j*ps+(ps-k-1)]!=((l+y)*(j+x)*(k+1))%256 && k!=3) return 0;
				}
				else
				{
					if(buf[i*pitch+j*ps+k]!=((l+y)*(j+x)*(k+1))%256 && k!=3) return 0;
				}
			}
		}
	}
	return 1;
}

// Makes sure the frame buffer has been cleared prior to a write
void clearfb(void)
{
	unsigned char *buf=NULL;
	if((buf=(unsigned char *)malloc(WIDTH*HEIGHT*3))==NULL)
		_throw("Could not allocate buffer");
	memset(buf, 0xFF, WIDTH*HEIGHT*3);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);
	glClearColor(0., 0., 0., 0.);
	glClear(GL_COLOR_BUFFER_BIT);
	glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, buf);
	check_errors("frame buffer read");
	for(int i=0; i<WIDTH*HEIGHT*3; i++)
	{
		if(buf[i]!=0) {fprintf(stderr, "Buffer was not cleared\n");  break;}
	}
	if(buf) free(buf);
}

//////////////////////////////////////////////////////////////////////
// The actual tests
//////////////////////////////////////////////////////////////////////

// Generic GL write test
void glwrite(int format)
{
	unsigned char *rgbaBuffer=NULL;  int i, n, ps=pix[format].pixelsize;
	double rbtime;

	try {

	fprintf(stderr, "glDrawPixels():               ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	clearfb();
	if((rgbaBuffer=(unsigned char *)malloc(WIDTH*HEIGHT*ps))==NULL)
		_throw("Could not allocate buffer");
	initbuf(0, 0, WIDTH, HEIGHT, ps, rgbaBuffer, 1, 0);
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			glDrawPixels(WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE, rgbaBuffer);
		}
		rbtime=timer.elapsed();
	} while(rbtime<2. && !check_errors("frame buffer write"));
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	if(rgbaBuffer) free(rgbaBuffer);
}

// Generic OpenGL readback test
void glread(int format)
{
	unsigned char *rgbaBuffer=NULL;  int i, n, ps=pix[format].pixelsize;
	double rbtime;

	try {

	fprintf(stderr, "glReadPixels() [bottom-up]:   ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glReadBuffer(GL_FRONT);
	if((rgbaBuffer=(unsigned char *)malloc(WIDTH*HEIGHT*ps))==NULL)
		_throw("Could not allocate buffer");
	memset(rgbaBuffer, 0, WIDTH*HEIGHT*ps);
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			glReadPixels(0, 0, WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE, rgbaBuffer);
		}
		rbtime=timer.elapsed();
		if(!cmpbuf(0, 0, WIDTH, HEIGHT, ps, rgbaBuffer, 1, 0, 0))
			_throw("ERROR: Bogus data read back.");
	} while (rbtime<2. && !check_errors("frame buffer read"));
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime));

	fprintf(stderr, "glReadPixels() [top-down]:    ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glReadBuffer(GL_FRONT);
	memset(rgbaBuffer, 0, WIDTH*HEIGHT*ps);
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			for(int j=0; j<HEIGHT; j++)
				glReadPixels(0, HEIGHT-j-1, WIDTH, 1, pix[format].glformat, GL_UNSIGNED_BYTE, &rgbaBuffer[WIDTH*ps*j]);
		}
		rbtime=timer.elapsed();
		if(!cmpbuf(0, 0, WIDTH, HEIGHT, ps, rgbaBuffer, 0, 0, 0))
			_throw("ERROR: Bogus data read back.");
	} while (rbtime<2. && !check_errors("frame buffer read"));
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	if(rgbaBuffer) free(rgbaBuffer);
}

void display(void)
{
	int format;

	for(format=0; format<FORMATS; format++)
	{
		fprintf(stderr, ">>>>>>>>>>  PIXEL FORMAT:  %s  <<<<<<<<<<\n", pix[format].name);
		glwrite(format);
		glread(format);
		fprintf(stderr, "\n");
	}

	exit(0);
}

//////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	fprintf(stderr, "\n%s v%s (Build %s)\n", bench_name, __VERSION, __BUILD);

	if(argc>1 && !stricmp(argv[1], "-window")) usewindow=1;

	try {

	XSetErrorHandler(xhandler);
	if(!(dpy=XOpenDisplay(0))) {fprintf(stderr, "Could not open display %s\n", XDisplayName(0));  exit(1);}

	if(DisplayWidth(dpy, DefaultScreen(dpy))<WIDTH && DisplayHeight(dpy, DefaultScreen(dpy))<HEIGHT)
	{
		fprintf(stderr, "ERROR: Please switch to a screen resolution of at least %d x %d.\n", WIDTH, HEIGHT);
		exit(1);
	}

	errifnot(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
		0, 0, usewindow?WIDTH:1, usewindow?HEIGHT:1, 0, WhitePixel(dpy, DefaultScreen(dpy)),
		BlackPixel(dpy, DefaultScreen(dpy))));
	if(usewindow) XMapWindow(dpy, win);
	XSync(dpy, False);
	pbufferinit(dpy, win);
	display();
	return 0;

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}
}
