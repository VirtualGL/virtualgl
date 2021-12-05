// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2013-2014, 2017-2019, 2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "Thread.h"
#include "vglutil.h"
#include "Timer.h"
#include "fbx.h"
#ifndef _WIN32
	#include <errno.h>
	#include "x11err.h"
#endif
#include "bmp.h"

using namespace util;


#ifndef _WIN32
extern "C" {
int xhandler(Display *dpy, XErrorEvent *xe)
{
	fprintf(stderr, "X11 Error: %s\n", x11error(xe->error_code));
	return 0;
}
}  // extern "C"
#endif


#define BENCH_NAME  "FBXtest"
#define N  2
#define MAXRGB  (1 << (depth / 3))
#define DEFAULT_WIDTH  1240
#define DEFAULT_HEIGHT  900

int drawableWidth = DEFAULT_WIDTH, drawableHeight = DEFAULT_HEIGHT, depth = 24;
bool doPixmap = false, doShm = true, doFS = false, doVid = false,
	doDisplay = false, interactive = false, advance = false, doStress = false;
int pixelOffset, retCode = 0;
double benchTime = 5.0;
#ifndef _WIN32
bool checkDB = false;
Window win = 0;
#endif
fbx_wh wh;
#ifdef _WIN32
#define FG()  SetForegroundWindow(wh)
#else
#define FG()
#endif

void nativeRead(bool), nativeWrite(bool);


void initBuf(int x, int y, int width, int pitch, int height, PF *pf,
	unsigned char *buf, int offset)
{
	int i, j;

	for(j = 0; j < height; j++)
	{
		for(i = 0; i < width; i++)
			pf->setRGB(&buf[j * pitch + i * pf->size], (i + x + offset) % MAXRGB,
				(j + y + offset) % MAXRGB, (j + y + i + x + offset) % MAXRGB);
	}
}


int cmpBuf(int x, int y, int width, int pitch, int height, PF *pf,
	unsigned char *buf, int offset, bool flip = false)
{
	int i, j;

	for(j = 0; j < height; j++)
	{
		int line = flip ? height - 1 - j : j;
		for(i = 0; i < width; i++)
		{
			bool ignore = false;
			#ifdef __APPLE__
			// In XQuartz, X[Shm]GetImage() picks up the little window resize
			// gadget at the bottom right, so we have to ignore those pixels.
			if(line >= height - 16 && i >= width - 16) ignore = true;
			#endif
			int r, g, b;
			pf->getRGB(&buf[line * pitch + i * pf->size], &r, &g, &b);
			if(r != (i + x + offset) % MAXRGB && !ignore)
				return 0;
			if(g != (j + y + offset) % MAXRGB && !ignore)
				return 0;
			if(b != (j + y + i + x + offset) % MAXRGB && !ignore)
				return 0;
		}
	}
	return 1;
}


// Makes sure the frame buffer has been cleared prior to a write
void clearFB(void)
{
	#ifdef _WIN32

	if(wh)
	{
		HDC hdc = 0;  RECT rect;
		TRY_W32(hdc = GetDC(wh));
		TRY_W32(GetClientRect(wh, &rect));
		TRY_W32(PatBlt(hdc, 0, 0, rect.right, rect.bottom, BLACKNESS));
		TRY_W32(ReleaseDC(wh, hdc));
	}

	#else

	if(wh.dpy && wh.d && !doPixmap)
	{
		XSetWindowBackground(wh.dpy, wh.d,
			BlackPixel(wh.dpy, DefaultScreen(wh.dpy)));
		XClearWindow(wh.dpy, wh.d);
		XSync(wh.dpy, False);
	}

	#endif
}


