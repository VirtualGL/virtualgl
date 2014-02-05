/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2010-2011, 2013 D. R. Commander
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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "rrutil.h"
#include "rrtimer.h"
#include "rrerror.h"
#include <errno.h>
#define GL_GLEXT_PROTOTYPES
#include "../common/glx.h"
#ifdef MESAGLU
#include <mesa/glu.h>
#else
#include <GL/glu.h>
#endif
#include "x11err.h"
#ifdef _WIN32
#define snprintf _snprintf
#endif


static int ALIGN=1;
#define PAD(w) (((w)+(ALIGN-1))&(~(ALIGN-1)))
#define BMPPAD(pitch) ((pitch+(sizeof(int)-1))&(~(sizeof(int)-1)))


typedef struct _pixelformat
{
	unsigned long roffset, goffset, boffset;
	int pixelsize;
	int glformat;
	int bgr;
	const char *name;
} pixelformat;

static int FORMATS=3
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

pixelformat pix[4
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
	{0, 0, 0, 1, GL_RED, 0, "RED"},
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


#ifdef XDK
// Exceed likes to redefine stdio, so we un-redefine it :/
#undef fprintf
#undef printf
#undef putchar
#undef putc
#undef puts
#undef fputc
#undef fputs
#undef perror
#define GLX11
#endif
#ifndef GLX_DRAWABLE_TYPE
#define GLX_DRAWABLE_TYPE 0x8010
#endif
#ifndef GLX_PBUFFER_BIT
#define GLX_PBUFFER_BIT 0x00000004
#endif
#ifndef GLX_PBUFFER_HEIGHT
#define GLX_PBUFFER_HEIGHT 0x8040
#endif
#ifndef GLX_PBUFFER_WIDTH
#define GLX_PBUFFER_WIDTH 0x8041
#endif


#define bench_name		"GLreadtest"

#define _WIDTH            701
#define _HEIGHT           701

int WIDTH=_WIDTH, HEIGHT=_HEIGHT;
Display *dpy=NULL;  Window win=0;  Pixmap pm=0;  GLXPixmap glxpm=0;
XVisualInfo *v=NULL;  GLXFBConfig c=0;
GLXContext ctx=0;
#ifndef GLX11
GLXPbuffer pbuffer=0;
#endif

rrtimer timer;
int usewindow=0, usepixmap=0, visualid=0, loops=1,
	usealpha=0;
#ifdef GL_VERSION_1_5
int pbo=0;
#endif
double benchtime=1.0;

#define STRLEN 256


char *sigfig(int fig, char *string, double value)
{
	char format[80];
	double _l=(value==0.0)? 0.0:log10(fabs(value));  int l;
	if(_l<0.)
	{
		l=(int)fabs(floor(_l));
		snprintf(format, 79, "%%%d.%df", fig+l+1, fig+l-1);
	}
	else
	{
		l=(int)_l+1;
		if(fig<=l) snprintf(format, 79, "%%.0f");
		else snprintf(format, 79, "%%%d.%df", fig+1, fig-l);
	}
	snprintf(string, STRLEN-1, format, value);
	return(string);
}


extern "C" {
int xhandler(Display *dpy, XErrorEvent *xe)
{
	fprintf(stderr, "X11 Error: %s\n", x11error(xe->error_code));
	return 0;
}
} // extern "C"


void findvisual(void)
{
	int winattribs[]={GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, None, None, None};
	int winattribsdb[]={GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None, None, None};

	#ifndef GLX11
	int pbattribs[]={GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
		GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, None,
		None, None};
	int pbattribsdb[]={GLX_DOUBLEBUFFER, 1, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DRAWABLE_TYPE,
		GLX_PBUFFER_BIT, None, None, None};
	int pbattribsci[]={GLX_BUFFER_SIZE, 8, GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, None};
	int pbattribscidb[]={GLX_DOUBLEBUFFER, 1, GLX_BUFFER_SIZE, 8,
		GLX_RENDER_TYPE, GLX_COLOR_INDEX_BIT, GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		None};
	GLXFBConfig *fbconfigs=NULL;  int nelements=0;
	#endif

	// Use GLX 1.1 functions here in case we're remotely displaying to
	// something that doesn't support GLX 1.3
	if(usewindow || usepixmap)
	{
		try
		{
			if(visualid)
			{
				XVisualInfo vtemp;  int n=0;
				vtemp.visualid=visualid;
				v=XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n);
				if(!v || !n) _throw("Could not obtain visual");
				printf("Visual = 0x%.2x\n", (unsigned int)v->visualid);
				return;
			}
			if(usealpha)
			{
				winattribs[7]=GLX_ALPHA_SIZE;  winattribs[8]=8;
				winattribsdb[8]=GLX_ALPHA_SIZE;  winattribsdb[9]=8;
			}
			if(!(v=glXChooseVisual(dpy, DefaultScreen(dpy), winattribs))
				&& !(v=glXChooseVisual(dpy, DefaultScreen(dpy), winattribsdb)))
				_throw("Could not obtain Visual");
			printf("Visual = 0x%.2x\n", (unsigned int)v->visualid);
			return;
		}
		catch(...)
		{
			if(v) {XFree(v);  v=NULL;}  throw;
		}
	}

	#ifndef GLX11
	if(usealpha)
	{
		pbattribs[10]=GLX_ALPHA_SIZE;  pbattribs[11]=8;
		pbattribsdb[12]=GLX_ALPHA_SIZE;  pbattribsdb[13]=8;
	}
	fbconfigs=glXChooseFBConfig(dpy, DefaultScreen(dpy), pbattribs, &nelements);
	if(!fbconfigs) fbconfigs=glXChooseFBConfig(dpy, DefaultScreen(dpy),
		pbattribsdb, &nelements);
	if(!nelements || !fbconfigs)
	{
		if(fbconfigs) XFree(fbconfigs);
		_throw("Could not obtain Visual");
	}
	c=fbconfigs[0];  XFree(fbconfigs);

	int fbcid=-1;
	glXGetFBConfigAttrib(dpy, c, GLX_FBCONFIG_ID, &fbcid);
	printf("FB Config = 0x%.2x\n", fbcid);
	#endif
}


