/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2010 D. R. Commander
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

#include "x11trans.h"
#include "rrutil.h"
#include "rrtimer.h"
#include "fakerconfig.h"


void usage(char **argv)
{
	printf("\nUSAGE: %s [-tilesize <n>] [-sync] [-bottomup]\n", argv[0]);
	printf("\n");
	printf("-tilesize = width/height of each inter-frame difference tile\n");
	printf("            [default = %d x %d pixels]\n", (int)fconfig.tilesize,
		(int)fconfig.tilesize);
	printf("-sync = Test synchronous blitting code\n");
	printf("-bottomup = Test bottom-up blitting code\n");
	printf("-nodbe = Disable use of the DOUBLE-BUFFER extension\n");
	printf("\n");
	exit(1);
}


void fillframe(unsigned char *buf, int w, int pitch, int h, int ps, int on)
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
	x11trans x11t;  rrtimer t;  double elapsed;
	Display *dpy;  Window win;
	int WIDTH=700, HEIGHT=700;
	bool dosync=false, bottomup=false;

	try
	{
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

		rrfb *f;

		fprintf(stderr, "\nTesting full-frame blits ...\n");

		int fill=0, frames=0;  t.start();
		do
		{
			x11t.synchronize();
			errifnot(f=x11t.getframe(dpy, win, WIDTH, HEIGHT));
			WIDTH=f->_h.framew;  HEIGHT=f->_h.frameh;
			fillframe(f->_bits, WIDTH, f->_pitch, HEIGHT, f->_pixelsize, fill);
			if(bottomup) {f->_flags|=RRFRAME_BOTTOMUP;}
			fill=1-fill;
			x11t.sendframe(f, dosync);
			frames++;
		} while((elapsed=t.elapsed())<2.);

		fprintf(stderr, "%f Megapixels/sec\n",
			(double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

		fprintf(stderr, "\nTesting full-frame blits (spoiling) ...\n");

		fill=0, frames=0;  int clientframes=0;  t.start();
		do
		{
			errifnot(f=x11t.getframe(dpy, win, WIDTH, HEIGHT));
			WIDTH=f->_h.framew;  HEIGHT=f->_h.frameh;
			fillframe(f->_bits, WIDTH, f->_pitch, HEIGHT, f->_pixelsize, fill);
			if(bottomup) {f->_flags|=RRFRAME_BOTTOMUP;}
			fill=1-fill;
			x11t.sendframe(f, dosync);
			clientframes++;  frames++;
		} while((elapsed=t.elapsed())<2.);

		fprintf(stderr, "%f Megapixels/sec (server)\n",
			(double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);
		fprintf(stderr, "%f Megapixels/sec (client)\n",
			(double)WIDTH*(double)HEIGHT*(double)clientframes/1000000./elapsed);

		fprintf(stderr, "\nTesting half-frame blits ...\n");

		fill=0, frames=0;  t.start();
		do
		{
			x11t.synchronize();
			errifnot(f=x11t.getframe(dpy, win, WIDTH, HEIGHT));
			WIDTH=f->_h.framew;  HEIGHT=f->_h.frameh;
			memset(f->_bits, 0, f->_pitch*HEIGHT);
			fillframe(f->_bits, WIDTH, f->_pitch, HEIGHT/2, f->_pixelsize, fill);
			if(bottomup) {f->_flags|=RRFRAME_BOTTOMUP;}
			fill=1-fill;
			x11t.sendframe(f, dosync);
			frames++;
		} while((elapsed=t.elapsed())<2.);

		fprintf(stderr, "%f Megapixels/sec\n",
			(double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

		fprintf(stderr, "\nTesting zero-frame blits ...\n");

		frames=0;  t.start();
		do
		{
			x11t.synchronize();
			errifnot(f=x11t.getframe(dpy, win, WIDTH, HEIGHT));
			WIDTH=f->_h.framew;  HEIGHT=f->_h.frameh;
			fillframe(f->_bits, WIDTH, f->_pitch, HEIGHT/2, f->_pixelsize, 1);
			if(bottomup) {f->_flags|=RRFRAME_BOTTOMUP;}
			x11t.sendframe(f, dosync);
			frames++;
		} while((elapsed=t.elapsed())<2.);

		fprintf(stderr, "%f Megapixels/sec\n",
			(double)WIDTH*(double)HEIGHT*(double)frames/1000000./elapsed);

	}
	catch(rrerror &e)
	{
		fprintf(stderr, "%s--\n%s\n", e.getMethod(), e.getMessage());
	}
}
