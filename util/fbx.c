/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2010-2013, 2015, 2017-2020 D. R. Commander
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

/* This library abstracts fast frame buffer access */

#include <string.h>
#include <stdlib.h>
#include "fbx.h"
#include "vglutil.h"


#ifdef _WIN32
#define MINWIDTH  160
#define MINHEIGHT  24
#endif


static int errorLine = -1;
static FILE *warningFile = NULL;


#if defined(_WIN32)

#define BMPPAD(pitch)  ((pitch + (sizeof(int) - 1)) & (~(sizeof(int) - 1)))

static char lastError[1024] = "No error";

#define THROW(m) \
{ \
	strncpy(lastError, m, 1023);  errorLine = __LINE__; \
	goto finally; \
}

#define TRY_W32(f) \
{ \
	if(!(f)) \
	{ \
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), \
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)lastError, 1024, \
			NULL); \
		errorLine = __LINE__;  goto finally; \
	} \
}

#define TRY_X11(f) \
{ \
	if(!(f)) \
	{ \
		snprintf(lastError, 1023, "X11 Error (window may have disappeared)"); \
		errorLine = __LINE__;  goto finally; \
	} \
}

#else

static char *lastError = "No error";

#define THROW(m)  { lastError = m;  errorLine = __LINE__;  goto finally; }

#define TRY_X11(f) \
	if(!(f)) \
	{ \
		lastError = "X11 Error (window may have disappeared)"; \
		errorLine = __LINE__;  goto finally; \
	}

#ifndef X_ShmAttach
#define X_ShmAttach  1
#endif

#endif  /* _WIN32 */


#ifdef _WIN32

typedef struct _BMINFO
{
	BITMAPINFO bmi;  RGBQUAD cmap[256];
} BMINFO;

#else

#include <errno.h>

#ifdef USESHM

static unsigned long serial = 0;  static int extok = 1;
static XErrorHandler prevHandler = NULL;

static int xhandler(Display *dpy, XErrorEvent *e)
{
	if(e->serial == serial && (e->minor_code == X_ShmAttach
		&& (e->error_code == BadAccess || e->error_code == BadRequest)))
	{
		extok = 0;  return 0;
	}
	if(prevHandler && prevHandler != xhandler) return prevHandler(dpy, e);
	else return 0;
}
#endif

#endif


char *fbx_geterrmsg(void)
{
	return lastError;
}


int fbx_geterrline(void)
{
	return errorLine;
}


void fbx_printwarnings(FILE *stream)
{
	warningFile = stream;
}


