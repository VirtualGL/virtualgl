/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
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
#include <GL/glu.h>
#include <GL/wglext.h>

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

static int FORMATS=4
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
	{0, 0, 0, 1, GL_LUMINANCE, 0, "LUM"},
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

#define bench_name		"WGLreadtest"

#define _WIDTH            701
#define _HEIGHT           701

int WIDTH=_WIDTH, HEIGHT=_HEIGHT;
rrtimer timer;
int usewindow=0;

PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB=NULL;
PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB=NULL;
PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB=NULL;
PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB=NULL;

void loadsymbols(HDC hdc)
{
	HGLRC ctx=0;  HMODULE hmod=0;  int state=0;

	try {

	if(!wglGetCurrentContext() || !wglGetCurrentDC())
	{
		state=SaveDC(hdc);
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR), 1,
			PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0,
			PFD_MAIN_PLANE, 0, 0, 0, 0
		};
		int pixelformat=0;
		if(!(pixelformat=ChoosePixelFormat(hdc, &pfd))
			|| !SetPixelFormat(hdc, pixelformat, &pfd)
			|| !(ctx=wglCreateContext(hdc)) || !wglMakeCurrent(hdc, ctx))
				_throww32m("Could not create temporary OpenGL context");
	}

	if(!wglCreatePbufferARB)
	{
		wglCreatePbufferARB=(PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
		if(!wglCreatePbufferARB)
			_throw("Could not load symbol wglCreatePbufferARB");
	}
	if(!wglDestroyPbufferARB)
	{
		wglDestroyPbufferARB=(PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
		if(!wglDestroyPbufferARB)
			_throw("Could not load symbol wglDestroyPbufferARB");
	}
	if(!wglGetPbufferDCARB)
	{
		wglGetPbufferDCARB=(PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
		if(!wglGetPbufferDCARB)
			_throw("Could not load symbol wglGetPbufferDCARB");
	}
	if(!wglReleasePbufferDCARB)
	{
		wglReleasePbufferDCARB=(PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
		if(!wglReleasePbufferDCARB)
			_throw("Could not load symbol wglReleasePbufferDCARB");
	}

	wglDeleteContext(ctx);
	if(state!=0) RestoreDC(hdc, state);

	}	catch(...)
	{
		if(ctx) wglDeleteContext(ctx);
		if(state!=0) RestoreDC(hdc, state);
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
			if(pix[format].glformat==GL_LUMINANCE
				|| pix[format].glformat==GL_RED)
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

int cmpbuf(int x, int y, int w, int h, int format, unsigned char *buf, int bassackwards)
{
	int i, j, l, ps=pix[format].pixelsize;
	for(i=0; i<h; i++)
	{
		l=bassackwards?h-i-1:i;
		for(j=0; j<w; j++)
		{
			if(pix[format].glformat==GL_LUMINANCE
				|| pix[format].glformat==GL_RED)
			{
				if(buf[l*PAD(w*ps)+j*ps]!=((i+y)*(j+x))%256) return 0;
			}
			else
			{
//				if(buf[l*PAD(w*ps)+j*ps+pix[format].roffset]!=((i+y)*(j+x))%256) return 0;
//				if(buf[l*PAD(w*ps)+j*ps+pix[format].goffset]!=((i+y)*(j+x)*2)%256) return 0;
//				if(buf[l*PAD(w*ps)+j*ps+pix[format].boffset]!=((i+y)*(j+x)*3)%256) return 0;
				if(buf[l*PAD(w*ps)+j*ps+pix[format].roffset]!=((i+y)*(j+x))%256
				||
				buf[l*PAD(w*ps)+j*ps+pix[format].goffset]!=((i+y)*(j+x)*2)%256
				||
				buf[l*PAD(w*ps)+j*ps+pix[format].boffset]!=((i+y)*(j+x)*3)%256)
				fprintf(stderr, "%d,%d: %d,%d,%d should be %d,%d,%d\n",
					l, j, buf[l*PAD(w*ps)+j*ps+pix[format].roffset],
					buf[l*PAD(w*ps)+j*ps+pix[format].roffset],
					buf[l*PAD(w*ps)+j*ps+pix[format].boffset], ((i+y)*(j+x))%256,
					((i+y)*(j+x)*2)%256, ((i+y)*(j+x)*3)%256);
// return 0;
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

//////////////////////////////////////////////////////////////////////
// The actual tests
//////////////////////////////////////////////////////////////////////

// Generic GL write test
void glwrite(int format)
{
	unsigned char *rgbaBuffer=NULL;  int n, ps=pix[format].pixelsize;
	double rbtime;

	try {

	fprintf(stderr, "glDrawPixels():               ");
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
		glDrawPixels(WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE, rgbaBuffer);
		glFinish();
		n++;
	} while((rbtime=timer.elapsed())<1.0 || n<2);

	if(!check_errors("frame buffer write"))
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
	n=0;
	timer.start();
	do
	{
		if(pix[format].glformat==GL_LUMINANCE)
		{
			glPushAttrib(GL_PIXEL_MODE_BIT);
			glPixelTransferf(GL_RED_SCALE, (GLfloat)0.299);
			glPixelTransferf(GL_GREEN_SCALE, (GLfloat)0.587);
			glPixelTransferf(GL_BLUE_SCALE, (GLfloat)0.114);
		}
		glReadPixels(0, 0, WIDTH, HEIGHT, pix[format].glformat, GL_UNSIGNED_BYTE, rgbaBuffer);
		if(pix[format].glformat==GL_LUMINANCE)
		{
			glPopAttrib();
		}
		n++;
	} while((rbtime=timer.elapsed())<1.0 || n<2);
	if(!cmpbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer, 0))
		_throw("ERROR: Bogus data read back.");
	if(!check_errors("frame buffer read"))
		fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(WIDTH*HEIGHT)/((double)1000000.*rbtime));

	fprintf(stderr, "glReadPixels() [top-down]:    ");
	glPixelStorei(GL_UNPACK_ALIGNMENT, ALIGN);
	glPixelStorei(GL_PACK_ALIGNMENT, ALIGN); 
	glReadBuffer(GL_FRONT);
	memset(rgbaBuffer, 0, PAD(WIDTH*ps)*HEIGHT);
	n=0;
	timer.start();
	do
	{
		if(pix[format].glformat==GL_LUMINANCE)
		{
			glPushAttrib(GL_PIXEL_MODE_BIT);
			glPixelTransferf(GL_RED_SCALE, (GLfloat)0.299);
			glPixelTransferf(GL_GREEN_SCALE, (GLfloat)0.587);
			glPixelTransferf(GL_BLUE_SCALE, (GLfloat)0.114);
		}
		for (i=0; i<n; i++)
		{
			for(int j=0; j<HEIGHT; j++)
				glReadPixels(0, HEIGHT-j-1, WIDTH, 1, pix[format].glformat, GL_UNSIGNED_BYTE, &rgbaBuffer[PAD(WIDTH*ps)*j]);
		}
		if(pix[format].glformat==GL_LUMINANCE)
		{
			glPopAttrib();
		}
		n++;
	} while((rbtime=timer.elapsed())<1.0 || n<2);
	if(!cmpbuf(0, 0, WIDTH, HEIGHT, format, rgbaBuffer, 1))
		_throw("ERROR: Bogus data read back.");
	if(!check_errors("frame buffer read"))
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
		glread(format);
		fprintf(stderr, "\n");
	}

	exit(0);
}

void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [-h|-?] [-window]\n", argv[0]);
	fprintf(stderr, "       [-width <n>] [-height <n>] [-align <n>]\n");
	fprintf(stderr, "\n-h or -? = This screen\n");
	fprintf(stderr, "-window = Render to a window instead of a Pbuffer\n");
	fprintf(stderr, "-width = Set drawable width to n pixels (default: %d)\n", _WIDTH);
	fprintf(stderr, "-height = Set drawable height to n pixels (default: %d)\n", _HEIGHT);
	fprintf(stderr, "-align = Set row alignment to n bytes (default: %d)\n", ALIGN);
	fprintf(stderr, "\n");
	exit(0);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
		case WM_CREATE:
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	HWND hwnd=0;  int retval=0;
	WNDCLASSEX wndclass;
	int bw=GetSystemMetrics(SM_CXFIXEDFRAME)*2;
	int bh=GetSystemMetrics(SM_CYFIXEDFRAME)*2+GetSystemMetrics(SM_CYCAPTION);
	DWORD winstyle=WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	int pixelformat=0;
	HPBUFFERARB pbuffer=0;  HDC hdc=0, pbhdc=0;  HGLRC ctx=0;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0,
		PFD_MAIN_PLANE, 0, 0, 0, 0
	};


	try {

	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = GetModuleHandle(NULL);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "WGLreadtest";
	wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	if(!RegisterClassEx(&wndclass)) _throww32m("Cannot create window class");

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
	}

	if(argc<2) fprintf(stderr, "\n%s -h for advanced usage.\n", argv[0]);

	fprintf(stderr, "\nRendering to %s\n", usewindow?"window":"Pbuffer");

	if(GetSystemMetrics(SM_CXSCREEN)<WIDTH || GetSystemMetrics(SM_CYSCREEN)<HEIGHT)
	{
		fprintf(stderr, "ERROR: Please switch to a screen resolution of at least %d x %d.\n", WIDTH, HEIGHT);
		exit(1);
	}

	if(usewindow) winstyle|=WS_VISIBLE;

	if(!(hwnd=CreateWindowEx(0, "WGLreadtest", "WGLreadtest", winstyle, 0,  0,
		usewindow? WIDTH+bw:1, usewindow? HEIGHT+bh:1, NULL, NULL,
		GetModuleHandle(NULL), NULL)))
		_throww32m("Cannot create window");

	RECT rect;
	GetClientRect(hwnd, &rect);
	fprintf(stderr, "Drawable size = %d x %d pixels\n",
		usewindow? rect.right-rect.left:WIDTH,
		usewindow? rect.bottom-rect.top:HEIGHT);

	fprintf(stderr, "Using %d-byte row alignment\n", ALIGN);

	if(!(hdc=GetDC(hwnd)))
		_throww32m("Could not obtain window device context");
	if(usewindow) pfd.dwFlags|=PFD_DRAW_TO_WINDOW;
	if(!(pixelformat=ChoosePixelFormat(hdc, &pfd)))
		_throww32m("Could not obtain pixel format");

	fprintf(stderr, "Using pixel format %d\n", pixelformat);

	if(usewindow)
	{
		if(!SetPixelFormat(hdc, pixelformat, &pfd))
			_throww32m("Could not set pixel format");
		if(!(ctx=wglCreateContext(hdc)))
			_throww32m("Could not create OpenGL context");
		if(!wglMakeCurrent(hdc, ctx))
			_throww32m("Could not swap in OpenGL context");
	}
	else
	{
		loadsymbols(hdc);
		int pbattribs[]={0};
		if(!(pbuffer=wglCreatePbufferARB(hdc, pixelformat, WIDTH, HEIGHT,
			pbattribs))) _throww32m("Could not create Pbuffer");
		if(!(pbhdc=wglGetPbufferDCARB(pbuffer)))
			_throww32m("Could not obtain Pbuffer device context");
		if(!(ctx=wglCreateContext(pbhdc)))
			_throww32m("Could not create OpenGL context");
		if(!wglMakeCurrent(pbhdc, ctx))
			_throww32m("Could not swap in OpenGL context");
	}
	fprintf(stderr, "OpenGL Renderer: %s\n\n", glGetString(GL_RENDERER));

	display();

	} catch(rrerror &e)
	{
		fprintf(stderr, "%s\n", e.getMessage());  retval=-1;
	}

	if(ctx) wglDeleteContext(ctx);
	if(pbuffer && pbhdc) wglReleasePbufferDCARB(pbuffer, pbhdc);
	if(pbuffer) wglDestroyPbufferARB(pbuffer);
	if(hdc) ReleaseDC(hwnd, hdc);

	return retval;
}
