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

static int jpegsub[RR_SUBSAMPOPT]={HPJ_444, HPJ_422, HPJ_411};

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
		hnew->size=hnew->framew*hnew->frameh*ps;
		checkheader(hnew);
		if(ps<3 || ps>4) _throw("Only true color bitmaps are supported");
		if(hnew->framew!=h.framew || hnew->frameh!=h.frameh || pixelsize!=ps
		|| !bits)
		{
			if(bits) delete [] bits;
			errifnot(bits=new unsigned char[hnew->framew*hnew->frameh*ps+1]);
			pixelsize=ps;  pitch=ps*hnew->framew;
		}
		memcpy(&h, hnew, sizeof(rrframeheader));
	}

	rrframe *getstrip(int startline, int endline)
	{
		rrframe *f;
		if(!bits || !pitch || !pixelsize) _throw("Frame not initialized");
		if(startline<0 || startline>h.height-1 || endline<=0 || endline>h.height)
			_throw("Invalid argument");
		errifnot(f=new rrframe(false));
		f->h=h;
		f->h.height=endline-startline;
		f->h.y+=startline;
		f->pixelsize=pixelsize;
		f->flags=flags;
		f->pitch=pitch;
		bool bu=(flags&RRBMP_BOTTOMUP);
		f->bits=&bits[pitch*(bu? h.height-endline:startline)];
		return f;
	}

	void zero(void)
	{
		if(!h.frameh || !pitch) return;
		memset(bits, 0, pitch*h.frameh);
	}

	bool stripequals(rrframe *last, int startline, int endline)
	{
		bool bu=(flags&RRBMP_BOTTOMUP);
		if(last && h.width==last->h.width && h.height==last->h.height
		&& h.framew==last->h.framew && h.frameh==last->h.frameh
		&& h.qual==last->h.qual && h.subsamp==last->h.subsamp
		&& pixelsize==last->pixelsize && h.winid==last->h.winid
		&& h.dpynum==last->h.dpynum && bits && last->pitch==pitch
		&& startline>=0 && startline<=h.height-1 && endline>0 && endline<=h.height
		&& last->bits && !memcmp(&bits[pitch*(bu? h.height-endline:startline)],
			&last->bits[pitch*(bu? h.height-endline:startline)],
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
				memcpy(bits, f.bits, f.h.framew*f.h.frameh*f.pixelsize);
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
		rrout.print("h->dpynum  = %d\n", hdr->dpynum);
		rrout.print("h->framew  = %d\n", hdr->framew);
		rrout.print("h->frameh  = %d\n", hdr->frameh);
		rrout.print("h->width   = %d\n", hdr->width);
		rrout.print("h->height  = %d\n", hdr->height);
		rrout.print("h->x       = %d\n", hdr->x);
		rrout.print("h->y       = %d\n", hdr->y);
		rrout.print("h->qual    = %d\n", hdr->qual);
		rrout.print("h->subsamp = %d\n", hdr->subsamp);
		rrout.print("h->eof     = %d\n", hdr->eof);
	}

	void checkheader(rrframeheader *hdr)
	{
		if(!hdr || hdr->framew<1 || hdr->frameh<1 || hdr->width<1 || hdr->height<1
		|| hdr->x+hdr->width>hdr->framew || hdr->y+hdr->height>hdr->frameh)
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
		if(b.h.qual>100 || b.h.subsamp>RR_SUBSAMPOPT-1)
			throw(rrerror("JPEG compressor", "Invalid argument"));
		init(&b.h);
		if(b.flags&RRBMP_BOTTOMUP) hpjflags|=HPJ_BOTTOMUP;
		if(b.flags&RRBMP_BGR) hpjflags|=HPJ_BGR;
		unsigned long size;
		hpj(hpjCompress(hpjhnd, b.bits, b.h.width, b.pitch, b.h.height, b.pixelsize,
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
		if(hnew->framew!=h.framew || hnew->frameh!=h.frameh || !bits)
		{
			if(bits) delete [] bits;
			errifnot(bits=new unsigned char[HPJBUFSIZE(hnew->framew, hnew->frameh)]);
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
		fbx(fbx_init(&fb, wh, hnew->framew, hnew->frameh, 1));
		if(hnew->framew>fb.width || hnew->frameh>fb.height)
		{
			XSync(wh.dpy, False);
			fbx(fbx_init(&fb, wh, hnew->framew, hnew->frameh, 1));
		}
		memcpy(&h, hnew, sizeof(rrframeheader));
		if(h.framew>fb.width) h.framew=fb.width;
		if(h.frameh>fb.height) h.frameh=fb.height;
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
		int width=min(f.h.width, fb.width-f.h.x);
		int height=min(f.h.height, fb.height-f.h.y);
		if(width>0 && height>0 && f.h.width<=width && f.h.height<=height)
		{
			if(!hpjhnd)
			{
				if((hpjhnd=hpjInitDecompress())==NULL) throw(rrerror("rrfb::decompressor", hpjGetErrorStr()));
			}
			hpj(hpjDecompress(hpjhnd, f.bits, f.h.size, (unsigned char *)&fb.bits[fb.pitch*f.h.y+f.h.x*fbx_ps[fb.format]],
				width, fb.pitch, height, fbx_ps[fb.format], hpjflags));
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
		int _w=h.width, _h=h.height;
		XWindowAttributes xwa;
		if(!XGetWindowAttributes(wh.dpy, wh.win, &xwa))
		{
			rrout.print("Failed to get window attributes\n");
			return;
		}
		if(h.x+h.width>xwa.width || h.y+h.height>xwa.height)
		{
			rrout.print("WARNING: bitmap (%d,%d) at (%d,%d) extends beyond window (%d,%d)\n",
				h.width, h.height, h.x, h.y, xwa.width, xwa.height);
			_w=min(h.width, xwa.width-h.x);
			_h=min(h.height, xwa.height-h.y);
		}
		if(h.x+h.width<=fb.width && h.y+h.height<=fb.height)
		fbx(fbx_write(&fb, h.x, h.y, h.x, h.y, _w, _h));
	}

	void drawstrip(int startline, int endline)
	{
		if(startline<0 || startline>h.frameh-1 || endline<0 || endline>h.frameh)
			return;
		if(flags&RRBMP_BOTTOMUP)
		{
			for(int i=startline; i<endline; i++)
				fbx(fbx_awrite(&fb, 0, fb.height-i-1, 0, i, 0, 1));
		}
		else
		fbx(fbx_awrite(&fb, 0, startline, 0, startline, 0, endline-startline));
	}

	void sync(void) {fbx(fbx_sync(&fb));}

	private:

	fbx_wh wh;
	fbx_struct fb;
	hpjhandle hpjhnd;
};

// Bitmap drawn using OpenGL

#ifdef USEGL
#include <GL/glx.h>

class rrglframe : public rrframe
{
	public:

	rrglframe(char *dpystring, Window _win) : rrframe(), madecurrent(false),
		win(_win), hpjhnd(NULL)
	{
		XVisualInfo *v=NULL;
		try
		{
			if(!dpystring || !win) throw(rrerror("rrglframe::rrglframe", "Invalid argument"));
			if(!(dpy=XOpenDisplay(dpystring))) _throw("Could not open display");
			pixelsize=3;
			XWindowAttributes xwa;
			XGetWindowAttributes(dpy, win, &xwa);
			XVisualInfo vtemp;  int n=0;
			vtemp.visualid=xwa.visual->visualid;
			if(!(v=XGetVisualInfo(dpy, VisualIDMask, &vtemp, &n)) || n==0)
				_throw("Could not obtain visual");
			if(!(ctx=glXCreateContext(dpy, v, NULL, True)))
				_throw("Could not create GLX context");
			XFree(v);  v=NULL;
		}
		catch(...)
		{
			if(v) XFree(v);
			throw;
		}
	}

	~rrglframe(void)
	{
		if(ctx && dpy) {glXMakeCurrent(dpy, 0, 0);  glXDestroyContext(dpy, ctx);  ctx=0;}
		if(dpy) {XCloseDisplay(dpy);  dpy=NULL;}
		if(hpjhnd) {hpjDestroy(hpjhnd);  hpjhnd=NULL;}
	}

	void init(rrframeheader *hnew)
	{
		int flags=RRBMP_BOTTOMUP;
		#ifdef GL_BGR_EXT
		if(littleendian()) flags=RRBMP_BGR;
		#endif
		rrframe::init(hnew, 3, flags);
	}

	rrglframe& operator= (rrjpeg& f)
	{
		int hpjflags=HPJ_BOTTOMUP;
		if(!f.bits || f.h.size<1) _throw("JPEG not initialized");
		init(&f.h);
		if(!bits) _throw("Bitmap not initialized");
		if(flags&RRBMP_BGR) hpjflags|=HPJ_BGR;
		int width=min(f.h.width, h.framew-f.h.x);
		int height=min(f.h.height, h.frameh-f.h.y);
		if(width>0 && height>0 && f.h.width<=width && f.h.height<=height)
		{
			if(!hpjhnd)
			{
				if((hpjhnd=hpjInitDecompress())==NULL) throw(rrerror("rrglframe::decompressor", hpjGetErrorStr()));
			}
			int y=max(0, h.frameh-f.h.y-height);
			hpj(hpjDecompress(hpjhnd, f.bits, f.h.size, (unsigned char *)&bits[pitch*y+f.h.x*pixelsize],
				width, pitch, height, pixelsize, hpjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		int format=GL_RGB;
		#ifdef GL_BGR_EXT
		if(littleendian()) format=GL_BGR_EXT;
		#endif
		if(!madecurrent)
		{
			if(!glXMakeCurrent(dpy, win, ctx))
				_throw("Could not make context current");
			madecurrent=true;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glDrawPixels(h.framew, h.frameh, format, GL_UNSIGNED_BYTE, bits);
		glXSwapBuffers(dpy, win);
	}

	private:

	bool madecurrent;
	Display *dpy;  Window win;
	GLXContext ctx;
	hpjhandle hpjhnd;
};
#endif

#endif
