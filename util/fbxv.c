/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009, 2014, 2018-2019 D. R. Commander
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

/* This library abstracts X Video drawing */
#include <string.h>
#include <stdlib.h>
#include "fbxv.h"
#include "x11err.h"


static int errorLine = -1;
static FILE *warningFile = NULL;


static char lastError[1024] = "No error";

#define THROW(m) \
{ \
	snprintf(lastError, 1023, "%s", m);  errorLine = __LINE__;  goto finally; \
}

#define TRY_X11(f) \
{ \
	int __err = 0; \
	if((__err = (f)) != Success) \
	{ \
		snprintf(lastError, 1023, \
			"X11 %s Error (window may have disappeared)", x11error(__err)); \
		errorLine = __LINE__;  goto finally; \
	} \
}

#define ERRIFNOT(f) \
{ \
	if(!(f)) \
	{ \
		snprintf(lastError, 1023, "X11 Error (window may have disappeared)"); \
		errorLine = __LINE__;  goto finally; \
	} \
}


#ifdef USESHM
static unsigned long serial = 0;  static int extok = 1;
static XErrorHandler prevHandler = NULL;

#ifndef X_ShmAttach
#define X_ShmAttach  1
#endif

static int xhandler(Display *dpy, XErrorEvent *e)
{
	if(e->serial == serial && (e->minor_code == X_ShmAttach
		&& e->error_code == BadAccess))
	{
		extok = 0;  return 0;
	}
	if(prevHandler && prevHandler != xhandler) return prevHandler(dpy, e);
	else return 0;
}
#endif


char *fbxv_geterrmsg(void)
{
	return lastError;
}


int fbxv_geterrline(void)
{
	return errorLine;
}


void fbxv_printwarnings(FILE *stream)
{
	warningFile = stream;
}


