/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005-2008 Sun Microsystems, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "rrutil.h"
#include "rrtimer.h"
#include "rrthread.h"
#include "rrmutex.h"
#define __FAKERCONFIG_STATICDEF__
#include "fakerconfig.h"
#define __FAKERHASH_STATICDEF__
#include "faker-winhash.h"
#include "faker-pfhash.h"
#include "detours.h"


// Globals
#define _isfront(drawbuf) (drawbuf==GL_FRONT || drawbuf==GL_FRONT_AND_BACK \
	|| drawbuf==GL_FRONT_LEFT || drawbuf==GL_FRONT_RIGHT || drawbuf==GL_LEFT \
	|| drawbuf==GL_RIGHT)
#define _isright(drawbuf) (drawbuf==GL_RIGHT || drawbuf==GL_FRONT_RIGHT \
	|| drawbuf==GL_BACK_RIGHT)

static inline int _drawingtofront(void)
{
	GLint drawbuf=GL_BACK;
	glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isfront(drawbuf);
}

static inline int _drawingtoright(void)
{
	GLint drawbuf=GL_LEFT;
	glGetIntegerv(GL_DRAW_BUFFER, &drawbuf);
	return _isright(drawbuf);
}


static rrcs globalmutex;
static int __shutdown=0;

static inline int isdead(void)
{
	int retval=0;
	globalmutex.lock(false);
	retval=__shutdown;
	globalmutex.unlock(false);
	return retval;
}


static void __vgl_cleanup(void)
{
	if(winhash::isalloc()) winh.killhash();
	if(pfhash::isalloc()) pfh.killhash();
}


void __vgl_safeexit(int retcode)
{
	int shutdown;
	globalmutex.lock(false);
	shutdown=__shutdown;
	if(!__shutdown)
	{
		__shutdown=1;
		__vgl_cleanup();
	}
	globalmutex.unlock(false);
	if(!shutdown) exit(retcode);
	else ExitThread(0);
}


static char _message[256];
#define _die(f,m) { \
	if(!isdead()) { \
		rrout.print("[VGL] ERROR: in %s--\n[VGL]    %s\n", f, m); \
		sprintf(_message, "ERROR: in %s--\n[VGL]    %s\n", f, m); \
		MessageBox(NULL, _message, "VirtualGL Error", MB_OK|MB_ICONEXCLAMATION); \
	  __vgl_safeexit(1); \
	} \
}
#define dttry(f) { \
	DWORD err=(DWORD)(f);  if(err!=NO_ERROR) throw w32error("Detours", err); \
}


#define TRY() try {
#define CATCH() } catch(rrerror &e) {_die(e.getMethod(), e.getMessage());}


#define prargd(a) rrout.print("%s=0x%.8lx(%s) ", #a, (unsigned long)a, a?DisplayString(a):"NULL")
#define prargs(a) rrout.print("%s=%s ", #a, a?a:"NULL")
#define prargx(a) rrout.print("%s=0x%.8lx ", #a, (unsigned long)a)
#define prargi(a) rrout.print("%s=%d ", #a, a)
#define prargv(a) rrout.print("%s=0x%.8lx(0x%.2lx) ", #a, (unsigned long)a, a?a->visualid:0)
#define prargc(a) rrout.print("%s=0x%.8lx(0x%.2x) ", #a, (unsigned long)a, a?_FBCID(a):0)
#define prargal11(a) if(a) {  \
	rrout.print("attrib_list=[");  \
	for(int __an=0; attrib_list[__an]!=None; __an++) {  \
		rrout.print("0x%.4x", attrib_list[__an]);  \
		if(attrib_list[__an]!=GLX_USE_GL && attrib_list[__an]!=GLX_DOUBLEBUFFER  \
			&& attrib_list[__an]!=GLX_STEREO && attrib_list[__an]!=GLX_RGBA)  \
			rrout.print("=0x%.4x", attrib_list[++__an]);  \
		rrout.print(" ");  \
	}  rrout.print("] ");}
#define prargal13(a) if(a) {  \
	rrout.print("attrib_list=[");  \
	for(int __an=0; a[__an]!=None; __an+=2) {  \
		rrout.print("0x%.4x=0x%.4x ", a[__an], a[__an+1]);  \
	}  rrout.print("] ");}

