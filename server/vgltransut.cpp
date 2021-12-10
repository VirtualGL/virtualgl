// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2011, 2014, 2017-2019, 2021 D. R. Commander
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

#include "VGLTrans.h"
#include "vglutil.h"
#include "Timer.h"
#include "bmp.h"
#include "fakerconfig.h"

using namespace util;
using namespace common;
using namespace server;


void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s <bitmap file> [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-client <hostname or IP> = Hostname or IP address where the frames should be\n");
	fprintf(stderr, "                           sent (the VirtualGL Client must be running on that\n");
	fprintf(stderr, "                           machine) or 0 for local test only\n");
	fprintf(stderr, "                           (default: %s)\n",
		strlen(fconfig.client) > 0 ?
		fconfig.client : "read from DISPLAY environment");
	fprintf(stderr, "-port <p> = TCP port on which the VirtualGL Client is listening\n");
	fprintf(stderr, "            (default: %d)\n",
		fconfig.port < 0 ? RR_DEFAULTPORT : fconfig.port);
	fprintf(stderr, "-samp <s> = JPEG chrominance subsampling factor: 0 (gray), 1, 2, or 4\n");
	fprintf(stderr, "            (default: %d)\n", fconfig.subsamp);
	fprintf(stderr, "-qual <q> = JPEG quality, 1 <= <q> <= 100 (default: %d)\n",
		fconfig.qual);
	fprintf(stderr, "-tilesize <n> = Width/height of each multithreaded compression/interframe\n");
	fprintf(stderr, "                comparison tile (default: %d x %d pixels)\n",
		fconfig.tilesize, fconfig.tilesize);
	fprintf(stderr, "-rgb = Use RGB (uncompressed) encoding (default is JPEG)\n");
	fprintf(stderr, "-np <n> = Number of threads to use for compression (default: %d)\n\n",
		fconfig.np);
	exit(1);
}


