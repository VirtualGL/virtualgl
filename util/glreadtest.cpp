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
#ifdef USEGLP
#include <GL/glp.h>
#endif
#include <GL/glu.h>
#include "x11err.h"

static int ALIGN=1;
#define PAD(w) (((w)+(ALIGN-1))&(~(ALIGN-1)))
#define BMPPAD(pitch) ((pitch+(sizeof(int)-1))&(~(sizeof(int)-1)))

//////////////////////////////////////////////////////////////////////
// Structs and globals
//////////////////////////////////////////////////////////////////////

typedef struct _pixelformat
{
	unsigned long roffset, goffset, boffset;
	int pixelsize;
	int glformat;
	int bgr;
	const char *name;
} pixelformat;

static const int FORMATS=2
 #ifdef GL_BGRA_EXT
 +1
 #endif
 #ifdef GL_BGR_EXT
 +1
 #endif
 #ifdef GL_ABGR_EXT
 +1
 #endif
 ;

pixelformat pix[2
 #ifdef GL_BGRA_EXT
 +1
 #endif
 #ifdef GL_BGR_EXT
 +1
 #endif
 #ifdef GL_ABGR_EXT
 +1
 #endif
 ]={
	#ifdef GL_BGRA_EXT
	{2, 1, 0, 4, GL_BGRA_EXT, 1, "BGRA"},
	#endif
	#ifdef GL_ABGR_EXT
	{3, 2, 1, 4, GL_ABGR_EXT, 0, "ABGR"},
	#endif
	#ifdef GL_BGR_EXT
	{2, 1, 0, 3, GL_BGR_EXT, 1, "BGR"},
	#endif
	{0, 1, 2, 4, GL_RGBA, 0, "RGBA"},
	{0, 1, 2, 3, GL_RGB, 0, "RGB"},
};

#define bench_name		"GLreadtest"

#define _WIDTH            701
#define _HEIGHT           701
#define N                 5

int WIDTH=_WIDTH, HEIGHT=_HEIGHT;
Display *dpy=NULL;  Window win=0;
rrtimer timer;
int useglp=0;
#ifdef USEGLP
int glpdevice=-1;
#endif
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
	GLXFBConfig *fbconfigs=NULL;  int nelements=0;
	GLXPbuffer pbuffer=0;
	GLXContext ctx=0;

	// Use GLX 1.1 functions here in case we're remotely displaying to
	// something that doesn't support GLX 1.3
	if(usewindow)
	{
		XVisualInfo *v=NULL;

		try {

		int fbattribsold[]={GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8, None};
		if(!(v=glXChooseVisual(dpy, DefaultScreen(dpy), fbattribsold)))
			_throw("Could not obtain Visual");
		if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
			_throw("Could not create GL context");
		XFree(v);
		glXMakeCurrent(dpy, win, ctx);
		return;

		} catch(...)
		{
			if(ctx) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);}
			if(v) XFree(v);
			throw;
		}
	}

	try {

	if(!useglp) {errifnot(win);  errifnot(dpy);}

	#ifdef USEGLP
	if(useglp)
	{
		fbattribs[6]=None;
		fbconfigs=glPChooseFBConfig(glpdevice, fbattribs, &nelements);
	}
	else
	#endif
	fbconfigs=glXChooseFBConfig(dpy, DefaultScreen(dpy), fbattribs, &nelements);
	if(!nelements || !fbconfigs) _throw("Could not obtain Visual");

	#ifdef USEGLP
	if(useglp)
		ctx=glPCreateNewContext(fbconfigs[0], GLX_RGBA_TYPE, NULL);
	else
	#endif
	ctx=glXCreateNewContext(dpy, fbconfigs[0], GLX_RGBA_TYPE, NULL, True);
	if(!ctx)	_throw("Could not create GL context");

	pbattribs[1]=WIDTH;  pbattribs[3]=HEIGHT;
	#ifdef USEGLP
	if(useglp)
		pbuffer=glPCreateBuffer(fbconfigs[0], pbattribs);
	else
	#endif
	pbuffer=glXCreatePbuffer(dpy, fbconfigs[0], pbattribs);
	if(!pbuffer) _throw("Could not create Pbuffer");

	#ifdef USEGLP
	if(useglp)
		glPMakeContextCurrent(pbuffer, pbuffer, ctx);
	else
	#endif
	glXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx);

	} catch(...)
	{
		if(pbuffer)
		{
			#ifdef USEGLP
			if(useglp) glPDestroyBuffer(pbuffer);
			else
			#endif
			glXDestroyPbuffer(dpy, pbuffer);
		}
		if(ctx)
		{
			#ifdef USEGLP
			if(useglp)
			{
				glPMakeContextCurrent(0, 0, 0);  glPDestroyContext(ctx);
			}
			else
			#endif
			{
				glXMakeContextCurrent(dpy, 0, 0, 0);  glXDestroyContext(dpy, ctx);
			}
		}
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
void initbuf(int x, int y, int w, int h, int format, unsigned char *buf)
{
	int i, j, ps=pix[format].pixelsize;
	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			buf[(i*w+j)*ps+pix[format].roffset]=((i+y)*(j+x))%256;
			buf[(i*w+j)*ps+pix[format].goffset]=((i+y)*(j+x)*2)%256;
			buf[(i*w+j)*ps+pix[format].boffset]=((i+y)*(j+x)*3)%256;
		}
	}
}

