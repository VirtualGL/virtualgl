/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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
#define __FAKERCONFIG_STATICDEF__
#include "fakerconfig.h"

void usage(char **argv)
{
	printf("\nUSAGE: %s [-tilesize <n>] [-sync] [-bottomup]\n", argv[0]);
	printf("\n");
	printf("-tilesize = width/height of each inter-frame difference tile\n");
	printf("            [default = %d x %d pixels]\n", (int)fconfig.tilesize, (int)fconfig.tilesize);
	printf("-sync = Test synchronous blitting code\n");
	printf("-bottomup = Test bottom-up blitting code\n");
	printf("-nodbe = Disable use of the DOUBLE-BUFFER extension\n");
	printf("\n");
	exit(1);
}

void fillbmp(unsigned char *buf, int w, int pitch, int h, int ps, int on)
{
	unsigned char *ptr;  unsigned char pixel[3];
	ptr=buf;
	for(int i=0; i<h; i++, ptr+=pitch)
	{
		if(on) {pixel[0]=pixel[2]=i%256;  pixel[1]=255-(i%256);}
		else {pixel[0]=i%256;  pixel[1]=0;  pixel[2]=255-(i%256);}
		for(int j=0; j<w; j++) memcpy(&ptr[ps*j], pixel, 3);
	}
}

int main(int argc, char **argv)
{
	rrblitter blitter;  rrtimer t;  double elapsed;
	Display *dpy;  Window win;
	int WIDTH=700, HEIGHT=700;
	bool dosync=false, bottomup=false;

	try {

	if(argc>1)
	{
		for(int i=1; i<argc; i++)
		{
			if(!stricmp(argv[i], "-h")) usage(argv);
			if(!stricmp(argv[i], "-sync")) dosync=true;
			if(!stricmp(argv[i], "-bottomup")) bottomup=true;
			if(!stricmp(argv[i], "-tilesize") && i<argc-1)
			{
				fconfig.tilesize=atoi(argv[i+1]);  i++;
			}
		}
	}

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
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT, false));
		WIDTH=b->_h.framew;  HEIGHT=b->_h.frameh;
		fillbmp(b->_bits, WIDTH, b->_pitch, HEIGHT, b->_pixelsize, fill);
		if(bottomup) {b->_flags|=RRBMP_BOTTOMUP;}
		fill=1-fill;
		blitter.sendframe(b, dosync);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	fprintf(stderr, "\nTesting full-frame blits (spoiling) ...\n");

	fill=0, frames=0;  int clientframes=0;  t.start();
	do
	{
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT, true));
		WIDTH=b->_h.framew;  HEIGHT=b->_h.frameh;
		fillbmp(b->_bits, WIDTH, b->_pitch, HEIGHT, b->_pixelsize, fill);
		if(bottomup) {b->_flags|=RRBMP_BOTTOMUP;}
		fill=1-fill;
		blitter.sendframe(b, dosync);
		clientframes++;  frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec (server)\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);
	fprintf(stderr, "%f Megapixels/sec (client)\n", (double)WIDTH*(double)HEIGHT*(double)clientframes/1000000./elapsed);

	fprintf(stderr, "\nTesting half-frame blits ...\n");

	fill=0, frames=0;  t.start();
	do
	{
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT, false));
		WIDTH=b->_h.framew;  HEIGHT=b->_h.frameh;
		memset(b->_bits, 0, b->_pitch*HEIGHT);
		fillbmp(b->_bits, WIDTH, b->_pitch, HEIGHT/2, b->_pixelsize, fill);
		if(bottomup) {b->_flags|=RRBMP_BOTTOMUP;}
		fill=1-fill;
		blitter.sendframe(b, dosync);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	fprintf(stderr, "\nTesting zero-frame blits ...\n");

	frames=0;  t.start();
	do
	{
		errifnot(b=blitter.getbitmap(dpy, win, WIDTH, HEIGHT, false));
		WIDTH=b->_h.framew;  HEIGHT=b->_h.frameh;
		fillbmp(b->_bits, WIDTH, b->_pitch, HEIGHT/2, b->_pixelsize, 1);
		if(bottomup) {b->_flags|=RRBMP_BOTTOMUP;}
		blitter.sendframe(b, dosync);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	fprintf(stderr, "%f Megapixels/sec\n", (double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	} catch(rrerror &e) {fprintf(stderr, "%s--\n%s\n", e.getMethod(), e.getMessage());}
}
