// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2010, 2014, 2017-2021 D. R. Commander
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

#include "X11Trans.h"
#include "vglutil.h"
#include "Timer.h"
#include "fakerconfig.h"

using namespace util;
using namespace common;
using namespace server;


extern "C" void _vgl_disableFaker(void) {}
extern "C" void _vgl_enableFaker(void) {}


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-sync = Test synchronous blitting code\n");
	fprintf(stderr, "-bottomup = Test bottom-up blitting code\n");
	fprintf(stderr, "\n");
	exit(1);
}


void fillFrame(unsigned char *buf, int width, int pitch, int height, PF *pf,
	int on)
{
	unsigned char *ptr;  unsigned char pixel[4];
	int maxRGB = (1 << pf->bpc);

	ptr = buf;
	for(int i = 0; i < height; i++, ptr += pitch)
	{
		if(on)
		{
			pf->setRGB(pixel, i % maxRGB, maxRGB - 1 - (i % maxRGB), i % maxRGB);
		}
		else
		{
			pf->setRGB(pixel, i % maxRGB, 0, maxRGB - 1 - (i % maxRGB));
		}
		for(int j = 0; j < width; j++) memcpy(&ptr[pf->size * j], pixel, pf->size);
	}
}


int main(int argc, char **argv)
{
	X11Trans trans;  Timer timer;  double elapsed;
	Display *dpy;  Window win;
	int WIDTH = 700, HEIGHT = 700;
	bool dosync = false, bottomup = false;

	try
	{
		if(argc > 1) for(int i = 1; i < argc; i++)
		{
			if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
			else if(!stricmp(argv[i], "-sync")) dosync = true;
			else if(!stricmp(argv[i], "-bottomup")) bottomup = true;
			else usage(argv);
		}

		if(!XInitThreads())
			THROW("XInitThreads failed");
		if(!(dpy = XOpenDisplay(0)))
			THROW("Could not open display");
		if(!(win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, WIDTH,
			HEIGHT, 0, WhitePixel(dpy, DefaultScreen(dpy)),
			BlackPixel(dpy, DefaultScreen(dpy)))))
			THROW("Could not create window");
		ERRIFNOT(XMapRaised(dpy, win));

		FBXFrame *f;

		fprintf(stderr, "\nTesting full-frame blits ...\n");

		int fill = 0, frames = 0;  timer.start();
		do
		{
			trans.synchronize();
			ERRIFNOT(f = trans.getFrame(dpy, win, WIDTH, HEIGHT));
			WIDTH = f->hdr.framew;  HEIGHT = f->hdr.frameh;
			fillFrame(f->bits, WIDTH, f->pitch, HEIGHT, f->pf, fill);
			if(bottomup) f->flags |= FRAME_BOTTOMUP;
			fill = 1 - fill;
			trans.sendFrame(f, dosync);
			frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		fprintf(stderr, "%f Megapixels/sec\n",
			(double)WIDTH * (double)HEIGHT * (double)frames / 1000000. / elapsed);

		fprintf(stderr, "\nTesting full-frame blits (spoiling) ...\n");

		fill = 0, frames = 0;  int clientframes = 0;  timer.start();
		do
		{
			ERRIFNOT(f = trans.getFrame(dpy, win, WIDTH, HEIGHT));
			WIDTH = f->hdr.framew;  HEIGHT = f->hdr.frameh;
			fillFrame(f->bits, WIDTH, f->pitch, HEIGHT, f->pf, fill);
			if(bottomup) f->flags |= FRAME_BOTTOMUP;
			fill = 1 - fill;
			trans.sendFrame(f, dosync);
			clientframes++;  frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		fprintf(stderr, "%f Megapixels/sec (server)\n",
			(double)WIDTH * (double)HEIGHT * (double)frames / 1000000. /
				elapsed);
		fprintf(stderr, "%f Megapixels/sec (client)\n",
			(double)WIDTH * (double)HEIGHT * (double)clientframes / 1000000. /
				elapsed);

		fprintf(stderr, "\nTesting half-frame blits ...\n");

		fill = 0, frames = 0;  timer.start();
		do
		{
			trans.synchronize();
			ERRIFNOT(f = trans.getFrame(dpy, win, WIDTH, HEIGHT));
			WIDTH = f->hdr.framew;  HEIGHT = f->hdr.frameh;
			memset(f->bits, 0, f->pitch * HEIGHT);
			fillFrame(f->bits, WIDTH, f->pitch, HEIGHT / 2, f->pf, fill);
			if(bottomup) f->flags |= FRAME_BOTTOMUP;
			fill = 1 - fill;
			trans.sendFrame(f, dosync);
			frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		fprintf(stderr, "%f Megapixels/sec\n",
			(double)WIDTH * (double)HEIGHT * (double)frames / 1000000. / elapsed);

		fprintf(stderr, "\nTesting zero-frame blits ...\n");

		frames = 0;  timer.start();
		do
		{
			trans.synchronize();
			ERRIFNOT(f = trans.getFrame(dpy, win, WIDTH, HEIGHT));
			WIDTH = f->hdr.framew;  HEIGHT = f->hdr.frameh;
			fillFrame(f->bits, WIDTH, f->pitch, HEIGHT / 2, f->pf, 1);
			if(bottomup) f->flags |= FRAME_BOTTOMUP;
			trans.sendFrame(f, dosync);
			frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		fprintf(stderr, "%f Megapixels/sec\n",
			(double)WIDTH * (double)HEIGHT * (double)frames / 1000000. / elapsed);
	}
	catch(std::exception &e)
	{
		fprintf(stderr, "%s--\n%s\n", GET_METHOD(e), e.what());
		return -1;
	}
	return 0;
}
