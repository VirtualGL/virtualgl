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

#include "rrdisplayclient.h"
#include "rrtimer.h"
#include "bmp.h"
#include "fakerconfig.h"

FakerConfig fconfig;

void usage(char **argv)
{
	printf("\nUSAGE: %s <bitmap file>\n", argv[0]);
	printf("       [-client <machine:x.x>] [-samp <n>] [-qual <n>]\n");
	printf("       [-tilesize <n>] [-ssl] [-np <n>]\n\n");
	printf("-client = X Display where the video should be sent (VGL client must be running\n");
	printf("          on that machine) or 0 for local test only\n");
	printf("          [default = %s]\n", fconfig.client? fconfig.client:"read from DISPLAY environment");
	printf("-samp = JPEG YCbCr subsampling: 411, 422, or 444 [default = %d]\n", fconfig.currentsubsamp);
	printf("-qual = JPEG quality, 1-100 inclusive [default = %d]\n", fconfig.currentqual);
	printf("-tilesize = width/height of each inter-frame difference tile\n");
	printf("            [default = %d x %d pixels]\n", fconfig.tilesize, fconfig.tilesize);
	printf("-ssl = use SSL tunnel [default = %s]\n", fconfig.ssl? "On":"Off");
	printf("-np <n> = number of processors to use for compression [default = %d]\n", fconfig.np);
	printf("\n");
	exit(1);
}

