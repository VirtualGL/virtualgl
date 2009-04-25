/* Copyright (C)2009 D. R. Commander
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


#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static char __lasterror[1024]="No error";
#define _throw(m) {fprintf(stderr, "ERROR in line %d:\n%s\n", __LINE__,  m); \
	goto bailout;}
#define _throww32(m) { \
	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), __lasterror, 1024, NULL)) \
		fprintf(stderr, "WIN32 ERROR in line %d:\n%s\n%s", __LINE__, m, __lasterror); \
	goto bailout; \
}

PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;

HWND win=0;  HDC hdc=0;
int usearb=0;

int loadsymbols(HDC hdc)
{
	HGLRC ctx=0;
	HMODULE hmod=0;
	int state=0, retval=0;

	if(!wglGetCurrentContext() || !wglGetCurrentDC())
	{
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR), 1,
			PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0,
			PFD_MAIN_PLANE, 0, 0, 0, 0
		};
		int pixelformat=0;
		state=SaveDC(hdc);
		if(!(pixelformat=ChoosePixelFormat(hdc, &pfd))
			|| !SetPixelFormat(hdc, pixelformat, &pfd)
			|| !(ctx=wglCreateContext(hdc)) || !wglMakeCurrent(hdc, ctx))
				{retval=-1;  _throww32("Could not create temporary OpenGL context");}
	}

	if(!wglGetPixelFormatAttribivARB)
	{
		wglGetPixelFormatAttribivARB=(PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
		if(!wglGetPixelFormatAttribivARB)
			{retval=-1;  _throw("Could not load symbol wglGetPixelFormatAttribivARB");}
	}

	bailout:
	if(ctx) wglDeleteContext(ctx);
	if(state!=0) RestoreDC(hdc, state);
	return retval;
}


#define getattrib(a) {int _n=0, _attrib=a;  \
	if(!wglGetPixelFormatAttribivARB(hdc, i, 0, 1, &_attrib, &attrib)) \
		_throww32("Could not retrieve value for attribute "#a);}

void display(HWND win)
{
	HDC hdc=0;
	int i, n=0, pf=0;
	PIXELFORMATDESCRIPTOR pfd;
	UINT attrib;

	if(!(hdc=GetDC(win))) _throww32("Cannot create device context");

	if(usearb)
	{
		printf("          rg d st  colorbuffer   ax dp st  accumbuffer   a drw\n");
		printf(" id ol ul ci b ro sz  r  g  b  a bf th cl sz  r  g  b  a c typ\n");
		printf("--------------------------------------------------------------\n");

		if(loadsymbols(hdc)<0) return;
		attrib=WGL_NUMBER_PIXEL_FORMATS_ARB;
		if(!wglGetPixelFormatAttribivARB(hdc, 0, 0, 1, &attrib, &n))
			_throww32("No pixel formats");

		for(i=1; i<=n; i++)
		{
			getattrib(WGL_SUPPORT_OPENGL_ARB);
			if(!attrib) continue;
			printf("%3d ", i);
			getattrib(WGL_NUMBER_OVERLAYS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_NUMBER_UNDERLAYS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_PIXEL_TYPE_ARB);
			printf("%2s ", attrib==WGL_TYPE_RGBA_ARB? "r ":
				attrib==WGL_TYPE_COLORINDEX_ARB? "c ":"..");
			getattrib(WGL_DOUBLE_BUFFER_ARB);
			printf("%1s ", attrib==1? "y":".");
			getattrib(WGL_STEREO_ARB);
			printf("%2s ", attrib==1? "y":".");
			getattrib(WGL_COLOR_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_RED_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_GREEN_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_BLUE_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ALPHA_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_AUX_BUFFERS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_DEPTH_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_STENCIL_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ACCUM_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ACCUM_RED_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ACCUM_GREEN_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ACCUM_BLUE_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ACCUM_ALPHA_BITS_ARB);
			printf("%2d ", attrib);
			getattrib(WGL_ACCELERATION_ARB);
			printf("%1s ", attrib==WGL_NO_ACCELERATION_ARB? "n":
				attrib==WGL_GENERIC_ACCELERATION_ARB? "g":"f");
			getattrib(WGL_DRAW_TO_WINDOW_ARB);
			printf("%1s", attrib==1? "w":".");
			getattrib(WGL_DRAW_TO_BITMAP_ARB);
			printf("%1s", attrib==1? "b":".");
			getattrib(WGL_DRAW_TO_PBUFFER_ARB);
			printf("%1s", attrib==1? "p":".");
			printf("\n");
		}
	}
	else
	{
		printf("                layer   rg d st  colorbuffer   ax dp st  accumbuffer   s drw\n");
		printf(" id flgs ol ul   mask   ci b ro sz  r  g  b  a bf th cl sz  r  g  b  a w typ\n");
		printf("----------------------------------------------------------------------------\n");

		if(!(n=DescribePixelFormat(hdc, 1, sizeof(pfd), &pfd)))
			_throww32("No pixel formats");

		for(i=1; i<=n; i++)
		{
			if(!(pf=DescribePixelFormat(hdc, i, sizeof(pfd), &pfd)))
				_throww32("Could not describe pixel format");
			if(!(pfd.dwFlags&PFD_SUPPORT_OPENGL)) continue;
			printf("%3d %.4x %2d %2d %.8x %2s %1s %2s %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %1s ",
				i,
				pfd.dwFlags,
				pfd.bReserved & 0xF,
				pfd.bReserved & 0xF0,
				pfd.dwLayerMask,
				pfd.iPixelType==PFD_TYPE_RGBA? "r ":
					pfd.iPixelType==PFD_TYPE_COLORINDEX? "c ":"..",
				pfd.dwFlags & PFD_DOUBLEBUFFER? "y":".",
				pfd.dwFlags & PFD_STEREO? " y":" .",
				pfd.cColorBits,
				pfd.cRedBits,
				pfd.cGreenBits,
				pfd.cBlueBits,
				pfd.cAlphaBits,
				pfd.cAuxBuffers,
				pfd.cDepthBits,
				pfd.cStencilBits,
				pfd.cAccumBits,
				pfd.cAccumRedBits,
				pfd.cAccumGreenBits,
				pfd.cAccumBlueBits,
				pfd.cAccumAlphaBits,
				(pfd.dwFlags&PFD_GENERIC_FORMAT && !(pfd.dwFlags&PFD_GENERIC_ACCELERATED))?
					"y":".");
			printf("%1s", pfd.dwFlags&PFD_DRAW_TO_WINDOW?"w":".");
			printf("%1s", pfd.dwFlags&PFD_DRAW_TO_BITMAP?"b":".");
			printf("%1s", ".");
			printf("\n");
		}
	}

	bailout:
	if(hdc) {ReleaseDC(win, hdc);  hdc=0;}
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
			display(hwnd);
			PostQuitMessage(0);
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}


void usage(char **argv)
{
	printf("USAGE: %s [-c|-h]\n", argv[0]);
	printf("-c = use wglChoosePixelFormatARB\n");
	printf("-h = Show usage\n");
	exit(0);
}


int main(int argc, char **argv)
{
	int i;
	WNDCLASSEX wndclass;  MSG msg;
	DWORD winstyle=WS_OVERLAPPEDWINDOW | WS_VISIBLE;

	for(i=0; i<argc; i++)
	{
		if(!strnicmp(argv[i], "-h", 2)) usage(argv);
		if(!strnicmp(argv[i], "-c", 2)) usearb=1;
	}

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
	wndclass.lpszClassName = "WGLinfo";
	wndclass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	if(!RegisterClassEx(&wndclass)) _throww32("Cannot create window class");

	if(!(win=CreateWindowEx(0, "WGLinfo", "WGLinfo", winstyle, 0,  0,
		1, 1, NULL, NULL, GetModuleHandle(NULL), NULL)))
		_throww32("Cannot create window");

	while(1)
	{
		BOOL ret;
		if((ret=GetMessage(&msg, NULL, 0, 0))==-1)
			{_throww32("GetMessage() failed");}
		else if(ret==0) break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(win) {DestroyWindow(win);  win=0;}
	return msg.wParam;

	bailout:
	if(win) {DestroyWindow(win);  win=0;}
	return -1;
}
