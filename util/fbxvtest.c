/* Copyright (C)2009-2010, 2014, 2017, 2019 D. R. Commander
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
#include "vglutil.h"


#define WIDTH  1241
#define HEIGHT  903
#define BENCHTIME  5.0


#define THROW(m) \
{ \
	retval = -1;  printf("ERROR: %s\n", m);  goto bailout; \
}

#define TRY_FBXV(f) \
{ \
	if((f) == -1) \
	{ \
		printf("FBXV ERROR in line %d: %s\n", fbxv_geterrline(), \
			fbxv_geterrmsg()); \
		retval = -1; \
		goto bailout; \
	} \
}


double testTime = BENCHTIME;
Display *dpy = NULL;  Window win = 0;
fbxv_struct s, s1;
int width = -1, height = -1, useShm = 1, scale = 1, interactive = 0;
char *filename = NULL;
Atom protoAtom = 0, deleteAtom = 0;


static void initBuf(fbxv_struct *fb, int id, int seed)
{
	int i, j, c;
	int yindex0, yindex1, uindex, vindex;

	switch(id)
	{
		case YUY2_PACKED:
			yindex0 = 0;  uindex = 1;  yindex1 = 2;  vindex = 3;  break;
		case UYVY_PACKED:
			uindex = 0;  yindex0 = 1;  vindex = 2;  yindex1 = 3;  break;
		default:
			yindex0 = yindex1 = uindex = vindex = 0;  break;
	}
	if(id == YUY2_PACKED || id == UYVY_PACKED)
	{
		for(j = 0; j < fb->xvi->height; j++)
		{
			for(i = 0; i < fb->xvi->width; i += 2)
			{
				fb->xvi->data[fb->xvi->pitches[0] * j + i * 2 + yindex0] =
					(j + seed) * (i + seed);
				fb->xvi->data[fb->xvi->pitches[0] * j + i * 2 + yindex1] =
					(j + seed + 1) * (i + seed + 1);
				fb->xvi->data[fb->xvi->pitches[0] * j + i * 2 + uindex] =
					(j + seed) * (i + seed);
				fb->xvi->data[fb->xvi->pitches[0] * j + i * 2 + vindex] =
					(j + seed) * (i + seed);
			}
		}
	}
	else
	{
		for(c = 0; c < fb->xvi->num_planes; c++)
			for(j = 0; j < fb->xvi->height; j++)
				for(i = 0; i < fb->xvi->width; i++)
					fb->xvi->data[fb->xvi->offsets[c] +
						fb->xvi->pitches[c] * (c == 0 ? j : j / 2) +
						(c == 0 ? i : i / 2)] = (j + seed) * (i + seed);
	}
}


static int doTest(int id, char *name)
{
	int iter = 0, i, retval = 0;
	double elapsed = 0., t;
	FILE *file = NULL;

	printf("Testing %s format (ID=0x%.8x) %s...\n", name, id,
		useShm ? "[SHM] " : "");

	memset(&s, 0, sizeof(s));
	memset(&s1, 0, sizeof(s));
	TRY_FBXV(fbxv_init(&s, dpy, win, width / scale, height / scale, id, useShm));
	if(!filename)
	{
		TRY_FBXV(fbxv_init(&s1, dpy, win, width / scale, height / scale, id,
			useShm));
	}
	printf("Image:\n");
	printf("  Data size:   %d\n", s.xvi->data_size);
	printf("  Dimensions:  %d x %d\n", s.xvi->width, s.xvi->height);
	printf("  Planes:      %d\n", s.xvi->num_planes);
	printf("  Pitches:     [");
	for(i = 0; i < s.xvi->num_planes; i++)
	{
		printf("%d", s.xvi->pitches[i]);
		if(i != s.xvi->num_planes - 1) printf(", ");
	}
	printf("]\n");
	printf("  Offsets:     [");
	for(i = 0; i < s.xvi->num_planes; i++)
	{
		printf("%d", s.xvi->offsets[i]);
		if(i != s.xvi->num_planes - 1) printf(", ");
	}
	printf("]\n");
	printf("XV Port = %d\n", s.port);

	if(filename)
	{
		file = fopen(filename, "rb");
		if(!file)
		{
			printf("Could not open %s\n", filename);
			exit(-1);
		}
		if(fread(s.xvi->data, s.xvi->data_size, 1, file) != 1)
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
	t = GetTime();
	do
	{
		if(filename || iter % 2 == 0)
			{ TRY_FBXV(fbxv_write(&s, 0, 0, 0, 0, 0, 0, width, height)); }
		else { TRY_FBXV(fbxv_write(&s1, 0, 0, 0, 0, 0, 0, width, height)); }
		iter++;
	} while((elapsed = GetTime() - t) < testTime);
	printf("%f Mpixels/sec\n",
		(double)(width * height) / 1000000. * (double)iter / elapsed);

	bailout:
	fbxv_term(&s);
	return retval;
}


static int doInteractiveTest(int id, char *name)
{
	int iter = 0, retval = 0;
	printf("Interactive test, %s format (ID=0x%.8x) %s...\n", name, id,
		useShm ? "[SHM] " : "");
	memset(&s, 0, sizeof(s));

	while(1)
	{
		int doDisplay = 0;

		while(1)
		{
			XEvent event;
			XNextEvent(dpy, &event);
			switch(event.type)
			{
				case Expose:
					doDisplay = 1;
					break;
				case ConfigureNotify:
					width = event.xconfigure.width;  height = event.xconfigure.height;
					break;
				case KeyPress:
				{
					char buf[10];
					XLookupString(&event.xkey, buf, sizeof(buf), NULL, NULL);
					switch(buf[0])
					{
						case 27: case 'q': case 'Q':
							return 0;
					}
					break;
				}
				case MotionNotify:
					if(event.xmotion.state & Button1Mask) doDisplay = 1;
					break;
				case ClientMessage:
				{
					XClientMessageEvent *cme = (XClientMessageEvent *)&event;
					if(cme->message_type == protoAtom
						&& cme->data.l[0] == (long)deleteAtom)
						return 0;
				}
			}
			if(XPending(dpy) <= 0) break;
		}
		if(doDisplay)
		{
			TRY_FBXV(fbxv_init(&s, dpy, win, width / scale, height / scale, id,
				useShm));
			initBuf(&s, id, iter);  iter++;
			TRY_FBXV(fbxv_write(&s, 0, 0, 0, 0, 0, 0, width, height));
		}
	}

	bailout:
	return retval;
}


static void usage(char **argv)
{
	fprintf(stderr, "\nUSAGE: %s [options]\n\n", argv[0]);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "-noshm = Do not use shared memory version of XvPutImage()\n");
	fprintf(stderr, "-i = Interactive test\n");
	fprintf(stderr, "-scale <s> = Downsample/Upsample image by scaling factor <s> in both\n");
	fprintf(stderr, "             x and y directions\n");
	fprintf(stderr, "-width <w> = Set image width to <w>\n");
	fprintf(stderr, "-height <h> = Set image height to <h>\n");
	fprintf(stderr, "-file <filename> = Load YUV data from file.  -width, -height and -format must\n");
	fprintf(stderr, "                   be specified.\n");
	fprintf(stderr, "-time <t> = Set benchmark time to <t> seconds (default: %.1f)\n",
		BENCHTIME);
	fprintf(stderr, "-format <f> = Pixel format (YUY2, YV12, UYVY, or I420)\n\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int id = -1, n = 0, i, retval = 0;
	XVisualInfo vtemp, *vis = NULL;
	XSetWindowAttributes swa;

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(!strcasecmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!strcasecmp(argv[i], "-noshm")) useShm = 0;
		else if(!strcasecmp(argv[i], "-i")) interactive = 1;
		else if(!strcasecmp(argv[i], "-scale") && i < argc - 1)
		{
			scale = atoi(argv[++i]);
			if(scale < 1) usage(argv);
		}
		else if(!strcasecmp(argv[i], "-width") && i < argc - 1)
		{
			width = atoi(argv[++i]);
			if(width < 1) usage(argv);
		}
		else if(!strcasecmp(argv[i], "-height") && i < argc - 1)
		{
			height = atoi(argv[++i]);
			if(height < 1) usage(argv);
		}
		else if(!strcasecmp(argv[i], "-file") && i < argc - 1)
			filename = argv[++i];
		else if(!strcasecmp(argv[i], "-time") && i < argc - 1)
		{
			testTime = atof(argv[++i]);
			if(testTime <= 0.0) usage(argv);
		}
		else if(!strcasecmp(argv[i], "-format") && i < argc - 1)
		{
			char *temp = argv[++i];
			if(!strcasecmp(temp, "yuy2")) id = YUY2_PACKED;
			else if(!strcasecmp(temp, "yv12")) id = YV12_PLANAR;
			else if(!strcasecmp(temp, "uyvy")) id = UYVY_PACKED;
			else if(!strcasecmp(temp, "i420")) id = I420_PLANAR;
			else usage(argv);
		}
		else usage(argv);
	}

	if(filename && (width < 0 || height < 0 || id < 0))
	{
		printf("ERROR: Width, height, and format must be specified\n");
		exit(1);
	}
	if(width < 0) width = WIDTH;
	if(height < 0) height = HEIGHT;

	fbxv_printwarnings(stdout);

	if(!(dpy = XOpenDisplay(NULL))) THROW("Could not open display");

	vtemp.depth = 24;  vtemp.class = TrueColor;
	if((vis = XGetVisualInfo(dpy, VisualDepthMask | VisualClassMask, &vtemp,
		&n)) == NULL || n == 0)
		THROW("Could not obtain a TrueColor visual");
	swa.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), vis->visual,
		AllocNone);
	swa.border_pixel = 0;
	swa.background_pixel = 0;
	swa.event_mask = 0;
	if(interactive)
		swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask |
			PointerMotionMask | ButtonPressMask;
	printf("Visual ID = 0x%.2x\n", (unsigned int)vis->visualid);
	if(!(protoAtom = XInternAtom(dpy, "WM_PROTOCOLS", False)))
		THROW("Cannot obtain WM_PROTOCOLS atom");
	if(!(deleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", False)))
		THROW("Cannot obtain WM_DELETE_WINDOW atom");
	if(!(win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, width, height, 0,
		vis->depth, InputOutput, vis->visual,
		CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &swa)))
		THROW("Could not create window");
	XFree(vis);  vis = NULL;
	XSetWMProtocols(dpy, win, &deleteAtom, 1);
	if(!XMapRaised(dpy, win))
		THROW("Could not show window");
	XSync(dpy, False);

	if(interactive)
	{
		if(doInteractiveTest(I420_PLANAR, "I420 Planar") < 0)
		{
			retval = -1;  goto bailout;
		}
	}
	else if(id >= 0)
	{
		if(doTest(id, "User-specified") < 0) { retval = -1;  goto bailout; }
	}
	else
	{
		if(doTest(I420_PLANAR, "I420 Planar") < 0) { retval = -1;  goto bailout; }
		printf("\n");
		if(doTest(YV12_PLANAR, "YV12 Planar") < 0) { retval = -1;  goto bailout; }
		printf("\n");
		if(doTest(YUY2_PACKED, "YUY2 Packed") < 0) { retval = -1;  goto bailout; }
		printf("\n");
		if(doTest(UYVY_PACKED, "UYVY Packed") < 0) { retval = -1;  goto bailout; }
		printf("\n");
	}

	bailout:
	if(dpy && win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);
	return retval;
}