void pbufferinit(void)
{
	#ifndef	GLX11
	int pbattribs[]={GLX_PBUFFER_WIDTH, 0, GLX_PBUFFER_HEIGHT, 0, None};
	#endif

	// Use GLX 1.1 functions here in case we're remotely displaying to
	// something that doesn't support GLX 1.3
	if(usewindow || usepixmap)
	{
		if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
			_throw("Could not create GL context");

		if(usepixmap)
		{
			if(!(glxpm=glXCreateGLXPixmap(dpy, v, pm)))
				_throw("Could not creaate GLX pixmap");
		}
		glXMakeCurrent(dpy, usepixmap? glxpm:win, ctx);
		return;
	}

	#ifndef GLX11
	ctx=glXCreateNewContext(dpy, c, GLX_RGBA_TYPE, NULL, True);
	if(!ctx)	_throw("Could not create GL context");

	pbattribs[1]=WIDTH;  pbattribs[3]=HEIGHT;
	pbuffer=glXCreatePbuffer(dpy, c, pbattribs);
	if(!pbuffer) _throw("Could not create Pbuffer");

	glXMakeContextCurrent(dpy, pbuffer, pbuffer, ctx);
	#endif
}


char glerrstr[STRLEN]="No error";

static void check_errors(const char * tag)
{
	int i, error=0;  char *s;
	i=glGetError();
	if(i!=GL_NO_ERROR) error=1;
	while(i!=GL_NO_ERROR)
	{
		s=(char *)gluErrorString(i);
		if(s) snprintf(glerrstr, STRLEN-1, "OpenGL ERROR in %s: %s\n", tag, s);
		else snprintf(glerrstr, STRLEN-1, "OpenGL ERROR #%d in %s\n", i, tag);
		i=glGetError();
	}
	if(error) _throw(glerrstr);
}


