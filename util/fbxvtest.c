/* Copyright (C)2009-2010, 2014 D. R. Commander
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fbxv.h"
#include "Timer.h"


#define WIDTH 1241
#define HEIGHT 903


#define _throw(m) {  \
	printf("ERROR: %s\n", m);  goto bailout;  \
}

#define _fbxv(f)  \
	if((f)==-1) {  \
		printf("FBXV ERROR in line %d: %s\n", fbxv_geterrline(),  \
			fbxv_geterrmsg());  \
		goto bailout;  \
	}


double testTime=5.;
Display *dpy=NULL;  Window win=0;
fbxv_struct s, s1;
int width=-1, height=-1, useShm=1, scale=1, interactive=0;
char *filename=NULL;
Atom protoAtom=0, deleteAtom=0;


void initBuf(fbxv_struct *s, int id, int seed)
{
	int i, j, c;
	int yindex0, yindex1, uindex, vindex;

	switch(id)
	{
		case YUY2_PACKED:
			yindex0=0;  uindex=1;  yindex1=2;  vindex=3;  break;
		case UYVY_PACKED:
			uindex=0;  yindex0=1;  vindex=2;  yindex1=3;  break;
		default:
			yindex0=yindex1=uindex=vindex=0;  break;
	}
	if(id==YUY2_PACKED || id==UYVY_PACKED)
	{
		for(j=0; j<s->xvi->height; j++)
		{
			for(i=0; i<s->xvi->width; i+=2)
			{
				s->xvi->data[s->xvi->pitches[0]*j + i*2 + yindex0]
					=(j+seed)*(i+seed);
				s->xvi->data[s->xvi->pitches[0]*j + i*2 + yindex1]
					=(j+seed+1)*(i+seed+1);
				s->xvi->data[s->xvi->pitches[0]*j + i*2 + uindex]
					=(j+seed)*(i+seed);
				s->xvi->data[s->xvi->pitches[0]*j + i*2 + vindex]
					=(j+seed)*(i+seed);
			}
		}
	}
	else
	{
		for(c=0; c<s->xvi->num_planes; c++)
			for(j=0; j<s->xvi->height; j++)
				for(i=0; i<s->xvi->width; i++)
					s->xvi->data[s->xvi->offsets[c] + s->xvi->pitches[c]*(c==0? j:j/2)
						+ (c==0? i:i/2)] = (j+seed)*(i+seed);
	}
}


void doTest(int id, char *name)
{
	int iter=0, i;
	double elapsed=0., t;
	FILE *file=NULL;

	printf("Testing %s format (ID=0x%.8x) ...\n", name, id);

	memset(&s, 0, sizeof(s));
	memset(&s1, 0, sizeof(s));
	_fbxv(fbxv_init(&s, dpy, win, width/scale, height/scale, id, useShm));
	if(!filename)
	{
		_fbxv(fbxv_init(&s1, dpy, win, width/scale, height/scale, id, useShm));
	}
	printf("Image:\n");
	printf("  Data size:   %d\n", s.xvi->data_size);
	printf("  Dimensions:  %d x %d\n", s.xvi->width, s.xvi->height);
	printf("  Planes:      %d\n", s.xvi->num_planes);
	printf("  Pitches:     [");
	for(i=0; i<s.xvi->num_planes; i++)
	{
		printf("%d", s.xvi->pitches[i]);
		if(i!=s.xvi->num_planes-1) printf(", ");
	}
	printf("]\n");
	printf("  Offsets:     [");
	for(i=0; i<s.xvi->num_planes; i++)
	{
		printf("%d", s.xvi->offsets[i]);
		if(i!=s.xvi->num_planes-1) printf(", ");
	}
	printf("]\n");
	printf("XV Port = %d\n", s.port);

	if(filename)
	{
		file=fopen(filename, "rb");
		if(!file)
		{
			printf("Could not open %s\n", filename);
			exit(-1);
		}
		if(fread(s.xvi->data, s.xvi->data_size, 1, file)!=1)
		{
			printf("Could not read %s\n", filename);
			exit(-1);
		}
		fclose(file);
	}

	if(!filename)
	{
		initBuf(&s, id, 0);
		initBuf(&s1, id, 128);
	}
	t=getTime();
	do
	{
		if(filename || iter%2==0)
			{ _fbxv(fbxv_write(&s, 0, 0, 0, 0, 0, 0, width, height)); }
		else { _fbxv(fbxv_write(&s1, 0, 0, 0, 0, 0, 0, width, height)); }
		iter++;
	} while((elapsed=getTime()-t)<testTime);
	printf("%f Mpixels/sec\n",
		(double)(width*height)/1000000.*(double)iter/elapsed);

	bailout:
	fbxv_term(&s);
}


void doInteractiveTest(Display *dpy, int id, char *name)
{
	int iter=0;
	printf("Interactive test, %s format (ID=0x%.8x) ...\n", name, id);
	memset(&s, 0, sizeof(s));

	while(1)
	{
		int doDisplay=0;

		while(1)
		{
			XEvent event;
			XNextEvent(dpy, &event);
			switch (event.type)
			{
				case Expose:
					doDisplay=1;
					break;
				case ConfigureNotify:
					width=event.xconfigure.width;  height=event.xconfigure.height;
					break;
				case KeyPress:
				{
					char buf[10];
					XLookupString(&event.xkey, buf, sizeof(buf), NULL, NULL);
					switch(buf[0])
					{
						case 27: case 'q': case 'Q':
							return;
					}
					break;
				}
				case MotionNotify:
					if(event.xmotion.state & Button1Mask) doDisplay=1;
					break;
				case ClientMessage:
				{
					XClientMessageEvent *cme=(XClientMessageEvent *)&event;
					if(cme->message_type==protoAtom && cme->data.l[0]==deleteAtom)
						return;
				}
			}
			if(XPending(dpy)<=0) break;
		}
		if(doDisplay)
		{
			_fbxv(fbxv_init(&s, dpy, win, width/scale, height/scale, id, useShm));
			initBuf(&s, id, iter);  iter++;
			_fbxv(fbxv_write(&s, 0, 0, 0, 0, 0, 0, width, height));
		}
	}

	bailout:
	return;
}


int main(int argc, char **argv)
{
	int id=-1, n=0, i;
	XVisualInfo vtemp, *vis=NULL;
	XSetWindowAttributes swa;

	if(argc>1)
	{
		for(i=0; i<argc; i++)
		{
			if(!strcasecmp(argv[i], "-noshm")) useShm=0;
			else if(!strcasecmp(argv[i], "-i")) interactive=1;
			else if(!strcasecmp(argv[i], "-scale") && i<argc-1)
			{
				scale=atoi(argv[++i]);
				if(scale<1) scale=1;
			}
			else if(!strcasecmp(argv[i], "-width") && i<argc-1)
			{
				int temp=atoi(argv[++i]);
				if(temp>0) width=temp;
			}
			else if(!strcasecmp(argv[i], "-height") && i<argc-1)
			{
				int temp=atoi(argv[++i]);
				if(temp>0) height=temp;
			}
			else if(!strcasecmp(argv[i], "-file") && i<argc-1)
				filename=argv[++i];
			else if(!strcasecmp(argv[i], "-time") && i<argc-1)
			{
				double temp=atof(argv[++i]);
				if(temp>0.) testTime=temp;
			}
			else if(!strcasecmp(argv[i], "-format") && i<argc-1)
			{
				char *temp=argv[++i];
				if(!strcasecmp(temp, "yuy2")) id=YUY2_PACKED;
				else if(!strcasecmp(temp, "yv12")) id=YV12_PLANAR;
				else if(!strcasecmp(temp, "uyvy")) id=UYVY_PACKED;
				else if(!strcasecmp(temp, "i420")) id=I420_PLANAR;
			}
			else if(!strncasecmp(argv[i], "-h", 1) || !strcmp(argv[i], "-?"))
			{
				printf("\nUSAGE: %s [arguments]\n\n", argv[0]);
				printf("Arguments:\n");
				printf("-noshm = Do not use shared memory version of XvPutImage()\n");
				printf("-i = Interactive test\n");
				printf("-scale <s> = Downsample/Upsample image by scaling factor <s> in both\n");
				printf("             x and y directions\n");
				printf("-width <w> = Set image width to <w>\n");
				printf("-height <h> = Set image height to <h>\n");
				printf("-file <f> = Load YUV data from file, i.e. a .yuv file generated by\n");
				printf("            jpgtest -yuv.  -width and -height must be specified.\n");
				printf("-time <t> = Set benchmark time to <t> seconds\n");
				printf("-format <f> = Pixel format (YUY2, YV12, UYVY, or I420)\n\n");
				exit(0);
			}
		}
	}

	if(filename && (width<0 || height<0 || id<0))
	{
		printf("ERROR: Width, height, and format must be specified\n");
		exit(1);
	}
	if(width<0) width=WIDTH;
	if(height<0) height=HEIGHT;

	fbxv_printwarnings(stdout);

	if(!(dpy=XOpenDisplay(NULL))) _throw("Could not open display");

	vtemp.depth=24;  vtemp.class=TrueColor;
	if((vis=XGetVisualInfo(dpy, VisualDepthMask|VisualClassMask, &vtemp,
		&n))==NULL || n==0)
		_throw("Could not obtain a TrueColor visual");
	swa.colormap=XCreateColormap(dpy, DefaultRootWindow(dpy), vis->visual,
		AllocNone);
	swa.border_pixel=0;
	swa.background_pixel=0;
	swa.event_mask=0;
	if(interactive)
		swa.event_mask=StructureNotifyMask|ExposureMask
			|KeyPressMask|PointerMotionMask|ButtonPressMask;
	printf("Visual ID = 0x%.2x\n", (unsigned int)vis->visualid);
	if(!(protoAtom=XInternAtom(dpy, "WM_PROTOCOLS", False)))
		_throw("Cannot obtain WM_PROTOCOLS atom");
	if(!(deleteAtom=XInternAtom(dpy, "WM_DELETE_WINDOW", False)))
		_throw("Cannot obtain WM_DELETE_WINDOW atom");
	if(!(win=XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, width, height, 0,
		vis->depth, InputOutput, vis->visual,
		CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &swa)))
		_throw("Could not create window");
	XFree(vis);  vis=NULL;
	XSetWMProtocols(dpy, win, &deleteAtom, 1);
	if(!XMapRaised(dpy, win))
		_throw("Could not show window");
	XSync(dpy, False);

	if(interactive) doInteractiveTest(dpy, I420_PLANAR, "I420 Planar");
	else if(id>=0)
		doTest(id, "User-specified");
	else
	{
		doTest(I420_PLANAR, "I420 Planar");
		printf("\n");
		doTest(YV12_PLANAR, "YV12 Planar");
		printf("\n");
		doTest(YUY2_PACKED, "YUY2 Packed");
		printf("\n");
		doTest(UYVY_PACKED, "UYVY Packed");
		printf("\n");
	}

	bailout:
	if(dpy && win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
	return 0;
}