// Platform-specific write test
void nativeWrite(bool useShm)
{
	fbx_struct fb;  int i = 0;  double drawTime;
	Timer timer, timer2;

	memset(&fb, 0, sizeof(fb));

	try
	{
		TRY_FBX(fbx_init(&fb, wh, 0, 0, useShm ? 1 : 0));
		if(useShm && !fb.shm) THROW("MIT-SHM not available");
		fprintf(stderr, "Native Pixel Format:  %s\n", fb.pf->name);
		if(fb.width != drawableWidth || fb.height != drawableHeight)
		{
			fprintf(stderr, "WARNING:  Requested size = %d x %d  Actual size = %d x %d\n",
				drawableWidth, drawableHeight, fb.width, fb.height);
		}

		clearFB();
		if(useShm)
			fprintf(stderr, "FBX bottom-up write [SHM]:     ");
		else
			fprintf(stderr, "FBX bottom-up write:           ");
		i = 0;  drawTime = 0;  timer2.start();
		do
		{
			#ifndef _WIN32
			if(checkDB)
			{
				memset(fb.bits, 255, fb.pitch * fb.height);
				TRY_FBX(fbx_awrite(&fb, 0, 0, 0, 0, 0, 0));
			}
			#endif
			initBuf(0, 0, fb.width, fb.pitch, fb.height, fb.pf,
				(unsigned char *)fb.bits, i);
			timer.start();
			TRY_FBX(fbx_flip(&fb, 0, 0, 0, 0));
			TRY_FBX(fbx_write(&fb, 0, 0, 0, 0, 0, 0));
			drawTime += timer.elapsed();
			i++;
		} while(timer2.elapsed() < benchTime);
		fprintf(stderr, "%f Mpixels/sec",
			(double)i * (double)(fb.width * fb.height) / (1000000. * drawTime));
		memset(fb.bits, 0, fb.pitch * fb.height);
		TRY_FBX(fbx_read(&fb, 0, 0));
		if(!cmpBuf(0, 0, fb.width, fb.pitch, fb.height, fb.pf,
			(unsigned char *)fb.bits, i - 1, true))
		{
			fprintf(stderr, " (ERROR CHECK FAILED)\n");
			retCode = -1;
		}
		else fprintf(stderr, " (no errors)\n");

		clearFB();
		if(useShm)
			fprintf(stderr, "FBX 1/4 top-down write [SHM]:  ");
		else
			fprintf(stderr, "FBX 1/4 top-down write:        ");
		i = 0;  drawTime = 0.;  timer2.start();
		do
		{
			#ifndef _WIN32
			if(checkDB)
			{
				memset(fb.bits, 255, fb.pitch * fb.height);
				TRY_FBX(fbx_awrite(&fb, 0, 0, 0, 0, 0, 0));
			}
			#endif
			initBuf(0, 0, fb.width, fb.pitch, fb.height, fb.pf,
				(unsigned char *)fb.bits, i);
			timer.start();
			TRY_FBX(fbx_write(&fb, 0, 0, fb.width / 2, fb.height / 2, fb.width / 2,
				fb.height / 2));
			drawTime += timer.elapsed();
			i++;
		} while(timer2.elapsed() < benchTime);
		fprintf(stderr, "%f Mpixels/sec",
			(double)i * (double)(fb.width * fb.height) / (4000000. * drawTime));
		memset(fb.bits, 0, fb.pitch * fb.height);
		TRY_FBX(fbx_read(&fb, 0, 0));
		if(!cmpBuf(0, 0, fb.width / 2, fb.pitch, fb.height / 2,
			fb.pf, (unsigned char *)&fb.bits[fb.height / 2 * fb.pitch +
				fb.width / 2 * fb.pf->size], i - 1))
		{
			fprintf(stderr, " (ERROR CHECK FAILED)\n");
			retCode = -1;
		}
		else fprintf(stderr, " (no errors)\n");

		clearFB();
		if(useShm)
			fprintf(stderr, "FBX top-down write [SHM]:      ");
		else
			fprintf(stderr, "FBX top-down write:            ");
		i = 0;  drawTime = 0.;  timer2.start();
		do
		{
			#ifndef _WIN32
			if(checkDB)
			{
				memset(fb.bits, 255, fb.pitch * fb.height);
				TRY_FBX(fbx_awrite(&fb, 0, 0, 0, 0, 0, 0));
			}
			#endif
			initBuf(0, 0, fb.width, fb.pitch, fb.height, fb.pf,
				(unsigned char *)fb.bits, i);
			timer.start();
			TRY_FBX(fbx_write(&fb, 0, 0, 0, 0, 0, 0));
			drawTime += timer.elapsed();
			i++;
		} while(timer2.elapsed() < benchTime);
		fprintf(stderr, "%f Mpixels/sec",
			(double)i * (double)(fb.width * fb.height) / (1000000. * drawTime));
		memset(fb.bits, 0, fb.pitch * fb.height);
		TRY_FBX(fbx_read(&fb, 0, 0));
		if(!cmpBuf(0, 0, fb.width, fb.pitch, fb.height, fb.pf,
			(unsigned char *)fb.bits, i - 1))
		{
			fprintf(stderr, " (ERROR CHECK FAILED)\n");
			retCode = -1;
		}
		else fprintf(stderr, " (no errors)\n");

	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n", e.what());  retCode = -1;
	}

	pixelOffset = i - 1;

	fbx_term(&fb);
}


