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

#include "rr.h"
#ifdef _WIN32
#define XDK
#endif
#include "fbx.h"
#include "hpjpeg.h"
#include "rrmutex.h"
#include "rrlog.h"
#include "rrutil.h"
#include <string.h>

static int jpegsub[RR_SUBSAMP]={HPJ_444, HPJ_422, HPJ_411};

// Header contained in all image structures
#pragma pack(1)
typedef struct _rrframeheader
{
	unsigned int size;       // For JPEG images, this contains the size (in bytes)
                           // of the compressed images.  For uncompressed images,
                           // it should be 0.
	unsigned int winid;      // Usually the X-Window ID, but can be used for other purposes
	unsigned short winw;     // The width of the source window
	unsigned short winh;     // The height of the source window
	unsigned short bmpw;     // The width of the source bitmap
	unsigned short bmph;     // The height of the source bitmap
	unsigned short bmpx;     // The X offset of the bitmap within the window
	unsigned short bmpy;     // The Y offset of the bitmap within the window
	unsigned char qual;      // Quality of destination JPEG (0-100)
	unsigned char subsamp;   // Subsampling of destination JPEG
                           // (RR_411, RR_422, or RR_444)
	unsigned char eof;       // 1 if this is the last (or only) packet in the frame
	unsigned char dpynum;    // Display number on the client machine
} rrframeheader;
#pragma pack()

// Bitmap flags
#define RRBMP_BOTTOMUP   1  // Bottom-up bitmap (as opposed to top-down)
#define RRBMP_BGR        2  // BGR or BGRA pixel order
#define RRBMP_ALPHAFIRST 4  // BGR buffer is really ABGR, and RGB buffer is really ARGB

// Uncompressed bitmap

class rrframe
{
	public:

	rrframe(bool _primary=true) : bits(NULL), pitch(0), pixelsize(0), flags(0),
		strip_height(RR_DEFAULTSTRIPHEIGHT), primary(_primary)
	{
		memset(&h, 0, sizeof(rrframeheader));
		_ready.lock();
	}

	virtual ~rrframe(void)
	{
		if(bits && primary) delete [] bits;
	}

	void init(rrframeheader *hnew, int ps, int fl=RRBMP_BGR)
	{
		if(!hnew) throw(rrerror("rrframe::init", "Invalid argument"));
		flags=fl;
		hnew->size=hnew->winw*hnew->winh*ps;
		checkheader(hnew);
		if(ps<3 || ps>4) _throw("Only true color bitmaps are supported");
		if(hnew->winw!=h.winw || hnew->winh!=h.winh || pixelsize!=ps
		|| !bits)
		{
			if(bits) delete [] bits;
			errifnot(bits=new unsigned char[hnew->winw*hnew->winh*ps+1]);
			pixelsize=ps;  pitch=ps*hnew->winw;
		}
		memcpy(&h, hnew, sizeof(rrframeheader));
	}

	rrframe *getstrip(int startline, int endline)
	{
		rrframe *f;
		if(!bits || !pitch || !pixelsize) _throw("Frame not initialized");
		if(startline<0 || startline>h.bmph-1 || endline<=0 || endline>h.bmph)
			_throw("Invalid argument");
		errifnot(f=new rrframe(false));
		f->h=h;
		f->h.bmph=endline-startline;
		f->h.bmpy+=startline;
		f->pixelsize=pixelsize;
		f->flags=flags;
		f->pitch=pitch;
		bool bu=(flags&RRBMP_BOTTOMUP);
		f->bits=&bits[pitch*(bu? h.bmph-endline:startline)];
		return f;
	}

	void zero(void)
	{
		if(!h.winh || !pitch) return;
		memset(bits, 0, pitch*h.winh);
	}

