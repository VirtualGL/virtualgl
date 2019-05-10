/* Copyright (C)2007 Sun Microsystems, Inc.
 * Copyright (C)2011, 2013-2015, 2017-2019 D. R. Commander
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

/* This program's goal is to reproduce, as closely as possible, the image
   output of the nVidia SphereMark demo by R. Stephen Glanville using the
   simplest available rendering method.  GLXSpheres is meant primarily to
   serve as an image pipeline benchmark for VirtualGL. */


#include <GL/glx.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Timer.h"
#include "vglutil.h"


#define THROW(m) \
{ \
	fprintf(stderr, "ERROR in line %d:\n%s\n", __LINE__,  m); \
	retval = -1; \
	goto bailout; \
}

#define CATCH(f) \
{ \
	if((f) == -1) \
	{ \
		retval = -1; \
		goto bailout; \
	} \
}

#define NP2(i)  ((i) > 0 ? (1 << (int)(log((double)(i)) / log(2.))) : 0)

#define SPHERE_RED(f)  fabs(MAXI * (2. * f - 1.))
#define SPHERE_GREEN(f)  fabs(MAXI * (2. * fmod(f + 2. / 3., 1.) - 1.))
#define SPHERE_BLUE(f)  fabs(MAXI * (2. * fmod(f + 1. / 3., 1.) - 1.))


#define DEF_WIDTH  1240
#define DEF_HEIGHT  900

#define DEF_SLICES  32
#define DEF_STACKS  32

#define DEF_SPHERES  20

#define _2PI  6.283185307180
#define MAXI  (220. / 255.)

#define NSCHEMES  7
enum { GRAY = 0, RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN };

#define DEFBENCHTIME  2.0

Display *dpy = NULL;  Window win = 0, olWin = 0;
GLXContext ctx = 0, olCtx = 0;
int useStereo = 0, useOverlay = 0, useDC = 0, useImm = 0, interactive = 0,
	olDB = 1, loColor = 0, maxFrames = 0, totalFrames = 0, directCtx = True,
	bpc = -1, deadYet = 0;
int rshift = 0, gshift = 0, bshift = 0;
double benchTime = DEFBENCHTIME;
int nColors = 0, nOlColors, colorScheme = GRAY;
Colormap colormap = 0, olColormap = 0;

int sphereList = 0, fontListBase = 0;
GLUquadricObj *sphereQuad = NULL;
int slices = DEF_SLICES, stacks = DEF_STACKS, spheres = DEF_SPHERES;
GLfloat x = 0., y = 0., z = -3.;
GLfloat outerAngle = 0., middleAngle = 0., innerAngle = 0.;
GLfloat loneSphereColor = 0.;
unsigned int transPixel = 0;

int width = DEF_WIDTH, height = DEF_HEIGHT;


static int setColorScheme(Colormap cmap, int colors, int bits, int scheme)
{
	XColor xc[1024];  int i, maxColors = (1 << bits), retval = 0;

	if(!colors || !cmap) THROW("Color map not allocated");
	if(scheme < 0 || scheme > NSCHEMES - 1 || !cmap) THROW("Invalid argument");

	for(i = 0; i < colors; i++)
	{
		xc[i].flags = DoRed | DoGreen | DoBlue;
		xc[i].pixel =
			(cmap == olColormap) ? i : (i << rshift) | (i << gshift) | (i << bshift);
		xc[i].red = xc[i].green = xc[i].blue = 0;
		if(scheme == GRAY || scheme == RED || scheme == YELLOW
			|| scheme == MAGENTA)
			xc[i].red = (i * (maxColors / colors)) << (16 - bits);
		if(scheme == GRAY || scheme == GREEN || scheme == YELLOW
			|| scheme == CYAN)
			xc[i].green = (i * (maxColors / colors)) << (16 - bits);
		if(scheme == GRAY || scheme == BLUE || scheme == MAGENTA
			|| scheme == CYAN)
			xc[i].blue = (i * (maxColors / colors)) << (16 - bits);
	}
	XStoreColors(dpy, cmap, xc, colors);

	bailout:
	return retval;
}


