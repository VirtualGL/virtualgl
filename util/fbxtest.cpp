/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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

#ifdef XDK  // For some reason, -UWIN32 from the command line doesn't work with C++ files
 #undef WIN32
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "rrthread.h"
#include "rrutil.h"
#include "rrtimer.h"
#include "fbx.h"
#ifndef WIN32
 #include <errno.h>
 #include "x11err.h"
#endif


//////////////////////////////////////////////////////////////////////
// Error handling
//////////////////////////////////////////////////////////////////////

#ifndef WIN32
int xhandler(Display *dpy, XErrorEvent *xe)
{
	fprintf(stderr, "X11 Error: %s\n", x11error(xe->error_code));
	return 0;
}
#endif


//////////////////////////////////////////////////////////////////////
// Structs and globals
//////////////////////////////////////////////////////////////////////

#define bench_name		"FBXtest"

#define MIN_SCREEN_WIDTH  1024
#define MIN_SCREEN_HEIGHT 768
#define WIDTH             701
#define HEIGHT            701
#define N                 2

int width, height;
int checkdb=0, doshm=1;
fbx_wh wh;
rrtimer timer;
#ifdef WIN32
#define fg() SetForegroundWindow(wh)
#else
#define fg()
#endif

void nativeread(int), nativewrite(int);

//////////////////////////////////////////////////////////////////////
// Buffer initialization and checking
//////////////////////////////////////////////////////////////////////

void initbuf(int x, int y, int w, int pitch, int h, int format, unsigned char *buf)
{
	int i, j, ps=fbx_ps[format];
	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			memset(&buf[i*pitch+j*ps], 0, fbx_ps[format]);
			if(format==FBX_INDEX)
				buf[i*pitch+j]=(j+x+i+y)%32;
			else
			{
				buf[i*pitch+j*ps+fbx_roffset[format]]=255;
				buf[i*pitch+j*ps+fbx_goffset[format]]=(j+x)%256;
				buf[i*pitch+j*ps+fbx_boffset[format]]=(i+y)%256;
			}
		}
	}
}

int cmpbuf(int x, int y, int w, int pitch, int h, int format, unsigned char *buf)
{
	int i, j, ps=fbx_ps[format];
	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			if(format==FBX_INDEX)
			{
				if(buf[i*pitch+j]!=(j+x+i+y)%32) return 0;
			}
			else
			{
				if(buf[i*pitch+j*ps+fbx_roffset[format]]!=255) return 0;
				if(buf[i*pitch+j*ps+fbx_goffset[format]]!=(j+x)%256) return 0;
				if(buf[i*pitch+j*ps+fbx_boffset[format]]!=(i+y)%256) return 0;
			}
		}
	}
	return 1;
}

// Makes sure the frame buffer has been cleared prior to a write
void clearfb(void)
{
	#ifdef WIN32
	if(wh)
	{
		HDC hdc=0;  RECT rect;
		tryw32(hdc=GetDC(wh));
		tryw32(GetClientRect(wh, &rect));
		tryw32(PatBlt(hdc, 0, 0, rect.right, rect.bottom, BLACKNESS));
		tryw32(ReleaseDC(wh, hdc));
	}
	#else
	if(wh.dpy && wh.win)
		XSetWindowBackground(wh.dpy, wh.win, BlackPixel(wh.dpy, DefaultScreen(wh.dpy)));
	#endif
	return;
}

//////////////////////////////////////////////////////////////////////
// The actual tests
//////////////////////////////////////////////////////////////////////

// Platform-specific write test
void nativewrite(int useshm)
{
	fbx_struct s;  int n, i;  double rbtime;
	memset(&s, 0, sizeof(s));

	try {

	fbx(fbx_init(&s, wh, 0, 0, useshm));
	if(useshm && !s.shm) _throw("MIT-SHM not available");
	fprintf(stderr, "Native Pixel Format:  %s", fbx_formatname(s.format));
	fprintf(stderr, "\n");
	if(s.width!=width || s.height!=height)
		_throw("The benchmark window lost input focus or was obscured.\nSkipping native write test\n");

	clearfb();
	initbuf(0, 0, width, s.pitch, height, s.format, (unsigned char *)s.bits);
	if(useshm)
		fprintf(stderr, "FBX bottom-up write [SHM]:        ");
	else
		fprintf(stderr, "FBX bottom-up write:              ");
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			if(checkdb)
			{
				memset(s.bits, 255, s.pitch*s.height);
				for(int j=0; j<height; j++)
					fbx(fbx_awrite(&s, 0, j, 0, height-j-1, width, 1));
				initbuf(0, 0, width, s.pitch, height, s.format, (unsigned char *)s.bits);
			}
			for(int j=0; j<height; j++)
				fbx(fbx_awrite(&s, 0, j, 0, height-j-1, width, 1));
			fbx_sync(&s);
		}
		rbtime=timer.elapsed();
	} while(rbtime<1.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	clearfb();
	if(useshm)
		fprintf(stderr, "FBX top-down write [SHM]:         ");
	else
		fprintf(stderr, "FBX top-down write:               ");
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			if(checkdb)
			{
				memset(s.bits, 255, s.pitch*s.height);
				fbx(fbx_awrite(&s, 0, 0, 0, 0, 0, 0));
				initbuf(0, 0, width, s.pitch, height, s.format, (unsigned char *)s.bits);
			}
			fbx(fbx_write(&s, 0, 0, 0, 0, 0, 0));
		}
		rbtime=timer.elapsed();
	} while(rbtime<1.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	fbx_term(&s);
}

