/* Copyright (C)2007 Sun Microsystems, Inc.
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

/* This program's goal is to reproduce, as closely as possible, the image
   output of the NVidia SphereMark demo by R. Stephen Glanville using the
   simplest available rendering method.  GLXSpheres is meant primarily to
   serve as an image pipeline benchmark for VirtualGL. */


#include <GL/glx.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rrtimer.h"
#include "rrutil.h"


#define _throw(m) {fprintf(stderr, "ERROR (%d): %s\n", __LINE__,  m); \
	goto bailout;}
#define np2(i) ((i)>0? (1<<(int)(log((double)(i))/log(2.))) : 0)
#define _catch(f) {if((f)==-1) goto bailout;}

#define spherered(f) fabs(MAXI*(2.*f-1.))
#define spheregreen(f) fabs(MAXI*(2.*fmod(f+2./3., 1.)-1.))
#define sphereblue(f) fabs(MAXI*(2.*fmod(f+1./3., 1.)-1.))


#define DEF_WIDTH 1240
#define DEF_HEIGHT 900

#define DEF_SLICES 32
#define DEF_STACKS 32

#define NSPHERES 20

#define _2PI 6.283185307180
#define MAXI (220./255.)

#define NSCHEMES 7
enum {GREY=0, RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN};


Display *dpy=NULL;  Window win=0, olwin=0;
int usestereo=0, useoverlay=0, useci=0;
int ncolors=0, colorscheme=GREY;
Colormap colormap=0, olcolormap=0;
GLXContext ctx=0, olctx=0;

int spherelist=0;
GLUquadric *spherequad=NULL;
int slices=DEF_SLICES, stacks=DEF_STACKS;
float x=0., y=0., z=-3.;
float outer_angle=0., middle_angle=0., inner_angle=0.;
float lonesphere_color=0.;

int width=DEF_WIDTH, height=DEF_HEIGHT;


int setcolorscheme(Colormap c, int scheme)
{
	XColor xc[256];  int i;
	if(!ncolors || !colormap) _throw("Color map not allocated");
	if(scheme<0 || scheme>NSCHEMES-1 || !c) _throw("Invalid argument");

	for(i=0; i<ncolors; i++)
	{
		xc[i].flags=DoRed | DoGreen | DoBlue;
		xc[i].pixel=i;
		xc[i].red=xc[i].green=xc[i].blue=0;
		if(scheme==GREY || scheme==RED || scheme==YELLOW || scheme==MAGENTA)
			xc[i].red=(i*(256/ncolors))<<8;
		if(scheme==GREY || scheme==GREEN || scheme==YELLOW || scheme==CYAN)
			xc[i].green=(i*(256/ncolors))<<8;
		if(scheme==GREY || scheme==BLUE || scheme==MAGENTA || scheme==CYAN)
			xc[i].blue=(i*(256/ncolors))<<8;
	}
	XStoreColors(dpy, c, xc, ncolors);
	return 0;

	bailout:
	return -1;	
}


void reshape(int w, int h)
{
	if(w<=0) w=1;  if(h<=0) h=1;
	width=w;  height=h;
}


void setspherecolor(float c)
{
	if(useci)
	{
		GLfloat ndx=c*(float)(ncolors-1);
		GLfloat mat_ndxs[]={ndx*0.3, ndx*0.8, ndx};
		glIndexf(ndx);
		glMaterialfv(GL_FRONT, GL_COLOR_INDEXES, mat_ndxs);
	}
	else
	{
		float mat[4]={spherered(c), spheregreen(c), sphereblue(c), 0.25};
		glColor3f(spherered(c), spheregreen(c), sphereblue(c));
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
	}
}