// Platform-specific readback test
void nativeRead(bool useShm)
{
	fbx_struct fb;  int i, error = 0;  double readTime;
	Timer timer, timer2;

	memset(&fb, 0, sizeof(fb));

	try
	{
		TRY_FBX(fbx_init(&fb, wh, 0, 0, useShm ? 1 : 0));
		if(useShm && !fb.shm) THROW("MIT-SHM not available");
		if(fb.width != drawableWidth || fb.height != drawableHeight)
		{
			fprintf(stderr, "WARNING:  Requested size = %d x %d  Actual size = %d x %d\n",
				drawableWidth, drawableHeight, fb.width, fb.height);
		}

		if(useShm)
			fprintf(stderr, "FBX read [SHM]:                ");
		else
			fprintf(stderr, "FBX read:                      ");
		memset(fb.bits, 0, fb.width * fb.height * fb.pf->size);
		i = 0;  readTime = 0.;  timer2.start();
		do
		{
			timer.start();
			TRY_FBX(fbx_read(&fb, 0, 0));
			readTime += timer.elapsed();
			if(!cmpBuf(0, 0, fb.width, fb.pitch, fb.height, fb.pf,
				(unsigned char *)fb.bits, pixelOffset))
				error = 1;
			i++;
		} while(timer2.elapsed() < benchTime);
		fprintf(stderr, "%f Mpixels/sec",
			(double)i * (double)(fb.width * fb.height) / (1000000. * readTime));
		if(error)
		{
			fprintf(stderr, " (ERROR CHECK FAILED)\n");
			retCode = -1;
		}
		else fprintf(stderr, " (no errors)\n");

	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n", e.what());  retCode = -1;
	}

	fbx_term(&fb);
}


// This serves as a unit test for the FBX library
class WriteThread : public Runnable
{
	public:

		WriteThread(int myRank_, int iter_, bool useShm_) : myRank(myRank_),
			iter(iter_), useShm(useShm_) {}

		void run(void)
		{
			int i;  fbx_struct fb;
			memset(&fb, 0, sizeof(fb));

			try
			{
				int myWidth, myHeight, myX = 0, myY = 0;

				TRY_FBX(fbx_init(&fb, wh, 0, 0, useShm ? 1 : 0));
				if(myRank < 2) { myWidth = fb.width / 2;  myX = 0; }
				else { myWidth = fb.width - fb.width / 2;  myX = fb.width / 2; }
				if(myRank % 2 == 0) { myHeight = fb.height / 2;  myY = 0; }
				else { myHeight = fb.height - fb.height / 2;  myY = fb.height / 2; }
				fbx_term(&fb);
				TRY_FBX(fbx_init(&fb, wh, myWidth, myHeight, useShm ? 1 : 0));
				if(useShm && !fb.shm) THROW("MIT-SHM not available");
				initBuf(myX, myY, fb.width, fb.pitch, fb.height, fb.pf,
					(unsigned char *)fb.bits, 0);
				for(i = 0; i < iter; i++)
					TRY_FBX(fbx_write(&fb, 0, 0, myX, myY, fb.width, fb.height));
				fbx_term(&fb);
			}
			catch(...)
			{
				fbx_term(&fb);  retCode = -1;  throw;
			}
		}

