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

int main(int argc, char **argv)
{
	rrtimer t;  double elapsed;
	unsigned char *buf=NULL, *buf2=NULL, *buf3=NULL;
	Display *dpy=NULL;  Window win=0;  int dpynum=0;

	try {

	int qual, subsamp;
	if(argc<5 || (qual=atoi(argv[3]))<0 || qual>100 || (subsamp=atoi(argv[2]))<0
		|| subsamp>RR_SUBSAMP)
	{
		printf("USAGE: %s <bitmap file> <subsamp> <qual> <server> [striph]\n", argv[0]);
		printf("subsamp = 0=none, 1=4:2:2, 2=4:1:1\n");
		printf("qual = 0-100 inclusive\n");
		printf("server = machine where RRXClient is running (0=local test only)\n");
		printf("striph = height of each inter-frame difference tile\n");
		exit(1);
	}

	int striph=RR_DEFAULTSTRIPHEIGHT, temp;
	if(argc>5 && (temp=atoi(argv[5]))>0) striph=temp;
	printf("Strip height = %d pixels\n", striph);
	char *servername=argv[4];
	if(!stricmp(servername, "0")) servername=NULL;
	unsigned short port=0;
	if(servername) port=RR_DEFAULTPORT;

	rrdisplayclient rrdpy(servername, port, false);
	int w, h, d;
	const char *err;
	if((err=loadbmp(argv[1], &buf, &w, &h, &d, 0))!=NULL) _throw(err);
	if((err=loadbmp(argv[1], &buf2, &w, &h, &d, 0))!=NULL) _throw(err);
	if((err=loadbmp(argv[1], &buf3, &w, &h, &d, 0))!=NULL) _throw(err);
	printf("Source image: %d x %d x %d-bit\n", w, h, d*8);

	if(servername)
	{
		if(!XInitThreads()) _throw("Could not initialize X threads");
		if((dpy=XOpenDisplay(0))==NULL) _throw("Could not open display");
		if((win=XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
			w, h, 0, WhitePixel(dpy, DefaultScreen(dpy)),
			BlackPixel(dpy, DefaultScreen(dpy))))==0) _throw("Could not create window");
		printf("Creating window %d\n", win);
		errifnot(XMapRaised(dpy, win));
		XSync(dpy, False);
		dpynum=0;  char *dpystring=NULL, *ptr=NULL;
		dpystring=DisplayString(dpy);
		if((ptr=strchr(dpystring, ':'))!=NULL && strlen(ptr)>1) dpynum=atoi(ptr+1);
	}

	int i;
	for(i=0; i<w*h*d; i++) buf2[i]=255-buf2[i];
	for(i=0; i<w*h*d/2; i++) buf3[i]=255-buf3[i];

	rrframe *b;

	printf("\nTesting full-frame send ...\n");

	int frames=0, fill=0;  t.start();
	do
	{
		errifnot(b=rrdpy.getbitmap(w, h, d));
		if(fill) memcpy(b->bits, buf, w*h*d);
		else memcpy(b->bits, buf2, w*h*d);
		b->h.qual=qual;  b->h.subsamp=subsamp;
		b->h.dpynum=dpynum;  b->h.winid=win;
		b->strip_height=striph;
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
			if(fill) memcpy(b->bits, buf, w*h*d);
			else memcpy(b->bits, buf2, w*h*d);
			frames++;  continue;
		}
		errifnot(b=rrdpy.getbitmap(w, h, d));
		if(fill) memcpy(b->bits, buf, w*h*d);
		else memcpy(b->bits, buf2, w*h*d);
		b->h.qual=qual;  b->h.subsamp=subsamp;
		b->h.dpynum=dpynum;  b->h.winid=win;
		b->strip_height=striph;
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
		if(fill) memcpy(b->bits, buf, w*h*d);
		else memcpy(b->bits, buf3, w*h*d);
		b->h.qual=qual;  b->h.subsamp=subsamp;
		b->h.dpynum=dpynum;  b->h.winid=win;
		b->strip_height=striph;
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
		memcpy(b->bits, buf, w*h*d);
		b->h.qual=qual;  b->h.subsamp=subsamp;
		b->h.dpynum=dpynum;  b->h.winid=win;
		b->strip_height=striph;
		rrdpy.sendframe(b);
		frames++;
	} while((elapsed=t.elapsed())<2.);

	printf("%f Megapixels/sec\n", (double)w*(double)h*(double)frames/1000000./elapsed);

	} catch(rrerror &e) {printf("%s--\n%s\n", e.getMethod(), e.getMessage());}

	if(win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
	if(buf) free(buf);  if(buf2) free(buf2);  if(buf3) free(buf3);
}
