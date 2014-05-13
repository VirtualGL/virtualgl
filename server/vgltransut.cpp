/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2009-2011, 2014 D. R. Commander
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

#include "VGLTrans.h"
#include "vglutil.h"
#include "Timer.h"
#include "bmp.h"
#include "fakerconfig.h"

using namespace vglutil;
using namespace vglcommon;
using namespace vglserver;


void usage(char **argv)
{
	printf("\nUSAGE: %s <bitmap file>\n", argv[0]);
	printf("       [-client <machine:x.x>] [-samp <n>] [-qual <n>]\n");
	printf("       [-tilesize <n>] [-np <n>] [-rgb]");
	#ifdef USESSL
	printf(" [-ssl]");
	#endif
	printf("\n\n");
	printf("-client = X Display where the video should be sent (VGL client must be running\n");
	printf("          on that machine) or 0 for local test only\n");
	printf("          [default = %s]\n", strlen(fconfig.client)>0? fconfig.client:"read from DISPLAY environment");
	printf("-samp = JPEG chrominance subsampling factor: 0 (gray), 1, 2, or 4\n");
	printf("        [default = %d]\n", fconfig.subsamp);
	printf("-qual = JPEG quality, 1-100 inclusive [default = %d]\n", fconfig.qual);
	printf("-tilesize = width/height of each inter-frame difference tile\n");
	printf("            [default = %d x %d pixels]\n", fconfig.tilesize, fconfig.tilesize);
	printf("-rgb = Use RGB (uncompressed) encoding (default is JPEG)\n");
	#ifdef USESSL
	printf("-ssl = use SSL tunnel [default = %s]\n", fconfig.ssl? "On":"Off");
	#endif
	printf("-np <n> = number of processors to use for compression [default = %d]\n", fconfig.np);
	printf("\n");
	exit(1);
}


