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

// This library abstracts fast frame buffer access
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fbx.h"

#define MINWIDTH  160
#define MINHEIGHT 24

static int __line=-1;

#ifdef WIN32

 static char __lasterror[1024]="No error";
 #define _throw(m) {strncpy(__lasterror, m, 1023);  __line=__LINE__;  goto finally;}
 #define w32(f) {if(!(f)) {FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),  \
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)__lasterror, 1024, NULL);  \
	__line=__LINE__;  goto finally;}}
 typedef struct _BMINFO {BITMAPINFO bmi;  RGBQUAD cmap[256];} BMINFO;

#else

 #include <errno.h>
 #include "x11err.h"
 static char *__lasterror="No error";
 #define _throw(m) {__lasterror=m;  __line=__LINE__;  goto finally;}
 #define x11(f) if(!(f)) {__lasterror="X11 Error";  __line=__LINE__;  goto finally;}

 #ifdef USESHM
 unsigned long serial=0;  int __shmok=1;
 XErrorHandler prevhandler=NULL;

 int _fbx_xhandler(Display *dpy, XErrorEvent *e)
 {
	if(e->serial==serial && e->minor_code==X_ShmAttach && e->error_code==BadAccess)
	{
		__shmok=0;  return 0;
	}
	if(prevhandler && prevhandler!=_fbx_xhandler) return prevhandler(dpy, e);
	else return 0;
 }
 #endif

#endif

char *fbx_geterrmsg(void)
{
	return __lasterror;
}

int fbx_geterrline(void)
{
	return __line;
}