void renderspheres(int buf)
{
	int i;
	GLfloat xaspect, yaspect;
	GLfloat stereocameraoffset=0., frustumassymetry=0.;
	GLfloat neardist=1.5, fardist=40., zeroparallaxdist=17.;
	glDrawBuffer(buf);

	xaspect=(GLfloat)width/(GLfloat)(min(width, height));
	yaspect=(GLfloat)height/(GLfloat)(min(width, height));

	if(buf==GL_BACK_LEFT)
		stereocameraoffset=-2.*xaspect*zeroparallaxdist/neardist*0.035;
	else if(buf==GL_BACK_RIGHT)
		stereocameraoffset=2.*xaspect*zeroparallaxdist/neardist*0.035;

	if(buf==GL_BACK_LEFT || buf==GL_BACK_RIGHT)
		frustumassymetry=-stereocameraoffset*neardist/zeroparallaxdist;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-xaspect+frustumassymetry, xaspect+frustumassymetry,
		-yaspect, yaspect, neardist, fardist);
	glTranslatef(-stereocameraoffset, 0., 0.);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, width, height);

	// Begin rendering
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(20.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Lone sphere
	glPushMatrix();
	glTranslatef(0., 0., z);
	setspherecolor(lonesphere_color);
	glCallList(spherelist);
	glPopMatrix();

	// Outer ring
	glPushMatrix();
	glRotatef(outer_angle, 0., 0., 1.);
	for(i=0; i<NSPHERES; i++)
	{
		double f=(double)i/(double)NSPHERES;
		glPushMatrix();
		glTranslatef(sin(_2PI*f)*5., cos(_2PI*f)*5., -10.);
		setspherecolor(f);
		glCallList(spherelist);
		glPopMatrix();
	}
	glPopMatrix();

	// Middle ring
	glPushMatrix();
	glRotatef(middle_angle, 0., 0., 1.);
	for(i=0; i<NSPHERES; i++)
	{
		double f=(double)i/(double)NSPHERES;
		glPushMatrix();
		glTranslatef(sin(_2PI*f)*5., cos(_2PI*f)*5., -17.);
		setspherecolor(f);
		glCallList(spherelist);
		glPopMatrix();
	}
	glPopMatrix();

	// Inner ring
	glPushMatrix();
	glRotatef(inner_angle, 0., 0., 1.);
	for(i=0; i<NSPHERES; i++)
	{
		double f=(double)i/(double)NSPHERES;
		glPushMatrix();
		glTranslatef(sin(_2PI*f)*5., cos(_2PI*f)*5., -29.);
		setspherecolor(f);
		glCallList(spherelist);
		glPopMatrix();
	}
	glPopMatrix();
}


int display(void)
{
	static int first=1;
	static double start=0., elapsed=0., mpixels=0.;
	static unsigned long frames=0;

	if(first)
	{
		GLfloat id4[]={1., 1., 1., 1.};
		GLfloat light0_amb[]={0.3, 0.3, 0.3, 1.};
		GLfloat light0_dif[]={0.8, 0.8, 0.8, 1.};
		GLfloat light0_pos[]={1., 1., 1., 0.};

		spherelist=glGenLists(1);
		if(!(spherequad=gluNewQuadric()))
			_throw("Could not allocate GLU quadric object");
		glNewList(spherelist, GL_COMPILE);
		gluSphere(spherequad, 1.3, slices, stacks);
		glEndList();

		if(useci)
		{
			glMaterialf(GL_FRONT, GL_SHININESS, 50.);
		}
		else
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, id4);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, id4);
			glMaterialfv(GL_FRONT, GL_SPECULAR, id4);
			glMaterialf(GL_FRONT, GL_SHININESS, 50.);

			glLightfv(GL_LIGHT0, GL_AMBIENT, light0_amb);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_dif);
			glLightfv(GL_LIGHT0, GL_SPECULAR, id4);
		}
		glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
		glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		first=0;
	}

	z-=0.5;
	if(z<-29.)
	{
		if(useci)
		{
			colorscheme=(colorscheme+1)%NSCHEMES;
			_catch(setcolorscheme(colormap, colorscheme));
		}
		z=-3.5;
	}
	outer_angle+=0.1;  if(outer_angle>360.) outer_angle-=360.;
	middle_angle-=0.37;  if(middle_angle<-360.) middle_angle+=360.;
	inner_angle+=0.63;  if(inner_angle>360.) inner_angle-=360.;
	lonesphere_color+=0.005;
	if(lonesphere_color>1.) lonesphere_color-=1.;

	if(usestereo)
	{
		renderspheres(GL_BACK_LEFT);  renderspheres(GL_BACK_RIGHT);
	}
	else renderspheres(GL_BACK);

	glXSwapBuffers(dpy, win);

	if(start>0.)
	{
		elapsed+=rrtime()-start;  frames++;
		mpixels+=(double)width*(double)height/1000000.;
		if(elapsed>2.)
		{
			printf("%f frames/sec - %f Mpixels/sec\n", (double)frames/elapsed,
				mpixels/elapsed);
			elapsed=mpixels=0.;  frames=0;
		}
	}
	start=rrtime();
	return 0;

	bailout:
	if(spherequad) gluDeleteQuadric(spherequad);
	return -1;
}


int event_loop(Display *dpy, Window win)
{
	while (1)
	{
		while(XPending(dpy)>0)
		{
			XEvent event;
			XNextEvent(dpy, &event);
			switch (event.type)
			{
				case Expose:
					break;
				case ConfigureNotify:
					reshape(event.xconfigure.width, event.xconfigure.height);
					break;
				case KeyPress:
				{
					char buf[10];  int key;
					key=XLookupString(&event.xkey, buf, sizeof(buf), NULL, NULL);
					switch(buf[0])
					{
						case 27: case 'q': case 'Q':
							return 0;
					}
				}
			}
		}
		_catch(display());
	}

	bailout:
	return -1;
}


void usage(char **argv)
{
	printf("USAGE: %s [-h|-?] [-i] [-o] [-s]\n\n", argv[0]);
	printf("-i = Use color index rendering (default is RGB)\n");
	printf("-o = Test 8-bit transparent overlays\n");
	printf("-s = Use stereographic rendering initially\n");
	printf("     (this can be switched on and off in the application)\n");
	exit(0);
}