void initbuf(int x, int y, int w, int h, int format, unsigned char *buf)
{
	int i, j, ps=pix[format].pixelsize;
	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			if(pix[format].glformat==GL_RED)
				buf[(i*w+j)*ps]=((i+y)*(j+x))%256;
			else
			{
				buf[(i*w+j)*ps+pix[format].roffset]=((i+y)*(j+x))%256;
				buf[(i*w+j)*ps+pix[format].goffset]=((i+y)*(j+x)*2)%256;
				buf[(i*w+j)*ps+pix[format].boffset]=((i+y)*(j+x)*3)%256;
			}
		}
	}
}


int cmpbuf(int x, int y, int w, int h, int format, unsigned char *buf,
	int bassackwards)
{
	int i, j, l, ps=pix[format].pixelsize;
	for(i=0; i<h; i++)
	{
		l=bassackwards?h-i-1:i;
		for(j=0; j<w; j++)
		{
			if(pix[format].glformat==GL_RED)
			{
				if(buf[l*PAD(w*ps)+j*ps]!=((i+y)*(j+x))%256) return 0;
			}
			else
			{
				if(buf[l*PAD(w*ps)+j*ps+pix[format].roffset]!=((i+y)*(j+x))%256)
					return 0;
				if(buf[l*PAD(w*ps)+j*ps+pix[format].goffset]!=((i+y)*(j+x)*2)%256)
					return 0;
				if(buf[l*PAD(w*ps)+j*ps+pix[format].boffset]!=((i+y)*(j+x)*3)%256)
					return 0;
			}
		}
	}
	return 1;
}


// Makes sure the frame buffer has been cleared prior to a write
void clearfb(int format)
{
	unsigned char *buf=NULL;  int ps=3, glformat=GL_RGB;
	if((buf=(unsigned char *)malloc(WIDTH*HEIGHT*ps))==NULL)
		_throw("Could not allocate buffer");
	memset(buf, 0xFF, WIDTH*HEIGHT*ps);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);
	glClearColor(0., 0., 0., 0.);
	glClear(GL_COLOR_BUFFER_BIT);
	glReadPixels(0, 0, WIDTH, HEIGHT, glformat, GL_UNSIGNED_BYTE, buf);
	check_errors("frame buffer read");
	for(int i=0; i<WIDTH*HEIGHT*ps; i++)
	{
		if(buf[i]!=0) {fprintf(stderr, "Buffer was not cleared\n");  break;}
	}
	if(buf) free(buf);
}


