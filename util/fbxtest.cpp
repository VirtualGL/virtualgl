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
#define N                 5

int width, height;
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
	fprintf(stderr, "Native Pixel Format:  %d-bit ", s.ps*8);
	for(i=0; i<s.ps*8; i++)
	{
		if(s.rmask&(1<<i)) fprintf(stderr, "R");  else if(s.gmask&(1<<i)) fprintf(stderr, "G");
		else if(s.bmask&(1<<i)) fprintf(stderr, "B");
	}
	fprintf(stderr, "\n");
	if(s.width!=width || s.height!=height)
		_throw("The benchmark window lost input focus or was obscured.\nSkipping native write test\n");
	clearfb();
	initbuf(0, 0, width, height, s.ps, (unsigned char *)s.bits, 0, s.width*s.ps!=s.pitch);
	if(useshm)
		fprintf(stderr, "FB write (O/S Native SHM):      ");
	else
		fprintf(stderr, "FB write (O/S Native):          ");
	n=N;
	do
	{
		n+=n;
		timer.start();
		for (i=0; i<n; i++)
		{
			fbx(fbx_write(&s, 0, 0, 0, 0, 0, 0));
		}
		rbtime=timer.elapsed();
	} while(rbtime<2.);
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
	if(useshm && !s.shm) _throw("MIT-SHM not available");
	if(s.width!=width || s.height!=height)
		_throw("The benchmark window lost input focus or was obscured.\nSkipping native read test\n");
	if(useshm)
		fprintf(stderr, "FB read (O/S Native SHM):       ");
	else
		fprintf(stderr, "FB read (O/S Native):           ");
	memset(s.bits, 0, width*height*s.ps);
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
		if(!cmpbuf(0, 0, width, height, s.ps, (unsigned char *)s.bits, 0, littleendian(), s.width*s.ps!=s.pitch))
			_throw("ERROR: Bogus data read back.");
	} while (rbtime<2.);
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
			initbuf(myx, myy, mywidth, myheight, stressfb.ps, (unsigned char *)stressfb.bits, 0,
				stressfb.width*stressfb.ps!=stressfb.pitch);
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
			memset(stressfb.bits, 0, mywidth*myheight*stressfb.ps);
			for(i=0; i<iter; i++)
				fbx(fbx_read(&stressfb, myx, myy));
			if(!cmpbuf(myx, myy, mywidth, myheight, stressfb.ps, (unsigned char *)stressfb.bits, 0, littleendian(),
				stressfb.width*stressfb.ps!=stressfb.pitch))
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
		fprintf(stderr, "FB write (4 bmps, 4 blits SHM): ");
	else
		fprintf(stderr, "FB write (4 bmps, 4 blits):     ");
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
	} while (rbtime<2.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	try {

	if(useshm)
		fprintf(stderr, "FB read (4 bmps, 4 blits SHM):  ");
	else
		fprintf(stderr, "FB read (4 bmps, 4 blits):      ");
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
	} while (rbtime<2.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	try {

	clearfb();
	if(useshm)
		fprintf(stderr, "FB write (1 bmp, 4 blits SHM):  ");
	else
		fprintf(stderr, "FB write (1 bmp, 4 blits):      ");
	memset(&stressfb0, 0, sizeof(stressfb0));
	fbx(fbx_init(&stressfb0, wh, width, height, useshm));
	if(useshm && !stressfb0.shm) _throw("MIT-SHM not supported");
	initbuf(0, 0, width, height, stressfb0.ps, (unsigned char *)stressfb0.bits, 0,
		stressfb0.width*stressfb0.ps!=stressfb0.pitch);
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
	} while (rbtime<2.);
	fbx_term(&stressfb0);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}

	fg();  nativeread(useshm);

	fbx_term(&stressfb0);
	return;
}

void display(void)
{
	#ifndef WIN32
	fg();  nativewrite(1);
	fg();  nativeread(1);
	#endif
	fg();  nativewrite(0);
	fg();  nativeread(0);
	fprintf(stderr, "\n");

	#ifndef WIN32
	fg();  nativestress(1);
	#endif
	fg();  nativestress(0);
	fprintf(stderr, "\n");

	exit(0);
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
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
#endif


//////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
	fprintf(stderr, "\n%s v%s (Build %s)\n", bench_name, __VERSION, __BUILD);

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
	tryw32(wh=CreateWindowEx(NULL, bench_name, bench_name, WS_OVERLAPPED |
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

	errifnot(wh.win=XCreateSimpleWindow(wh.dpy, DefaultRootWindow(wh.dpy),
		0, 0, width, height, 0, WhitePixel(wh.dpy, DefaultScreen(wh.dpy)),
		BlackPixel(wh.dpy, DefaultScreen(wh.dpy))));
	errifnot(XMapRaised(wh.dpy, wh.win));
	XSync(wh.dpy, False);
	display();
	return 0;

	#endif

	} catch(rrerror &e) {fprintf(stderr, "%s\n", e.getMessage());}
}