// Platform-specific readback test
void nativeread(int useshm)
{
	fbx_struct s;  int n, i;  double rbtime;
	memset(&s, 0, sizeof(s));

	try {

	fbx(fbx_init(&s, wh, 0, 0, useshm));
	int ps=fbx_ps[s.format];
	if(useshm && !s.shm) _throw("MIT-SHM not available");
	if(s.width!=width || s.height!=height)
		_throw("The benchmark window lost input focus or was obscured.\nSkipping native read test\n");
	if(useshm)
		fprintf(stderr, "FBX read [SHM]:                   ");
	else
		fprintf(stderr, "FBX read:                         ");
	memset(s.bits, 0, width*height*ps);
	n=N;
	do
	{
		n+=n;
		timer.start();
		for(i=0; i<n; i++)
		{
			fbx(fbx_read(&s, 0, 0));
		}
		rbtime=timer.elapsed();
		if(!cmpbuf(0, 0, width, s.pitch, height, s.format, (unsigned char *)s.bits))
			_throw("ERROR: Bogus data read back.");
	} while (rbtime<1.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	fbx_term(&s);
}

// This serves as a unit test for the FBX library
class writethread : public Runnable
{
	public:
		writethread(int _myrank, int _iter, int _useshm) : myrank(_myrank),
			iter(_iter), useshm(_useshm) {}

		void run(void)
		{
			int i;  fbx_struct stressfb;
			memset(&stressfb, 0, sizeof(stressfb));

			try {

			int mywidth, myheight, myx=0, myy=0;
			if(myrank<2) {mywidth=width/2;  myx=0;}
			else {mywidth=width-width/2;  myx=width/2;}
			if(myrank%2==0) {myheight=height/2;  myy=0;}
			else {myheight=height-height/2;  myy=height/2;}
			fbx(fbx_init(&stressfb, wh, mywidth, myheight, useshm));
			if(useshm && !stressfb.shm) _throw("MIT-SHM not available");
			initbuf(myx, myy, mywidth, stressfb.pitch, myheight, stressfb.format, (unsigned char *)stressfb.bits);
			for (i=0; i<iter; i++)
				fbx(fbx_write(&stressfb, 0, 0, myx, myy, mywidth, myheight));

			} catch(...) {fbx_term(&stressfb);  throw;}
		}

	private:
		int myrank, iter, useshm;
};

fbx_struct stressfb0;

class writethread1 : public Runnable
{
	public:
		writethread1(int _myrank, int _iter, int _useshm) : myrank(_myrank),
			iter(_iter), useshm(_useshm) {}

		void run(void)
		{
			int i, mywidth, myheight, myx=0, myy=0;
			if(myrank<2) {mywidth=width/2;  myx=0;}
			else {mywidth=width-width/2;  myx=width/2;}
			if(myrank%2==0) {myheight=height/2;  myy=0;}
			else {myheight=height-height/2;  myy=height/2;}
			for (i=0; i<iter; i++)
				fbx(fbx_write(&stressfb0, myx, myy, myx, myy, mywidth, myheight));
		}

	private:
		int myrank, iter, useshm;
};

class readthread : public Runnable
{
	public:
		readthread(int _myrank, int _iter, int _useshm) : myrank(_myrank),
			iter(_iter), useshm(_useshm) {}

		void run(void)
		{
			fbx_struct stressfb;
			memset(&stressfb, 0, sizeof(stressfb));

			try {

			int i, mywidth, myheight, myx=0, myy=0;
			if(myrank<2) {mywidth=width/2;  myx=0;}
			else {mywidth=width-width/2;  myx=width/2;}
			if(myrank%2==0) {myheight=height/2;  myy=0;}
			else {myheight=height-height/2;  myy=height/2;}
			fbx(fbx_init(&stressfb, wh, mywidth, myheight, useshm));
			if(useshm && !stressfb.shm) _throw("MIT-SHM not available");
			int ps=fbx_ps[stressfb.format];
			memset(stressfb.bits, 0, mywidth*myheight*ps);
			for(i=0; i<iter; i++)
				fbx(fbx_read(&stressfb, myx, myy));
			if(!cmpbuf(myx, myy, mywidth, stressfb.pitch, myheight, stressfb.format, (unsigned char *)stressfb.bits))
				_throw("ERROR: Bogus data read back.");

			} catch(...) {fbx_term(&stressfb);  throw;}
		}

	private:
		int myrank, iter, useshm;
};

void nativestress(int useshm)
{
	int i, n;  double rbtime;
	Thread *t[4];

	memset(&stressfb0, 0, sizeof(stressfb0));

	try {

	clearfb();
	if(useshm)
		fprintf(stderr, "FBX write [4 bmps, 4 blits SHM]:  ");
	else
		fprintf(stderr, "FBX write [4 bmps, 4 blits]:      ");
	n=N;
	do
	{
		n+=n;
		timer.start();
		writethread *wt[4];
		for(i=0; i<4; i++)
		{
			wt[i]=new writethread(i, n, useshm);
			t[i]=new Thread(wt[i]);
			t[i]->start();
		}
		for(i=0; i<4; i++) t[i]->stop();
		for(i=0; i<4; i++) t[i]->checkerror();
		for(i=0; i<4; i++) {delete t[i];  delete wt[i];}
		rbtime=timer.elapsed();
	} while (rbtime<1.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	try {

	if(useshm)
		fprintf(stderr, "FBX read [4 bmps, 4 blits SHM]:   ");
	else
		fprintf(stderr, "FBX read [4 bmps, 4 blits]:       ");
	n=N;
	do
	{
		n+=n;
		timer.start();
		readthread *rt[4];
		for(i=0; i<4; i++)
		{
			rt[i]=new readthread(i, n, useshm);
			t[i]=new Thread(rt[i]);
			t[i]->start();
		}
		for(i=0; i<4; i++) t[i]->stop();
		for(i=0; i<4; i++) t[i]->checkerror();
		for(i=0; i<4; i++) {delete t[i];  delete rt[i];}
		rbtime=timer.elapsed();
	} while (rbtime<1.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	try {

	clearfb();
	if(useshm)
		fprintf(stderr, "FBX write [1 bmp, 4 blits SHM]:   ");
	else
		fprintf(stderr, "FBX write [1 bmp, 4 blits]:       ");
	memset(&stressfb0, 0, sizeof(stressfb0));
	fbx(fbx_init(&stressfb0, wh, width, height, useshm));
	if(useshm && !stressfb0.shm) _throw("MIT-SHM not supported");
	initbuf(0, 0, width, stressfb0.pitch, height, stressfb0.format, (unsigned char *)stressfb0.bits);
	n=N;
	do
	{
		n+=n;
		timer.start();
		writethread1 *wt1[4];
		for(i=0; i<4; i++)
		{
			wt1[i]=new writethread1(i, n, useshm);
			t[i]=new Thread(wt1[i]);
			t[i]->start();
		}
		for(i=0; i<4; i++) t[i]->stop();
		for(i=0; i<4; i++) t[i]->checkerror();
		for(i=0; i<4; i++) {delete t[i];  delete wt1[i];}
		rbtime=timer.elapsed();
	} while (rbtime<1.);
	fbx_term(&stressfb0);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	fg();  nativeread(useshm);

	fbx_term(&stressfb0);
	return;
}

void display(void)
{
	fprintf(stderr, "-- Performance tests --\n");
	#ifndef WIN32
	if(doshm)
	{
		fg();  nativewrite(1);
		fg();  nativeread(1);
	}
	#endif
	fg();  nativewrite(0);
	fg();  nativeread(0);
	fprintf(stderr, "\n");

	fprintf(stderr, "-- Stress tests --\n");
	#ifndef WIN32
	if(doshm)
	{
		fg();  nativestress(1);
	}
	#endif
	fg();  nativestress(0);
	fprintf(stderr, "\n");
}

#ifdef WIN32
LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
		case WM_CREATE:
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_PAINT:
			display();
			PostQuitMessage(0);
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
#endif


//////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	int i;

	fprintf(stderr, "\n%s v%s (Build %s)\n\n", bench_name, __VERSION, __BUILD);

	if(argc>1) for(i=1; i<argc; i++)
	{
		if(!stricmp(argv[i], "-checkdb"))
		{
			checkdb=1;
			fprintf(stderr, "Checking double buffering.  Watch for flashing to indicate that it is\n");
			fprintf(stderr, "not enabled.  Performance will be sub-optimal.\n");
		}
		if(!stricmp(argv[i], "-noshm"))
		{
			doshm=0;
		}
		if(!stricmp(argv[i], "-verbose"))
		{
			fbx_printwarnings(stderr);
		}
	}

	try {

	#ifdef WIN32

	WNDCLASSEX wndclass;  MSG msg;
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
	wndclass.lpszClassName = bench_name;
	wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	tryw32(RegisterClassEx(&wndclass));
	width=GetSystemMetrics(SM_CXSCREEN);
	height=GetSystemMetrics(SM_CYSCREEN);

	#else

	if(!XInitThreads()) {fprintf(stderr, "ERROR: Could not initialize Xlib thread safety\n");  exit(1);}
	XSetErrorHandler(xhandler);
	if(!(wh.dpy=XOpenDisplay(0))) {fprintf(stderr, "Could not open display %s\n", XDisplayName(0));  exit(1);}
	width=DisplayWidth(wh.dpy, DefaultScreen(wh.dpy));
	height=DisplayHeight(wh.dpy, DefaultScreen(wh.dpy));

	#endif

	if(width<MIN_SCREEN_WIDTH && height<MIN_SCREEN_HEIGHT)
	{
		fprintf(stderr, "ERROR: Please switch to a screen resolution of at least %d x %d.\n", MIN_SCREEN_WIDTH, MIN_SCREEN_HEIGHT);
		exit(1);
	}
	width=WIDTH;
	height=HEIGHT;

	#ifdef WIN32

	int bw=GetSystemMetrics(SM_CXFIXEDFRAME)*2;
	int bh=GetSystemMetrics(SM_CYFIXEDFRAME)*2+GetSystemMetrics(SM_CYCAPTION);
	tryw32(wh=CreateWindowEx(0, bench_name, bench_name, WS_OVERLAPPED |
		WS_SYSMENU | WS_CAPTION | WS_VISIBLE, 0,  0, width+bw, height+bh, NULL,
		NULL, GetModuleHandle(NULL), NULL));
	UpdateWindow(wh);
	BOOL ret;
	while(1)
	{
		if((ret=GetMessage(&msg, NULL, 0, 0))==-1) _throww32();
		else if(ret==0) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;

	#else

	XVisualInfo vtemp, *v=NULL;  int n=0;
	XSetWindowAttributes swa;
	Window root=DefaultRootWindow(wh.dpy);

	vtemp.depth=24;  vtemp.c_class=TrueColor;
	if((v=XGetVisualInfo(wh.dpy, VisualDepthMask|VisualClassMask, &vtemp, &n))!=NULL && n!=0)
	{
		swa.colormap=XCreateColormap(wh.dpy, root, v->visual, AllocNone);
		swa.border_pixel=0;
		swa.event_mask=0;
		errifnot(wh.win=XCreateWindow(wh.dpy, root, 0, 0, width, height, 0,
			v->depth, InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa));
		errifnot(XMapRaised(wh.dpy, wh.win));
		XSync(wh.dpy, False);
		display();
		XDestroyWindow(wh.dpy, wh.win);
		XFree(v);  v=NULL;
	}
	else fprintf(stderr, "No RGB visuals available.  Skipping those tests.\n\n");

	vtemp.depth=8;  vtemp.c_class=PseudoColor;
	if((v=XGetVisualInfo(wh.dpy, VisualDepthMask|VisualClassMask, &vtemp, &n))!=NULL && n!=0)
	{
		swa.colormap=XCreateColormap(wh.dpy, root, v->visual, AllocAll);
		swa.border_pixel=0;
		swa.event_mask=0;
    XColor xc[32];  int i;
		errifnot(v->colormap_size==256);
		for(i=0; i<32; i++)
		{
			xc[i].red=(i<16? i*16:255)<<8;
			xc[i].green=(i<16? i*16:255-(i-16)*16)<<8;
			xc[i].blue=(i<16? 255:255-(i-16)*16)<<8;
			xc[i].flags = DoRed | DoGreen | DoBlue;
			xc[i].pixel=i;
		}
		XStoreColors(wh.dpy, swa.colormap, xc, 32);
		errifnot(wh.win=XCreateWindow(wh.dpy, root, 0, 0, width, height, 0,
			v->depth, InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask,
			&swa));
		errifnot(XMapRaised(wh.dpy, wh.win));
		XSync(wh.dpy, False);
		display();
		XDestroyWindow(wh.dpy, wh.win);
		XFreeColormap(wh.dpy, swa.colormap);
		XFree(v);  v=NULL;
	}
	else fprintf(stderr, "No Pseudocolor visuals available.  Skipping those tests.\n\n");

	return 0;

	#endif

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}
}