// Generic GL write test
void glwrite(int format)
{
	unsigned char *rgbaBuffer=NULL;  int n, ps=pix[format].pixelsize;
	double rbtime;
	char temps[STRLEN];

	try {

	fprintf(stderr, "glDrawPixels():   ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); 
	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	clearfb(format);
	if((rgbaBuffer=(unsigned char *)malloc(WIDTH*HEIGHT*ps))==NULL)
		_throw("Could not allocate buffer");
	initbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer);
	n=0;
	timer.start();
	do
	{
		glDrawPixels(WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE,
			rgbaBuffer);
		glFinish();
		n++;
	} while((rbtime=timer.elapsed())<benchtime || n<2);

	double avgmps=(double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime);
	check_errors("frame buffer write");
	fprintf(stderr, "%s Mpixels/sec\n", sigfig(4, temps, avgmps));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	if(rgbaBuffer) free(rgbaBuffer);
}


// Generic OpenGL readback test
void glread(int format)
{
	unsigned char *rgbaBuffer=NULL;  int n, ps=pix[format].pixelsize;
	double rbtime, readpixelstime;  rrtimer timer2;
	#ifdef GL_VERSION_1_5
	GLuint bufferid=0;
	#endif
	char temps[STRLEN];

	try {

	fprintf(stderr, "glReadPixels():   ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, ALIGN);
	glPixelStorei(GL_PACK_ALIGNMENT, ALIGN);
	glReadBuffer(GL_FRONT);
	#ifdef GL_VERSION_1_5
	if(pbo)
	{
		const char *ext=(const char *)glGetString(GL_EXTENSIONS);
		if(!ext || !strstr(ext, "GL_ARB_pixel_buffer_object"))
			_throw("GL_ARB_pixel_buffer_object extension not available");
		glGenBuffers(1, &bufferid);
		if(!bufferid) _throw("Could not generate PBO buffer");
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bufferid);
		glBufferData(GL_PIXEL_PACK_BUFFER_EXT, PAD(WIDTH*ps)*HEIGHT, NULL,
			GL_STREAM_READ);
		check_errors("PBO initialization");
		int temp=0;
		glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER_EXT, GL_BUFFER_SIZE,
			&temp);
		if(temp!=PAD(WIDTH*ps)*HEIGHT) _throw("Could not generate PBO buffer");
		temp=0;
		glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING_EXT, &temp);
		if(temp!=pbo) _throw("Could not bind PBO buffer");
	}
	#endif
	if((rgbaBuffer=(unsigned char *)malloc(PAD(WIDTH*ps)*HEIGHT))==NULL)
		_throw("Could not allocate buffer");
	memset(rgbaBuffer, 0, PAD(WIDTH*ps)*HEIGHT);
	n=0;  rbtime=readpixelstime=0.;
	double tmin=0., tmax=0., ssq=0., sum=0.;  int first=1;
	do
	{
		timer.start();
		if(pix[format].glformat==GL_LUMINANCE)
		{
			glPushAttrib(GL_PIXEL_MODE_BIT);
			glPixelTransferf(GL_RED_SCALE, (GLfloat)0.299);
			glPixelTransferf(GL_GREEN_SCALE, (GLfloat)0.587);
			glPixelTransferf(GL_BLUE_SCALE, (GLfloat)0.114);
		}
		#ifdef GL_VERSION_1_5
		if(pbo)
		{
			unsigned char *pixels=NULL;
			glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, bufferid);
			timer2.start();
			glReadPixels(0, 0, WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE,
				NULL);
			readpixelstime+=timer2.elapsed();
			pixels=(unsigned char *)glMapBuffer(GL_PIXEL_PACK_BUFFER_EXT,
				GL_READ_ONLY);
			if(!pixels) _throw("Could not map buffer");
			memcpy(rgbaBuffer, pixels, PAD(WIDTH*ps)*HEIGHT);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT);
		}
		else
		#endif
		{
			timer2.start();
			glReadPixels(0, 0, WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE,
				rgbaBuffer);
			readpixelstime+=timer2.elapsed();
		}

		if(pix[format].glformat==GL_LUMINANCE)
		{
			glPopAttrib();
		}
		double elapsed=timer.elapsed();
		if(first) {tmin=tmax=elapsed;  first=0;}
		else
		{
			if(elapsed<tmin) tmin=elapsed;
			if(elapsed>tmax) tmax=elapsed;
		}		
		n++;
		rbtime+=elapsed;
		ssq+=pow((double)(WIDTH*HEIGHT)/((double)1000000.*elapsed), 2.0);
		sum+=(double)(WIDTH*HEIGHT)/((double)1000000.*elapsed);
	} while(rbtime<benchtime || n<2);
	if(!cmpbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer, 0))
		_throw("ERROR: Bogus data read back.");
	double mean=sum/(double)n;
	double stddev=sqrt((ssq - 2.0*mean*sum + mean*mean*(double)n)/(double)n);
	double avgmps=(double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime);
	double minmps=(double)(WIDTH*HEIGHT)/((double)1000000.*tmax);
	double maxmps=(double)(WIDTH*HEIGHT)/((double)1000000.*tmin);
	check_errors("frame buffer read");
	fprintf(stderr, "%s Mpixels/sec ", sigfig(4, temps, avgmps));
	fprintf(stderr, "(min = %s, ", sigfig(4, temps, minmps));
	fprintf(stderr, "max = %s, ", sigfig(4, temps, maxmps));
	fprintf(stderr, "sdev = %s)\n", sigfig(4, temps, stddev));
	fprintf(stderr, "glReadPixels() accounted for %s%% of total readback time\n",
		sigfig(4, temps, readpixelstime/rbtime*100.0));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	if(rgbaBuffer) free(rgbaBuffer);
	#ifdef GL_VERSION_1_5
	if(pbo && bufferid>0)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
		glDeleteBuffers(1, &bufferid);
	}
	#endif
}