int fbx_init(fbx_struct *s, fbx_wh wh, int width, int height, int useshm)
{
	int w, h;
	#ifdef WIN32
	BMINFO bminfo;  HBITMAP hmembmp=0;  RECT rect;  HDC hdc=NULL;
	#else
	XWindowAttributes xwinattrib;  int dummy1, dummy2, shmok=1;
	#endif

	if(!s) _throw("Invalid argument");

	#ifdef WIN32

	if(!wh) _throw("Invalid argument");
	w32(GetClientRect(wh, &rect));
	if(width>0) w=width;  else {w=rect.right-rect.left;  if(w<=0) w=MINWIDTH;}
	if(height>0) h=height;  else {h=rect.bottom-rect.top;  if(h<=0) h=MINHEIGHT;}
	if(s->wh==wh)
	{
		if(w==s->width && h==s->height && s->hmdc && s->hdib && s->bits) return 0;
		else if(fbx_term(s)==-1) return -1;
	}
	memset(s, 0, sizeof(fbx_struct));
	s->wh=wh;

	w32(hdc=GetDC(s->wh));
	w32(s->hmdc=CreateCompatibleDC(hdc));
	w32(hmembmp=CreateCompatibleBitmap(hdc, w, h));
	w32(GetDeviceCaps(hdc, RASTERCAPS)&RC_BITBLT);
	w32(GetDeviceCaps(s->hmdc, RASTERCAPS)&RC_DI_BITMAP);
	bminfo.bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bminfo.bmi.bmiHeader.biBitCount=0;
	w32(GetDIBits(s->hmdc, hmembmp, 0, 1, NULL, &bminfo.bmi, DIB_RGB_COLORS));
	w32(GetDIBits(s->hmdc, hmembmp, 0, 1, NULL, &bminfo.bmi, DIB_RGB_COLORS));
	w32(DeleteObject(hmembmp));  hmembmp=0;  // We only needed it to get the screen properties
	s->ps=bminfo.bmi.bmiHeader.biBitCount/8;
	if(width>0) bminfo.bmi.bmiHeader.biWidth=width;
	if(height>0) bminfo.bmi.bmiHeader.biHeight=height;
	s->width=bminfo.bmi.bmiHeader.biWidth;
	s->height=bminfo.bmi.bmiHeader.biHeight;

	if(s->ps<3)
	{
		// Make the buffer BGRA
		bminfo.bmi.bmiHeader.biCompression=BI_BITFIELDS;
		bminfo.bmi.bmiHeader.biBitCount=32;
		s->ps=4;
		(*(DWORD *)&bminfo.bmi.bmiColors[0])=0xFF0000;
		(*(DWORD *)&bminfo.bmi.bmiColors[1])=0xFF00;
		(*(DWORD *)&bminfo.bmi.bmiColors[2])=0xFF;
	}

	s->pitch=BMPPAD(s->width*s->ps);  // Windoze bitmaps are always padded

	if(bminfo.bmi.bmiHeader.biCompression==BI_BITFIELDS)
	{
		s->rmask=(*(DWORD *)&bminfo.bmi.bmiColors[0]);
		s->gmask=(*(DWORD *)&bminfo.bmi.bmiColors[1]);
		s->bmask=(*(DWORD *)&bminfo.bmi.bmiColors[2]);
	}
	else
	{
		s->rmask=0xFF;
		s->gmask=0xFF00;
		s->bmask=0xFF0000;
	}

	if(s->rmask>s->bmask) s->bgr=1;  else s->bgr=0;
	bminfo.bmi.bmiHeader.biHeight=-bminfo.bmi.bmiHeader.biHeight;  // Our convention is top down
	w32(s->hdib=CreateDIBSection(hdc, &bminfo.bmi, DIB_RGB_COLORS, (void **)&s->bits, NULL, 0));
	w32(SelectObject(s->hmdc, s->hdib));
	ReleaseDC(s->wh, hdc);
	return 0;

	finally:
	if(hmembmp) DeleteObject(hmembmp);
	if(hdc) ReleaseDC(s->wh, hdc);

	#else

	if(!wh.dpy || !wh.win) _throw("Invalid argument");
	x11(XGetWindowAttributes(wh.dpy, wh.win, &xwinattrib));
	if(width>0 && width<=xwinattrib.width) w=width;  else w=xwinattrib.width;
	if(height>0 && height<=xwinattrib.height) h=height;  else h=xwinattrib.height;
	if(s->wh.dpy==wh.dpy && s->wh.win==wh.win)
	{
		if(w==s->width && h==s->height && s->xi && s->xgc && s->bits) return 0;
		else if(fbx_term(s)==-1) return -1;
	}
	memset(s, 0, sizeof(fbx_struct));
	s->wh.dpy=wh.dpy;  s->wh.win=wh.win;

	#ifdef USESHM
	if(useshm && XShmQueryExtension(s->wh.dpy))
	{
		s->shminfo.shmid=-1;
		if(!(s->xi=XShmCreateImage(s->wh.dpy, xwinattrib.visual, xwinattrib.depth, ZPixmap, NULL,
			&s->shminfo, w, h)))
		{
			useshm=0;  goto noshm;
		}
		if((s->shminfo.shmid=shmget(IPC_PRIVATE, s->xi->bytes_per_line*s->xi->height, IPC_CREAT|0777))==-1)
		{
			useshm=0;  XDestroyImage(s->xi);  goto noshm;
		}
		if((s->shminfo.shmaddr=s->xi->data=(char *)shmat(s->shminfo.shmid, 0, 0))
			==(char *)-1)
		{
			useshm=0;  XDestroyImage(s->xi);  shmctl(s->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		s->shminfo.readOnly=False;
		XLockDisplay(s->wh.dpy);
		XSync(s->wh.dpy, False);
		prevhandler=XSetErrorHandler(_fbx_xhandler);
		__shmok=1;
		serial=NextRequest(s->wh.dpy);
		XShmAttach(s->wh.dpy, &s->shminfo);
		XSync(s->wh.dpy, False);
		XSetErrorHandler(prevhandler);
		shmok=__shmok;
		XUnlockDisplay(s->wh.dpy);
		if(!shmok)
		{
			useshm=0;  XDestroyImage(s->xi);  shmctl(s->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		s->xattach=1;  s->shm=1;
	}
	else useshm=0;
	noshm:
	if(!useshm)
	#endif
	{
		if(XdbeQueryExtension(s->wh.dpy, &dummy1, &dummy2))
		{
			s->bb=XdbeAllocateBackBufferName(s->wh.dpy, s->wh.win, XdbeUndefined);
		}
		x11(s->xi=XCreateImage(s->wh.dpy, xwinattrib.visual, xwinattrib.depth, ZPixmap, 0, NULL,
			w, h, 8, 0));
		if((s->xi->data=(char *)malloc(s->xi->bytes_per_line*s->xi->height))==NULL)
			_throw("Memory allocation error");
	}
	s->ps=s->xi->bits_per_pixel/8;
	s->width=s->xi->width;
	s->height=s->xi->height;
	s->pitch=s->xi->bytes_per_line;
	if(s->width!=w || s->height!=h) _throw("Bitmap returned does not match requested size");
	s->rmask=s->xi->red_mask;  s->gmask=s->xi->green_mask;  s->bmask=s->xi->blue_mask;
	if((s->rmask&0x01 && s->xi->byte_order==MSBFirst) || 
	   (s->bmask&0x01 && s->xi->byte_order==LSBFirst)) s->bgr=1;
	else s->bgr=0;
	s->bits=s->xi->data;
	x11(s->xgc=XCreateGC(s->wh.dpy, s->bb? s->bb:s->wh.win, 0, NULL));
	return 0;

	finally:

	#endif

	fbx_term(s);
	return -1;
}

int fbx_read(fbx_struct *s, int winx, int winy)
{
	int wx, wy;
	#ifdef WIN32
	fbx_gc gc;
	#else
	int x=0, y=0;  XWindowAttributes xwinattrib, rwinattrib;  Window dummy;
	#endif
	if(!s) _throw("Invalid argument");
	wx=winx>=0?winx:0;  wy=winy>=0?winy:0;

	#ifdef WIN32
	if(!s->hmdc || s->width<=0 || s->height<=0 || !s->bits || !s->wh) _throw("Not initialized");
	w32(gc=GetDC(s->wh));
	w32(BitBlt(s->hmdc, 0, 0, s->width, s->height, gc, wx, wy, SRCCOPY));
	w32(ReleaseDC(s->wh, gc));
	return 0;
	#else
	if(!s->wh.dpy || !s->wh.win || !s->xi || !s->bits) _throw("Not initialized");
	#ifdef USESHM
	if(!s->xattach && s->shm) {x11(XShmAttach(s->wh.dpy, &s->shminfo));  s->xattach=1;}
	#endif

	x11(XGetWindowAttributes(s->wh.dpy, s->wh.win, &xwinattrib));
	if(s->wh.win!=xwinattrib.root)
	{
		x11(XGetWindowAttributes(s->wh.dpy, xwinattrib.root, &rwinattrib));
		XTranslateCoordinates(s->wh.dpy, s->wh.win, xwinattrib.root, 0, 0, &x, &y, &dummy);
		x=wx+x;  y=wy+y;
		if(x<0) x=0;  if(y<0) y=0;
		if(x>rwinattrib.width-s->width) x=rwinattrib.width-s->width;
		if(y>rwinattrib.height-s->height) y=rwinattrib.height-s->height;
		#ifdef USESHM
		if(s->shm)
		{
			x11(XShmGetImage(s->wh.dpy, xwinattrib.root, s->xi, x, y, AllPlanes));
		}
		else
		#endif
		{
			x11(XGetSubImage(s->wh.dpy, xwinattrib.root, x, y, s->width, s->height, AllPlanes, ZPixmap, s->xi, 0, 0));
		}
	}
	else
	{
		#ifdef USESHM
		if(s->shm)
		{
			x11(XShmGetImage(s->wh.dpy, xwinattrib.root, s->xi, wx, wy, AllPlanes));
		}
		else
		#endif
		{
			x11(XGetSubImage(s->wh.dpy, xwinattrib.root, wx, wy, s->width, s->height, AllPlanes, ZPixmap, s->xi, 0, 0));
		}
	}
	return 0;
	#endif

	finally:
	return -1;
}

int fbx_write(fbx_struct *s, int bmpx, int bmpy, int winx, int winy, int w, int h)
{
	int bx, by, wx, wy, bw, bh;
	#ifdef WIN32
	BITMAPINFO bmi;  fbx_gc gc;
	#else
	XdbeSwapInfo si;
	#endif

	bx=bmpx>=0?bmpx:0;  by=bmpy>=0?bmpy:0;  bw=w>0?w:s->width;  bh=h>0?h:s->height;
	wx=winx>=0?winx:0;  wy=winy>=0?winy:0;
	if(bw>s->width) bw=s->width;  if(bh>s->height) bh=s->height;
	if(bx+bw>s->width) bw=s->width-bx;  if(by+bh>s->height) bh=s->height-by;

	#ifdef WIN32
	if(!s->wh || s->width<=0 || s->height<=0 || !s->bits || !s->ps) _throw("Not initialized");
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(bmi);
	bmi.bmiHeader.biWidth=s->width;
	bmi.bmiHeader.biHeight=-s->height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=s->ps*8;
	bmi.bmiHeader.biCompression=BI_RGB;
	w32(gc=GetDC(s->wh));
	w32(SetDIBitsToDevice(gc, wx, wy, bw, bh, bx, 0, 0, bh, &s->bits[by*s->ps*s->width], &bmi,
		DIB_RGB_COLORS));
	w32(ReleaseDC(s->wh, gc));
	return 0;
	#else
	si.swap_window=s->wh.win;
	si.swap_action=XdbeUndefined;
	if(!s->wh.dpy || !s->wh.win || !s->xi || !s->bits) _throw("Not initialized");
	#ifdef USESHM
	if(s->shm)
	{
		if(!s->xattach) {x11(XShmAttach(s->wh.dpy, &s->shminfo));  s->xattach=1;}
		x11(XShmPutImage(s->wh.dpy, s->bb? s->bb:s->wh.win, s->xgc, s->xi, bx, by, wx, wy, bw, bh, True));
	}
	else
	#endif
	XPutImage(s->wh.dpy, s->bb? s->bb:s->wh.win, s->xgc, s->xi, bx, by, wx, wy, bw, bh);
	if(s->bb)
	{
		XdbeBeginIdiom(s->wh.dpy);
		XdbeSwapBuffers(s->wh.dpy, &si, 1);
		XdbeEndIdiom(s->wh.dpy);
	}
	XFlush(s->wh.dpy);
	XSync(s->wh.dpy, False);
	return 0;
	#endif

	finally:
	return -1;
}

int fbx_term(fbx_struct *s)
{
	if(!s) _throw("Invalid argument");
	#ifdef WIN32
	if(s->hdib) DeleteObject(s->hdib);
	if(s->hmdc) DeleteDC(s->hmdc);
	#else
	if(s->bb) {XdbeDeallocateBackBufferName(s->wh.dpy, s->bb);  s->bb=0;}
	if(s->xi) 
	{
		if(s->xi->data && !s->shm) {free(s->xi->data);  s->xi->data=NULL;}
		XDestroyImage(s->xi);
	}
	#ifdef USESHM
	if(s->shm)
	{
		if(s->xattach) {XShmDetach(s->wh.dpy, &s->shminfo);  XSync(s->wh.dpy, False);}
		if(s->shminfo.shmaddr!=NULL) shmdt(s->shminfo.shmaddr);
		if(s->shminfo.shmid!=-1) shmctl(s->shminfo.shmid, IPC_RMID, 0);
	}
	#endif
	if(s->xgc) XFreeGC(s->wh.dpy, s->xgc);
	#endif
	memset(s, 0, sizeof(fbx_struct));
	return 0;

	finally:
	return -1;
}
