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

#ifndef __RRFRAME_H
#define __RRFRAME_H

#include "rrcommon.h"
#include "rr.h"
#ifdef _WIN32
#define XDK
#endif
#include "fbx.h"
#include "hpjpeg.h"
#include "rrerror.h"
#include <string.h>
#include <pthread.h>

static int jpegsub[RR_SUBSAMP]={HPJ_444, HPJ_422, HPJ_411};

// Uncompressed bitmap

class rrframe
{
	public:

	rrframe(void)
	{
		memset(&h, 0, sizeof(rrframeheader));
		bits=NULL;
		tryunix(pthread_mutex_init(&_ready, NULL));  tryunix(pthread_mutex_lock(&_ready));
		tryunix(pthread_mutex_init(&_complete, NULL));
		pixelsize=0;  pitch=0;  flags=0;
	}

	virtual ~rrframe(void)
	{
		pthread_mutex_unlock(&_ready);  pthread_mutex_destroy(&_ready);
		pthread_mutex_unlock(&_complete);  pthread_mutex_destroy(&_complete);
		if(bits) delete [] bits;
	}

	void init(rrframeheader *hnew, int pixelsize, int flags=RRBMP_BGR)
	{
		if(!hnew) throw(rrerror("rrframe::init", "Invalid argument"));
		this->flags=flags;
		hnew->size=hnew->winw*hnew->winh*pixelsize;
		checkheader(hnew);
		if(pixelsize<3 || pixelsize>4) _throw("Only true color bitmaps are supported");
		if(hnew->winw!=h.winw || hnew->winh!=h.winh || this->pixelsize!=pixelsize
		|| !bits)
		{
			if(bits) delete [] bits;
			errifnot(bits=new unsigned char[hnew->winw*hnew->winh*pixelsize]);
			this->pixelsize=pixelsize;  pitch=pixelsize*hnew->winw;
		}
		memcpy(&h, hnew, sizeof(rrframeheader));
	}

	void zero(void)
	{
		if(!h.winh || !pitch) return;
		memset(bits, 0, pitch*h.winh);
	}

	void ready(void) {pthread_mutex_unlock(&_ready);}
	void waituntilready(void) {tryunix(pthread_mutex_lock(&_ready));}
	void complete(void) {pthread_mutex_unlock(&_complete);}
	void waituntilcomplete(void) {tryunix(pthread_mutex_lock(&_complete));}

	rrframe& operator= (rrframe& f)
	{
		if(this!=&f && f.bits)
		{
			if(f.bits)
			{
				init(&f.h, f.pixelsize);
				memcpy(bits, f.bits, f.h.winw*f.h.winh*f.pixelsize);
			}
		}
		return *this;
	}

	rrframeheader h;
	unsigned char *bits;
	int pitch, pixelsize, flags;

	protected:

	void dumpheader(rrframeheader *h)
	{
		hpprintf("h->size    = %lu\n", h->size);
		hpprintf("h->winid   = 0x%.8x\n", h->winid);
		hpprintf("h->winw    = %d\n", h->winw);
		hpprintf("h->winh    = %d\n", h->winh);
		hpprintf("h->bmpw    = %d\n", h->bmpw);
		hpprintf("h->bmph    = %d\n", h->bmph);
		hpprintf("h->bmpx    = %d\n", h->bmpx);
		hpprintf("h->bmpy    = %d\n", h->bmpy);
		hpprintf("h->qual    = %d\n", h->qual);
		hpprintf("h->subsamp = %d\n", h->subsamp);
		hpprintf("h->eof     = %d\n", h->eof);
	}

	void checkheader(rrframeheader *h)
	{
		if(!h
		|| h->winw<1 || h->winh<1 || h->bmpw<1 || h->bmph<1
		|| h->bmpx+h->bmpw>h->winw || h->bmpy+h->bmph>h->winh)
			throw(rrerror("rrframe::checkheader", "Invalid header"));

	}

	pthread_mutex_t _ready;
	pthread_mutex_t _complete;
};

// Compressed JPEG

class rrjpeg : public rrframe
{
	public:

	rrjpeg(void) : rrframe(), hpjhnd(NULL)
	{
		if(!(hpjhnd=hpjInitCompress())) _throw(hpjGetErrorStr());
		pixelsize=3;
	}

	~rrjpeg(void)
	{
		if(hpjhnd) hpjDestroy(hpjhnd);
	}

	rrjpeg& operator= (rrbmp& b)
	{
		int hpjflags=0;
		if(!b.bits) _throw("Bitmap not initialized");
		if(b.pixelsize<3 || b.pixelsize>4) _throw("Only true color bitmaps are supported");
		if(b.h.qual>100 || b.h.subsamp>RR_SUBSAMP-1)
			throw(rrerror("JPEG compressor", "Invalid argument"));
		init(&b.h);
		int pitch=b.h.bmpw*b.pixelsize;
		if(b.flags&RRBMP_EOLPAD) pitch=HPJPAD(pitch);
		if(b.flags&RRBMP_BOTTOMUP) hpjflags|=HPJ_BOTTOMUP;
		if(b.flags&RRBMP_BGR) hpjflags|=HPJ_BGR;
		unsigned long size;
		hpj(hpjCompress(hpjhnd, b.bits, b.h.bmpw, pitch, b.h.bmph, b.pixelsize,
			bits, &size, jpegsub[b.h.subsamp], b.h.qual, hpjflags));
		h.size=(unsigned int)size;
		return *this;
	}