void display(void)
{
	int format;

	for(format=0; format<FORMATS; format++)
	{
		fprintf(stderr, ">>>>>>>>>>  PIXEL FORMAT:  %s  <<<<<<<<<<\n",
			pix[format].name);

		#if defined(GL_ABGR_EXT) || defined(GL_BGRA_EXT) || defined(GL_BGR_EXT)
		const char *ext=(const char *)glGetString(GL_EXTENSIONS), *compext=NULL;
		#ifdef GL_ABGR_EXT
		if(pix[format].glformat==GL_ABGR_EXT) compext="GL_EXT_abgr";
		#endif
		#ifdef GL_BGRA_EXT
		if(pix[format].glformat==GL_BGRA_EXT) compext="GL_EXT_bgra";
		#endif
		#ifdef GL_BGR_EXT
		if(pix[format].glformat==GL_BGR_EXT) compext="GL_EXT_bgra";
		#endif
		if(compext && (!ext || !strstr(ext, compext)))
		{
			fprintf(stderr, "%s extension not available.  Skipping ...\n\n",
				compext);
			continue;
		}
		#endif

		glwrite(format);
		for(int i=0; i<loops; i++) glread(format);
		fprintf(stderr, "\n");
	}

	exit(0);
}

void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [-h|-?] [-window] [-pm]", argv[0]);
	#ifdef GL_VERSION_1_5
	fprintf(stderr, " [-pbo]\n");
	#else
	fprintf(stderr, "\n");
	#endif
	fprintf(stderr, "       [-width <n>] [-height <n>] [-align <n>] [-visualid <xx>] [-alpha]\n");
	fprintf(stderr, "       [-rgb] [-rgba] [-bgr] [-bgra] [-abgr] [-time <t>] [-loop <l>]");
	fprintf(stderr, "\n-h or -? = This screen\n");
	fprintf(stderr, "-window = Render to a window instead of a Pbuffer\n");
	fprintf(stderr, "-pm = Render to a pixmap instead of a Pbuffer\n");
	#ifdef GL_VERSION_1_5
	fprintf(stderr, "-pbo = Use pixel buffer objects to perform readback\n");
	#endif
	fprintf(stderr, "-width = Set drawable width to n pixels (default: %d)\n", _WIDTH);
	fprintf(stderr, "-height = Set drawable height to n pixels (default: %d)\n", _HEIGHT);
	fprintf(stderr, "-align = Set row alignment to n bytes (default: %d)\n", ALIGN);
	fprintf(stderr, "-visualid = Ignore visual selection and use this visual ID (hex) instead\n");
	fprintf(stderr, "-alpha = Create Pbuffer/window using 32-bit instead of 24-bit visual\n");
	fprintf(stderr, "-rgb = Test only RGB pixel format\n");
	fprintf(stderr, "-rgba = Test only RGBA pixel format\n");
	fprintf(stderr, "-bgr = Test only BGR pixel format\n");
	fprintf(stderr, "-bgra = Test only BGRA pixel format\n");
	fprintf(stderr, "-abgr = Test only ABGR pixel format\n");
	fprintf(stderr, "-time <t> = Run each test for <t> seconds\n");
	fprintf(stderr, "-loop <l> = Run readback test <l> times in a row\n");
	fprintf(stderr, "\n");
	exit(0);
}