static void reshape(int newWidth, int newHeight)
{
	if(newWidth <= 0) newWidth = 1;
	if(newHeight <= 0) newHeight = 1;
	width = newWidth;  height = newHeight;

	if(useOverlay && olWin)
	{
		XWindowChanges changes;
		changes.width = width;
		changes.height = height;
		XConfigureWindow(dpy, olWin, CWWidth | CWHeight, &changes);
	}
}


static void setSphereColor(GLfloat color)
{
	if(useDC)
	{
		GLfloat mat[] = { color, color, color };
		glColor3f(color, color, color);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
	}
	else
	{
		GLfloat mat[4] =
		{
			SPHERE_RED(color), SPHERE_GREEN(color), SPHERE_BLUE(color), 0.25
		};
		glColor3f(SPHERE_RED(color), SPHERE_GREEN(color), SPHERE_BLUE(color));
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
	}
}


static void renderSpheres(int buf)
{
	int i;
	GLfloat xAspect, yAspect;
	GLfloat stereoCameraOffset = 0.;
	GLfloat nearDist = 1.5, farDist = 40., zeroParallaxDist = 17.;

	glDrawBuffer(buf);

	xAspect = (GLfloat)width / (GLfloat)(min(width, height));
	yAspect = (GLfloat)height / (GLfloat)(min(width, height));

	if(buf == GL_BACK_LEFT)
		stereoCameraOffset = -xAspect * zeroParallaxDist / nearDist * 0.035;
	else if(buf == GL_BACK_RIGHT)
		stereoCameraOffset = xAspect * zeroParallaxDist / nearDist * 0.035;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-xAspect, xAspect, -yAspect, yAspect, nearDist, farDist);
	glTranslatef(-stereoCameraOffset, 0., 0.);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);

	/* Begin rendering */
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(20.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Lone sphere */
	glPushMatrix();
	glTranslatef(0., 0., z);
	setSphereColor(loneSphereColor);
	if(useImm) gluSphere(sphereQuad, 1.3, slices, stacks);
	else glCallList(sphereList);
	glPopMatrix();

	/* Outer ring */
	glPushMatrix();
	glRotatef(outerAngle, 0., 0., 1.);
	for(i = 0; i < spheres; i++)
	{
		double f = (double)i / (double)spheres;
		glPushMatrix();
		glTranslatef(sin(_2PI * f) * 5., cos(_2PI * f) * 5., -10.);
		setSphereColor(f);
		if(useImm) gluSphere(sphereQuad, 1.3, slices, stacks);
		else glCallList(sphereList);
		glPopMatrix();
	}
	glPopMatrix();

	/* Middle ring */
	glPushMatrix();
	glRotatef(middleAngle, 0., 0., 1.);
	for(i = 0; i < spheres; i++)
	{
		double f = (double)i / (double)spheres;
		glPushMatrix();
		glTranslatef(sin(_2PI * f) * 5., cos(_2PI * f) * 5., -17.);
		setSphereColor(f);
		if(useImm) gluSphere(sphereQuad, 1.3, slices, stacks);
		else glCallList(sphereList);
		glPopMatrix();
	}
	glPopMatrix();

	/* Inner ring */
	glPushMatrix();
	glRotatef(innerAngle, 0., 0., 1.);
	for(i = 0; i < spheres; i++)
	{
		double f = (double)i / (double)spheres;
		glPushMatrix();
		glTranslatef(sin(_2PI * f) * 5., cos(_2PI * f) * 5., -29.);
		setSphereColor(f);
		if(useImm) gluSphere(sphereQuad, 1.3, slices, stacks);
		else glCallList(sphereList);
		glPopMatrix();
	}
	glPopMatrix();

	/* Move the eye back to the middle */
	glMatrixMode(GL_PROJECTION);
	glTranslatef(stereoCameraOffset, 0., 0.);
}


