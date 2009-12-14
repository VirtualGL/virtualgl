/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009 D. R. Commander
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

// This library abstracts fast frame buffer access
#include <string.h>
#include <stdlib.h>
#include "fbxv.h"
#include "x11err.h"

static int __line=-1;
static FILE *__warningfile=NULL;

static char __lasterror[1024]="No error";
#define _throw(m) {snprintf(__lasterror, 1023, "%s", m);  __line=__LINE__; \
	goto finally;}
#define x11(f) {int __err=0;  if((__err=(f))!=Success) { \
	snprintf(__lasterror, 1023, "X11 %s Error (window may have disappeared)", \
		x11error(__err)); \
	__line=__LINE__;  goto finally;}}
#define errifnot(f) {if(!(f)) { \
	snprintf(__lasterror, 1023, "X11 Error (window may have disappeared)"); \
	__line=__LINE__;  goto finally;}}

#ifdef USESHM
static unsigned long serial=0;  static int __extok=1;
static XErrorHandler prevhandler=NULL;

int _fbxv_xhandler(Display *dpy, XErrorEvent *e)
{
	if(e->serial==serial && (e->minor_code==X_ShmAttach
		&& e->error_code==BadAccess))
	{
		__extok=0;  return 0;
	}
	if(prevhandler && prevhandler!=_fbxv_xhandler) return prevhandler(dpy, e);
	else return 0;
}
#endif

char *fbxv_geterrmsg(void)
{
	return __lasterror;
}

int fbxv_geterrline(void)
{
	return __line;
}

void fbxv_printwarnings(FILE *stream)
{
	__warningfile=stream;
}