#define opentrace(f)  \
	double __vgltracetime=0.;  \
	if(fconfig.trace) {  \
		rrout.print("[VGL] %s (", #f);  \

#define starttrace()  \
		__vgltracetime=rrtime();  \
	}

#define stoptrace()  \
	if(fconfig.trace) {  \
		__vgltracetime=rrtime()-__vgltracetime;

#define closetrace()  \
		rrout.PRINT(") %f ms\n", __vgltracetime*1000.);  \
	}


// Symbols
static int (WINAPI *__ChoosePixelFormat)(HDC, CONST PIXELFORMATDESCRIPTOR *)
	=ChoosePixelFormat;
static HGLRC (WINAPI *__wglCreateContext)(HDC)=wglCreateContext;
static BOOL (WINAPI *__wglMakeCurrent)(HDC, HGLRC)=wglMakeCurrent;
static BOOL (WINAPI *__DestroyWindow)(HWND)=DestroyWindow;
static void (WINAPI *__glDrawBuffer)(GLenum)=glDrawBuffer;
static void (WINAPI *__glFlush)(void)=glFlush;
static void (WINAPI *__glFinish)(void)=glFinish;
static void (WINAPI *__glPopAttrib)(void)=glPopAttrib;
static void (WINAPI *__glViewport)(GLint, GLint, GLsizei, GLsizei)=glViewport;
BOOL (WINAPI *__SwapBuffers)(HDC)=SwapBuffers;

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB=NULL;
PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB=NULL;
PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB=NULL;
PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB=NULL;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB=NULL;


static void __vgl_loadsymbols(HDC hdc)
{
	HGLRC ctx=0;
	HMODULE hmod=0;
	int state=0;

	try {

	if(!wglGetCurrentContext() || !wglGetCurrentDC())
	{
		state=SaveDC(hdc);
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR), 1,
			PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0,
			PFD_MAIN_PLANE, 0, 0, 0, 0
		};
		int pixelformat=0;
		if(!(pixelformat=__ChoosePixelFormat(hdc, &pfd))
			|| !SetPixelFormat(hdc, pixelformat, &pfd)
			|| !(ctx=__wglCreateContext(hdc)) || !__wglMakeCurrent(hdc, ctx))
				_throww32m("Could not create temporary OpenGL context");
	}

	if(!wglChoosePixelFormatARB)
	{
		wglChoosePixelFormatARB=(PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		if(!wglChoosePixelFormatARB)
			_throw("Could not load symbol wglChoosePixelFormatARB");
	}
	if(!wglCreatePbufferARB)
	{
		wglCreatePbufferARB=(PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
		if(!wglCreatePbufferARB)
			_throw("Could not load symbol wglCreatePbufferARB");
	}
	if(!wglDestroyPbufferARB)
	{
		wglDestroyPbufferARB=(PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
		if(!wglDestroyPbufferARB)
			_throw("Could not load symbol wglDestroyPbufferARB");
	}
	if(!wglGetPbufferDCARB)
	{
		wglGetPbufferDCARB=(PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
		if(!wglGetPbufferDCARB)
			_throw("Could not load symbol wglGetPbufferDCARB");
	}
	if(!wglGetPixelFormatAttribivARB)
	{
		wglGetPixelFormatAttribivARB=(PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
		if(!wglGetPixelFormatAttribivARB)
			_throw("Could not load symbol wglGetPixelFormatAttribivARB");
	}
	if(!wglReleasePbufferDCARB)
	{
		wglReleasePbufferDCARB=(PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
		if(!wglReleasePbufferDCARB)
			_throw("Could not load symbol wglReleasePbufferDCARB");
	}

	wglDeleteContext(ctx);
	if(state!=0) RestoreDC(hdc, state);

	}	catch(...)
	{
		if(ctx) wglDeleteContext(ctx);
		if(state!=0) RestoreDC(hdc, state);
		throw;
	}
}


static void __vgl_fakerinit(HDC hdc)
{
	static int init=0;

	rrcs::safelock l(globalmutex);
	if(init) return;
	init=1;

	fconfig.reloadenv();
	if(fconfig.log) rrout.logto(fconfig.log);

	#ifdef __DEBUG__
	if(getenv("VGL_DEBUG"))
	{
		rrout.print("[VGL] Attach debugger to process %d ...\n", GetCurrentProcessId());
		fgetc(stdin);
	}
	#endif

	__vgl_loadsymbols(hdc);
}


int __declspec(dllexport) WINAPI fake_ChoosePixelFormat(HDC hdc,
	CONST PIXELFORMATDESCRIPTOR *ppfd)
{
	int pixelformat=0, pbpf=-1;
	int attribs[256], i=0;

	TRY();

	__vgl_fakerinit(hdc);

		opentrace(ChoosePixelFormat);  prargx(hdc);  starttrace();

	if(ppfd && (ppfd->dwFlags)&PFD_SUPPORT_OPENGL)
	{
		attribs[i++]=WGL_ACCELERATION_ARB;  attribs[i++]=WGL_FULL_ACCELERATION_ARB;
		if(ppfd->cAccumBits!=0)
		{
			attribs[i++]=WGL_ACCUM_BITS_ARB;  attribs[i++]=ppfd->cAccumBits;
		}
		if(ppfd->cAccumRedBits!=0)
		{
			attribs[i++]=WGL_ACCUM_RED_BITS_ARB;  attribs[i++]=ppfd->cAccumRedBits;
		}
		if(ppfd->cAccumGreenBits!=0)
		{
			attribs[i++]=WGL_ACCUM_GREEN_BITS_ARB;
			attribs[i++]=ppfd->cAccumGreenBits;
		}
		if(ppfd->cAccumBlueBits!=0)
		{
			attribs[i++]=WGL_ACCUM_BLUE_BITS_ARB;  attribs[i++]=ppfd->cAccumBlueBits;
		}
		if(ppfd->cAccumAlphaBits!=0)
		{
			attribs[i++]=WGL_ACCUM_ALPHA_BITS_ARB;
			attribs[i++]=ppfd->cAccumAlphaBits;
		}
		if(ppfd->cAlphaBits!=0)
		{
			attribs[i++]=WGL_ALPHA_BITS_ARB;  attribs[i++]=ppfd->cAlphaBits;
		}
		if(ppfd->cAlphaShift!=0)
		{
			attribs[i++]=WGL_ALPHA_SHIFT_ARB;  attribs[i++]=ppfd->cAlphaShift;
		}
		if(ppfd->cAuxBuffers!=0)
		{
			attribs[i++]=WGL_AUX_BUFFERS_ARB;  attribs[i++]=ppfd->cAuxBuffers;
		}
		if(ppfd->cBlueBits!=0)
		{
			attribs[i++]=WGL_BLUE_BITS_ARB;  attribs[i++]=ppfd->cBlueBits;
		}
		else if(ppfd->iPixelType==PFD_TYPE_RGBA)
		{
			attribs[i++]=WGL_BLUE_BITS_ARB;  attribs[i++]=8;
		}
		if(ppfd->cBlueShift!=0)
		{
			attribs[i++]=WGL_BLUE_SHIFT_ARB;  attribs[i++]=ppfd->cBlueShift;
		}
		if(ppfd->cColorBits!=0)
		{
			attribs[i++]=WGL_COLOR_BITS_ARB;  attribs[i++]=ppfd->cColorBits;
		}
		if(ppfd->cDepthBits!=0 && !(ppfd->dwFlags&PFD_DEPTH_DONTCARE))
		{
			attribs[i++]=WGL_DEPTH_BITS_ARB;  attribs[i++]=ppfd->cDepthBits;
		}
		attribs[i++]=WGL_DRAW_TO_PBUFFER_ARB;  attribs[i++]=1;
		if(!(ppfd->dwFlags&PFD_DOUBLEBUFFER_DONTCARE))
		{
			attribs[i++]=WGL_DOUBLE_BUFFER_ARB;
			attribs[i++]=(ppfd->dwFlags&PFD_DOUBLEBUFFER)? 1:0;
		}
		if(ppfd->cGreenBits!=0)
		{
			attribs[i++]=WGL_GREEN_BITS_ARB;  attribs[i++]=ppfd->cGreenBits;
		}
		else if(ppfd->iPixelType==PFD_TYPE_RGBA)
		{
			attribs[i++]=WGL_GREEN_BITS_ARB;  attribs[i++]=8;
		}

		if(ppfd->cGreenShift!=0)
		{
			attribs[i++]=WGL_GREEN_SHIFT_ARB;  attribs[i++]=ppfd->cGreenShift;
		}
		attribs[i++]=WGL_NEED_PALETTE_ARB;
		attribs[i++]=(ppfd->dwFlags&PFD_NEED_PALETTE)? 1:0;
		attribs[i++]=WGL_NEED_SYSTEM_PALETTE_ARB;  
		attribs[i++]=(ppfd->dwFlags&PFD_NEED_SYSTEM_PALETTE)? 1:0;
		attribs[i++]=WGL_PIXEL_TYPE_ARB;
		attribs[i++]=(ppfd->iPixelType==PFD_TYPE_COLORINDEX)?
			WGL_TYPE_COLORINDEX_ARB:WGL_TYPE_RGBA_ARB;
		if(ppfd->cRedBits!=0)
		{
			attribs[i++]=WGL_RED_BITS_ARB;  attribs[i++]=ppfd->cRedBits;
		}
		else if(ppfd->iPixelType==PFD_TYPE_RGBA)
		{
			attribs[i++]=WGL_RED_BITS_ARB;  attribs[i++]=8;
		}
		if(ppfd->cRedShift!=0)
		{
			attribs[i++]=WGL_RED_SHIFT_ARB;  attribs[i++]=ppfd->cRedShift;
		}
		if(ppfd->cStencilBits!=0)
		{
			attribs[i++]=WGL_STENCIL_BITS_ARB;  attribs[i++]=ppfd->cStencilBits;
		}
		if(!(ppfd->dwFlags&PFD_STEREO_DONTCARE))
		{
			attribs[i++]=WGL_STEREO_ARB;
			attribs[i++]=(ppfd->dwFlags&PFD_STEREO)? 1:0;
		}
		attribs[i++]=WGL_SUPPORT_OPENGL_ARB;
		attribs[i++]=(ppfd->dwFlags&PFD_SUPPORT_OPENGL)? 1:0;
		attribs[i++]=WGL_SUPPORT_GDI_ARB;
		attribs[i++]=(ppfd->dwFlags&PFD_SUPPORT_GDI)? 1:0;
		attribs[i++]=WGL_DRAW_TO_BITMAP_ARB;
		attribs[i++]=(ppfd->dwFlags&PFD_DRAW_TO_BITMAP)? 1:0;

		attribs[i++]=0;

		unsigned int count=0;
		if(!wglChoosePixelFormatARB(hdc, attribs, NULL, 1, &pbpf,
			&count))
			_throww32m("Could not create Pbuffer pixel format");
		if(pbpf<0 || count<1)
			_throw("Could not create Pbuffer pixel format");
		pixelformat=__ChoosePixelFormat(hdc, ppfd);
		if(!pixelformat) return 0;
		pfh.add(hdc, pixelformat, pbpf);
	}
	else pixelformat=__ChoosePixelFormat(hdc, ppfd);

		stoptrace();  prargal13(attribs);  prargi(pixelformat);  prargi(pbpf);
		closetrace();

	CATCH();

	return pixelformat;
};


HGLRC __declspec(dllexport) WINAPI fake_wglCreateContext(HDC hdc)
{
	HGLRC ctx=0;
	pbwin *pbw=NULL;

	TRY();

	__vgl_fakerinit(hdc);

		opentrace(wglCreateContext);  prargx(hdc);  starttrace();

	HWND hwnd=WindowFromDC(hdc);
	if(hwnd)
	{
		if(!winh.findpb(hwnd, NULL, pbw))
		{
			int pf=pfh.find(hdc, GetPixelFormat(hdc));
			if(!pf) _throw("Current pixel format was not obtained using ChoosePixelFormat().  I'm stymied.");
			pbw=new pbwin(hwnd, hdc, pf);
			errifnot(pbw);
			winh.add(hwnd, NULL, pbw);
		}
		ctx=__wglCreateContext(pbw->gethdc());
	}
	else ctx=__wglCreateContext(hdc);

		stoptrace();  prargx(ctx);  prargx(hwnd);  if(pbw) prargx(pbw->gethdc());
		closetrace();

	CATCH();

	return ctx;
};


BOOL __declspec(dllexport) WINAPI fake_wglMakeCurrent(HDC hdc, HGLRC ctx)
{
	BOOL retval=FALSE;
	pbwin *pbw=NULL;

	TRY();

	__vgl_fakerinit(hdc);

		opentrace(wglMakeCurrent);  prargx(hdc);  prargx(ctx);  starttrace();

	HWND hwnd=WindowFromDC(hdc);

	// Equivalent of a glFlush()
	HDC curhdc=wglGetCurrentDC();
	if(wglGetCurrentContext() && curhdc && winh.findpb(0, curhdc, pbw))
	{
		pbwin *newpbw;
		if(hdc==0 || ctx==0 || !winh.findpb(hwnd, 0, newpbw)
			|| newpbw->gethdc()!=curhdc)
		{
			if(_drawingtofront() || pbw->_dirty) pbw->readback(GL_FRONT, false);
		}
	}

	// If the DC doesn't belong to a window, we pass it through unmodified,
	// else we map it to a Pbuffer
	if(hdc && ctx && hwnd)
	{
		winh.findpb(hwnd, 0, pbw);
		if(pbw) hdc=pbw->updatehdc();
	}

	retval=__wglMakeCurrent(hdc, ctx);
	if(winh.findpb(0, hdc, pbw)) pbw->clear();

		stoptrace();  closetrace();

	CATCH();
	return retval;
}


BOOL __declspec(dllexport) WINAPI fake_DestroyWindow(HWND hwnd)
{
	BOOL retval=FALSE;

	TRY();

	HDC dc=GetDC(hwnd);
	errifnot(dc);
	__vgl_fakerinit(dc);
	DeleteDC(dc);

		opentrace(DestroyWindow);  prargx(hwnd);  starttrace();

	winh.remove(hwnd, NULL);
	retval=__DestroyWindow(hwnd);

		stoptrace();  closetrace();

	CATCH();

	return retval;
}


BOOL __declspec(dllexport) WINAPI fake_SwapBuffers(HDC hdc)
{
	BOOL retval=FALSE;

	TRY();

	__vgl_fakerinit(hdc);

		opentrace(SwapBuffers);  prargx(hdc);  starttrace();

	pbwin *pbw=NULL;
	HWND hwnd=WindowFromDC(hdc);
	if(hwnd && winh.findpb(hwnd, 0, pbw))
	{
		pbw->readback(GL_BACK, false);
		pbw->swapbuffers();
		retval=TRUE;
	}
	else retval=__SwapBuffers(hdc);

		stoptrace();  prargx(hwnd);  if(pbw) prargx(pbw->gethdc());
		closetrace();  

	CATCH();

	return retval;
}


static void _doGLreadback(bool spoillast)
{
	pbwin *pbw;
	HDC hdc;
	hdc=wglGetCurrentDC();
	if(!hdc) return;
	if(winh.findpb(0, hdc, pbw))
	{
		if(_drawingtofront() || pbw->_dirty)
		{
				opentrace(_doGLreadback);  prargx(pbw->gethdc());
				prargi(spoillast);  starttrace();

			pbw->readback(GL_FRONT, spoillast);

				stoptrace();  closetrace();
		}
	}
}


void __declspec(dllexport) WINAPI fake_glFlush(void)
{
	TRY();

	__vgl_fakerinit(0);

		if(fconfig.trace) rrout.print("[VGL] glFlush()\n");

	__glFlush();
	_doGLreadback(true);
	CATCH();
}


void __declspec(dllexport) WINAPI fake_glFinish(void)
{
	TRY();

	__vgl_fakerinit(0);

		if(fconfig.trace) rrout.print("[VGL] glFinish()\n");

	__glFinish();
	_doGLreadback(false);
	CATCH();
}


// If the application switches the draw buffer before calling glFlush(), we
// set a lazy readback trigger
void __declspec(dllexport) WINAPI fake_glDrawBuffer(GLenum mode)
{
	TRY();

		opentrace(glDrawBuffer);  prargx(mode);  starttrace();

	pbwin *pbw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	HDC hdc=wglGetCurrentDC();
	if(hdc && winh.findpb(0, hdc, pbw))
	{
		before=_drawingtofront();
		rbefore=_drawingtoright();
		__glDrawBuffer(mode);
		after=_drawingtofront();
		rafter=_drawingtoright();
		if(before && !after) pbw->_dirty=true;
		if(rbefore && !rafter && pbw->stereo()) pbw->_rdirty=true;
	}
	else __glDrawBuffer(mode);

		stoptrace();  if(hdc && pbw) {prargi(pbw->_dirty);
		prargi(pbw->_rdirty);  prargx(pbw->gethdc());}  closetrace();

	CATCH();
}

// glPopAttrib() can change the draw buffer state as well :/
void __declspec(dllexport) WINAPI fake_glPopAttrib(void)
{
	TRY();

	__vgl_fakerinit(0);

		opentrace(glPopAttrib);  starttrace();

	pbwin *pbw=NULL;  int before=-1, after=-1, rbefore=-1, rafter=-1;
	HDC hdc=wglGetCurrentDC();
	if(hdc && winh.findpb(0, hdc, pbw))
	{
		before=_drawingtofront();
		rbefore=_drawingtoright();
		__glPopAttrib();
		after=_drawingtofront();
		rafter=_drawingtoright();
		if(before && !after) pbw->_dirty=true;
		if(rbefore && !rafter && pbw->stereo()) pbw->_rdirty=true;
	}
	else __glPopAttrib();

		stoptrace();  if(hdc && pbw) {prargi(pbw->_dirty);
		prargi(pbw->_rdirty);  prargx(pbw->gethdc());}  closetrace();

	CATCH();
}


void __declspec(dllexport) WINAPI fake_glViewport(GLint x, GLint y,
	GLsizei width, GLsizei height)
{
	TRY();

	__vgl_fakerinit(0);

		opentrace(glViewport);  prargi(x);  prargi(y);  prargi(width);
		prargi(height);  starttrace();

	HGLRC ctx=wglGetCurrentContext();
	HDC hdc=wglGetCurrentDC();
	HDC newhdc=0;
	if(hdc && ctx)
	{
		newhdc=hdc;
		pbwin *pbw=NULL;
		winh.findpb(0, hdc, pbw);
		if(pbw) newhdc=pbw->updatehdc();
		if(newhdc!=hdc)
		{
			__wglMakeCurrent(newhdc, ctx);
			if(pbw) {pbw->clear();}
		}
	}
	__glViewport(x, y, width, height);

		stoptrace();  if(hdc!=newhdc) {prargx(hdc);  prargx(newhdc);}
		closetrace();

	CATCH();
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	TRY();

	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			DetourRestoreAfterWith();
			dttry(DetourTransactionBegin());
			dttry(DetourUpdateThread(GetCurrentThread()));
			dttry(DetourAttach(&(PVOID&)__ChoosePixelFormat, fake_ChoosePixelFormat));
			dttry(DetourAttach(&(PVOID&)__wglCreateContext, fake_wglCreateContext));
			dttry(DetourAttach(&(PVOID&)__wglMakeCurrent, fake_wglMakeCurrent));
			dttry(DetourAttach(&(PVOID&)__DestroyWindow, fake_DestroyWindow));
			dttry(DetourAttach(&(PVOID&)__SwapBuffers, fake_SwapBuffers));
			dttry(DetourAttach(&(PVOID&)__glDrawBuffer, fake_glDrawBuffer));
			dttry(DetourAttach(&(PVOID&)__glFlush, fake_glFlush));
			dttry(DetourAttach(&(PVOID&)__glFinish, fake_glFinish));
			dttry(DetourAttach(&(PVOID&)__glPopAttrib, fake_glPopAttrib));
			dttry(DetourAttach(&(PVOID&)__glViewport, fake_glViewport));
			dttry(DetourTransactionCommit());
			break;
		case DLL_PROCESS_DETACH:
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			dttry(DetourDetach(&(PVOID&)__ChoosePixelFormat, fake_ChoosePixelFormat));
			dttry(DetourDetach(&(PVOID&)__wglCreateContext, fake_wglCreateContext));
			dttry(DetourDetach(&(PVOID&)__wglMakeCurrent, fake_wglMakeCurrent));
			dttry(DetourDetach(&(PVOID&)__DestroyWindow, fake_DestroyWindow));
			dttry(DetourDetach(&(PVOID&)__SwapBuffers, fake_SwapBuffers));
			dttry(DetourDetach(&(PVOID&)__glDrawBuffer, fake_glDrawBuffer));
			dttry(DetourDetach(&(PVOID&)__glFlush, fake_glFlush));
			dttry(DetourDetach(&(PVOID&)__glFinish, fake_glFinish));
			dttry(DetourDetach(&(PVOID&)__glPopAttrib, fake_glPopAttrib));
			dttry(DetourDetach(&(PVOID&)__glViewport, fake_glViewport));
			DetourTransactionCommit();
			break;
	}
	return TRUE;

	CATCH();
	return FALSE;
}