	bool stripequals(rrframe *last, int startline, int endline)
	{
		bool bu=(flags&RRBMP_BOTTOMUP);
		if(last && h.bmpw==last->h.bmpw && h.bmph==last->h.bmph
		&& h.winw==last->h.winw && h.winh==last->h.winh
		&& h.qual==last->h.qual && h.subsamp==last->h.subsamp
		&& pixelsize==last->pixelsize && h.winid==last->h.winid
		&& h.dpynum==last->h.dpynum && bits && last->pitch==pitch
		&& startline>=0 && startline<=h.bmph-1 && endline>0 && endline<=h.bmph
		&& last->bits && !memcmp(&bits[pitch*(bu? h.bmph-endline:startline)],
			&last->bits[pitch*(bu? h.bmph-endline:startline)],
			pitch*(endline-startline))) return true;
		return false;
	}

	void ready(void) {_ready.unlock();}
	void waituntilready(void) {_ready.lock();}
	void complete(void) {_complete.unlock();}
	void waituntilcomplete(void) {_complete.lock();}

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
	int pitch, pixelsize, flags, strip_height;

	protected:

	void dumpheader(rrframeheader *hdr)
	{
		rrout.print("h->size    = %lu\n", hdr->size);
		rrout.print("h->winid   = 0x%.8x\n", hdr->winid);
		rrout.print("h->winw    = %d\n", hdr->winw);
		rrout.print("h->winh    = %d\n", hdr->winh);
		rrout.print("h->bmpw    = %d\n", hdr->bmpw);
		rrout.print("h->bmph    = %d\n", hdr->bmph);
		rrout.print("h->bmpx    = %d\n", hdr->bmpx);
		rrout.print("h->bmpy    = %d\n", hdr->bmpy);
		rrout.print("h->qual    = %d\n", hdr->qual);
		rrout.print("h->subsamp = %d\n", hdr->subsamp);
		rrout.print("h->eof     = %d\n", hdr->eof);
	}

	void checkheader(rrframeheader *hdr)
	{
		if(!hdr || hdr->winw<1 || hdr->winh<1 || hdr->bmpw<1 || hdr->bmph<1
		|| hdr->bmpx+hdr->bmpw>hdr->winw || hdr->bmpy+hdr->bmph>hdr->winh)
			throw(rrerror("rrframe::checkheader", "Invalid header"));
	}

	rrmutex _ready;
	rrmutex _complete;
	friend class rrjpeg;
	bool primary;
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

	rrjpeg& operator= (rrframe& b)
	{
		int hpjflags=0;
		if(!b.bits) _throw("Bitmap not initialized");
		if(b.pixelsize<3 || b.pixelsize>4) _throw("Only true color bitmaps are supported");
		if(b.h.qual>100 || b.h.subsamp>RR_SUBSAMP-1)
			throw(rrerror("JPEG compressor", "Invalid argument"));
		init(&b.h);
		if(b.flags&RRBMP_BOTTOMUP) hpjflags|=HPJ_BOTTOMUP;
		if(b.flags&RRBMP_BGR) hpjflags|=HPJ_BGR;
		unsigned long size;
		hpj(hpjCompress(hpjhnd, b.bits, b.h.bmpw, b.pitch, b.h.bmph, b.pixelsize,
			bits, &size, jpegsub[b.h.subsamp], b.h.qual, hpjflags));
		h.size=(unsigned int)size;
		return *this;
	}

	void init(rrframeheader *hnew, int ps)
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
	friend class rrfb;
};

// Bitmap created from shared graphics memory

class rrfb : public rrframe
{
	public:

	rrfb(Display *dpy, Window win) : rrframe()
	{
		if(!dpy || !win) throw(rrerror("rrfb::rrfb", "Invalid argument"));
		XFlush(dpy);
		init(DisplayString(dpy), win);
	}

	rrfb(char *dpystring, Window win) : rrframe()
	{
		init(dpystring, win);
	}