static int renderOverlay(void)
{
	int i, j, w = width / 8, h = height / 8;  unsigned char *buf = NULL;
	int index = (int)(loneSphereColor * (GLfloat)(nOlColors - 1));
	int retval = 0;

	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);
	glRasterPos3f(-0.5, 0.5, 0.0);
	glClearIndex((GLfloat)transPixel);
	glClear(GL_COLOR_BUFFER_BIT);
	glIndexf(loneSphereColor * (GLfloat)(nOlColors - 1));
	glBegin(GL_LINES);
	glVertex2f(-1. + loneSphereColor * 2., -1.);
	glVertex2f(-1. + loneSphereColor * 2., 1.);
	glVertex2f(-1., -1. + loneSphereColor * 2.);
	glVertex2f(1., -1. + loneSphereColor * 2.);
	glEnd();
	if(w && h)
	{
		if((buf = (unsigned char *)malloc(w * h)) == NULL)
			THROW("Could not allocate memory");
		for(i = 0; i < h; i++)
			for(j = 0; j < w; j++)
			{
				if(((i / 4) % 2) != ((j / 4) % 2)) buf[i * w + j] = 0;
				else buf[i * w + j] = index;
			}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glDrawPixels(w, h, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buf);
	}
	glRasterPos3f(0., 0., 0.);
	glListBase(fontListBase);
	glCallLists(24, GL_UNSIGNED_BYTE, "GLX Spheres Overlay Test");

	bailout:
	if(buf) free(buf);
	return retval;
}


