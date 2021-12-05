// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2006-2007 Sun Microsystems, Inc.
// Copyright (C)2011, 2013-2014, 2016-2019, 2021 D. R. Commander
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "vglutil.h"
#include "Timer.h"
#include "fbx.h"
#ifndef _WIN32
	extern "C"
	{
		#define NeedFunctionPrototypes  1
		#include "dsimple.h"
	}
	#include <signal.h>
	#include <sys/time.h>
	#include <sys/resource.h>
	#include <X11/extensions/XTest.h>
#endif

using namespace util;


#define THROW(f, l, m) \
{ \
	fprintf(stderr, "%s (%d):\n%s\n", f, l, m);  fflush(stderr);  exit(1); \
}
#define TRY_FBX(a) \
{ \
	if((a) == -1) THROW("fbx.c", fbx_geterrline(), fbx_geterrmsg()); \
}

#ifdef _WIN32
char errMsg[256];
#define THROW_W32(f) \
{ \
	if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errMsg, 255, NULL)) \
		strncpy(errMsg, "Error in FormatMessage()", 256); \
	THROW(f, __LINE__, errMsg); \
}
#endif


#define DEFSAMPLERATE  100
#define DEFBENCHTIME  30.0


#ifdef _WIN32
char *program_name;

#define MOVE_MOUSE() \
{ \
	for(i = 0; i < 4; i++) \
	{ \
		inputs[i].type = INPUT_MOUSE; \
		inputs[i].mi.dwFlags = downFlags | MOUSEEVENTF_MOVE | \
			MOUSEEVENTF_MOVE_NOCOALESCE | MOUSEEVENTF_ABSOLUTE; \
		inputs[i].mi.dx = 65535 * currentX / GetSystemMetrics(SM_CXSCREEN); \
		inputs[i].mi.dy = 65535 * currentY / GetSystemMetrics(SM_CYSCREEN); \
		if(moveX > 0) \
		{ \
			currentX += incX; \
			if(currentX <= winRect.left + x + 16 - moveX \
				|| currentX >= winRect.left + x + 16 + moveX) \
				incX = -incX; \
		} \
		if(moveY > 0) \
		{ \
			currentY += incY; \
			if(currentY <= winRect.top + y + 16 - moveY \
				|| currentY >= winRect.top + y + 16 + moveY) \
				incY = -incY; \
		} \
	} \
	if(!SendInput(4, inputs, sizeof(INPUT))) \
		THROW_W32("Could not inject mouse events"); \
}

#else

Bool deadYet = False;

void handler(int type)
{
	deadYet = true;
}

#define MOVE_MOUSE() \
{ \
	XTestFakeMotionEvent(wh.dpy, DefaultScreen(wh.dpy), currentX, currentY, \
		CurrentTime); \
	if(moveX > 0) \
	{ \
		currentX += incX; \
		if(currentX <= rootx + 16 - moveX \
			|| currentX >= rootx + 16 + moveX) \
			incX = -incX; \
	} \
	if(moveY > 0) \
	{ \
		currentY += incY; \
		if(currentY <= rooty + 16 - moveY \
			|| currentY >= rooty + 16 + moveY) \
			incY = -incY; \
	} \
}

#endif