	void init(rrframeheader *hnew, int pixelsize)
	{
		init(hnew);
	}

	void init(rrframeheader *hnew)
	{
		checkheader(hnew);
		if(hnew->winw!=h.winw || hnew->winh!=h.winh || !bits)
		{
			if(bits) delete [] bits;
			errifnot(bits=new unsigned char[HPJBUFSIZE(hnew->winw, hnew->winh)]);
		}
		memcpy(&h, hnew, sizeof(rrframeheader));
	}

	private:

	hpjhandle hpjhnd;
	friend class rrbitmap;
};

// Bitmap created from shared graphics memory

class rrbitmap : public rrframe
{
	public:

	rrbitmap(Display *dpy, Window win) : rrframe()
	{
		if(!dpy || !win) throw(rrerror("rrbitmap::rrbitmap", "Invalid argument"));
		XFlush(dpy);
		init(DisplayString(dpy), win);
	}

	rrbitmap(char *dpystring, Window win) : rrframe()
	{
		init(dpystring, win);
	}

	void init(char *dpystring, Window win)
	{
		if(!dpystring || !win) throw(rrerror("rrbitmap::init", "Invalid argument"));
		if(!(wh.dpy=XOpenDisplay(dpystring)))
			throw(rrerror("rrbitmap::init", "Could not open display"));
		wh.win=win;
		hpjhnd=NULL;
		memset(&fb, 0, sizeof(fbx_struct));
	}

	~rrbitmap(void)
	{
		if(fb.bits) fbx_term(&fb);  if(bits) bits=NULL;
		if(hpjhnd) hpjDestroy(hpjhnd);
		if(wh.dpy) XCloseDisplay(wh.dpy);
	}

	void init(rrframeheader *hnew)
	{
		checkheader(hnew);
		fbx(fbx_init(&fb, wh, hnew->winw, hnew->winh, 1));
		if(hnew->winw>fb.width || hnew->winh>fb.height)
		{
			XSync(wh.dpy, False);
			fbx(fbx_init(&fb, wh, hnew->winw, hnew->winh, 1));
		}
		if(fb.ps<3 || fb.ps>4) _throw("Display must be 24-bit or 32-bit true color");
		memcpy(&h, hnew, sizeof(rrframeheader));
		if(h.winw>fb.width) h.winw=fb.width;
		if(h.winh>fb.height) h.winh=fb.height;
		pixelsize=fb.ps;  pitch=fb.xi->bytes_per_line;  bits=(unsigned char *)fb.bits;
		flags=0;
		if(fb.bgr) flags|=RRBMP_BGR;
		if(fb.xi->bytes_per_line!=fb.width*fb.ps) flags|=RRBMP_EOLPAD;
	}

	rrbitmap& operator= (rrjpeg& f)
	{
		int hpjflags=0;
		if(!f.bits || f.h.size<1)
			_throw("JPEG not initialized");
		init(&f.h);
		if(!fb.xi) _throw("Bitmap not initialized");
		if(fb.bgr) hpjflags|=HPJ_BGR;
		int bmpw=min(f.h.bmpw, fb.width-f.h.bmpx);
		int bmph=min(f.h.bmph, fb.height-f.h.bmpy);
		if(bmpw>0 && bmph>0 && f.h.bmpw<=bmpw && f.h.bmph<=bmph)
		{
			if(!hpjhnd)
			{
				if((hpjhnd=hpjInitDecompress())==NULL) throw(rrerror("rrfb::decompressor", hpjGetErrorStr()));
			}
			hpj(hpjDecompress(hpjhnd, f.bits, f.h.size, (unsigned char *)&fb.bits[fb.xi->bytes_per_line*f.h.bmpy+f.h.bmpx*fb.ps],
				bmpw, fb.xi->bytes_per_line, bmph, fb.ps, hpjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		fbx(fbx_write(&fb, 0, 0, 0, 0, fb.width, fb.height));
	}

	void draw(void)
	{
		int _w=h.bmpw, _h=h.bmph;
		XWindowAttributes xwa;
		if(!XGetWindowAttributes(wh.dpy, wh.win, &xwa))
		{
			hpprintf("Failed to get window attributes\n");
			return;
		}
		if(h.bmpx+h.bmpw>xwa.width || h.bmpy+h.bmph>xwa.height)
		{
			hpprintf("WARNING: bitmap (%d,%d) at (%d,%d) extends beyond window (%d,%d)\n",
				h.bmpw, h.bmph, h.bmpx, h.bmpy, xwa.width, xwa.height);
			_w=min(h.bmpw, xwa.width-h.bmpx);
			_h=min(h.bmph, xwa.height-h.bmpy);
		}
		if(h.bmpx+h.bmpw<=fb.width && h.bmpy+h.bmph<=fb.height)
		fbx(fbx_write(&fb, h.bmpx, h.bmpy, h.bmpx, h.bmpy, _w, _h));
	}

	private:

	fbx_wh wh;
	fbx_struct fb;
	hpjhandle hpjhnd;
};

#endif