	private:

		int myRank, iter;
		bool useShm;
};


class ReadThread : public Runnable
{
	public:

		ReadThread(int myRank_, int iter_, bool useShm_) : myRank(myRank_),
			iter(iter_), useShm(useShm_) {}

		void run(void)
		{
			fbx_struct fb;
			memset(&fb, 0, sizeof(fb));

			try
			{
				int i, myWidth, myHeight, myX = 0, myY = 0;

				TRY_FBX(fbx_init(&fb, wh, 0, 0, useShm ? 1 : 0));
				if(myRank < 2) { myWidth = fb.width / 2;  myX = 0; }
				else { myWidth = fb.width - fb.width / 2;  myX = fb.width / 2; }
				if(myRank % 2 == 0) { myHeight = fb.height / 2;  myY = 0; }
				else { myHeight = fb.height - fb.height / 2;  myY = fb.height / 2; }
				fbx_term(&fb);
				TRY_FBX(fbx_init(&fb, wh, myWidth, myHeight, useShm ? 1 : 0));
				if(useShm && !fb.shm) THROW("MIT-SHM not available");
				memset(fb.bits, 0, fb.width * fb.height * fb.pf->size);
				for(i = 0; i < iter; i++)
					TRY_FBX(fbx_read(&fb, myX, myY));
				if(!cmpBuf(myX, myY, fb.width, fb.pitch, fb.height, fb.pf,
					(unsigned char *)fb.bits, 0))
					THROW("ERROR: Bogus data read back.");
				fbx_term(&fb);
			}
			catch(...)
			{
				fbx_term(&fb);  retCode = -1;  throw;
			}
		}

	private:

		int myRank, iter;
		bool useShm;
};


void nativeStress(bool useShm)
{
	int i, n;  double testTime;
	Thread *thread[4];
	Timer timer;

	try
	{
		clearFB();
		if(useShm)
			fprintf(stderr, "FBX write [multithreaded SHM]:   ");
		else
			fprintf(stderr, "FBX write [multithreaded]:       ");
		n = N;
		do
		{
			n += n;
			timer.start();
			WriteThread *writeThread[4];
			for(i = 0; i < 4; i++)
			{
				writeThread[i] = new WriteThread(i, n, useShm ? 1 : 0);
				thread[i] = new Thread(writeThread[i]);
				thread[i]->start();
			}
			for(i = 0; i < 4; i++) thread[i]->stop();
			for(i = 0; i < 4; i++) thread[i]->checkError();
			for(i = 0; i < 4; i++)
			{
				delete thread[i];  delete writeThread[i];
			}
			testTime = timer.elapsed();
		} while(testTime < 1.);
		fprintf(stderr, "%f Mpixels/sec\n",
			(double)n * (double)(drawableWidth * drawableHeight) /
				(1000000. * testTime));

	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n", e.what());
	}

	try
	{
		if(useShm)
			fprintf(stderr, "FBX read [multithreaded SHM]:    ");
		else
			fprintf(stderr, "FBX read [multithreaded]:        ");
		n = N;
		do
		{
			n += n;
			timer.start();
			ReadThread *readThread[4];
			for(i = 0; i < 4; i++)
			{
				readThread[i] = new ReadThread(i, n, useShm);
				thread[i] = new Thread(readThread[i]);
				thread[i]->start();
			}
			for(i = 0; i < 4; i++) thread[i]->stop();
			for(i = 0; i < 4; i++) thread[i]->checkError();
			for(i = 0; i < 4; i++)
			{
				delete thread[i];  delete readThread[i];
			}
			testTime = timer.elapsed();
		} while(testTime < 1.);
		fprintf(stderr, "%f Mpixels/sec\n",
			(double)n * (double)(drawableWidth * drawableHeight) /
				(1000000. * testTime));

	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n", e.what());  retCode = -1;
	}

}