	void init(char *dpystring, Window win)
	{
		hpjhnd=NULL;
		memset(&fb, 0, sizeof(fbx_struct));
		if(!dpystring || !win) throw(rrerror("rrfb::init", "Invalid argument"));
		if(!(wh.dpy=XOpenDisplay(dpystring)))
			throw(rrerror("rrfb::init", "Could not open display"));
		wh.win=win;
	}

	~rrfb(void)
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
		memcpy(&h, hnew, sizeof(rrframeheader));
		if(h.winw>fb.width) h.winw=fb.width;
		if(h.winh>fb.height) h.winh=fb.height;
		pixelsize=fbx_ps[fb.format];  pitch=fb.pitch;  bits=(unsigned char *)fb.bits;
		flags=0;
		if(fbx_bgr[fb.format]) flags|=RRBMP_BGR;
		if(fbx_alphafirst[fb.format]) flags|=RRBMP_ALPHAFIRST;
	}

	rrfb& operator= (rrjpeg& f)
	{
		int hpjflags=0;
		if(!f.bits || f.h.size<1)
			_throw("JPEG not initialized");
		init(&f.h);
		if(!fb.xi) _throw("Bitmap not initialized");
		if(fbx_bgr[fb.format]) hpjflags|=HPJ_BGR;
		if(fbx_alphafirst[fb.format]) hpjflags|=HPJ_ALPHAFIRST;
		int bmpw=min(f.h.bmpw, fb.width-f.h.bmpx);
		int bmph=min(f.h.bmph, fb.height-f.h.bmpy);
		if(bmpw>0 && bmph>0 && f.h.bmpw<=bmpw && f.h.bmph<=bmph)
		{
			if(!hpjhnd)
			{
				if((hpjhnd=hpjInitDecompress())==NULL) throw(rrerror("rrfb::decompressor", hpjGetErrorStr()));
			}
			hpj(hpjDecompress(hpjhnd, f.bits, f.h.size, (unsigned char *)&fb.bits[fb.pitch*f.h.bmpy+f.h.bmpx*fbx_ps[fb.format]],
				bmpw, fb.pitch, bmph, fbx_ps[fb.format], hpjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		if(flags&RRBMP_BOTTOMUP)
		{
			for(int i=0; i<fb.height; i++)
				fbx(fbx_awrite(&fb, 0, fb.height-i-1, 0, i, 0, 1));
			fbx(fbx_sync(&fb));
		}
		else
			fbx(fbx_write(&fb, 0, 0, 0, 0, fb.width, fb.height));
	}

	void draw(void)
	{
		int _w=h.bmpw, _h=h.bmph;
		XWindowAttributes xwa;
		if(!XGetWindowAttributes(wh.dpy, wh.win, &xwa))
		{
			rrout.print("Failed to get window attributes\n");
			return;
		}
		if(h.bmpx+h.bmpw>xwa.width || h.bmpy+h.bmph>xwa.height)
		{
			rrout.print("WARNING: bitmap (%d,%d) at (%d,%d) extends beyond window (%d,%d)\n",
				h.bmpw, h.bmph, h.bmpx, h.bmpy, xwa.width, xwa.height);
			_w=min(h.bmpw, xwa.width-h.bmpx);
			_h=min(h.bmph, xwa.height-h.bmpy);
		}
		if(h.bmpx+h.bmpw<=fb.width && h.bmpy+h.bmph<=fb.height)
		fbx(fbx_write(&fb, h.bmpx, h.bmpy, h.bmpx, h.bmpy, _w, _h));
	}

	void drawstrip(int startline, int endline)
	{
		if(startline<0 || startline>h.winh-1 || endline<0 || endline>h.winh)
			return;
		fbx(fbx_awrite(&fb, 0, startline, 0, startline, 0, endline-startline));
	}

	void sync(void) {fbx(fbx_sync(&fb));}

	private:

	fbx_wh wh;
	fbx_struct fb;
	hpjhandle hpjhnd;
};

#endif