int fbxv_init(fbxv_struct *s, Display *dpy, Window win, int width, int height,
	unsigned int format, int useshm)
{
	int w, h, nadaptors=0, i, j, k, shmok=1, nformats;
	unsigned int dummy1, dummy2, dummy3, dummy4, dummy5;
	XWindowAttributes xwinattrib;
	XvAdaptorInfo *ai=NULL;
	XvImageFormatValues *ifv=NULL;

	if(!s) _throw("Invalid argument");

	if(!dpy || !win) _throw("Invalid argument");
	errifnot(XGetWindowAttributes(dpy, win, &xwinattrib));
	if(width>0) w=width;  else w=xwinattrib.width;
	if(height>0) h=height;  else h=xwinattrib.height;
	if(s->dpy==dpy && s->win==win)
	{
		if(w==s->reqwidth && h==s->reqheight && s->xvi && s->xgc && s->xvi->data)
			return 0;
		else if(fbxv_term(s)==-1) return -1;
	}
	memset(s, 0, sizeof(fbxv_struct));
	s->dpy=dpy;  s->win=win;
	s->reqwidth=width;  s->reqheight=height;

	if(XvQueryExtension(dpy, &dummy1, &dummy2, &dummy3, &dummy4, &dummy5)
		!=Success)
		_throw("X Video Extension not available");
	if(XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &nadaptors, &ai)!=Success)
		_throw("Could not query X Video adaptors");
	if(nadaptors<1 || !ai) _throw("No X Video adaptors available");

	s->port=-1;
	for(i=0; i<nadaptors; i++)
	{
		for(j=ai[i].base_id; j<ai[i].base_id+ai[i].num_ports; j++)
		{
			nformats=0;
			ifv=XvListImageFormats(dpy, j, &nformats);
			if(ifv && nformats>0)
			{
				for(k=0; k<nformats; k++)
				{
					if(ifv[k].id==format)
					{
						XFree(ifv);  s->port=j;
						goto found;
					}
				}
			}
			XFree(ifv);
		}
	}
	found:
	XvFreeAdaptorInfo(ai);  ai=NULL;
	if(s->port==-1)
		_throw("The X Video implementation on the 2D X Server does not support the desired pixel format");

	#ifdef USESHM
	if(useshm && XShmQueryExtension(s->dpy))
	{
		static int alreadywarned=0;
		s->shminfo.shmid=-1;
		if(!(s->xvi=XvShmCreateImage(dpy, s->port, format, 0, width, height,
			&s->shminfo)))
		{
			useshm=0;  goto noshm;
		}
		if((s->shminfo.shmid=shmget(IPC_PRIVATE, s->xvi->data_size, IPC_CREAT|0777))==-1)
		{
			useshm=0;  XFree(s->xvi);  goto noshm;
		}
		if((s->shminfo.shmaddr=s->xvi->data=(char *)shmat(s->shminfo.shmid, 0, 0))
			==(char *)-1)
		{
			useshm=0;  XFree(s->xvi);  shmctl(s->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		s->shminfo.readOnly=False;
		XLockDisplay(dpy);
		XSync(dpy, False);
		prevhandler=XSetErrorHandler(_fbxv_xhandler);
		__extok=1;
		serial=NextRequest(dpy);
		XShmAttach(dpy, &s->shminfo);
		XSync(dpy, False);
		XSetErrorHandler(prevhandler);
		shmok=__extok;
		if(!alreadywarned && !shmok && __warningfile)
		{
			fprintf(__warningfile, "[FBX] WARNING: MIT-SHM extension failed to initialize (this is normal on a\n");
			fprintf(__warningfile, "[FBX]    remote connection.)\n");
			alreadywarned=1;
		}
		XUnlockDisplay(dpy);
		shmctl(s->shminfo.shmid, IPC_RMID, 0);
		if(!shmok)
		{
			useshm=0;  XFree(s->xvi);  shmdt(s->shminfo.shmaddr);
			shmctl(s->shminfo.shmid, IPC_RMID, 0);  goto noshm;
		}
		s->xattach=1;  s->shm=1;
	}
	else if(useshm)
	{
		static int alreadywarned=0;
		if(!alreadywarned && __warningfile)
		{
			fprintf(__warningfile, "[FBX] WARNING: MIT-SHM extension not available.\n");
			alreadywarned=1;
		}
		useshm=0;
	}
	noshm:
	if(!useshm)
	#endif
	{
		if(!(s->xvi=XvCreateImage(dpy, s->port, format, 0, width, height)))
			_throw("Could not create XvImage structure");
		if(!(s->xvi->data=malloc(s->xvi->data_size)))
			_throw("Memory allocation failure");
	}
	if(!(s->xgc=XCreateGC(dpy, win, 0, NULL)))
		_throw("Could not create X11 graphics context");
	return 0;

	finally:
	fbxv_term(s);
	return -1;
}

int fbxv_write(fbxv_struct *s, int srcx, int srcy, int srcw, int srch,
	int dstx, int dsty, int dstw, int dsth)
{
	int sx, sy, dx, dy, sw, sh;
	if(!s) _throw("Invalid argument");

	sx=srcx>=0? srcx:0;  sy=srcy>=0? srcy:0;
	sw=srcw>0? srcw:s->xvi->width;  sh=srch>0? srch:s->xvi->height;
	dx=dstx>=0? dstx:0;  dy=dsty>=0? dsty:0;
	if(sw>s->xvi->width) sw=s->xvi->width;
	if(sh>s->xvi->height) sh=s->xvi->height;
	if(sx+sw>s->xvi->width) sw=s->xvi->width-sx;
	if(sy+sh>s->xvi->height) sh=s->xvi->height-sy;

	#ifdef USESHM
	if(s->shm)
	{
		if(!s->xattach) {errifnot(XShmAttach(s->dpy, &s->shminfo));  s->xattach=1;}
		x11(XvShmPutImage(s->dpy, s->port, s->win, s->xgc, s->xvi, sx, sy, sw, sh,
			dx, dy, dstw, dsth, False));
	}
	else
	#endif
	x11(XvPutImage(s->dpy, s->port, s->win, s->xgc, s->xvi, sx, sy, sw, sh, dx,
		dy, dstw, dsth));
	XFlush(s->dpy);
	if(s->shm) XSync(s->dpy, False);
	return 0;

	finally:
	return -1;
}

int fbxv_term(fbxv_struct *s)
{
	if(!s) _throw("Invalid argument");
	if(s->xvi) 
	{
		if(s->xvi->data && !s->shm) {free(s->xvi->data);  s->xvi->data=NULL;}
	}
	#ifdef USESHM
	if(s->shm)
	{
		if(s->xattach) {XShmDetach(s->dpy, &s->shminfo);  XSync(s->dpy, False);}
		if(s->shminfo.shmaddr!=NULL) shmdt(s->shminfo.shmaddr);
		if(s->shminfo.shmid!=-1) shmctl(s->shminfo.shmid, IPC_RMID, 0);
	}
	#endif
	if(s->xvi) XFree(s->xvi);
	if(s->xgc) XFreeGC(s->dpy, s->xgc);
	memset(s, 0, sizeof(fbxv_struct));
	return 0;

	finally:
	return -1;
}
