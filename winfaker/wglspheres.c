/* Copyright (C)2008 Sun Microsystems, Inc.
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


#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rrtimer.h"


static char __lasterror[1024]="No error";
#define _throw(m) {fprintf(stderr, "ERROR in line %d:\n%s\n", __LINE__,  m); \
	goto bailout;}
#define _throww32(m) { \
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), __lasterror, 1024, NULL)) \
		fprintf(stderr, "WIN32 ERROR in line %d:\n%s\n%s", __LINE__, m, __lasterror); \
	goto bailout; \
}

#define np2(i) ((i)>0? (1<<(int)(log((double)(i))/log(2.))) : 0)
#define _catch(f) {if((f)==-1) goto bailout;}

#define spherered(f) (GLfloat)fabs(MAXI*(2.*f-1.))
#define spheregreen(f) (GLfloat)fabs(MAXI*(2.*fmod(f+2./3., 1.)-1.))
#define sphereblue(f) (GLfloat)fabs(MAXI*(2.*fmod(f+1./3., 1.)-1.))


#define DEF_WIDTH 1240
#define DEF_HEIGHT 900

#define DEF_SLICES 32
#define DEF_STACKS 32

#define NSPHERES 20

#define _2PI 6.283185307180
#define MAXI (220./255.)


HWND win=0;  HDC hdc=0;
int usestereo=0, useimm=0, interactive=0, locolor=0;
HGLRC ctx=0;

int spherelist=0, fontlistbase=0;
GLUquadricObj *spherequad=NULL;
int slices=DEF_SLICES, stacks=DEF_STACKS;
float x=0., y=0., z=-3.;
float outer_angle=0., middle_angle=0., inner_angle=0.;
float lonesphere_color=0.;
unsigned int transpixel=0;
int dodisplay=0, advance=0;
int width=DEF_WIDTH, height=DEF_HEIGHT;


void reshape(int w, int h)
{
	if(w<=0) w=1;  if(h<=0) h=1;
	width=w;  height=h;
}


void setspherecolor(double c)
{
	GLfloat mat[4]={spherered(c), spheregreen(c), sphereblue(c), 0.25};
	glColor3d(spherered(c), spheregreen(c), sphereblue(c));
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
}


void renderspheres(int buf)
{
	int i;
	GLfloat xaspect, yaspect;
	GLfloat stereocameraoffset=0.;
	GLfloat neardist=1.5, fardist=40., zeroparallaxdist=17.;
	glDrawBuffer(buf);

	xaspect=(GLfloat)width/(GLfloat)(min(width, height));
	yaspect=(GLfloat)height/(GLfloat)(min(width, height));

	if(buf==GL_BACK_LEFT)
		stereocameraoffset=-xaspect*zeroparallaxdist/neardist*0.035f;
	else if(buf==GL_BACK_RIGHT)
		stereocameraoffset=xaspect*zeroparallaxdist/neardist*0.035f;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-xaspect, xaspect, -yaspect, yaspect, neardist, fardist);
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
	if(useimm) gluSphere(spherequad, 1.3, slices, stacks);
	else glCallList(spherelist);
	glPopMatrix();

	// Outer ring
	glPushMatrix();
	glRotatef(outer_angle, 0., 0., 1.);
	for(i=0; i<NSPHERES; i++)
	{
		double f=(double)i/(double)NSPHERES;
		glPushMatrix();
		glTranslatef((GLfloat)sin(_2PI*f)*5.0f, (GLfloat)cos(_2PI*f)*5.0f, -10.0f);
		setspherecolor(f);
		if(useimm) gluSphere(spherequad, 1.3, slices, stacks);
		else glCallList(spherelist);
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
		glTranslatef((GLfloat)sin(_2PI*f)*5.0f, (GLfloat)cos(_2PI*f)*5.0f, -17.0f);
		setspherecolor(f);
		if(useimm) gluSphere(spherequad, 1.3, slices, stacks);
		else glCallList(spherelist);
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
		glTranslatef((GLfloat)sin(_2PI*f)*5.0f, (GLfloat)cos(_2PI*f)*5.0f, -29.0f);
		setspherecolor(f);
		if(useimm) gluSphere(spherequad, 1.3, slices, stacks);
		else glCallList(spherelist);
		glPopMatrix();
	}
	glPopMatrix();

	// Move the eye back to the middle
	glMatrixMode(GL_PROJECTION);
	glTranslatef(stereocameraoffset, 0., 0.);
}


int display(int advance)
{
	static int first=1;
	static double start=0., elapsed=0., mpixels=0.;
	static unsigned long frames=0;
	static char temps[256];
	GLfloat xaspect, yaspect;

	if(first)
	{
		GLfloat id4[]={1.0f, 1.0f, 1.0f, 1.0f};
		GLfloat light0_amb[]={0.3f, 0.3f, 0.3f, 1.0f};
		GLfloat light0_dif[]={0.8f, 0.8f, 0.8f, 1.0f};
		GLfloat light0_pos[]={1.0f, 1.0f, 1.0f, 0.0f};

		spherelist=glGenLists(1);
		if(!(spherequad=gluNewQuadric()))
			_throw("Could not allocate GLU quadric object");
		glNewList(spherelist, GL_COMPILE);
		gluSphere(spherequad, 1.3, slices, stacks);
		glEndList();

		if(!locolor)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, id4);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, id4);
			glMaterialfv(GL_FRONT, GL_SPECULAR, id4);
			glMaterialf(GL_FRONT, GL_SHININESS, 50.);

			glLightfv(GL_LIGHT0, GL_AMBIENT, light0_amb);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_dif);
			glLightfv(GL_LIGHT0, GL_SPECULAR, id4);
			glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
			glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
  	}

		if(locolor) glShadeModel(GL_FLAT);
		else glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		fontlistbase=glGenLists(256+1);
		if(!wglUseFontBitmaps(hdc, 0, 256, fontlistbase))
			_throww32("Cannot create font display lists");
		_snprintf(temps, 255, "Measuring performance ...");

		first=0;
	}

	if(advance)
	{
		z-=0.5;
		if(z<-29.) z=-3.5;
		outer_angle+=0.1f;  if(outer_angle>360.) outer_angle-=360.;
		middle_angle-=0.37f;  if(middle_angle<-360.) middle_angle+=360.;
		inner_angle+=0.63f;  if(inner_angle>360.) inner_angle-=360.;
		lonesphere_color+=0.005f;
		if(lonesphere_color>1.) lonesphere_color-=1.;
	}

	if(usestereo)
	{
		renderspheres(GL_BACK_LEFT);  renderspheres(GL_BACK_RIGHT);
	}
	else renderspheres(GL_BACK);

	glDrawBuffer(GL_BACK);
	glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_LIST_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glColor3f(1., 1., 1.);
	xaspect=(GLfloat)width/(GLfloat)(min(width, height));
	yaspect=(GLfloat)height/(GLfloat)(min(width, height));
	glRasterPos3f(-0.95f*xaspect, -0.95f*yaspect, -1.5f); 
	glListBase(fontlistbase);
	glCallLists(strlen(temps), GL_UNSIGNED_BYTE, temps);
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	SwapBuffers(hdc);

	if(start>0.)
	{
		elapsed+=rrtime()-start;  frames++;
		mpixels+=(double)width*(double)height/1000000.;
		if(elapsed>2.)
		{
			_snprintf(temps, 255, "%f frames/sec - %f Mpixels/sec",
				(double)frames/elapsed, mpixels/elapsed);
			printf("%s\n", temps);
			elapsed=mpixels=0.;  frames=0;
		}
	}

	start=rrtime();
	return 0;

	bailout:
	if(spherequad) {gluDeleteQuadric(spherequad);  spherequad=NULL;}
	return -1;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
		case WM_CREATE:
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_PAINT:
			dodisplay=1;
			return 0;
		case WM_SIZE:
			reshape(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_CHAR:
			if(wParam==27 || wParam=='q' || wParam=='Q')
			{
				PostQuitMessage(0);
				return 0;
			}
			break;
		case WM_MOUSEMOVE:
			if(wParam & MK_LBUTTON)
			{
				dodisplay=advance=1;
				return 0;
			}
			break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


void usage(char **argv)
{
	printf("USAGE: %s [-h|-?] [-i] [-l] [-m] [-s] [-fs] [-p <p>]\n\n", argv[0]);
	printf("-i = Interactive mode.  Frames advance in response to mouse movement\n");
	printf("-l = Generate an image with < 24 colors (useful for testing TurboVNC)\n");
	printf("-m = Use immediate mode rendering (default is display list)\n");
	printf("-p <p> = Use (approximately) <p> polygons to render scene\n");
	printf("-s = Use stereographic rendering initially\n");
	printf("     (this can be switched on and off in the application)\n");
	printf("-fs = Full-screen mode\n");
	exit(0);
}


int main(int argc, char **argv)
{
	int i;
	WNDCLASSEX wndclass;  MSG msg;
	int bw=GetSystemMetrics(SM_CXFRAME)*2;
	int bh=GetSystemMetrics(SM_CYFRAME)*2+GetSystemMetrics(SM_CYCAPTION);
	DWORD winstyle=WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	int pixelformat=0;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0,
		PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	int fullscreen=0, winwidth=width+bw, winheight=height+bh;
	RECT rect;

	for(i=0; i<argc; i++)
	{
		if(!strnicmp(argv[i], "-h", 2)) usage(argv);
		if(!strnicmp(argv[i], "-?", 2)) usage(argv);
		if(!strnicmp(argv[i], "-i", 2)) interactive=1;
		if(!strnicmp(argv[i], "-l", 2)) locolor=1;
		if(!strnicmp(argv[i], "-m", 2)) useimm=1;
		if(!strnicmp(argv[i], "-p", 2) && i<argc-1)
		{
			int npolys=atoi(argv[++i]);
			if(npolys>0)
			{
				slices=stacks=(int)(sqrt((double)npolys/((double)(3*NSPHERES+1))));
				if(slices<1) slices=stacks=1;
			}
		}
		if(!stricmp(argv[i], "-fs"))
		{
			winwidth=GetSystemMetrics(SM_CXSCREEN);
			winheight=GetSystemMetrics(SM_CYSCREEN);
			winstyle=WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE;
			fullscreen=1;
		}
		if(!strnicmp(argv[i], "-s", 2))
		{
			pfd.dwFlags|=PFD_STEREO;
			usestereo=1;
		}
	}

	fprintf(stderr, "Polygons in scene: %d\n", (NSPHERES*3+1)*slices*stacks);

	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = GetModuleHandle(NULL);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "WGLspheres";
	wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	if(!RegisterClassEx(&wndclass)) _throww32("Cannot create window class");

	if(!(win=CreateWindowEx(0, "WGLspheres", "WGLspheres", winstyle, 0,  0,
		winwidth, winheight, NULL, NULL, GetModuleHandle(NULL), NULL)))
		_throww32("Cannot create window");

	GetClientRect(win, &rect);
	fprintf(stderr, "Client area of window: %d x %d pixels\n",
		rect.right-rect.left, rect.bottom-rect.top);

	if(!(hdc=GetDC(win))) _throww32("Cannot create device context");

	if(!(pixelformat=ChoosePixelFormat(hdc, &pfd)))
		_throww32("Cannot create pixel format");
	fprintf(stderr, "Pixel Format of window: %d\n", pixelformat);
	if(!SetPixelFormat(hdc, pixelformat, &pfd))
		_throww32("Cannot set pixel format");

	if(!(ctx=wglCreateContext(hdc))) _throww32("Cannot create OpenGL context");

	if(!wglMakeCurrent(hdc, ctx))
		_throww32("Cannot make OpenGL context current");

	fprintf(stderr, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

	while(1)
	{
		BOOL ret;
		advance=0, dodisplay=0;
		if((ret=GetMessage(&msg, NULL, 0, 0))==-1)
			{_throww32("GetMessage() failed");}
		else if(ret==0) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(!interactive) {_catch(display(1));}
		else {if(dodisplay) {_catch(display(advance));}}
	}

	if(ctx) {wglDeleteContext(ctx);  ctx=0;}
	if(hdc) {ReleaseDC(win, hdc);  hdc=0;}
	if(win) {DestroyWindow(win);  win=0;}
	return msg.wParam;

	bailout:
	if(ctx) {wglDeleteContext(ctx);  ctx=0;}
	if(hdc) {ReleaseDC(win, hdc);  hdc=0;}
	if(win) {DestroyWindow(win);  win=0;}
	return -1;
}