void usage(void)
{
	printf("\n");
	printf("USAGE: %s [options]\n\n", program_name);
	printf("Options:\n");
	printf("-lb = Simulate holding down the left mouse button while the benchmark is\n");
	printf("      running\n");
	printf("-mb = Simulate holding down the middle mouse button while the benchmark is\n");
	printf("      running\n");
	printf("-mx <p> = Simulate continuous horizontal mouse movement while the benchmark\n");
	printf("          is running (<p> specifies the maximum range of motion, in pixels,\n");
	printf("          relative to the center of the sampling block)\n");
	printf("-my <p> = Simulate continuous vertical mouse movement while the benchmark\n");
	printf("          is running (<p> specifies the maximum range of motion, in pixels,\n");
	printf("          relative to the center of the sampling block)\n");
	printf("-rb = Simulate holding down the right mouse button while the benchmark is\n");
	printf("      running\n");
	printf("-s <s> = Sample the window <s> times per second (default: %d)\n",
		DEFSAMPLERATE);
	printf("-t <t> = Run the benchmark for <t> seconds (default: %.1f)\n",
		DEFBENCHTIME);
	printf("-wh <wh> = Explicitly specify a window, if auto-detect fails\n");
	printf("           (<wh> is the window handle in hex)\n");
	printf("-x <x> = specify x offset of the sampling block, relative to the window\n");
	printf("-y <y> = specify y offset of the sampling block, relative to the window\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int i;  fbx_wh wh;
	double benchTime = DEFBENCHTIME, elapsed;  Timer timer;
	int sampleRate = DEFSAMPLERATE, x = -1, y = -1;
	#ifdef _WIN32
	INPUT inputs[4];
	RECT winRect;
	DWORD downFlags = 0, upFlags = 0;
	#else
	unsigned int button = 0;
	int rootx, rooty;
	#endif
	int moveX = 0, moveY = 0, incX = -1, incY = -1, currentX = 0, currentY = 0;

	program_name = argv[0];
	memset(&wh, 0, sizeof(wh));

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage();
		else if(!stricmp(argv[i], "-t") && i < argc - 1)
		{
			if((benchTime = atof(argv[++i])) <= 0.0) usage();
		}
		else if(!stricmp(argv[i], "-s") && i < argc - 1)
		{
			if((sampleRate = atoi(argv[++i])) < 1) usage();
		}
		else if(!stricmp(argv[i], "-x") && i < argc - 1)
		{
			if((x = atoi(argv[++i])) < 0) usage();
		}
		else if(!stricmp(argv[i], "-y") && i < argc - 1)
		{
			if((y = atoi(argv[++i])) < 0) usage();
		}
		else if(!stricmp(argv[i], "-wh") && i < argc - 1)
		{
			#ifdef _WIN32
			size_t temp = 0;
			if(sscanf(argv[++i], "%zx", &temp) < 1 || temp <= 0) usage();
			wh = (fbx_wh)temp;
			#else
			unsigned int temp = 0;
			if(sscanf(argv[++i], "%x", &temp) < 1 || temp <= 0) usage();
			wh.d = (Drawable)temp;
			#endif
		}
		else if(!stricmp(argv[i], "-mx") && i < argc - 1)
		{
			if(sscanf(argv[++i], "%d", &moveX) < 1 || moveX <= 0) usage();
		}
		else if(!stricmp(argv[i], "-my") && i < argc - 1)
		{
			if(sscanf(argv[++i], "%d", &moveY) < 1 || moveY <= 0) usage();
		}
		else if(!stricmp(argv[i], "-lb"))
		{
			#ifdef _WIN32
			downFlags |= MOUSEEVENTF_LEFTDOWN;
			upFlags |= MOUSEEVENTF_LEFTUP;
			#else
			button = 1;
			#endif
		}
		else if(!stricmp(argv[i], "-mb"))
		{
			#ifdef _WIN32
			downFlags |= MOUSEEVENTF_MIDDLEDOWN;
			upFlags |= MOUSEEVENTF_MIDDLEUP;
			#else
			button = 2;
			#endif
		}
		else if(!stricmp(argv[i], "-rb"))
		{
			#ifdef _WIN32
			downFlags |= MOUSEEVENTF_RIGHTDOWN;
			upFlags |= MOUSEEVENTF_RIGHTUP;
			#else
			button = 3;
			#endif
		}
		else usage();
	}

	printf("\nThin Client Benchmark\n");

	#ifdef _WIN32

	if(!wh)
	{
		printf("Click the mouse in the window that you wish to monitor ... ");
		wh = GetForegroundWindow();
		int t, tOld = -1;
		timer.start();
		do
		{
			elapsed = timer.elapsed();
			t = (int)(10. - elapsed);
			if(t != tOld) { tOld = t;  printf("%.2d\b\b", t); }
			Sleep(50);
		} while(wh == GetForegroundWindow() && elapsed < 10.);
		if((wh = GetForegroundWindow()) == 0)
			THROW_W32("Could not get foreground window")
		FlashWindow(wh, TRUE);
		printf("  \n");
	}
	char temps[1024];
	GetWindowText(wh, temps, 1024);
	printf("Monitoring window 0x%.8zx (%s)\n", (size_t)wh, temps);

	#else

	signal(SIGINT, handler);
	signal(SIGTERM, handler);
	signal(SIGHUP, handler);

	if(!XInitThreads())
	{
		fprintf(stderr, "XInitThreads() failed\n");
		exit(1);
	}
	if(!(wh.dpy = XOpenDisplay(0)))
	{
		fprintf(stderr, "Could not open display %s\n", XDisplayName(0));
		exit(1);
	}
	if(moveX > 0 || moveY > 0)
	{
		int dummy;
		if(!XTestQueryExtension(wh.dpy, &dummy, &dummy, &dummy, &dummy))
		{
			fprintf(stderr, "XTEST extension not available\n");
			exit(1);
		}
	}
	if(!wh.d)
	{
		printf("Click the mouse in the window that you wish to monitor ...\n");
		wh.d = Select_Window(wh.dpy);
		XSetInputFocus(wh.dpy, wh.d, RevertToNone, CurrentTime);
	}

	#endif

	fbx_struct fb;
	memset(&fb, 0, sizeof(fb));
	TRY_FBX(fbx_init(&fb, wh, 0, 0, 1));
	int width = fb.width, height = fb.height;
	fbx_term(&fb);
	memset(&fb, 0, sizeof(fb));
	TRY_FBX(fbx_init(&fb, wh, 32, 32, 1));

	int frames = 0, samples = 0;
	if(x < 0) x = width / 2 - 16;
	if(x < 0) x = 0;
	if(y < 0) y = height / 2 - 16;
	if(y < 0) y = 0;
	unsigned char buf[32 * 32 * 4];
	int first = 1;

	printf("Sample block location: %d, %d\n", x, y);
	#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	#endif

	if(moveX > 0)
		printf("Simulating horizontal mouse movement +/- %d pixels\n", moveX);
	if(moveY > 0)
		printf("Simulating vertical mouse movement +/- %d pixels\n", moveY);
	if(moveX > 0 || moveY > 0)
	{
		#ifdef _WIN32
		memset(inputs, 0, sizeof(INPUT) * 4);
		if(!GetWindowRect(wh, &winRect))
			THROW_W32("Could not get window rectangle");
		currentX = winRect.left + x + (moveY > 0 ? 40 : 16);
		currentY = winRect.top + y + (moveX > 0 ? 40 : 16);
		#else
		Window child;
		XTranslateCoordinates(wh.dpy, wh.d, DefaultRootWindow(wh.dpy), x, y,
			&rootx, &rooty, &child);
		currentX = rootx + (moveY > 0 ? 40 : 16);
		currentY = rooty + (moveX > 0 ? 40 : 16);
		if(button > 0) XTestFakeButtonEvent(wh.dpy, button, True, CurrentTime);
		#endif
		MOVE_MOUSE();
	}
	timer.start();
	do
	{
		TRY_FBX(fbx_read(&fb, x, y));
		if(moveX > 0 || moveY > 0)
			MOVE_MOUSE();
		samples++;
		if(first)
		{
			first = 0;  frames++;
		}
		else
		{
			if(memcmp(buf, fb.bits, fb.pf->size * 32 * 32)) frames++;
		}
		memcpy(buf, fb.bits, fb.pf->size * 32 * 32);
		elapsed = timer.elapsed();
		#ifdef _WIN32
		int sleepTime =
			(int)(1000. * ((double)samples / (double)sampleRate - elapsed));
		if(sleepTime > 0) Sleep(sleepTime);
		#else
		int sleepTime =
			(int)(1000000. * ((double)samples / (double)sampleRate - elapsed));
		if(sleepTime > 0) usleep(sleepTime);
		if(deadYet) break;
		#endif
	} while((elapsed = timer.elapsed()) < benchTime);
	#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	inputs[3].mi.dwFlags = upFlags;
	SendInput(1, &inputs[3], sizeof(INPUT));
	#else
	if((moveX > 0 || moveY > 0) && button > 0)
	{
		XTestFakeButtonEvent(wh.dpy, button, False, CurrentTime);
		XSync(wh.dpy, False);
	}
	#endif

	printf("Samples: %d  Frames: %d  Time: %f s  Frames/sec: %f\n", samples,
		frames, elapsed, (double)frames / elapsed);

	return 0;
}