void display(void)
{
	if(doStress)
	{
		fprintf(stderr, "-- Stress tests --\n");
		#ifndef _WIN32
		if(doShm)
		{
			FG();  nativeStress(1);
		}
		#endif
		FG();  nativeStress(0);
		fprintf(stderr, "\n");
		return;
	}

	fprintf(stderr, "-- Performance tests --\n");
	#ifndef _WIN32
	if(doShm)
	{
		FG();  nativeWrite(1);
		FG();  nativeRead(1);
	}
	#endif
	FG();  nativeWrite(0);
	FG();  nativeRead(0);
	fprintf(stderr, "\n");
}


#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
		case WM_CREATE:
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_CHAR:
			if((wParam == 27 || wParam == 'q' || wParam == 'Q') && doVid)
			{
				PostQuitMessage(0);
				return 0;
			}
			break;
		case WM_PAINT:
			if(!doVid)
			{
				display();
				PostQuitMessage(0);
			}
			else
			{
				if(interactive) doDisplay = true;
				return 0;
			}
			break;
		case WM_MOUSEMOVE:
			if((wParam & MK_LBUTTON) && doVid && interactive)
			{
				doDisplay = advance = true;
				return 0;
			}
			break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
#endif


void event_loop(void)
{
	fbx_struct fb[10];
	int frame = 0, inc = -1;  bool first = true;
	unsigned long frames = 0;
	Timer timer;
	double elapsed, mpixels = 0.;
	char temps[256];

	for(int i = 0; i < 10; i++)
	{
		memset(&fb[i], 0, sizeof(fb[i]));
	}

	try
	{
		for(int i = 0; i < 10; i++)
		{
			TRY_FBX(fbx_init(&fb[i], wh, 0, 0, doShm ? 1 : 0));
			snprintf(temps, 256, "frame%d.ppm", i);
			unsigned char *buf = NULL;  int tempw = 0, temph = 0;
			if(bmp_load(temps, &buf, &tempw, 1, &temph, fb[i].pf->id,
				BMPORN_TOPDOWN) == -1)
				THROW(bmp_geterr());
			for(int j = 0; j < min(temph, fb[i].height); j++)
				memcpy(&fb[i].bits[fb[i].pitch * j], &buf[tempw * fb[i].pf->size * j],
					min(tempw, fb[i].width) * fb[i].pf->size);
			free(buf);
		}

		timer.start();
		while(1)
		{
			advance = false;  doDisplay = false;
			if(first)
			{
				doDisplay = true;  first = false;
			}

			#ifdef _WIN32

			int ret;  MSG msg;
			if((ret = GetMessage(&msg, NULL, 0, 0)) == -1) { THROW_W32(); }
			else if(ret == 0)
			{
				for(int i = 0; i < 10; i++) fbx_term(&fb[i]);
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			#else

			while(1)
			{
				XEvent event;
				if(XPending(wh.dpy) > 0) XNextEvent(wh.dpy, &event);
				else break;
				switch(event.type)
				{
					case Expose:
						doDisplay = true;
						break;
					case KeyPress:
					{
						char buf[10];
						XLookupString(&event.xkey, buf, sizeof(buf), NULL, NULL);
						switch(buf[0])
						{
							case 27: case 'q': case 'Q':
								for(int i = 0; i < 10; i++) fbx_term(&fb[i]);
								return;
						}
						break;
					}
					case MotionNotify:
						if(event.xmotion.state & Button1Mask) doDisplay = advance = true;
						break;
				}
			}

			#endif

			if(!interactive || doDisplay)
			{
				TRY_FBX(fbx_write(&fb[frame], 0, 0, 0, 0, 0, 0));
				if(!interactive || advance)
				{
					if(frame == 0 || frame == 9) inc = -1 * inc;
					frame += inc;  frames++;
					mpixels +=
						(double)fb[frame].width * (double)fb[frame].height / 1000000.;

					if((elapsed = timer.elapsed()) > 2.0)
					{
						snprintf(temps, 256, "%f frames/sec - %f Mpixels/sec",
							(double)frames / elapsed, mpixels / elapsed);
						printf("%s\n", temps);
						timer.start();  mpixels = 0.;  frames = 0;
					}
				}
			}
		}
	}
	catch(...)
	{
		for(int i = 0; i < 10; i++) fbx_term(&fb[i]);
		retCode = -1;
		throw;
	}

}


void usage(char **argv)
{
	fprintf(stderr, "USAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	#ifndef _WIN32
	fprintf(stderr, "-checkdb = Verify that double buffering is working correctly\n");
	fprintf(stderr, "-noshm = Do not use MIT-SHM extension to accelerate blitting\n");
	fprintf(stderr, "-pm = Blit to a pixmap rather than to a window\n");
	#endif
	fprintf(stderr, "-mt = Run multithreaded stress tests\n");
	fprintf(stderr, "-v = Print all warnings and informational messages from FBX\n");
	fprintf(stderr, "-fs = Full-screen mode\n");
	fprintf(stderr, "-time <t> = Run each benchmark for <t> seconds (default: %.1f)\n",
		benchTime);
	fprintf(stderr, "-size <wxh> = Specify drawable width & height (default: %dx%d)\n\n",
		DEFAULT_WIDTH, DEFAULT_HEIGHT);
	exit(1);
}


int main(int argc, char **argv)
{
	#ifdef _WIN32
	int winstyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_VISIBLE;
	#endif
	int i, screenWidth, screenHeight;

	fprintf(stderr, "\n%s v%s (Build %s)\n\n", BENCH_NAME, __VERSION, __BUILD);

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		#ifndef _WIN32
		else if(!stricmp(argv[i], "-checkdb"))
		{
			checkDB = true;
			fprintf(stderr, "Checking double buffering.  Watch for flashing to indicate that it is\n");
			fprintf(stderr, "not enabled.  Performance will be sub-optimal.\n");
		}
		#endif
		else if(!stricmp(argv[i], "-noshm"))
		{
			doShm = false;
		}
		else if(!stricmp(argv[i], "-vid")) doVid = true;
		else if(!stricmp(argv[i], "-v"))
		{
			fbx_printwarnings(stderr);
		}
		#ifndef _WIN32
		else if(!stricmp(argv[i], "-pm"))
		{
			doPixmap = true;  doShm = false;
		}
		#endif
		else if(!stricmp(argv[i], "-i")) interactive = true;
		else if(!stricmp(argv[i], "-mt")) doStress = true;
		else if(!stricmp(argv[i], "-fs"))
		{
			doFS = true;
			#ifdef _WIN32
			winstyle = WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE;
			#endif
		}
		else if(!stricmp(argv[i], "-time") && i < argc - 1)
		{
			if(sscanf(argv[++i], "%lf", &benchTime) < 1 || benchTime <= 0.0)
				usage(argv);
		}
		else if(!stricmp(argv[i], "-size") && i < argc - 1)
		{
			if(sscanf(argv[++i], "%dx%d", &drawableWidth, &drawableHeight) < 2
				|| drawableWidth < 1 || drawableHeight < 1)
				usage(argv);
		}
		else usage(argv);
	}

	try
	{
		#ifdef _WIN32

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
		wndclass.lpszClassName = BENCH_NAME;
		wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
		TRY_W32(RegisterClassEx(&wndclass));
		screenWidth = GetSystemMetrics(doFS ? SM_CXSCREEN : SM_CXMAXIMIZED);
		screenHeight = GetSystemMetrics(doFS ? SM_CYSCREEN : SM_CYMAXIMIZED);

		#else

		if(!XInitThreads())
		{
			fprintf(stderr, "ERROR: Could not initialize Xlib thread safety\n");
			exit(1);
		}
		XSetErrorHandler(xhandler);
		memset(&wh, 0, sizeof(fbx_wh));
		if(!(wh.dpy = XOpenDisplay(0)))
		{
			fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
			exit(1);
		}
		screenWidth = DisplayWidth(wh.dpy, DefaultScreen(wh.dpy));
		screenHeight = DisplayHeight(wh.dpy, DefaultScreen(wh.dpy));

		#endif

		if(!doFS && !doPixmap)
		{
			drawableWidth = min(drawableWidth, screenWidth);
			drawableHeight = min(drawableHeight, screenHeight);
		}
		fprintf(stderr, "%s size:  %d x %d\n", doPixmap ? "Pixmap" : "Window",
			drawableWidth, drawableHeight);

		#ifdef _WIN32

		int bw = GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
		int bh =
			GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
		TRY_W32(wh = CreateWindowEx(0, BENCH_NAME, BENCH_NAME, winstyle, 0, 0,
			drawableWidth + bw, drawableHeight + bh, NULL, NULL,
			GetModuleHandle(NULL), NULL));
		UpdateWindow(wh);
		BOOL ret;
		if(doVid)
		{
			event_loop();  return 0;
		}
		while(1)
		{
			if((ret = GetMessage(&msg, NULL, 0, 0)) == -1) THROW_W32();
			else if(ret == 0) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return (int)msg.wParam;

		#else

		XVisualInfo vtemp, *v = NULL;  int n = 0;
		XSetWindowAttributes swa;
		Window root = DefaultRootWindow(wh.dpy);

		vtemp.depth = DefaultDepth(wh.dpy, DefaultScreen(wh.dpy));
		if(vtemp.depth == 30) depth = 30;
		vtemp.c_class = TrueColor;
		vtemp.screen = DefaultScreen(wh.dpy);
		v = XGetVisualInfo(wh.dpy,
			VisualDepthMask | VisualClassMask | VisualScreenMask, &vtemp, &n);
		if(!v || !n) THROW("No RGB visuals available");
		int mask = CWBorderPixel | CWColormap | CWEventMask;
		swa.colormap = XCreateColormap(wh.dpy, root, v->visual, AllocNone);
		swa.border_pixel = 0;
		swa.event_mask = 0;
		if(doFS)
		{
			mask |= CWOverrideRedirect;  swa.override_redirect = True;
		}
		if(doVid)
		{
			if(interactive)
				swa.event_mask |= PointerMotionMask | ButtonPressMask | ExposureMask;
			swa.event_mask |= KeyPressMask;
		}
		if(doPixmap)
		{
			ERRIFNOT(win = XCreateWindow(wh.dpy, root, 0, 0, 1, 1, 0, v->depth,
				InputOutput, v->visual, mask, &swa));
			ERRIFNOT(wh.d = XCreatePixmap(wh.dpy, win, drawableWidth,
				drawableHeight, v->depth));
			wh.v = v->visual;
		}
		else
		{
			ERRIFNOT(wh.d = XCreateWindow(wh.dpy, root, 0, 0, drawableWidth,
				drawableHeight, 0, v->depth, InputOutput, v->visual, mask, &swa));
			ERRIFNOT(XMapRaised(wh.dpy, wh.d));
		}
		if(doFS) XSetInputFocus(wh.dpy, wh.d, RevertToParent, CurrentTime);
		XSync(wh.dpy, False);
		if(doVid) event_loop();
		else display();
		if(doPixmap)
		{
			XFreePixmap(wh.dpy, wh.d);
			XDestroyWindow(wh.dpy, win);
		}
		else XDestroyWindow(wh.dpy, wh.d);
		XFree(v);  v = NULL;

		#endif

	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s\n", e.what());  retCode = -1;
	}

	return retCode;
}