int fbx_init(fbx_struct *fb, fbx_wh wh, int width_, int height_, int useShm)
{
	int width, height, ps, i;
	unsigned int rmask, gmask, bmask;
	#ifdef _WIN32
	BMINFO bminfo;  HBITMAP hmembmp = 0;  RECT rect;  HDC hdc = NULL;
	#else
	XWindowAttributes xwa;  int shmok = 1, pixmap = 0;
	#endif

	if(!fb) THROW("Invalid argument");

	#ifdef _WIN32

	if(!wh) THROW("Invalid argument");
	TRY_W32(GetClientRect(wh, &rect));
	if(width_ > 0) width = width_;
	else
	{
		width = rect.right - rect.left;  if(width <= 0) width = MINWIDTH;
	}
	if(height_ > 0) height = height_;
	else
	{
		height = rect.bottom - rect.top;  if(height <= 0) height = MINHEIGHT;
	}
	if(fb->wh == wh)
	{
		if(width == fb->width && height == fb->height && fb->hmdc && fb->hdib
			&& fb->bits)
			return 0;
		else if(fbx_term(fb) == -1) return -1;
	}
	memset(fb, 0, sizeof(fbx_struct));
	fb->wh = wh;

	TRY_W32(hdc = GetDC(fb->wh));
	TRY_W32(fb->hmdc = CreateCompatibleDC(hdc));
	TRY_W32(hmembmp = CreateCompatibleBitmap(hdc, width, height));
	TRY_W32(GetDeviceCaps(hdc, RASTERCAPS) & RC_BITBLT);
	TRY_W32(GetDeviceCaps(fb->hmdc, RASTERCAPS) & RC_DI_BITMAP);
	bminfo.bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bminfo.bmi.bmiHeader.biBitCount = 0;
	TRY_W32(GetDIBits(fb->hmdc, hmembmp, 0, 1, NULL, &bminfo.bmi,
		DIB_RGB_COLORS));
	TRY_W32(GetDIBits(fb->hmdc, hmembmp, 0, 1, NULL, &bminfo.bmi,
		DIB_RGB_COLORS));
	TRY_W32(DeleteObject(hmembmp));  hmembmp = 0;
	/* (we only needed it to get the screen properties) */
	ps = bminfo.bmi.bmiHeader.biBitCount / 8;
	if(width > 0) bminfo.bmi.bmiHeader.biWidth = width;
	if(height > 0) bminfo.bmi.bmiHeader.biHeight = height;
	fb->width = bminfo.bmi.bmiHeader.biWidth;
	fb->height = bminfo.bmi.bmiHeader.biHeight;

	if(ps < 3)
	{
		/* Make the buffer BGRA */
		bminfo.bmi.bmiHeader.biCompression = BI_BITFIELDS;
		bminfo.bmi.bmiHeader.biBitCount = 32;
		ps = 4;
		(*(DWORD *)&bminfo.bmi.bmiColors[0]) = 0xFF0000;
		(*(DWORD *)&bminfo.bmi.bmiColors[1]) = 0xFF00;
		(*(DWORD *)&bminfo.bmi.bmiColors[2]) = 0xFF;
	}

	fb->pitch = BMPPAD(fb->width * ps);  /* Windoze bitmaps are always padded */

	if(bminfo.bmi.bmiHeader.biCompression == BI_BITFIELDS)
	{
		rmask = (*(DWORD *)&bminfo.bmi.bmiColors[0]);
		gmask = (*(DWORD *)&bminfo.bmi.bmiColors[1]);
		bmask = (*(DWORD *)&bminfo.bmi.bmiColors[2]);
	}
	else
	{
		rmask = 0xFF0000;
		gmask = 0xFF00;
		bmask = 0xFF;
	}

	for(i = 0; i < PIXELFORMATS; i++)
	{
		PF *pf = pf_get(i);
		if(rmask == pf->rmask && gmask == pf->gmask && bmask == pf->bmask
			&& ps == pf->size)
			fb->pf = pf;
	}
	if(!fb->pf || fb->pf->size == 0)
		THROW("Display has unsupported pixel format");

	bminfo.bmi.bmiHeader.biHeight = -bminfo.bmi.bmiHeader.biHeight;
	/* (our convention is top-down) */
	TRY_W32(fb->hdib = CreateDIBSection(hdc, &bminfo.bmi, DIB_RGB_COLORS,
		(void **)&fb->bits, NULL, 0));
	TRY_W32(SelectObject(fb->hmdc, fb->hdib));
	ReleaseDC(fb->wh, hdc);
	return 0;

	finally:
	if(hmembmp) DeleteObject(hmembmp);
	if(hdc) ReleaseDC(fb->wh, hdc);

	#else

	if(!wh.dpy || !wh.d) THROW("Invalid argument");
	if(wh.v)
	{
		TRY_X11(XGetGeometry(wh.dpy, wh.d, &xwa.root, &xwa.x, &xwa.y,
			(unsigned int *)&xwa.width, (unsigned int *)&xwa.height,
			(unsigned int *)&xwa.border_width, (unsigned int *)&xwa.depth));
		xwa.visual = wh.v;
		useShm = 0;
		pixmap = 1;
	}
	else
	{
		TRY_X11(XGetWindowAttributes(wh.dpy, wh.d, &xwa));
	}
	if(width_ > 0) width = width_;  else width = xwa.width;
	if(height_ > 0) height = height_;  else height = xwa.height;
	if(fb->wh.dpy == wh.dpy && fb->wh.d == wh.d)
	{
		if(width == fb->width && height == fb->height && fb->xi && fb->xgc
			&& fb->bits)
			return 0;
		else if(fbx_term(fb) == -1) return -1;
	}
	memset(fb, 0, sizeof(fbx_struct));
	fb->wh.dpy = wh.dpy;  fb->wh.d = wh.d;

	#ifdef USESHM
	if(!useShm)
	{
		static int alreadyWarned = 0;
		if(!alreadyWarned && warningFile)
		{
			fprintf(warningFile, "[FBX] Disabling shared memory blitting\n");
			alreadyWarned = 1;
		}
	}
	if(useShm && XShmQueryExtension(fb->wh.dpy))
	{
		static int alreadyWarned = 0;
		fb->shminfo.shmid = -1;
		if(!(fb->xi = XShmCreateImage(fb->wh.dpy, xwa.visual, xwa.depth, ZPixmap,
			NULL, &fb->shminfo, width, height)))
		{
			useShm = 0;  goto noshm;
		}
		if((fb->shminfo.shmid = shmget(IPC_PRIVATE,
			fb->xi->bytes_per_line * fb->xi->height + 1, IPC_CREAT | 0777)) == -1)
		{
			useShm = 0;  XDestroyImage(fb->xi);  goto noshm;
		}
		if((fb->shminfo.shmaddr = fb->xi->data =
			(char *)shmat(fb->shminfo.shmid, 0, 0)) == (char *)-1)
		{
			useShm = 0;  XDestroyImage(fb->xi);
			shmctl(fb->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		fb->shminfo.readOnly = False;
		XLockDisplay(fb->wh.dpy);
		XSync(fb->wh.dpy, False);
		prevHandler = XSetErrorHandler(xhandler);
		extok = 1;
		serial = NextRequest(fb->wh.dpy);
		XShmAttach(fb->wh.dpy, &fb->shminfo);
		XSync(fb->wh.dpy, False);
		XSetErrorHandler(prevHandler);
		shmok = extok;
		if(!alreadyWarned && !shmok && warningFile)
		{
			fprintf(warningFile,
				"[FBX] WARNING: MIT-SHM extension failed to initialize (this is normal on a\n");
			fprintf(warningFile,
				"[FBX]    remote X connection.)  Will use X Pixmap drawing instead.\n");
			alreadyWarned = 1;
		}
		XUnlockDisplay(fb->wh.dpy);
		if(shmok)
		{
			char *env = getenv("FBX_USESHMPIXMAPS");
			if(env && !strcmp(env, "1"))
			{
				static int alreadyWarned2 = 0;
				if(!alreadyWarned2 && warningFile)
				{
					fprintf(warningFile, "[FBX] Using MIT-SHM pixmaps\n");
					alreadyWarned2 = 1;
				}
				fb->pm = XShmCreatePixmap(fb->wh.dpy, fb->wh.d, fb->shminfo.shmaddr,
					&fb->shminfo, width, height, xwa.depth);
				if(!fb->pm) shmok = 0;
			}
		}
		shmctl(fb->shminfo.shmid, IPC_RMID, 0);
		if(!shmok)
		{
			useShm = 0;  XDestroyImage(fb->xi);  shmdt(fb->shminfo.shmaddr);
			shmctl(fb->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		fb->xattach = 1;  fb->shm = 1;
	}
	else if(useShm)
	{
		static int alreadyWarned = 0;
		if(!alreadyWarned && warningFile)
		{
			fprintf(warningFile,
				"[FBX] WARNING: MIT-SHM extension not available.  Will use X pixmap\n");
			fprintf(warningFile, "[FBX]    drawing instead.\n");
			alreadyWarned = 1;
		}
		useShm = 0;
	}
	noshm:
	if(!useShm)
	#endif
	{
		if(!pixmap)
			TRY_X11(fb->pm = XCreatePixmap(fb->wh.dpy, fb->wh.d, width, height,
				xwa.depth));
		TRY_X11(fb->xi = XCreateImage(fb->wh.dpy, xwa.visual, xwa.depth, ZPixmap,
			0, NULL, width, height, 8, 0));
		if((fb->xi->data =
			(char *)malloc(fb->xi->bytes_per_line * fb->xi->height + 1)) == NULL)
			THROW("Memory allocation error");
	}
	ps = fb->xi->bits_per_pixel / 8;
	fb->width = fb->xi->width;
	fb->height = fb->xi->height;
	fb->pitch = fb->xi->bytes_per_line;
	if(fb->width != width || fb->height != height)
		THROW("Bitmap returned does not match requested size");
	rmask = fb->xi->red_mask;  gmask = fb->xi->green_mask;
	bmask = fb->xi->blue_mask;
	if((fb->xi->byte_order == MSBFirst && LittleEndian())
		|| (fb->xi->byte_order == LSBFirst && !LittleEndian()))
	{
		if(ps == 4)
		{
			rmask = BYTESWAP(rmask);
			gmask = BYTESWAP(gmask);
			bmask = BYTESWAP(bmask);
		}
		else
		{
			rmask = BYTESWAP24(rmask);
			gmask = BYTESWAP24(gmask);
			bmask = BYTESWAP24(bmask);
		}
	}
	for(i = 0; i < PIXELFORMATS; i++)
	{
		PF *pf = pf_get(i);
		if(rmask == pf->rmask && gmask == pf->gmask && bmask == pf->bmask
			&& ps == pf->size)
			fb->pf = pf;
	}
	if(!fb->pf || fb->pf->size == 0)
		THROW("Display has unsupported pixel format");

	fb->bits = fb->xi->data;
	fb->pixmap = pixmap;
	TRY_X11(fb->xgc = XCreateGC(fb->wh.dpy, fb->pm ? fb->pm : fb->wh.d, 0,
		NULL));
	if(!useShm) XSetGraphicsExposures(fb->wh.dpy, fb->xgc, False);
	return 0;

	finally:

	#endif

	fbx_term(fb);
	return -1;
}


int fbx_read(fbx_struct *fb, int x_, int y_)
{
	int x, y;
	#ifdef _WIN32
	fbx_gc gc;
	#endif

	if(!fb) THROW("Invalid argument");

	x = x_ >= 0 ? x_ : 0;  y = y_ >= 0 ? y_ : 0;

	#ifdef _WIN32

	if(!fb->hmdc || fb->width <= 0 || fb->height <= 0 || !fb->bits || !fb->wh)
		THROW("Not initialized");
	TRY_W32(gc = GetDC(fb->wh));
	TRY_W32(BitBlt(fb->hmdc, 0, 0, fb->width, fb->height, gc, x, y, SRCCOPY));
	TRY_W32(ReleaseDC(fb->wh, gc));
	return 0;

	#else

	if(!fb->wh.dpy || !fb->wh.d || !fb->xi || !fb->bits)
		THROW("Not initialized");
	#ifdef USESHM
	if(!fb->xattach && fb->shm)
	{
		TRY_X11(XShmAttach(fb->wh.dpy, &fb->shminfo));  fb->xattach = 1;
	}
	#endif

	#ifdef USESHM
	if(fb->shm)
	{
		TRY_X11(XShmGetImage(fb->wh.dpy, fb->wh.d, fb->xi, x, y, AllPlanes));
	}
	else
	#endif
	{
		TRY_X11(XGetSubImage(fb->wh.dpy, fb->wh.d, x, y, fb->width, fb->height,
			AllPlanes, ZPixmap, fb->xi, 0, 0));
	}
	return 0;

	#endif

	finally:
	return -1;
}


int fbx_write(fbx_struct *fb, int srcX_, int srcY_, int dstX_, int dstY_,
	int width_, int height_)
{
	int srcX, srcY, dstX, dstY, width, height;
	#ifdef _WIN32
	BITMAPINFO bmi;  fbx_gc gc;
	#endif

	if(!fb) THROW("Invalid argument");

	srcX = srcX_ >= 0 ? srcX_ : 0;  srcY = srcY_ >= 0 ? srcY_ : 0;
	dstX = dstX_ >= 0 ? dstX_ : 0;  dstY = dstY_ >= 0 ? dstY_ : 0;
	width = width_ > 0 ? width_ : fb->width;
	height = height_ > 0 ? height_ : fb->height;

	if(width > fb->width) width = fb->width;
	if(height > fb->height) height = fb->height;
	if(srcX + width > fb->width) width = fb->width - srcX;
	if(srcY + height > fb->height) height = fb->height - srcY;

	#ifdef _WIN32

	if(!fb->wh || fb->width <= 0 || fb->height <= 0 || !fb->bits)
		THROW("Not initialized");
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(bmi);
	bmi.bmiHeader.biWidth = fb->width;
	bmi.bmiHeader.biHeight = -fb->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = fb->pf->size * 8;
	bmi.bmiHeader.biCompression = BI_RGB;
	TRY_W32(gc = GetDC(fb->wh));
	TRY_W32(SetDIBitsToDevice(gc, dstX, dstY, width, height, srcX, 0, 0, height,
		&fb->bits[srcY * fb->pitch], &bmi, DIB_RGB_COLORS));
	TRY_W32(ReleaseDC(fb->wh, gc));
	return 0;

	#else

	if(!fb->pm || !fb->shm)
		if(fbx_awrite(fb, srcX, srcY, dstX, dstY, width, height) == -1) return -1;
	if(fb->pm)
	{
		XCopyArea(fb->wh.dpy, fb->pm, fb->wh.d, fb->xgc, srcX, srcY, width, height,
			dstX, dstY);
	}
	XFlush(fb->wh.dpy);
	XSync(fb->wh.dpy, False);
	return 0;

	#endif

	finally:
	return -1;
}


int fbx_flip(fbx_struct *fb, int x_, int y_, int width_, int height_)
{
	int i, x, y, width, height, ps, pitch;
	char *tmpbuf = NULL, *srcptr, *dstptr;

	if(!fb) THROW("Invalid argument");

	x = x_ >= 0 ? x_ : 0;  y = y_ >= 0 ? y_ : 0;
	width = width_ > 0 ? width_ : fb->width;
	height = height_ > 0 ? height_ : fb->height;

	if(width > fb->width) width = fb->width;
	if(height > fb->height) height = fb->height;
	if(x + width > fb->width) width = fb->width - x;
	if(y + height > fb->height) height = fb->height - y;
	ps = fb->pf->size;  pitch = fb->pitch;

	if(!(tmpbuf = (char *)malloc(width * ps)))
		THROW("Memory allocation error");
	srcptr = &fb->bits[pitch * y + ps * x];
	dstptr = &fb->bits[pitch * (y + height - 1) + ps * x];
	for(i = 0; i < height / 2; i++, srcptr += pitch, dstptr -= pitch)
	{
		memcpy(tmpbuf, srcptr, width * ps);
		memcpy(srcptr, dstptr, width * ps);
		memcpy(dstptr, tmpbuf, width * ps);
	}
	free(tmpbuf);
	return 0;

	finally:
	free(tmpbuf);
	return -1;
}


#ifndef _WIN32

int fbx_awrite(fbx_struct *fb, int srcX_, int srcY_, int dstX_, int dstY_,
	int width_, int height_)
{
	int srcX, srcY, dstX, dstY, width, height;

	if(!fb) THROW("Invalid argument");

	srcX = srcX_ >= 0 ? srcX_ : 0;  srcY = srcY_ >= 0 ? srcY_ : 0;
	dstX = dstX_ >= 0 ? dstX_ : 0;  dstY = dstY_ >= 0 ? dstY_ : 0;
	width = width_ > 0 ? width_ : fb->width;
	height = height_ > 0 ? height_ : fb->height;

	if(width > fb->width) width = fb->width;
	if(height > fb->height) height = fb->height;
	if(srcX + width > fb->width) width = fb->width - srcX;
	if(srcY + height > fb->height) height = fb->height - srcY;
	if(!fb->wh.dpy || !fb->wh.d || !fb->xi || !fb->bits)
		THROW("Not initialized");

	#ifdef USESHM
	if(fb->shm)
	{
		if(!fb->xattach)
		{
			TRY_X11(XShmAttach(fb->wh.dpy, &fb->shminfo));  fb->xattach = 1;
		}
		TRY_X11(XShmPutImage(fb->wh.dpy, fb->wh.d, fb->xgc, fb->xi, srcX, srcY,
			dstX, dstY, width, height, False));
	}
	else
	#endif
	{
		Drawable draw = fb->pixmap ? fb->wh.d : fb->pm;
		if(draw == fb->pm) dstX = dstY = 0;
		XPutImage(fb->wh.dpy, draw, fb->xgc, fb->xi, srcX, srcY, dstX, dstY, width,
			height);
	}
	return 0;

	finally:
	return -1;
}

#endif


int fbx_sync(fbx_struct *fb)
{
	#ifdef _WIN32

	return 0;

	#else

	if(!fb) THROW("Invalid argument");
	if(fb->pm)
	{
		XCopyArea(fb->wh.dpy, fb->pm, fb->wh.d, fb->xgc, 0, 0, fb->width,
			fb->height, 0, 0);
	}
	XFlush(fb->wh.dpy);
	XSync(fb->wh.dpy, False);
	return 0;

	finally:
	return -1;

	#endif
}


int fbx_term(fbx_struct *fb)
{
	if(!fb) THROW("Invalid argument");

	#ifdef _WIN32

	if(fb->hdib) DeleteObject(fb->hdib);
	if(fb->hmdc) DeleteDC(fb->hmdc);

	#else

	if(fb->pm)
	{
		XFreePixmap(fb->wh.dpy, fb->pm);  fb->pm = 0;
	}
	if(fb->xi)
	{
		if(!fb->shm)
		{
			free(fb->xi->data);  fb->xi->data = NULL;
		}
		XDestroyImage(fb->xi);
	}
	#ifdef USESHM
	if(fb->shm)
	{
		if(fb->xattach)
		{
			XShmDetach(fb->wh.dpy, &fb->shminfo);  XSync(fb->wh.dpy, False);
		}
		if(fb->shminfo.shmaddr != NULL) shmdt(fb->shminfo.shmaddr);
		if(fb->shminfo.shmid != -1) shmctl(fb->shminfo.shmid, IPC_RMID, 0);
	}
	#endif
	if(fb->xgc) XFreeGC(fb->wh.dpy, fb->xgc);

	#endif

	memset(fb, 0, sizeof(fbx_struct));
	return 0;

	finally:
	return -1;
}