int main(int argc, char **argv)
{
	fprintf(stderr, "\n%s v%s (Build %s)\n", bench_name, __VERSION, __BUILD);

	#ifdef GLX11
	usewindow=1;
	#endif

	for(int i=0; i<argc; i++)
	{
		if(!stricmp(argv[i], "-h")) usage(argv);
		if(!stricmp(argv[i], "-?")) usage(argv);
		if(!stricmp(argv[i], "-window")) usewindow=1;
		if(!stricmp(argv[i], "-pm")) usepixmap=1;
		#ifdef GL_VERSION_1_5
		if(!stricmp(argv[i], "-pbo")) pbo=1;
		#endif
		if(!stricmp(argv[i], "-alpha")) usealpha=1;
		if(!stricmp(argv[i], "-rgb"))
		{
			pixelformat pixtemp={0, 1, 2, 3, GL_RGB, 0, "RGB"};
			pix[0]=pixtemp;
			FORMATS=1;
		}
		if(!stricmp(argv[i], "-rgba"))
		{
			pixelformat pixtemp={0, 1, 2, 4, GL_RGBA, 0, "RGBA"};
			pix[0]=pixtemp;
			FORMATS=1;
		}
		#ifdef GL_BGR_EXT
		if(!stricmp(argv[i], "-bgr"))
		{
			pixelformat pixtemp={2, 1, 0, 3, GL_BGR_EXT, 1, "BGR"};
			pix[0]=pixtemp;
			FORMATS=1;
		}
		#endif
		#ifdef GL_BGRA_EXT
		if(!stricmp(argv[i], "-bgra"))
		{
			pixelformat pixtemp={2, 1, 0, 4, GL_BGRA_EXT, 1, "BGRA"};
			pix[0]=pixtemp;
			FORMATS=1;
		}
		#endif
		#ifdef GL_ABGR_EXT
		if(!stricmp(argv[i], "-abgr"))
		{
			pixelformat pixtemp={3, 2, 1, 4, GL_ABGR_EXT, 0, "ABGR"};
			pix[0]=pixtemp;
			FORMATS=1;
		}
		#endif
		if(!stricmp(argv[i], "-loop") && i<argc-1)
		{
			int temp=atoi(argv[i+1]);  i++;
			if(temp>1) loops=temp;
		}
		if(!stricmp(argv[i], "-align") && i<argc-1)
		{
			int temp=atoi(argv[i+1]);  i++;
			if(temp>=1 && (temp&(temp-1))==0) ALIGN=temp;
		}
		if(!stricmp(argv[i], "-visualid") && i<argc-1)
		{
			int temp=0;
			sscanf(argv[i+1], "%x", &temp);
			if(temp>0) visualid=temp;  i++;
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
		if(!stricmp(argv[i], "-time") && i<argc-1)
		{
			double temp=atof(argv[i+1]);  i++;
			if(temp>0.0) benchtime=temp;
		}
	}

	if(argc<2) fprintf(stderr, "\n%s -h for advanced usage.\n", argv[0]);

	try {

		XSetErrorHandler(xhandler);
		if(!(dpy=XOpenDisplay(0)))
		{
			fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
			exit(1);
		}
		#ifdef GL_VERSION_1_5
		if(pbo) fprintf(stderr, "Using PBO's for readback\n");
		#endif

		if((DisplayWidth(dpy, DefaultScreen(dpy))<WIDTH ||
			DisplayHeight(dpy, DefaultScreen(dpy))<HEIGHT) && usewindow)
		{
			fprintf(stderr,
				"ERROR: Please switch to a screen resolution of at least %d x %d.\n",
				WIDTH, HEIGHT);
			exit(1);
		}

		findvisual();

		if(usewindow || usepixmap)
		{
			XSetWindowAttributes swa;
			Window root=DefaultRootWindow(dpy);
			swa.border_pixel=0;
			swa.event_mask=0;

			swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);
			errifnot(win=XCreateWindow(dpy, root, 0, 0, usepixmap? 1:WIDTH,
				usepixmap? 1:HEIGHT, 0, v->depth, InputOutput, v->visual,
				CWBorderPixel|CWColormap|CWEventMask, &swa));
			if(usepixmap)
			{
				errifnot(pm=XCreatePixmap(dpy, win, WIDTH, HEIGHT, v->depth));
			}
			else XMapWindow(dpy, win);
			XSync(dpy, False);
		}
		fprintf(stderr, "%s size = %d x %d pixels\n",
			usepixmap? "Pixmap" : usewindow? "Window" : "Pbuffer", WIDTH, HEIGHT);
		fprintf(stderr, "Using %d-byte row alignment\n\n", ALIGN);

		pbufferinit();
		display();
		return 0;

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	if(dpy)
	{
		glXMakeCurrent(dpy, 0, 0);
		if(ctx) glXDestroyContext(dpy, ctx);
		#ifndef GLX11
		if(pbuffer) glXDestroyPbuffer(dpy, pbuffer);
		#endif
		if(glxpm) glXDestroyGLXPixmap(dpy, glxpm);
		if(pm) XFreePixmap(dpy, pm);
		if(win) XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	if(v) XFree(v);
}