int main(int argc, char **argv)
{
	rrtimer t;  double elapsed;
	unsigned char *buf=NULL, *buf2=NULL, *buf3=NULL;
	Display *dpy=NULL;  Window win=0;
	int i;  int bgr=littleendian();

	try {

	bool localtest=false;
	if(argc<2) usage(argv);

	for(i=0; i<argc; i++)
	{
		if(!stricmp(argv[i], "-ssl")) fconfig.ssl=true;
		if(!strnicmp(argv[i], "-cl", 3) && i<argc-1)
		{
			fconfig.client=argv[i+1];  i++;
			if(!stricmp(fconfig.client, "0")) {localtest=true;  fconfig.client=NULL;}
		}
		if(!strnicmp(argv[i], "-sa", 3) && i<argc-1)
		{
			int subsamp=atoi(argv[i+1]);  i++;
			switch(subsamp)
			{
				case 411: fconfig.currentsubsamp=RR_411;  break;
				case 422: fconfig.currentsubsamp=RR_422;  break;
				case 444: fconfig.currentsubsamp=RR_444;  break;
				default: usage(argv);
			}
		}
		if(!strnicmp(argv[i], "-q", 2) && i<argc-1)
		{
			fconfig.currentqual=atoi(argv[i+1]);  i++;
		}
		if(!stricmp(argv[i], "-tilesize") && i<argc-1)
		{
			fconfig.tilesize=atoi(argv[i+1]);  i++;
		}
		if(!stricmp(argv[i], "-np") && i<argc-1)
		{
			fconfig.np=atoi(argv[i+1]);  i++;
		}
	}
	fconfig.sanitycheck();

	int w, h, d=3;

	if(loadbmp(argv[1], &buf, &w, &h, bgr?BMP_BGR:BMP_RGB, 1, 0)==-1) _throw(bmpgeterr());
	if(loadbmp(argv[1], &buf2, &w, &h, bgr?BMP_BGR:BMP_RGB, 1, 0)==-1) _throw(bmpgeterr());
	if(loadbmp(argv[1], &buf3, &w, &h, bgr?BMP_BGR:BMP_RGB, 1, 0)==-1) _throw(bmpgeterr());
	printf("Source image: %d x %d x %d-bit\n", w, h, d*8);

	if(!localtest)
	{
		if(!XInitThreads()) _throw("Could not initialize X threads");
		if((dpy=XOpenDisplay(0))==NULL) _throw("Could not open display");
		if((win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
			w, h, 0, WhitePixel(dpy, DefaultScreen(dpy)),
			BlackPixel(dpy, DefaultScreen(dpy))))==0) _throw("Could not create window");
		printf("Creating window %lu\n", (unsigned long)win);
		errifnot(XMapRaised(dpy, win));
		XSync(dpy, False);
		if(!fconfig.client) fconfig.client=DisplayString(dpy);
	}

	printf("Tile size = %d x %d pixels\n", fconfig.tilesize, fconfig.tilesize);

	rrdisplayclient rrdpy(fconfig.client);

	int i;
	for(i=0; i<w*h*d; i++) buf2[i]=255-buf2[i];
	for(i=0; i<w*h*d/2; i++) buf3[i]=255-buf3[i];

	rrframe *b;

	printf("\nTesting full-frame send ...\n");

	int frames=0, fill=0;  t.start();
	do
	{
		errifnot(b=rrdpy.getbitmap(w, h, d));
		if(fill) memcpy(b->_bits, buf, w*h*d);
		else memcpy(b->_bits, buf2, w*h*d);
		b->_h.qual=fconfig.currentqual;  b->_h.subsamp=fconfig.currentsubsamp;
		b->_h.winid=win;
		if(b->_flags&RRBMP_BGR && !bgr) b->_flags&=(~RRBMP_BGR);
		fill=1-fill;
		rrdpy.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	printf("%f Megapixels/sec\n", (double)w*(double)h*(double)frames/1000000./elapsed);

	printf("\nTesting full-frame send (spoiling) ...\n");

	fill=0, frames=0;  int clientframes=0;  t.start();
	do
	{
		if(!rrdpy.frameready())
		{
			if(fill) memcpy(b->_bits, buf, w*h*d);
			else memcpy(b->_bits, buf2, w*h*d);
			fill=1-fill;
			frames++;  continue;
		}
		errifnot(b=rrdpy.getbitmap(w, h, d));
		if(fill) memcpy(b->_bits, buf, w*h*d);
		else memcpy(b->_bits, buf2, w*h*d);
		b->_h.qual=fconfig.currentqual;  b->_h.subsamp=fconfig.currentsubsamp;
		b->_h.winid=win;
		fill=1-fill;
		rrdpy.sendframe(b);
		clientframes++;  frames++;
	} while((elapsed=t.elapsed())<2.);

	printf("%f Megapixels/sec (server)\n", (double)w*(double)h*(double)frames/1000000./elapsed);
	printf("%f Megapixels/sec (client)\n", (double)w*(double)h*(double)clientframes/1000000./elapsed);

	printf("\nTesting half-frame send ...\n");

	fill=0, frames=0;  t.start();
	do
	{
		errifnot(b=rrdpy.getbitmap(w, h, d));
		if(fill) memcpy(b->_bits, buf, w*h*d);
		else memcpy(b->_bits, buf3, w*h*d);
		b->_h.qual=fconfig.currentqual;  b->_h.subsamp=fconfig.currentsubsamp;
		b->_h.winid=win;
		fill=1-fill;
		rrdpy.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	printf("%f Megapixels/sec\n", (double)w*(double)h*(double)frames/1000000./elapsed);

	printf("\nTesting zero-frame send ...\n");

	frames=0;  t.start();
	do
	{
		errifnot(b=rrdpy.getbitmap(w, h, d));
		memcpy(b->_bits, buf, w*h*d);
		b->_h.qual=fconfig.currentqual;  b->_h.subsamp=fconfig.currentsubsamp;
		b->_h.winid=win;
		rrdpy.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	printf("%f Megapixels/sec\n", (double)w*(double)h*(double)frames/1000000./elapsed);

	} catch(rrerror &e) {printf("%s--\n%s\n", e.getMethod(), e.getMessage());}

	if(win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
	if(buf) free(buf);  if(buf2) free(buf2);  if(buf3) free(buf3);
}
