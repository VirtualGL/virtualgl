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

#include "rrblitter.h"
#include "rrtimer.h"

#define WIDTH 700
#define HEIGHT 700

int main(void)
{
	rrblitter blitter;  rrtimer t;  double elapsed;
	Display *dpy;  Window win;

	try {

	if(!XInitThreads()) _throw("XInitThreads failed");
	if(!(dpy=XOpenDisplay(0))) _throw("Could not open display");
	if(!(win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
		WIDTH, HEIGHT, 0, WhitePixel(dpy, DefaultScreen(dpy)),
		BlackPixel(dpy, DefaultScreen(dpy))))) _throw("Could not create window");
	errifnot(XMapRaised(dpy, win));

	rrfb *b;

	fprintf(stderr, "\nTesting full-frame blits ...\n");

	int fill=0, frames=0;  t.start();
	do
	{
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT));
		memset(b->bits, fill*0xff, b->pitch*HEIGHT);
		fill=1-fill;
		blitter.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	fprintf(stderr, "\nTesting full-frame blits (spoiling) ...\n");

	fill=0, frames=0;  int clientframes=0;  t.start();
	do
	{
		if(!blitter.frameready())
		{
			memset(b->bits, fill*0xff, b->pitch*HEIGHT);
			fill=1-fill;
			frames++;  continue;
		}
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT));
		memset(b->bits, fill*0xff, b->pitch*HEIGHT);
		fill=1-fill;
		blitter.sendframe(b);
		clientframes++;  frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec (server)\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);
	fprintf(stderr, "%f Megapixels/sec (client)\n", (double)WIDTH*(double)HEIGHT*(double)clientframes/1000000./elapsed);

	fprintf(stderr, "\nTesting half-frame blits ...\n");

	fill=0, frames=0;  t.start();
	do
	{
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT));
		memset(b->bits, 0, b->pitch*HEIGHT);
		memset(b->bits, fill*0xff, b->pitch*HEIGHT/2);
		fill=1-fill;
		blitter.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	fprintf(stderr, "\nTesting zero-frame blits ...\n");

	frames=0;  t.start();
	do
	{
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT));
		memset(b->bits, 0xff, b->pitch*HEIGHT);
		blitter.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	} catch(rrerror &e) {fprintf(stderr, "%s--\n%s\n", e.getMethod(), e.getMessage());}
}
