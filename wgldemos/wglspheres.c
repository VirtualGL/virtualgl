/* Copyright (C)2008 Sun Microsystems, Inc.
 * Copyright (C)2013-2014, 2019 D. R. Commander
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


#include <windows.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vglutil.h"


static char __lasterror[1024] = "No error";
#define THROW_W32(m) \
{ \
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), __lasterror, 1024, NULL)) \
		fprintf(stderr, "WIN32 ERROR in line %d:\n%s\n%s", __LINE__, m, \
			__lasterror); \
	goto bailout; \
}
#define THROW(m) \
{ \
	fprintf(stderr, "ERROR in line %d:\n%s\n", __LINE__,  m); \
	goto bailout; \
}
#define CATCH(f)  { if((f) == -1) goto bailout; }

#define SPHERE_RED(f) \
	(GLfloat)fabs(MAXI * (2. * f - 1.))
#define SPHERE_GREEN(f) \
	(GLfloat)fabs(MAXI * (2. * fmod(f + 2. / 3., 1.) - 1.))
#define SPHERE_BLUE(f) \
	(GLfloat)fabs(MAXI * (2. * fmod(f + 1. / 3., 1.) - 1.))


#define DEF_WIDTH  1240
#define DEF_HEIGHT  900

#define DEF_SLICES  32
#define DEF_STACKS  32

#define DEF_SPHERES  20

#define _2PI  6.283185307180
#define MAXI  (220. / 255.)

#define DEFBENCHTIME  2.0

HDC hdc = 0;  HWND win = 0;  HGLRC ctx = 0;
int useStereo = 0, useImm = 0, interactive = 0, loColor = 0, maxFrames = 0,
	totalFrames = 0;
double benchTime = DEFBENCHTIME;
int doDisplay = 0, advance = 0;

int sphereList = 0, fontListBase = 0;
GLUquadricObj *sphereQuad = NULL;
int slices = DEF_SLICES, stacks = DEF_STACKS, spheres = DEF_SPHERES;
GLfloat x = 0., y = 0., z = -3.;
GLfloat outerAngle = 0., middleAngle = 0., innerAngle = 0.;
GLfloat loneSphereColor = 0.;

int width = DEF_WIDTH, height = DEF_HEIGHT;


void reshape(int newWidth, int newHeight)
{
	if(newWidth <= 0) newWidth = 1;  if(newHeight <= 0) newHeight = 1;
	width = newWidth;  height = newHeight;
}


void setSphereColor(double color)
{
	GLfloat mat[4] =
	{
		SPHERE_RED(color), SPHERE_GREEN(color), SPHERE_BLUE(color), 0.25
	};
	glColor3f(SPHERE_RED(color), SPHERE_GREEN(color), SPHERE_BLUE(color));
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat);
}


void renderSpheres(int buf)
{
	int i;
	GLfloat xAspect, yAspect;
	GLfloat stereoCameraOffset = 0.;
	GLfloat nearDist = 1.5, farDist = 40., zeroParallaxDist = 17.;

	glDrawBuffer(buf);

	xAspect = (GLfloat)width / (GLfloat)(min(width, height));
	yAspect = (GLfloat)height / (GLfloat)(min(width, height));

	if(buf == GL_BACK_LEFT)
		stereoCameraOffset = -xAspect * zeroParallaxDist / nearDist * 0.035f;
	else if(buf == GL_BACK_RIGHT)
		stereoCameraOffset = xAspect * zeroParallaxDist / nearDist * 0.035f;

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
		glTranslatef((GLfloat)sin(_2PI * f) * 5.0f, (GLfloat)cos(_2PI * f) * 5.0f,
			-10.0f);
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
		glTranslatef((GLfloat)sin(_2PI * f) * 5.0f, (GLfloat)cos(_2PI * f) * 5.0f,
			-17.0f);
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
		glTranslatef((GLfloat)sin(_2PI * f) * 5.0f, (GLfloat)cos(_2PI * f) * 5.0f,
			-29.0f);
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


int display(int advance)
{
	static int first = 1;
	static double start = 0., elapsed = 0., mpixels = 0.;
	static unsigned long frames = 0;
	static char temps[256];
	GLfloat xAspect, yAspect;

	if(first)
	{
		GLfloat id4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		GLfloat light0Amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
		GLfloat light0Dif[] = { 0.8f, 0.8f, 0.8f, 1.0f };
		GLfloat light0Pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };

		sphereList = glGenLists(1);
		if(!(sphereQuad = gluNewQuadric()))
			THROW("Could not allocate GLU quadric object");
		glNewList(sphereList, GL_COMPILE);
		gluSphere(sphereQuad, 1.3, slices, stacks);
		glEndList();

		if(!loColor)
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, id4);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, id4);
			glMaterialfv(GL_FRONT, GL_SPECULAR, id4);
			glMaterialf(GL_FRONT, GL_SHININESS, 50.);

			glLightfv(GL_LIGHT0, GL_AMBIENT, light0Amb);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Dif);
			glLightfv(GL_LIGHT0, GL_SPECULAR, id4);
			glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
			glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
		}

		if(loColor) glShadeModel(GL_FLAT);
		else glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		fontListBase = glGenLists(256 + 1);
		if(!wglUseFontBitmaps(hdc, 0, 256, fontListBase))
		{
			DWORD err = GetLastError();
			fprintf(stderr, "WARNING: Cannot create font display lists\n");
			if(err != ERROR_SUCCESS
				&& FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), __lasterror, 1024, NULL))
				fprintf(stderr, "         %s", __lasterror);
		}
		snprintf(temps, 255, "Measuring performance ...");

		first = 0;
	}

	if(advance)
	{
		z -= 0.5;
		if(z < -29.) z = -3.5;
		outerAngle += 0.1f;  if(outerAngle > 360.) outerAngle -= 360.;
		middleAngle -= 0.37f;  if(middleAngle < -360.) middleAngle += 360.;
		innerAngle += 0.63f;  if(innerAngle > 360.) innerAngle -= 360.;
		loneSphereColor += 0.005f;
		if(loneSphereColor > 1.) loneSphereColor -= 1.;
	}

	if(useStereo)
	{
		renderSpheres(GL_BACK_LEFT);  renderSpheres(GL_BACK_RIGHT);
	}
	else renderSpheres(GL_BACK);

	glDrawBuffer(GL_BACK);
	glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_LIST_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glColor3f(1., 1., 1.);
	xAspect = (GLfloat)width / (GLfloat)(min(width, height));
	yAspect = (GLfloat)height / (GLfloat)(min(width, height));
	glRasterPos3f(-0.95f * xAspect, -0.95f * yAspect, -1.5f);
	glListBase(fontListBase);
	glCallLists((GLsizei)strlen(temps), GL_UNSIGNED_BYTE, temps);
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	SwapBuffers(hdc);

	if(start > 0.)
	{
		elapsed += GetTime() - start;  frames++;  totalFrames++;
		mpixels += (double)width * (double)height / 1000000.;
		if(elapsed > benchTime || (maxFrames && totalFrames > maxFrames))
		{
			snprintf(temps, 255, "%f frames/sec - %f Mpixels/sec",
				(double)frames / elapsed, mpixels / elapsed);
			printf("%s\n", temps);
			elapsed = mpixels = 0.;  frames = 0;
		}
	}
	if(maxFrames && totalFrames > maxFrames) goto bailout;

	start = GetTime();
	return 0;

	bailout:
	if(sphereQuad) { gluDeleteQuadric(sphereQuad);  sphereQuad = NULL; }
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
			if(interactive) doDisplay = 1;
			return 0;
		case WM_SIZE:
			reshape(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_CHAR:
			if(wParam == 27 || wParam == 'q' || wParam == 'Q')
			{
				PostQuitMessage(0);
				return 0;
			}
			break;
		case WM_MOUSEMOVE:
			if(wParam & MK_LBUTTON)
			{
				doDisplay = advance = 1;
				return 0;
			}
			break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


void usage(char **argv)
{
	printf("\nUSAGE: %s [options]\n\n", argv[0]);
	printf("Options:\n");
	printf("-fs = Full-screen mode\n");
	printf("-i = Interactive mode.  Frames advance in response to mouse movement\n");
	printf("-l = Use fewer than 24 colors (to force non-JPEG encoding in TurboVNC)\n");
	printf("-m = Use immediate mode rendering (default is display list)\n");
	printf("-p <p> = Use (approximately) <p> polygons to render scene\n");
	printf("         (max. is 57600 per sphere due to limitations of GLU.)\n");
	printf("-n <n> = Render (approximately) <n> spheres (default: %d)\n",
		DEF_SPHERES * 3 + 1);
	printf("-s = Use stereographic rendering initially\n");
	printf("     (this can be switched on and off in the application)\n");
	printf("-f <n> = max frames to render\n");
	printf("-bt <t> = print benchmark results every <t> seconds (default: %.1f)\n",
		DEFBENCHTIME);
	printf("-w <wxh> = specify window width and height\n");
	printf("\n");
	exit(1);
}


int main(int argc, char **argv)
{
	int i, nPolys = -1;
	WNDCLASSEX wndclass;  MSG msg;
	int bw = GetSystemMetrics(SM_CXFRAME) * 2;
	int bh = GetSystemMetrics(SM_CYFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
	DWORD winStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	int pixelFormat = 0;
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0,
		PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	int fullScreen = 0, pps;
	RECT rect;

	if(argc > 1) for(i = 1; i < argc; i++)
	{
		if(!stricmp(argv[i], "-h") || !strcmp(argv[i], "-?")) usage(argv);
		else if(!stricmp(argv[i], "-i")) interactive = 1;
		else if(!stricmp(argv[i], "-l")) loColor = 1;
		else if(!stricmp(argv[i], "-m")) useImm = 1;
		else if(!stricmp(argv[i], "-w") && i < argc - 1)
		{
			if(sscanf(argv[++i], "%dx%d", &width, &height) < 2 || width < 1
				|| height < 1)
				usage(argv);
			printf("Window dimensions: %d x %d\n", width, height);
		}
		else if(!stricmp(argv[i], "-fs"))
		{
			fullScreen = 1;
			winStyle = WS_POPUP | WS_VISIBLE;
			bw = bh = 0;
		}
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
		else if(!stricmp(argv[i], "-s"))
		{
			pfd.dwFlags |= PFD_STEREO;
			useStereo = 1;
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
	if(!RegisterClassEx(&wndclass)) THROW_W32("Cannot create window class");

	if(fullScreen)
	{
		width = GetSystemMetrics(SM_CXSCREEN);
		height = GetSystemMetrics(SM_CYSCREEN);
	}
	if(!(win = CreateWindowEx(0, "WGLspheres", "WGLspheres", winStyle, 0,  0,
		width + bw, height + bh, NULL, NULL, GetModuleHandle(NULL), NULL)))
		THROW_W32("Cannot create window");

	GetClientRect(win, &rect);
	fprintf(stderr, "Client area of window: %ld x %ld pixels\n",
		rect.right - rect.left, rect.bottom - rect.top);

	if(!(hdc = GetDC(win))) THROW_W32("Cannot create device context");

	if(!(pixelFormat = ChoosePixelFormat(hdc, &pfd)))
		THROW_W32("Cannot create pixel format");
	fprintf(stderr, "Pixel Format of window: %d\n", pixelFormat);
	if(!SetPixelFormat(hdc, pixelFormat, &pfd))
		THROW_W32("Cannot set pixel format");

	if(!(ctx = wglCreateContext(hdc))) THROW_W32("Cannot create OpenGL context");

	if(!wglMakeCurrent(hdc, ctx))
		THROW_W32("Cannot make OpenGL context current");

	fprintf(stderr, "OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

	while(1)
	{
		BOOL ret;
		advance = 0;  doDisplay = 0;

		if((ret = GetMessage(&msg, NULL, 0, 0)) == -1)
			{ THROW_W32("GetMessage() failed"); }
		else if(ret == 0) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if(!interactive) { CATCH(display(1)); }
		else if(doDisplay) { CATCH(display(advance)); }
	}

	if(ctx) { wglDeleteContext(ctx);  ctx = 0; }
	if(hdc) { ReleaseDC(win, hdc);  hdc = 0; }
	if(win) { DestroyWindow(win);  win = 0; }
	return msg.wParam;

	bailout:
	if(ctx) { wglDeleteContext(ctx);  ctx = 0; }
	if(hdc) { ReleaseDC(win, hdc);  hdc = 0; }
	if(win) { DestroyWindow(win);  win = 0; }
	return -1;
}