int cmpbuf(int x, int y, int w, int h, int format, unsigned char *buf, int bassackwards)
{
	int i, j, l, ps=pix[format].pixelsize;
	for(i=0; i<h; i++)
	{
		l=bassackwards?h-i-1:i;
		for(j=0; j<w; j++)
		{
			if(buf[l*PAD(w*ps)+j*ps+pix[format].roffset]!=((i+y)*(j+x))%256) return 0;
			if(buf[l*PAD(w*ps)+j*ps+pix[format].goffset]!=((i+y)*(j+x)*2)%256) return 0;
			if(buf[l*PAD(w*ps)+j*ps+pix[format].boffset]!=((i+y)*(j+x)*3)%256) return 0;
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
	initbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer);
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
	} while(rbtime<1. && !check_errors("frame buffer write"));
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
	glPixelStorei(GL_UNPACK_ALIGNMENT, ALIGN);
	glPixelStorei(GL_PACK_ALIGNMENT, ALIGN);
	glReadBuffer(GL_FRONT);
	if((rgbaBuffer=(unsigned char *)malloc(PAD(WIDTH*ps)*HEIGHT))==NULL)
		_throw("Could not allocate buffer");
	memset(rgbaBuffer, 0, PAD(WIDTH*ps)*HEIGHT);
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
		if(!cmpbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer, 0))
			_throw("ERROR: Bogus data read back.");
	} while (rbtime<1. && !check_errors("frame buffer read"));
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime));

	fprintf(stderr, "glReadPixels() [top-down]:    ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, ALIGN);
	glPixelStorei(GL_PACK_ALIGNMENT, ALIGN); 
	glReadBuffer(GL_FRONT);
	memset(rgbaBuffer, 0, PAD(WIDTH*ps)*HEIGHT);
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			for(int j=0; j<HEIGHT; j++)
				glReadPixels(0, HEIGHT-j-1, WIDTH, 1, pix[format].glformat, GL_UNSIGNED_BYTE, &rgbaBuffer[PAD(WIDTH*ps)*j]);
		}
		rbtime=timer.elapsed();
		if(!cmpbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer, 1))
			_throw("ERROR: Bogus data read back.");
	} while (rbtime<1. && !check_errors("frame buffer read"));
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

void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [-h|-?] [-window]\n", argv[0]);
	fprintf(stderr, "       [-width <n>] [-height <n>] [-align <n>]\n");
	#ifdef USEGLP
	fprintf(stderr, "       [-device <GLP device>]\n");
	#endif
	fprintf(stderr, "\n-h or -? = This screen\n");
	fprintf(stderr, "-window = Render to a window instead of a Pbuffer\n");
	fprintf(stderr, "-width = Set drawable width to n pixels (default: %d)\n", _WIDTH);
	fprintf(stderr, "-height = Set drawable height to n pixels (default: %d)\n", _HEIGHT);
	fprintf(stderr, "-align = Set row alignment to n bytes (default: %d)\n", ALIGN);
	#ifdef USEGLP
	fprintf(stderr, "-device = Set GLP device to use for rendering (default: Use GLX)\n");
	#endif
	fprintf(stderr, "\n");
	exit(0);
}

//////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	#ifdef USEGLP
	char *device=NULL;
	#endif
	fprintf(stderr, "\n%s v%s (Build %s)\n", bench_name, __VERSION, __BUILD);

	for(int i=0; i<argc; i++)
	{
		if(!stricmp(argv[i], "-h")) usage(argv);
		if(!stricmp(argv[i], "-?")) usage(argv);
		if(!stricmp(argv[i], "-window")) usewindow=1;
		if(!stricmp(argv[i], "-align") && i<argc-1)
		{
			int temp=atoi(argv[i+1]);  i++;
			if(temp>=1 && (temp&(temp-1))==0) ALIGN=temp;
		}
		if(!stricmp(argv[i], "-width") && i<argc-1)
		{
			int temp=atoi(argv[i+1]);  i++;
			if(temp>=1) WIDTH=temp;
		}
		if(!stricmp(argv[i], "-height") && i<argc-1)
		{
			int temp=atoi(argv[i+1]);  i++;
			if(temp>=1) HEIGHT=temp;
		}
		#ifdef USEGLP
		if(!strnicmp(argv[i], "-d", 2) && i<argc-1)
		{
			char **devices=NULL;  int ndevices=0;
			if((devices=glPGetDeviceNames(&ndevices))==NULL || ndevices<1)
			{
				fprintf(stderr, "ERROR: No GLP devices are registered.\n");
				exit(1);
			}
			if(!strnicmp(argv[i+1], "GLP", 3)) device=NULL;
			else device=argv[i+1];
			if((glpdevice=glPOpenDevice(device))<0)
			{
				fprintf(stderr, "ERROR: Could not open GLP device %s.\n", device);
				exit(1);
			}
			if(!device) device=devices[0];
			useglp=1;
		}
		#endif
	}

	try {

	if(usewindow && useglp)
		_throw("ERROR: Cannot render to a window if GLP mode is enabled.");
	if(argc<2) fprintf(stderr, "\n%s -h for advanced usage.\n", argv[0]);
	#ifdef USEGLP
	if(useglp) fprintf(stderr, "\nRendering to Pbuffer using GLP on device %s\n", device);
	#endif

	if(!useglp)
	{
		XSetErrorHandler(xhandler);
		if(!(dpy=XOpenDisplay(0))) {fprintf(stderr, "Could not open display %s\n", XDisplayName(0));  exit(1);}
		fprintf(stderr, "\nRendering to %s using GLX on display %s\n", usewindow?"window":"Pbuffer", DisplayString(dpy));

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
	}
	fprintf(stderr, "Drawable size = %d x %d pixels\n", WIDTH, HEIGHT);
	fprintf(stderr, "Using %d-byte row alignment\n\n", ALIGN);
	pbufferinit(dpy, win);
	display();
	return 0;

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}
}