int main(int argc, char **argv)
{
	Timer timer;  double elapsed;
	unsigned char *buf=NULL, *buf2=NULL, *buf3=NULL;
	Display *dpy=NULL;  Window win=0;
	int i;  int bgr=littleendian();

	try
	{
		fconfig_setcompress(fconfig, RRCOMP_JPEG);

		bool localtest=false;
		if(argc<2) usage(argv);

		for(i=0; i<argc; i++)
		{
			#ifdef USESSL
			if(!stricmp(argv[i], "-ssl")) fconfig.ssl=1;
			#endif
			if(!strnicmp(argv[i], "-cl", 3) && i<argc-1)
			{
				strncpy(fconfig.client, argv[i+1], MAXSTR-1);  i++;
				if(!stricmp(fconfig.client, "0"))
				{
					localtest=true;  fconfig.client[0]=0;
				}
			}
			if(!strnicmp(argv[i], "-sa", 3) && i<argc-1)
			{
				fconfig.subsamp=atoi(argv[i+1]);  i++;
			}
			if(!strnicmp(argv[i], "-q", 2) && i<argc-1)
			{
				fconfig.qual=atoi(argv[i+1]);  i++;
			}
			if(!stricmp(argv[i], "-tilesize") && i<argc-1)
			{
				fconfig.tilesize=atoi(argv[i+1]);  i++;
			}
			if(!stricmp(argv[i], "-np") && i<argc-1)
			{
				fconfig.np=atoi(argv[i+1]);  i++;
			}
			if(!stricmp(argv[i], "-rgb")) fconfig_setcompress(fconfig, RRCOMP_RGB);
		}
		if(fconfig.compress==RRCOMP_RGB) bgr=0;

		int w, h, d=3;

		if(bmp_load(argv[1], &buf, &w, 1, &h, bgr? BMPPF_BGR:BMPPF_RGB,
			BMPORN_TOPDOWN)==-1)
			_throw(bmp_geterr());
		if(bmp_load(argv[1], &buf2, &w, 1, &h, bgr? BMPPF_BGR:BMPPF_RGB,
			BMPORN_TOPDOWN)==-1)
			_throw(bmp_geterr());
		if(bmp_load(argv[1], &buf3, &w, 1, &h, bgr? BMPPF_BGR:BMPPF_RGB,
			BMPORN_TOPDOWN)==-1)
			_throw(bmp_geterr());
		printf("Source image: %d x %d x %d-bit\n", w, h, d*8);

		if(!localtest)
		{
			if(!XInitThreads()) _throw("Could not initialize X threads");
			if((dpy=XOpenDisplay(0))==NULL) _throw("Could not open display");
			if((win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
				w, h, 0, WhitePixel(dpy, DefaultScreen(dpy)),
				BlackPixel(dpy, DefaultScreen(dpy))))==0)
				_throw("Could not create window");
			printf("Creating window %lu\n", (unsigned long)win);
			_errifnot(XMapRaised(dpy, win));
			XSync(dpy, False);
			if(strlen(fconfig.client)==0)
				strncpy(fconfig.client, DisplayString(dpy), MAXSTR-1);
			fconfig_setdefaultsfromdpy(dpy);
		}

		printf("Tile size = %d x %d pixels\n", fconfig.tilesize,
			fconfig.tilesize);

		VGLTrans vglconn;
		if(!localtest) vglconn.connect(fconfig.client, fconfig.port);

		int i;
		for(i=0; i<w*h*d; i++) buf2[i]=255-buf2[i];
		for(i=0; i<w*h*d/2; i++) buf3[i]=255-buf3[i];

		Frame *f;

		printf("\nTesting full-frame send ...\n");

		int frames=0, fill=0;  timer.start();
		do
		{
			vglconn.synchronize();
			_errifnot(f=vglconn.getFrame(w, h, d, bgr?FRAME_BGR:0, false));
			if(fill) memcpy(f->bits, buf, w*h*d);
			else memcpy(f->bits, buf2, w*h*d);
			f->hdr.qual=fconfig.qual;  f->hdr.subsamp=fconfig.subsamp;
			f->hdr.winid=win;  f->hdr.compress=fconfig.compress;
			fill=1-fill;
			vglconn.sendFrame(f);
			frames++;
		} while((elapsed=timer.elapsed())<2.);

		printf("%f Megapixels/sec\n",
			(double)w*(double)h*(double)frames/1000000./elapsed);

		printf("\nTesting full-frame send (spoiling) ...\n");

		fill=0, frames=0;  int clientframes=0;  timer.start();
		do
		{
			_errifnot(f=vglconn.getFrame(w, h, d, bgr?FRAME_BGR:0, false));
			if(fill) memcpy(f->bits, buf, w*h*d);
			else memcpy(f->bits, buf2, w*h*d);
			f->hdr.qual=fconfig.qual;  f->hdr.subsamp=fconfig.subsamp;
			f->hdr.winid=win;  f->hdr.compress=fconfig.compress;
			fill=1-fill;
			vglconn.sendFrame(f);
			clientframes++;  frames++;
		} while((elapsed=timer.elapsed())<2.);

		printf("%f Megapixels/sec (server)\n",
			(double)w*(double)h*(double)frames/1000000./elapsed);
		printf("%f Megapixels/sec (client)\n",
			(double)w*(double)h*(double)clientframes/1000000./elapsed);

		printf("\nTesting half-frame send ...\n");

		fill=0, frames=0;  timer.start();
		do
		{
			vglconn.synchronize();
			_errifnot(f=vglconn.getFrame(w, h, d, bgr?FRAME_BGR:0, false));
			if(fill) memcpy(f->bits, buf, w*h*d);
			else memcpy(f->bits, buf3, w*h*d);
			f->hdr.qual=fconfig.qual;  f->hdr.subsamp=fconfig.subsamp;
			f->hdr.winid=win;  f->hdr.compress=fconfig.compress;
			fill=1-fill;
			vglconn.sendFrame(f);
			frames++;
		} while((elapsed=timer.elapsed())<2.);

		printf("%f Megapixels/sec\n",
			(double)w*(double)h*(double)frames/1000000./elapsed);

		printf("\nTesting zero-frame send ...\n");

		frames=0;  timer.start();
		do
		{
			vglconn.synchronize();
			_errifnot(f=vglconn.getFrame(w, h, d, bgr?FRAME_BGR:0, false));
			memcpy(f->bits, buf, w*h*d);
			f->hdr.qual=fconfig.qual;  f->hdr.subsamp=fconfig.subsamp;
			f->hdr.winid=win;  f->hdr.compress=fconfig.compress;
			vglconn.sendFrame(f);
			frames++;
		} while((elapsed=timer.elapsed())<2.);

		printf("%f Megapixels/sec\n",
			(double)w*(double)h*(double)frames/1000000./elapsed);
	}
	catch(Error &e)
	{
		printf("%s--\n%s\n", e.getMethod(), e.getMessage());
	}

	if(win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
	if(buf) free(buf);
	if(buf2) free(buf2);
	if(buf3) free(buf3);
}
