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
#include "pthread.h"
#include "hputil.h"
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
#define fbx(f) {if((f)==-1) throw(fbx_geterrmsg());}
#define errifnot(f) {if(!(f)) throw("Unexpected NULL condition");}
#ifdef WIN32
#define fg() SetForegroundWindow(wh)
#define tryw32(f) {if(!(f)) throw(hputil_strerror());}
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
			buf[i*pitch+j*ps+fbx_roffset[format]]=255;
			buf[i*pitch+j*ps+fbx_goffset[format]]=(j+x)%256;
			buf[i*pitch+j*ps+fbx_boffset[format]]=(i+y)%256;
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
			if(buf[i*pitch+j*ps+fbx_roffset[format]]!=255) return 0;
			if(buf[i*pitch+j*ps+fbx_goffset[format]]!=(j+x)%256) return 0;
			if(buf[i*pitch+j*ps+fbx_boffset[format]]!=(i+y)%256) return 0;
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
	if(useshm && !s.shm) throw("MIT-SHM not available");
	fprintf(stderr, "Native Pixel Format:  %s", fbx_formatname(s.format));
	fprintf(stderr, "\n");
	if(s.width!=width || s.height!=height)
		throw("The benchmark window lost input focus or was obscured.\nSkipping native write test\n");

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
		rbtime=hptime();
		for (i=0; i<n; i++)
		{
			for(int j=0; j<height; j++)
				fbx(fbx_awrite(&s, 0, j, 0, height-j-1, width, 1));
			fbx_sync(&s);
		}
		rbtime=hptime()-rbtime;
	} while(rbtime<2.);
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
		rbtime=hptime();
		for (i=0; i<n; i++)
		{
			fbx(fbx_write(&s, 0, 0, 0, 0, 0, 0));
		}
		rbtime=hptime()-rbtime;
	} while(rbtime<2.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(char *e) {fprintf(stderr, "%s\n", e);}
	catch(const char *e) {fprintf(stderr, "%s\n", e);}

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
	if(useshm && !s.shm) throw("MIT-SHM not available");
	if(s.width!=width || s.height!=height)
		throw("The benchmark window lost input focus or was obscured.\nSkipping native read test\n");
	if(useshm)
		fprintf(stderr, "FBX read [SHM]:                   ");
	else
		fprintf(stderr, "FBX read:                         ");
	memset(s.bits, 0, width*height*ps);
	n=N;
	do
	{
		n+=n;
		rbtime=hptime();
		for(i=0; i<n; i++)
		{
			fbx(fbx_read(&s, 0, 0));
		}
		rbtime=hptime()-rbtime;
		if(!cmpbuf(0, 0, width, s.pitch, height, s.format, (unsigned char *)s.bits))
			throw("ERROR: Bogus data read back.");
	} while (rbtime<2.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(char *e) {fprintf(stderr, "%s\n", e);}
	catch(const char *e) {fprintf(stderr, "%s\n", e);}

	fbx_term(&s);
}

// This serves as a unit test for the FBX library
class writethread
{
	public:
		writethread(int _myrank, int _iter, int _useshm) : myrank(_myrank),
			iter(_iter), useshm(_useshm), t(0)
		{
			int err;
			if((err=pthread_create(&t, NULL, run, this))!=0) throw(strerror(err));
		}

		~writethread(void) {stop(false);}

		void stop(bool checkerror)
		{
			char *err=NULL;
			if(t) {pthread_join(t, (void **)&err);  t=0;}
			if(err && checkerror) throw(err);
		}

		static void *run(void *param)
		{
			writethread *w=(writethread *)param;
			int i;  fbx_struct stressfb;
			memset(&stressfb, 0, sizeof(stressfb));

			try {

			int mywidth, myheight, myx=0, myy=0;
			if(w->myrank<2) {mywidth=width/2;  myx=0;}
			else {mywidth=width-width/2;  myx=width/2;}
			if(w->myrank%2==0) {myheight=height/2;  myy=0;}
			else {myheight=height-height/2;  myy=height/2;}
			fbx(fbx_init(&stressfb, wh, mywidth, myheight, w->useshm));
			if(w->useshm && !stressfb.shm) throw("MIT-SHM not available");
			initbuf(myx, myy, mywidth, stressfb.pitch, myheight, stressfb.format, (unsigned char *)stressfb.bits);
			for (i=0; i<w->iter; i++)
				fbx(fbx_write(&stressfb, 0, 0, myx, myy, mywidth, myheight));

			} catch(char *e) {fbx_term(&stressfb);  return (void *)e;}
			catch(const char *e) {fbx_term(&stressfb);  return (void *)e;}
			fbx_term(&stressfb);
			return 0;
		}

	private:
		int myrank, iter, useshm;
		pthread_t t;
};

fbx_struct stressfb0;

class writethread1
{
	public:
		writethread1(int _myrank, int _iter, int _useshm) : myrank(_myrank),
			iter(_iter), useshm(_useshm), t(0)
		{
			int err;
			if((err=pthread_create(&t, NULL, run, this))!=0) throw(strerror(err));
		}

		~writethread1(void) {stop(false);}

		void stop(bool checkerror)
		{
			char *err=NULL;
			if(t) {pthread_join(t, (void **)&err);  t=0;}
			if(err && checkerror) throw(err);
		}

		static void *run(void *param)
		{
			try {

			writethread1 *w=(writethread1 *)param;
			int i, mywidth, myheight, myx=0, myy=0;
			if(w->myrank<2) {mywidth=width/2;  myx=0;}
			else {mywidth=width-width/2;  myx=width/2;}
			if(w->myrank%2==0) {myheight=height/2;  myy=0;}
			else {myheight=height-height/2;  myy=height/2;}
			for (i=0; i<w->iter; i++)
				fbx(fbx_write(&stressfb0, myx, myy, myx, myy, mywidth, myheight));

			} catch(char *e) {return (void *)e;}
			catch(const char *e) {return (void *)e;}
			return 0;
		}

	private:
		int myrank, iter, useshm;
		pthread_t t;
};

class readthread
{
	public:
		readthread(int _myrank, int _iter, int _useshm) : myrank(_myrank),
			iter(_iter), useshm(_useshm), t(0)
		{
			int err;
			if((err=pthread_create(&t, NULL, run, this))!=0) throw(strerror(err));
		}

		~readthread(void) {stop(false);}

		void stop(bool checkerror)
		{
			char *err=NULL;
			if(t) {pthread_join(t, (void **)&err);  t=0;}
			if(err && checkerror) throw(err);
		}

		static void *run(void *param)
		{
			readthread *r=(readthread *)param;
			fbx_struct stressfb;
			memset(&stressfb, 0, sizeof(stressfb));

			try {

			int i, mywidth, myheight, myx=0, myy=0;
			if(r->myrank<2) {mywidth=width/2;  myx=0;}
			else {mywidth=width-width/2;  myx=width/2;}
			if(r->myrank%2==0) {myheight=height/2;  myy=0;}
			else {myheight=height-height/2;  myy=height/2;}
			fbx(fbx_init(&stressfb, wh, mywidth, myheight, r->useshm));
			if(r->useshm && !stressfb.shm) throw("MIT-SHM not available");
			int ps=fbx_ps[stressfb.format];
			memset(stressfb.bits, 0, mywidth*myheight*ps);
			for(i=0; i<r->iter; i++)
				fbx(fbx_read(&stressfb, myx, myy));
			if(!cmpbuf(myx, myy, mywidth, stressfb.pitch, myheight, stressfb.format, (unsigned char *)stressfb.bits))
				throw("ERROR: Bogus data read back.");

			} catch(char *e) {fbx_term(&stressfb);  return (void *)e;}
			catch(const char *e) {fbx_term(&stressfb);  return (void *)e;}
			fbx_term(&stressfb);
			return 0;
		}

	private:
		int myrank, iter, useshm;
		pthread_t t;
};

void nativestress(int useshm)
{
	int i, n;  double rbtime;

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
		rbtime=hptime();
		writethread *wt[4];
		for(i=0; i<4; i++)
			wt[i]=new writethread(i, n, useshm);
		for(i=0; i<4; i++) wt[i]->stop(true);
		for(i=0; i<4; i++) delete wt[i];
		rbtime=hptime()-rbtime;
	} while (rbtime<2.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(char *e) {fprintf(stderr, "%s\n", e);}
	catch(const char *e) {fprintf(stderr, "%s\n", e);}

	try {

	if(useshm)
		fprintf(stderr, "FBX read [4 bmps, 4 blits SHM]:   ");
	else
		fprintf(stderr, "FBX read [4 bmps, 4 blits]:       ");
	n=N;
	do
	{
		n+=n;
		rbtime=hptime();
		readthread *rt[4];
		for(i=0; i<4; i++)
			rt[i]=new readthread(i, n, useshm);
		for(i=0; i<4; i++) rt[i]->stop(true);
		for(i=0; i<4; i++) delete rt[i];
		rbtime=hptime()-rbtime;
	} while (rbtime<2.);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(char *e) {fprintf(stderr, "%s\n", e);}
	catch(const char *e) {fprintf(stderr, "%s\n", e);}

	try {

	clearfb();
	if(useshm)
		fprintf(stderr, "FBX write [1 bmp, 4 blits SHM]:   ");
	else
		fprintf(stderr, "FBX write [1 bmp, 4 blits]:       ");
	memset(&stressfb0, 0, sizeof(stressfb0));
	fbx(fbx_init(&stressfb0, wh, width, height, useshm));
	if(useshm && !stressfb0.shm) throw("MIT-SHM not supported");
	initbuf(0, 0, width, stressfb0.pitch, height, stressfb0.format, (unsigned char *)stressfb0.bits);
	n=N;
	do
	{
		n+=n;
		rbtime=hptime();
		writethread1 *wt1[4];
		for(i=0; i<4; i++)
			wt1[i]=new writethread1(i, n, useshm);
		for(i=0; i<4; i++) wt1[i]->stop(true);
		for(i=0; i<4; i++) delete wt1[i];
		rbtime=hptime()-rbtime;
	} while (rbtime<2.);
	fbx_term(&stressfb0);
	fprintf(stderr, "%f Mpixels/sec\n", (double)n*(double)(width*height)/((double)1000000.*rbtime));

	} catch(char *e) {fprintf(stderr, "%s\n", e);}
	catch(const char *e) {fprintf(stderr, "%s\n", e);}

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

	hptimer_init();

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
		if((ret=GetMessage(&msg, NULL, 0, 0))==-1) throw(hputil_strerror());
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

	} catch(char *e) {fprintf(stderr, "%s\n", e);}
	catch(const char *e) {fprintf(stderr, "%s\n", e);}
}