int main(int argc, char **argv)
{
	int i;
	XVisualInfo *v=NULL;
	int rgbattribs[]={GLX_RGBA, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 1, GLX_DOUBLEBUFFER, GLX_STEREO, None};
	int ciattribs[]={GLX_BUFFER_SIZE, 8, GLX_DEPTH_SIZE, 1, GLX_DOUBLEBUFFER, 
		None};
	int olattribs[]={GLX_BUFFER_SIZE, 8, GLX_DOUBLEBUFFER, GLX_LEVEL, 1, None};
	XSetWindowAttributes swa;  Window root;

	for(i=0; i<argc; i++)
	{
		if(!strnicmp(argv[i], "-h", 2)) usage(argv);
		if(!strnicmp(argv[i], "-?", 2)) usage(argv);
		if(!strnicmp(argv[i], "-i", 2)) useci=1;
		if(!strnicmp(argv[i], "-o", 2)) useoverlay=1;
		if(!strnicmp(argv[i], "-s", 2)) usestereo=1;
	}

	if((dpy=XOpenDisplay(0))==NULL) _throw("Could not open display");

	if(useci)
	{
		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), ciattribs))==NULL)
			_throw("Could not obtain index visual");
	}
	else
	{
		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), rgbattribs))==NULL)
		{
			printf("WARNING: Could not obtain stereo visual.  Trying mono.\n");
			rgbattribs[10]=None;
			usestereo=0;
			if((v=glXChooseVisual(dpy, DefaultScreen(dpy), rgbattribs))==NULL)
				_throw("Could not obtain RGB visual");
		}
	}
	fprintf(stderr, "Visual ID of %s: 0x%.2x\n", useoverlay? "underlay":"window",
		(int)v->visualid);

	root=DefaultRootWindow(dpy);
	swa.border_pixel=0;
	swa.event_mask=StructureNotifyMask|ExposureMask|KeyPressMask;
	if(useci)
	{
		swa.colormap=colormap=XCreateColormap(dpy, root, v->visual, AllocAll);
		ncolors=np2(v->colormap_size);
		if(ncolors<32) _throw("Color map is not large enough");
		_catch(setcolorscheme(colormap, colorscheme));
	}
	else swa.colormap=XCreateColormap(dpy, root, v->visual, AllocNone);

	if((win=XCreateWindow(dpy, root, 0, 0, width, height, 0, v->depth,
		InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask, &swa))==0)
		_throw("Could not create window");
	XStoreName(dpy, win, "GLX Spheres");
	XMapWindow(dpy, win);
	XSync(dpy, False);

	if((ctx=glXCreateContext(dpy, v, NULL, True))==0)
		_throw("Could not create rendering context");
	XFree(v);  v=NULL;

	if(useoverlay)
	{
		if((v=glXChooseVisual(dpy, DefaultScreen(dpy), olattribs))==NULL)
			_throw("Could not obtain overlay visual");

		swa.colormap=olcolormap=XCreateColormap(dpy, root, v->visual, AllocAll);
		ncolors=np2(v->colormap_size);
		if(ncolors<32) _throw("Color map is not large enough");

		_catch(setcolorscheme(colormap, colorscheme));

		if((olwin=XCreateWindow(dpy, win, 0, 0, width, height, 0, v->depth,
			InputOutput, v->visual, CWBorderPixel|CWColormap|CWEventMask, &swa))==0)
			_throw("Could not create window");
		XMapWindow(dpy, olwin);
		XSync(dpy, False);

		if((olctx=glXCreateContext(dpy, v, NULL, True))==0)
			_throw("Could not create overlay rendering context");

		XFree(v);  v=NULL;
	}

	if(!glXMakeCurrent(dpy, win, ctx))
		_throw("Could not bind rendering context");

	_catch(event_loop(dpy, win));

	if(dpy && olctx) {glXDestroyContext(dpy, olctx);  olctx=0;}
	if(dpy && olwin) {XDestroyWindow(dpy, olwin);  olwin=0;}
	if(dpy && ctx) {glXDestroyContext(dpy, ctx);  ctx=0;}
	if(dpy && win) {XDestroyWindow(dpy, win);  win=0;}
	if(dpy) XCloseDisplay(dpy);
	return 0;

	bailout:
	if(v) XFree(v);
	if(dpy && olctx) {glXDestroyContext(dpy, olctx);  olctx=0;}
	if(dpy && olwin) {XDestroyWindow(dpy, olwin);  olwin=0;}
	if(dpy && ctx) {glXDestroyContext(dpy, ctx);  ctx=0;}
	if(dpy && win) {XDestroyWindow(dpy, win);  win=0;}
	if(dpy) XCloseDisplay(dpy);
	return -1;
}