static int display(int advance)
{
	static int first = 1;
	static double start = 0., elapsed = 0., mpixels = 0.;
	static unsigned long frames = 0;
	static char temps[700];
	XFontStruct *fontInfo = NULL;  int minChar, maxChar;
	GLfloat xAspect, yAspect;
	int retval = 0;

	if(first)
	{
		GLfloat id4[] = { 1., 1., 1., 1. };
		GLfloat light0Amb[] = { 0.3, 0.3, 0.3, 1. };
		GLfloat light0Dif[] = { 0.8, 0.8, 0.8, 1. };
		GLfloat light0Pos[] = { 1., 1., 1., 0. };

		sphereList = glGenLists(1);
		if(!(sphereQuad = gluNewQuadric()))
			THROW("Could not allocate GLU quadric object");
		glNewList(sphereList, GL_COMPILE);
		gluSphere(sphereQuad, 1.3, slices, stacks);
		glEndList();

		if(!loColor)
		{
			if(useDC)
			{
				glMaterialf(GL_FRONT, GL_SHININESS, 50.);
			}
			else
			{
				glMaterialfv(GL_FRONT, GL_AMBIENT, id4);
				glMaterialfv(GL_FRONT, GL_DIFFUSE, id4);
				glMaterialfv(GL_FRONT, GL_SPECULAR, id4);
				glMaterialf(GL_FRONT, GL_SHININESS, 50.);

				glLightfv(GL_LIGHT0, GL_AMBIENT, light0Amb);
				glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Dif);
				glLightfv(GL_LIGHT0, GL_SPECULAR, id4);
			}
			glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
			glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		}

		if(loColor) glShadeModel(GL_FLAT);
		else glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		if(useOverlay)
		{
			glXMakeCurrent(dpy, olWin, olCtx);
		}
		if(!(fontInfo = XLoadQueryFont(dpy, "fixed")))
			THROW("Could not load X font");
		minChar = fontInfo->min_char_or_byte2;
		maxChar = fontInfo->max_char_or_byte2;
		fontListBase = glGenLists(maxChar + 1);
		glXUseXFont(fontInfo->fid, minChar, maxChar - minChar + 1,
			fontListBase + minChar);
		XFreeFont(dpy, fontInfo);  fontInfo = NULL;
		snprintf(temps, 700, "Measuring performance ...");
		if(useOverlay) glXMakeCurrent(dpy, win, ctx);

		first = 0;
	}

	if(advance)
	{
		z -= 0.5;
		if(z < -29.)
		{
			if(useDC || useOverlay) colorScheme = (colorScheme + 1) % NSCHEMES;
			if(useDC) CATCH(setColorScheme(colormap, nColors, bpc, colorScheme));
			if(useOverlay)
				CATCH(setColorScheme(olColormap, nOlColors, 8, colorScheme));
			z = -3.5;
		}
		outerAngle += 0.1;  if(outerAngle > 360.) outerAngle -= 360.;
		middleAngle -= 0.37;  if(middleAngle < -360.) middleAngle += 360.;
		innerAngle += 0.63;  if(innerAngle > 360.) innerAngle -= 360.;
		loneSphereColor += 0.005;
		if(loneSphereColor > 1.) loneSphereColor -= 1.;
	}

	if(useStereo)
	{
		renderSpheres(GL_BACK_LEFT);  renderSpheres(GL_BACK_RIGHT);
	}
	else renderSpheres(GL_BACK);

	glDrawBuffer(GL_BACK);
	if(useOverlay)
	{
		glXMakeCurrent(dpy, olWin, olCtx);
		CATCH(renderOverlay());
	}
	else glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_LIST_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	if(useOverlay || useDC) glColor3f(254., 254., 254.);
	else glColor3f(1., 1., 1.);
	if(useOverlay) glRasterPos3f(-0.95, -0.95, 0.);
	else
	{
		xAspect = (GLfloat)width / (GLfloat)(min(width, height));
		yAspect = (GLfloat)height / (GLfloat)(min(width, height));
		glRasterPos3f(-0.95 * xAspect, -0.95 * yAspect, -1.5);
	}
	glListBase(fontListBase);
	glCallLists(strlen(temps), GL_UNSIGNED_BYTE, temps);
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	if(useOverlay)
	{
		if(olDB) glXSwapBuffers(dpy, olWin);
		glXMakeCurrent(dpy, win, ctx);
	}
	glXSwapBuffers(dpy, win);

	if(start > 0.)
	{
		elapsed += GetTime() - start;  frames++;  totalFrames++;
		mpixels += (double)width * (double)height / 1000000.;
		if(elapsed > benchTime || (maxFrames && totalFrames > maxFrames))
		{
			snprintf(temps, 700, "%f frames/sec - %f Mpixels/sec",
				(double)frames / elapsed, mpixels / elapsed);
			printf("%s\n", temps);
			elapsed = mpixels = 0.;  frames = 0;
		}
	}
	if(maxFrames && totalFrames > maxFrames)
	{
		deadYet = 1;  goto bailout;
	}

	start = GetTime();
	return 0;

	bailout:
	if(sphereQuad) { gluDeleteQuadric(sphereQuad);  sphereQuad = NULL; }
	return retval;
}


Atom protoAtom = 0, deleteAtom = 0;


static int eventLoop(void)
{
	int retval = 0;

	while(!deadYet)
	{
		int advance = 0, doDisplay = 0;

		while(1)
		{
			XEvent event;
			if(interactive) XNextEvent(dpy, &event);
			else
			{
				if(XPending(dpy) > 0) XNextEvent(dpy, &event);
				else break;
			}
			switch(event.type)
			{
				case Expose:
					doDisplay = 1;
					break;
				case ConfigureNotify:
					reshape(event.xconfigure.width, event.xconfigure.height);
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
					if(event.xmotion.state & Button1Mask) doDisplay = advance = 1;
					break;
				case ClientMessage:
				{
					XClientMessageEvent *cme = (XClientMessageEvent *)&event;
					if(cme->message_type == protoAtom
						&& cme->data.l[0] == (long)deleteAtom)
						return 0;
				}
			}
			if(interactive)
			{
				if(XPending(dpy) <= 0) break;
			}
		}
		if(!interactive) { CATCH(display(1)); }
		else
		{
			if(doDisplay) { CATCH(display(advance)); }
		}
	}

	bailout:
	return retval;
}