int main(int argc, char **argv)
{
	Timer timer;  double elapsed;
	unsigned char *buf = NULL, *buf2 = NULL, *buf3 = NULL;
	Display *dpy = NULL;  Window win = 0;
	int i, retval = 0;  int bgr = LittleEndian();

	try
	{
		fconfig_setcompress(fconfig, RRCOMP_JPEG);

		bool localtest = false;
		if(argc < 2) usage(argv);
		if(!stricmp(argv[1], "-h") || !strcmp(argv[1], "-?")) usage(argv);

		if(argc > 2) for(i = 2; i < argc; i++)
		{
			if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
			else if(!stricmp(argv[i], "-client") && i < argc - 1)
			{
				strncpy(fconfig.client, argv[++i], MAXSTR - 1);
				if(!stricmp(fconfig.client, "0"))
				{
					localtest = true;  fconfig.client[0] = 0;
				}
			}
			else if(!stricmp(argv[i], "-port") && i < argc - 1)
			{
				fconfig.port = atoi(argv[++i]);
			}
			else if(!stricmp(argv[i], "-samp") && i < argc - 1)
			{
				fconfig.subsamp = atoi(argv[++i]);
			}
			else if(!stricmp(argv[i], "-qual") && i < argc - 1)
			{
				fconfig.qual = atoi(argv[++i]);
			}
			else if(!stricmp(argv[i], "-tilesize") && i < argc - 1)
			{
				fconfig.tilesize = atoi(argv[++i]);
			}
			else if(!stricmp(argv[i], "-np") && i < argc - 1)
			{
				fconfig.np = atoi(argv[++i]);
			}
			else if(!stricmp(argv[i], "-rgb"))
				fconfig_setcompress(fconfig, RRCOMP_RGB);
			else usage(argv);
		}
		if(fconfig.compress == RRCOMP_RGB) bgr = 0;

		int w, h, d = 3;

		if(bmp_load(argv[1], &buf, &w, 1, &h, bgr ? PF_BGR : PF_RGB,
			BMPORN_TOPDOWN) == -1)
			THROW(bmp_geterr());
		if(bmp_load(argv[1], &buf2, &w, 1, &h, bgr ? PF_BGR : PF_RGB,
			BMPORN_TOPDOWN) == -1)
			THROW(bmp_geterr());
		if(bmp_load(argv[1], &buf3, &w, 1, &h, bgr ? PF_BGR : PF_RGB,
			BMPORN_TOPDOWN) == -1)
			THROW(bmp_geterr());
		printf("Source image: %d x %d x %d-bit\n", w, h, d * 8);

		if(!localtest)
		{
			if(!XInitThreads()) THROW("Could not initialize X threads");
			if((dpy = XOpenDisplay(0)) == NULL) THROW("Could not open display");
			if((win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, w, h, 0,
				WhitePixel(dpy, DefaultScreen(dpy)),
				BlackPixel(dpy, DefaultScreen(dpy)))) == 0)
				THROW("Could not create window");
			printf("Creating window %lu\n", win);
			ERRIFNOT(XMapRaised(dpy, win));
			XSync(dpy, False);
			if(strlen(fconfig.client) == 0)
				strncpy(fconfig.client, DisplayString(dpy), MAXSTR - 1);
			fconfig_setdefaultsfromdpy(dpy);
		}

		printf("Tile size = %d x %d pixels\n", fconfig.tilesize, fconfig.tilesize);

		VGLTrans vglconn;
		if(!localtest) vglconn.connect(fconfig.client, fconfig.port);

		for(i = 0; i < w * h * d; i++) buf2[i] = 255 - buf2[i];
		for(i = 0; i < w * h * d / 2; i++) buf3[i] = 255 - buf3[i];

		Frame *f;

		printf("\nTesting full-frame send ...\n");

		int frames = 0, fill = 0;  timer.start();
		do
		{
			vglconn.synchronize();
			ERRIFNOT(f = vglconn.getFrame(w, h, bgr ? PF_BGR : PF_RGB, 0, false));
			if(fill) memcpy(f->bits, buf, w * h * d);
			else memcpy(f->bits, buf2, w * h * d);
			f->hdr.qual = fconfig.qual;  f->hdr.subsamp = fconfig.subsamp;
			f->hdr.winid = win;  f->hdr.compress = fconfig.compress;
			fill = 1 - fill;
			vglconn.sendFrame(f);
			frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		printf("%f Megapixels/sec\n",
			(double)w * (double)h * (double)frames / 1000000. / elapsed);

		printf("\nTesting full-frame send (spoiling) ...\n");

		fill = 0, frames = 0;  int clientframes = 0;  timer.start();
		do
		{
			ERRIFNOT(f = vglconn.getFrame(w, h, bgr ? PF_BGR : PF_RGB, 0, false));
			if(fill) memcpy(f->bits, buf, w * h * d);
			else memcpy(f->bits, buf2, w * h * d);
			f->hdr.qual = fconfig.qual;  f->hdr.subsamp = fconfig.subsamp;
			f->hdr.winid = win;  f->hdr.compress = fconfig.compress;
			fill = 1 - fill;
			vglconn.sendFrame(f);
			clientframes++;  frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		printf("%f Megapixels/sec (server)\n",
			(double)w * (double)h * (double)frames / 1000000. / elapsed);
		printf("%f Megapixels/sec (client)\n",
			(double)w * (double)h * (double)clientframes / 1000000. / elapsed);

		printf("\nTesting half-frame send ...\n");

		fill = 0, frames = 0;  timer.start();
		do
		{
			vglconn.synchronize();
			ERRIFNOT(f = vglconn.getFrame(w, h, bgr ? PF_BGR : PF_RGB, 0, false));
			if(fill) memcpy(f->bits, buf, w * h * d);
			else memcpy(f->bits, buf3, w * h * d);
			f->hdr.qual = fconfig.qual;  f->hdr.subsamp = fconfig.subsamp;
			f->hdr.winid = win;  f->hdr.compress = fconfig.compress;
			fill = 1 - fill;
			vglconn.sendFrame(f);
			frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		printf("%f Megapixels/sec\n",
			(double)w * (double)h * (double)frames / 1000000. / elapsed);

		printf("\nTesting zero-frame send ...\n");

		frames = 0;  timer.start();
		do
		{
			vglconn.synchronize();
			ERRIFNOT(f = vglconn.getFrame(w, h, bgr ? PF_BGR : PF_RGB, 0, false));
			memcpy(f->bits, buf, w * h * d);
			f->hdr.qual = fconfig.qual;  f->hdr.subsamp = fconfig.subsamp;
			f->hdr.winid = win;  f->hdr.compress = fconfig.compress;
			vglconn.sendFrame(f);
			frames++;
		} while((elapsed = timer.elapsed()) < 2.);

		printf("%f Megapixels/sec\n",
			(double)w * (double)h * (double)frames / 1000000. / elapsed);
	}
	catch(std::exception &e)
	{
		printf("%s--\n%s\n", GET_METHOD(e), e.what());
		retval = -1;
	}

	if(win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
	free(buf);
	free(buf2);
	free(buf3);
	return retval;
}