int fbxv_init(fbxv_struct *fb, Display *dpy, Window win, int width_,
	int height_, int format, int useShm)
{
	int width, height, k, shmok = 1, nformats;
	unsigned int dummy1, dummy2, dummy3, dummy4, dummy5, i, j, nadaptors = 0;
	XWindowAttributes xwa;
	XvAdaptorInfo *ai = NULL;
	XvImageFormatValues *ifv = NULL;

	if(!fb) THROW("Invalid argument");

	if(!dpy || !win) THROW("Invalid argument");
	ERRIFNOT(XGetWindowAttributes(dpy, win, &xwa));
	if(width_ > 0) width = width_;  else width = xwa.width;
	if(height_ > 0) height = height_;  else height = xwa.height;
	if(fb->dpy == dpy && fb->win == win)
	{
		if(width == fb->reqwidth && height == fb->reqheight && fb->xvi && fb->xgc
			&& fb->xvi->data)
			return 0;
		else if(fbxv_term(fb) == -1) return -1;
	}
	memset(fb, 0, sizeof(fbxv_struct));
	fb->dpy = dpy;  fb->win = win;
	fb->reqwidth = width;  fb->reqheight = height;

	if(XvQueryExtension(dpy, &dummy1, &dummy2, &dummy3, &dummy4, &dummy5)
		!= Success)
		THROW("X Video Extension not available");
	if(XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &nadaptors, &ai) != Success)
		THROW("Could not query X Video adaptors");
	if(nadaptors < 1 || !ai) THROW("No X Video adaptors available");

	fb->port = -1;
	for(i = 0; i < nadaptors; i++)
	{
		for(j = ai[i].base_id; j < ai[i].base_id + ai[i].num_ports; j++)
		{
			nformats = 0;
			ifv = XvListImageFormats(dpy, j, &nformats);
			if(ifv && nformats > 0)
			{
				for(k = 0; k < nformats; k++)
				{
					if(ifv[k].id == format)
					{
						XFree(ifv);  fb->port = j;
						goto found;
					}
				}
			}
			XFree(ifv);
		}
	}
	found:
	XvFreeAdaptorInfo(ai);  ai = NULL;
	if(fb->port == -1)
		THROW("The X Video implementation on the 2D X Server does not support the desired pixel format");

	#ifdef USESHM
	if(useShm && XShmQueryExtension(fb->dpy))
	{
		static int alreadyWarned = 0;
		fb->shminfo.shmid = -1;
		if(!(fb->xvi = XvShmCreateImage(dpy, fb->port, format, 0, width, height,
			&fb->shminfo)))
		{
			useShm = 0;  goto noshm;
		}
		if((fb->shminfo.shmid = shmget(IPC_PRIVATE, fb->xvi->data_size,
			IPC_CREAT | 0777)) == -1)
		{
			useShm = 0;  XFree(fb->xvi);  goto noshm;
		}
		if((fb->shminfo.shmaddr = fb->xvi->data =
			(char *)shmat(fb->shminfo.shmid, 0, 0)) == (char *)-1)
		{
			useShm = 0;  XFree(fb->xvi);  shmctl(fb->shminfo.shmid, IPC_RMID, 0);
			goto noshm;
		}
		fb->shminfo.readOnly = False;
		XLockDisplay(dpy);
		XSync(dpy, False);
		prevHandler = XSetErrorHandler(xhandler);
		extok = 1;
		serial = NextRequest(dpy);
		XShmAttach(dpy, &fb->shminfo);
		XSync(dpy, False);
		XSetErrorHandler(prevHandler);
		shmok = extok;
		if(!alreadyWarned && !shmok && warningFile)
		{
			fprintf(warningFile,
				"[FBX] WARNING: MIT-SHM extension failed to initialize (this is normal on a\n");
			fprintf(warningFile, "[FBX]    remote X connection.)\n");
			alreadyWarned = 1;
		}
		XUnlockDisplay(dpy);
		shmctl(fb->shminfo.shmid, IPC_RMID, 0);
		if(!shmok)
		{
			useShm = 0;  XFree(fb->xvi);  shmdt(fb->shminfo.shmaddr);
			shmctl(fb->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		fb->xattach = 1;  fb->shm = 1;
	}
	else if(useShm)
	{
		static int alreadyWarned = 0;
		if(!alreadyWarned && warningFile)
		{
			fprintf(warningFile, "[FBX] WARNING: MIT-SHM extension not available.\n");
			alreadyWarned = 1;
		}
		useShm = 0;
	}
	noshm:
	if(!useShm)
	#endif
	{
		if(!(fb->xvi = XvCreateImage(dpy, fb->port, format, 0, width, height)))
			THROW("Could not create XvImage structure");
		if(!(fb->xvi->data = malloc(fb->xvi->data_size)))
			THROW("Memory allocation failure");
	}
	if(!(fb->xgc = XCreateGC(dpy, win, 0, NULL)))
		THROW("Could not create X11 graphics context");
	return 0;

	finally:
	fbxv_term(fb);
	return -1;
}


int fbxv_write(fbxv_struct *fb, int srcX_, int srcY_, int srcWidth_,
	int srcHeight_, int dstX_, int dstY_, int dstWidth, int dstHeight)
{
	int srcX, srcY, dstX, dstY, srcWidth, srcHeight;
	if(!fb) THROW("Invalid argument");

	srcX = srcX_ >= 0 ? srcX_ : 0;
	srcY = srcY_ >= 0 ? srcY_ : 0;
	srcWidth = srcWidth_ > 0 ? srcWidth_ : fb->xvi->width;
	srcHeight = srcHeight_ > 0 ? srcHeight_ : fb->xvi->height;
	dstX = dstX_ >= 0 ? dstX_ : 0;
	dstY = dstY_ >= 0 ? dstY_ : 0;

	if(srcWidth > fb->xvi->width) srcWidth = fb->xvi->width;
	if(srcHeight > fb->xvi->height) srcHeight = fb->xvi->height;
	if(srcX + srcWidth > fb->xvi->width) srcWidth = fb->xvi->width - srcX;
	if(srcY + srcHeight > fb->xvi->height) srcHeight = fb->xvi->height - srcY;

	#ifdef USESHM
	if(fb->shm)
	{
		if(!fb->xattach)
		{
			ERRIFNOT(XShmAttach(fb->dpy, &fb->shminfo));  fb->xattach = 1;
		}
		TRY_X11(XvShmPutImage(fb->dpy, fb->port, fb->win, fb->xgc, fb->xvi, srcX,
			srcY, srcWidth, srcHeight, dstX, dstY, dstWidth, dstHeight, False));
	}
	else
	#endif

	TRY_X11(XvPutImage(fb->dpy, fb->port, fb->win, fb->xgc, fb->xvi, srcX, srcY,
		srcWidth, srcHeight, dstX, dstY, dstWidth, dstHeight));
	XFlush(fb->dpy);
	XSync(fb->dpy, False);
	return 0;

	finally:
	return -1;
}


int fbxv_term(fbxv_struct *fb)
{
	if(!fb) THROW("Invalid argument");
	if(fb->xvi && !fb->shm)
	{
		free(fb->xvi->data);  fb->xvi->data = NULL;
	}

	#ifdef USESHM
	if(fb->shm)
	{
		if(fb->xattach)
		{
			XShmDetach(fb->dpy, &fb->shminfo);  XSync(fb->dpy, False);
		}
		if(fb->shminfo.shmaddr != NULL) shmdt(fb->shminfo.shmaddr);
		if(fb->shminfo.shmid != -1) shmctl(fb->shminfo.shmid, IPC_RMID, 0);
	}
	#endif

	if(fb->xvi) XFree(fb->xvi);
	if(fb->xgc) XFreeGC(fb->dpy, fb->xgc);
	memset(fb, 0, sizeof(fbxv_struct));
	return 0;

	finally:
	return -1;
}