static void usage(char **argv)
{
	printf("\nUSAGE: %s [options]\n\n", argv[0]);
	printf("Options:\n");
	printf("-dc = Use DirectColor rendering (default is TrueColor)\n");
	printf("-fs = Full-screen mode\n");
	printf("-i = Interactive mode.  Frames advance in response to mouse movement\n");
	printf("-l = Use fewer than 24 colors (to force non-JPEG encoding in TurboVNC)\n");
	printf("-m = Use immediate mode rendering (default is display list)\n");
	printf("-o = Test 8-bit transparent overlays\n");
	printf("-p <p> = Use (approximately) <p> polygons to render scene\n");
	printf("         (max. is 57600 per sphere due to limitations of GLU.)\n");
	printf("-n <n> = Render (approximately) <n> spheres (default: %d)\n",
		DEF_SPHERES * 3 + 1);
	printf("-s = Use stereographic rendering initially\n");
	printf("     (this can be switched on and off in the application)\n");
	printf("-alpha = Use a visual with an alpha channel\n");
	printf("-bpc = Specify bits per component (default: determined from the default depth\n");
	printf("       of the X server)\n");
	printf("-f <n> = max frames to render\n");
	printf("-bt <t> = print benchmark results every <t> seconds (default: %.1f)\n",
		DEFBENCHTIME);
	printf("-w <wxh> = specify window width and height\n");
	printf("-ic = Use indirect rendering context\n");
	printf("-sc <s> = Create window on X screen # <s>\n");
	printf("\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int i, n = 0, useAlpha = 0, nPolys = -1, retval = 0;
	XVisualInfo *v = NULL;
	GLXFBConfig *c = NULL;
	int rgbAttribs[] = { GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 1,
		GLX_DOUBLEBUFFER, 1, GLX_STEREO, 0, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		None, None, None };
	int olAttribs[] = { GLX_BUFFER_SIZE, 8, GLX_LEVEL, 1,
		GLX_TRANSPARENT_TYPE, GLX_TRANSPARENT_INDEX, GLX_DOUBLEBUFFER, None };
	XSetWindowAttributes swa;  Window root;
	int fullScreen = 0;  unsigned long mask = 0;
	int screen = -1, pps;
	int fbcid = -1, red = -1, green = -1, blue = -1, alpha = -1;

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!stricmp(argv[i], "-dc"))
		{
			useDC = 1;  rgbAttribs[15] = GLX_DIRECT_COLOR;
		}
		else if(!stricmp(argv[i], "-ic")) directCtx = False;
		else if(!stricmp(argv[i], "-i")) interactive = 1;
		else if(!stricmp(argv[i], "-l")) loColor = 1;
		else if(!stricmp(argv[i], "-m")) useImm = 1;
		else if(!stricmp(argv[i], "-o")) useOverlay = 1;
		else if(!stricmp(argv[i], "-w") && i < argc - 1)
		{
			if(sscanf(argv[++i], "%dx%d", &width, &height) < 2 || width < 1
				|| height < 1)
				usage(argv);
			printf("Window dimensions: %d x %d\n", width, height);
		}
		else if(!stricmp(argv[i], "-fs")) fullScreen = 1;
		else if(!stricmp(argv[i], "-f") && i < argc - 1)
		{
			maxFrames = atoi(argv[++i]);
			if(maxFrames <= 0) usage(argv);
			printf("Number of frames to render: %d\n", maxFrames);
		}
		else if(!stricmp(argv[i], "-bt") && i < argc - 1)
		{
			benchTime = atof(argv[++i]);
			if(benchTime <= 0.0) usage(argv);
		}
		else if(!stricmp(argv[i], "-sc") && i < argc - 1)
		{
			screen = atoi(argv[++i]);
			if(screen < 0) usage(argv);
			printf("Rendering to screen %d\n", screen);
		}
		else if(!stricmp(argv[i], "-s"))
		{
			rgbAttribs[13] = 1;
			useStereo = 1;
		}
		else if(!stricmp(argv[i], "-alpha")) useAlpha = 1;
		else if(!stricmp(argv[i], "-bpc") && i < argc - 1)
		{
			int temp = atoi(argv[++i]);
			if(temp != 8 && temp != 10) usage(argv);
			bpc = temp;
		}
		else if(!stricmp(argv[i], "-n") && i < argc - 1)
		{
			int temp = atoi(argv[++i]);
			if(temp <= 0) usage(argv);
			spheres = (int)(((double)temp - 1.0) / 3.0 + 0.5);
			if(spheres < 1) spheres = 1;
		}
		else if(!stricmp(argv[i], "-p") && i < argc - 1)
		{
			nPolys = atoi(argv[++i]);
			if(nPolys <= 0) usage(argv);
		}
		else usage(argv);
	}

	if(nPolys >= 0)
	{
		slices = stacks =
			(int)(sqrt((double)nPolys / ((double)(3 * spheres + 1))) + 0.5);
		if(slices < 1) slices = stacks = 1;
	}

	pps = slices * stacks;
	if(pps > 57600)
	{
		fprintf(stderr, "WARNING: polygons per sphere clamped to 57600 due to limitations of GLU\n");
		pps = 57600;
	}
	fprintf(stderr, "Polygons in scene: %d (%d spheres * %d polys/spheres)\n",
		(spheres * 3 + 1) * pps, spheres * 3 + 1, pps);

	if((dpy = XOpenDisplay(0)) == NULL) THROW("Could not open display");
	if(screen < 0) screen = DefaultScreen(dpy);

	if(bpc < 0)
	{
		if(DefaultDepth(dpy, DefaultScreen(dpy)) == 30) bpc = 10;
		else bpc = 8;
	}
	rgbAttribs[3] = rgbAttribs[5] = rgbAttribs[7] = bpc;
	if(useAlpha)
	{
		rgbAttribs[16] = GLX_ALPHA_SIZE;
		rgbAttribs[17] = 32 - bpc * 3;
	}

	c = glXChooseFBConfig(dpy, screen, rgbAttribs, &n);
	if(!c || n < 1)
		THROW("Could not obtain RGB visual with requested properties");
	if((v = glXGetVisualFromFBConfig(dpy, c[0])) == NULL)
		THROW("Could not obtain RGB visual with requested properties");
	glXGetFBConfigAttrib(dpy, c[0], GLX_FBCONFIG_ID, &fbcid);
	glXGetFBConfigAttrib(dpy, c[0], GLX_RED_SIZE, &red);
	glXGetFBConfigAttrib(dpy, c[0], GLX_GREEN_SIZE, &green);
	glXGetFBConfigAttrib(dpy, c[0], GLX_BLUE_SIZE, &blue);
	glXGetFBConfigAttrib(dpy, c[0], GLX_ALPHA_SIZE, &alpha);
	fprintf(stderr, "GLX FB config ID of %s: 0x%.2x (%d/%d/%d/%d)\n",
		useOverlay ? "underlay" : "window", fbcid, red, green, blue, alpha);
	XFree(c);  c = NULL;
	fprintf(stderr, "Visual ID of %s: 0x%.2x\n",
		useOverlay ? "underlay" : "window", (int)v->visualid);

	root = RootWindow(dpy, screen);
	swa.border_pixel = 0;
	swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	swa.override_redirect = fullScreen ? True : False;
	if(useDC)
	{
		swa.colormap = colormap = XCreateColormap(dpy, root, v->visual, AllocAll);
		nColors = NP2(v->colormap_size);
		if(nColors < 32) THROW("Color map is not large enough");
		rshift = 0;  while((v->red_mask & (1 << rshift)) == 0) rshift++;
		gshift = 0;  while((v->green_mask & (1 << gshift)) == 0) gshift++;
		bshift = 0;  while((v->blue_mask & (1 << bshift)) == 0) bshift++;
		CATCH(setColorScheme(colormap, nColors, bpc, colorScheme));
	}
	else swa.colormap = XCreateColormap(dpy, root, v->visual, AllocNone);

	if(interactive) swa.event_mask |= PointerMotionMask | ButtonPressMask;

	mask = CWBorderPixel | CWColormap | CWEventMask;
	if(fullScreen)
	{
		mask |= CWOverrideRedirect;
		width = DisplayWidth(dpy, screen);
		height = DisplayHeight(dpy, screen);
	}
	if(!(protoAtom = XInternAtom(dpy, "WM_PROTOCOLS", False)))
		THROW("Cannot obtain WM_PROTOCOLS atom");
	if(!(deleteAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", False)))
		THROW("Cannot obtain WM_DELETE_WINDOW atom");
	if((win = XCreateWindow(dpy, root, 0, 0, width, height, 0, v->depth,
		InputOutput, v->visual, mask, &swa)) == 0)
		THROW("Could not create window");
	XSetWMProtocols(dpy, win, &deleteAtom, 1);
	XStoreName(dpy, win, "GLX Spheres");
	XMapWindow(dpy, win);
	if(fullScreen)
	{
		XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
		XGrabKeyboard(dpy, win, GrabModeAsync, True, GrabModeAsync, CurrentTime);
	}
	XSync(dpy, False);

	if((ctx = glXCreateContext(dpy, v, NULL, directCtx)) == 0)
		THROW("Could not create rendering context");
	fprintf(stderr, "Context is %s\n",
		glXIsDirect(dpy, ctx) ? "Direct" : "Indirect");
	XFree(v);  v = NULL;

	if(useOverlay)
	{
		if((v = glXChooseVisual(dpy, screen, olAttribs)) == NULL)
		{
			olAttribs[6] = None;  olDB = 0;
			if((v = glXChooseVisual(dpy, screen, olAttribs)) == NULL)
				THROW("Could not obtain overlay visual");
		}
		fprintf(stderr, "Visual ID of overlay: 0x%.2x\n", (int)v->visualid);

		swa.colormap = olColormap =
			XCreateColormap(dpy, root, v->visual, AllocAll);
		nOlColors = NP2(v->colormap_size);
		if(nOlColors < 32) THROW("Color map is not large enough");

		CATCH(setColorScheme(olColormap, nOlColors, 8, colorScheme));

		if((olWin = XCreateWindow(dpy, win, 0, 0, width, height, 0, v->depth,
			InputOutput, v->visual, CWBorderPixel | CWColormap | CWEventMask,
			&swa)) == 0)
			THROW("Could not create overlay window");
		XMapWindow(dpy, olWin);
		XSync(dpy, False);

		if((olCtx = glXCreateContext(dpy, v, NULL, directCtx)) == 0)
			THROW("Could not create overlay rendering context");
		fprintf(stderr, "Overlay context is %s\n",
			glXIsDirect(dpy, olCtx) ? "Direct" : "Indirect");

		XFree(v);  v = NULL;
	}

	if(!glXMakeCurrent(dpy, win, ctx))
		THROW("Could not bind rendering context");

	fprintf(stderr, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

	CATCH(eventLoop());

	bailout:
	if(v) XFree(v);
	if(c) XFree(c);
	if(dpy && olCtx) glXDestroyContext(dpy, olCtx);
	if(dpy && olWin) XDestroyWindow(dpy, olWin);
	if(dpy && ctx) glXDestroyContext(dpy, ctx);
	if(dpy && win) XDestroyWindow(dpy, win);
	if(dpy) XCloseDisplay(dpy);

	return retval;
}
