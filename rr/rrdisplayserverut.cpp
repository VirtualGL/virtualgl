/* Copyright (C)2005 Sun Microsystems, Inc.
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

#include "rrdisplayclient.h"
#include "fakerconfig.h"

FakerConfig fconfig;

#define WIDTH 301
#define HEIGHT 301

int main(int argc, char **argv)
{
	Display *dpy=NULL;  Window win=0;
	bool usessl=false;  int i;
	int iter=10000, frames=2;

	try {

	if(argc<3 || (iter=atoi(argv[1]))<1 || (frames=atoi(argv[2]))<1)
	{
		printf("USAGE: %s <iterations> <frames> [-client <machine:0.0>] [-ssl]\n", argv[0]);
		printf("-client = X Display where the video should be sent (VGL client must be running\n");
		printf("          on that machine)\n");
		printf("          [default = read from DISPLAY environment]\n");
		printf("-ssl = use SSL tunnel to connect to client\n");
		exit(1);
	}

	for(i=0; i<argc; i++)
	{
		if(!stricmp(argv[i], "-ssl")) usessl=true;
		if(!strnicmp(argv[i], "-cl", 3) && i<argc-1)
		{
			fconfig.client=argv[i+1];  i++;
		}
	}

	if(!XInitThreads()) _throw("Could not initialize X threads");
	if((dpy=XOpenDisplay(0))==NULL) _throw("Could not open display");
	if((win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
		WIDTH, HEIGHT, 0, WhitePixel(dpy, DefaultScreen(dpy)),
		BlackPixel(dpy, DefaultScreen(dpy))))==0) _throw("Could not create window");
	printf("Creating window %lu\n", (unsigned long)win);
	errifnot(XMapRaised(dpy, win));
	XSync(dpy, False);
	if(!fconfig.client) fconfig.client=DisplayString(dpy);

	rrframe *b;

	printf("\nTesting client for memory leaks and stability ...\n");
	printf("%d iterations\n", iter);

	for(int i=0; i<iter; i++)
	{
		rrdisplayclient *rrdpy=NULL;
		errifnot(rrdpy=new rrdisplayclient(fconfig.client));
		for(int f=0; f<frames; f++)
		{
			errifnot(b=rrdpy->getbitmap(WIDTH, HEIGHT, 3,
				littleendian()? RRBMP_BGR:0));
			memset(b->_bits, i%2==0?0:255, WIDTH*HEIGHT*3);
			for(int j=0; j<WIDTH*HEIGHT*3; j++) if(j%2==0) b->_bits[j]=i%2==0?255:0;
			b->_h.qual=50;  b->_h.subsamp=4;
			b->_h.winid=win;
			rrdpy->sendframe(b);
		}
		delete rrdpy;
	}

	} catch(rrerror &e) {printf("%s--\n%s\n", e.getMethod(), e.getMessage());}

	if(win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
}
