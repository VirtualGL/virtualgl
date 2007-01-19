/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
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
#define FBXX11
#endif
#include "fbx.h"
#include "turbojpeg.h"
#include "rrmutex.h"
#include "rrlog.h"
#include "rrutil.h"
#include <string.h>

#define jpegsub(s) (s>=4?TJ_411:s==2?TJ_422:s==1?TJ_444:s==0?TJ_GRAYSCALE:TJ_444)

// Bitmap flags
#define RRBMP_BOTTOMUP   1  // Bottom-up bitmap (as opposed to top-down)
#define RRBMP_BGR        2  // BGR or BGRA pixel order
#define RRBMP_ALPHAFIRST 4  // BGR buffer is really ABGR, and RGB buffer is really ARGB

// Uncompressed bitmap

class rrframe
{
	public:

	rrframe(bool primary=true) : _bits(NULL), _rbits(NULL), _pitch(0),
		_pixelsize(0), _flags(0), _isgl(false), _stereo(false), _primary(primary)
	{
		memset(&_h, 0, sizeof(rrframeheader));
		_ready.wait();
	}

	virtual ~rrframe(void)
	{
		if(_bits && _primary) delete [] _bits;
	}

	void init(rrframeheader &h, int pixelsize, int flags, bool stereo=false)
	{
		if(pixelsize<1) throw(rrerror("rrframe::init", "Invalid argument"));
		_flags=flags;
		h.size=h.framew*h.frameh*pixelsize;
		checkheader(h);
		if(h.framew!=_h.framew || h.frameh!=_h.frameh || pixelsize!=_pixelsize
			|| !_bits)
		{
			if(_bits) delete [] _bits;
			newcheck(_bits=new unsigned char[h.framew*h.frameh*pixelsize+1]);
		}
		if(stereo)
		{
			if(h.framew!=_h.framew || h.frameh!=_h.frameh || pixelsize!=_pixelsize
				|| !_rbits)
			{
				if(_rbits) delete [] _rbits;
				newcheck(_rbits=new unsigned char[h.framew*h.frameh*pixelsize+1]);
			}
		}
		else
		{
			if(_rbits) {delete [] _rbits;  _rbits=NULL;}
		}
		_pixelsize=pixelsize;  _pitch=pixelsize*h.framew;  _stereo=stereo;
		_h=h;
	}

	void init(unsigned char *bits, int w, int pitch, int h, int pixelsize,
		int flags)
	{
		_bits=bits;
		_h.framew=_h.width=w;
		_h.frameh=_h.height=h;
		_h.x=_h.y=0;
		_pixelsize=pixelsize;
		_h.size=_h.framew*_h.frameh*_pixelsize;
		checkheader(_h);
		_pitch=pitch;
		_primary=false;
	}

	rrframe *gettile(int x, int y, int w, int h)
	{
		rrframe *f;
		if(!_bits || !_pitch || !_pixelsize) _throw("Frame not initialized");
		if(x<0 || y<0 || w<1 || h<1 || (x+w)>_h.width || (y+h)>_h.height)
			throw rrerror("rrframe::gettile", "Argument out of range");
		errifnot(f=new rrframe(false));
		f->_h=_h;
		f->_h.height=h;
		f->_h.width=w;
		f->_h.y=y;
		f->_h.x=x;
		f->_pixelsize=_pixelsize;
		f->_flags=_flags;
		f->_pitch=_pitch;
		f->_stereo=_stereo;
		f->_isgl=_isgl;
		bool bu=(_flags&RRBMP_BOTTOMUP);
		f->_bits=&_bits[_pitch*(bu? _h.height-y-h:y)+_pixelsize*x];
		if(_stereo && _rbits) 
			f->_rbits=&_rbits[_pitch*(bu? _h.height-y-h:y)+_pixelsize*x];
		return f;
	}

	bool tileequals(rrframe *last, int x, int y, int w, int h)
	{
		bool bu=(_flags&RRBMP_BOTTOMUP);
		if(x<0 || y<0 || w<1 || h<1 || (x+w)>_h.width || (y+h)>_h.height)
			throw rrerror("rrframe::tileequals", "Argument out of range");
		if(last && _h.width==last->_h.width && _h.height==last->_h.height
		&& _h.framew==last->_h.framew && _h.frameh==last->_h.frameh
		&& _h.qual==last->_h.qual && _h.subsamp==last->_h.subsamp
		&& _pixelsize==last->_pixelsize && _h.winid==last->_h.winid
		&& _h.dpynum==last->_h.dpynum)
		{
			if(_bits && last->_bits)
			{
				unsigned char *newbits=&_bits[_pitch*(bu? _h.height-y-h:y)+_pixelsize*x];
				unsigned char *oldbits=&last->_bits[last->_pitch*(bu? _h.height-y-h:y)+_pixelsize*x];
				for(int i=0; i<h; i++)
				{
					if(memcmp(&newbits[_pitch*i], &oldbits[last->_pitch*i], _pixelsize*w))
						return false;
				}
			}
			if(_stereo && _rbits && last->_rbits)
			{
				unsigned char *newbits=&_rbits[_pitch*(bu? _h.height-y-h:y)+_pixelsize*x];
				unsigned char *oldbits=&last->_rbits[last->_pitch*(bu? _h.height-y-h:y)+_pixelsize*x];
				for(int i=0; i<h; i++)
				{
					if(memcmp(&newbits[_pitch*i], &oldbits[last->_pitch*i], _pixelsize*w))
						return false;
				}
			}
			return true;
		}
		return false;
	}

	void makeanaglyph(rrframe &r, rrframe &g, rrframe &b)
	{
		int rindex=_flags&RRBMP_BGR? 2:0, gindex=1, bindex=_flags&RRBMP_BGR? 0:2,
			i, j;
		unsigned char *sr=r._bits, *sg=g._bits, *sb=b._bits,
			*d=_bits, *dr, *dg, *db;

		if(_flags&RRBMP_ALPHAFIRST) {rindex++;  gindex++;  bindex++;}

		for(j=0; j<_h.frameh; j++, sr+=r._pitch, sg+=g._pitch, sb+=b._pitch,
			d+=_pitch)
		{
			for(i=0, dr=&d[rindex], dg=&d[gindex],
				db=&d[bindex]; i<_h.framew; i++, dr+=_pixelsize,
				dg+=_pixelsize, db+=_pixelsize)
			{
				*dr=sr[i];  *dg=sg[i];  *db=sb[i];
			}
		}
	}

	void ready(void) {_ready.signal();}
	void waituntilready(void) {_ready.wait();}
	void complete(void) {_complete.signal();}
	void waituntilcomplete(void) {_complete.wait();}

	rrframe& operator= (rrframe& f)
	{
		if(this!=&f && f._bits)
		{
			if(f._bits)
			{
				init(f._h, f._pixelsize, f._flags, f._stereo);
				memcpy(_bits, f._bits, f._h.framew*f._h.frameh*f._pixelsize);
			}
		}
		return *this;
	}

	rrframeheader _h;
	unsigned char *_bits;
	unsigned char *_rbits;
	int _pitch, _pixelsize, _flags;
	bool _isgl, _stereo;

	protected:

	void dumpheader(rrframeheader &h)
	{
		rrout.print("h.size    = %lu\n", h.size);
		rrout.print("h.winid   = 0x%.8x\n", h.winid);
		rrout.print("h.dpynum  = %d\n", h.dpynum);
		rrout.print("h.framew  = %d\n", h.framew);
		rrout.print("h.frameh  = %d\n", h.frameh);
		rrout.print("h.width   = %d\n", h.width);
		rrout.print("h.height  = %d\n", h.height);
		rrout.print("h.x       = %d\n", h.x);
		rrout.print("h.y       = %d\n", h.y);
		rrout.print("h.qual    = %d\n", h.qual);
		rrout.print("h.subsamp = %d\n", h.subsamp);
		rrout.print("h.flags   = %d\n", h.flags);
	}

	void checkheader(rrframeheader &h)
	{
		if(h.flags!=RR_EOF && (h.framew<1 || h.frameh<1 || h.width<1 || h.height<1
		|| h.x+h.width>h.framew || h.y+h.height>h.frameh))
		{
			throw(rrerror("rrframe::checkheader", "Invalid header"));
		}
	}

	#ifdef XDK
	static rrcs _Mutex;
	#endif
	rrevent _ready;
	rrevent _complete;
	friend class rrjpeg;
	bool _primary;
};

// Compressed JPEG

class rrjpeg : public rrframe
{
	public:

	rrjpeg(void) : rrframe(), _tjhnd(NULL)
	{
		if(!(_tjhnd=tjInitCompress())) _throw(tjGetErrorStr());
		_pixelsize=3;
		memset(&_rh, 0, sizeof(rrframeheader));
	}

	~rrjpeg(void)
	{
		if(_tjhnd) tjDestroy(_tjhnd);
	}

	rrjpeg& operator= (rrframe& b)
	{
		int tjflags=0;
		if(!b._bits) _throw("Bitmap not initialized");
		if(b._pixelsize<3 || b._pixelsize>4) _throw("Only true color bitmaps are supported");
		if(b._h.qual>100 || b._h.subsamp>16 || !isPow2(b._h.subsamp))
			throw(rrerror("JPEG compressor", "Invalid argument"));
		init(b._h, b._stereo? RR_LEFT:0);
		if(b._flags&RRBMP_BOTTOMUP) tjflags|=TJ_BOTTOMUP;
		if(b._flags&RRBMP_BGR) tjflags|=TJ_BGR;
		unsigned long size;
		tj(tjCompress(_tjhnd, b._bits, b._h.width, b._pitch, b._h.height, b._pixelsize,
			_bits, &size, jpegsub(b._h.subsamp), b._h.qual, tjflags));
		_h.size=(unsigned int)size;
		if(b._stereo && b._rbits)
		{
			init(b._h, RR_RIGHT);
			if(_rbits)
				tj(tjCompress(_tjhnd, b._rbits, b._h.width, b._pitch, b._h.height,
					b._pixelsize, _rbits, &size, jpegsub(b._h.subsamp), b._h.qual,
					tjflags));
			_rh.size=(unsigned int)size;
		}
		return *this;
	}

	void init(rrframeheader &h, int buffer)
	{
		checkheader(h);
		if(h.flags==RR_EOF) {_h=h;  return;}
		switch(buffer)
		{
			case RR_LEFT:
				if(h.width!=_h.width || h.height!=_h.height || !_bits)
				{
					if(_bits) delete [] _bits;
					newcheck(_bits=new unsigned char[TJBUFSIZE(h.width, h.height)]);
				}
				_h=h;  _h.flags=RR_LEFT;  _stereo=true;
				break;
			case RR_RIGHT:
				if(h.width!=_rh.width || h.height!=_rh.height || !_rbits)
				{
					if(_rbits) delete [] _rbits;
					newcheck(_rbits=new unsigned char[TJBUFSIZE(h.width, h.height)]);
				}
				_rh=h;  _rh.flags=RR_RIGHT;  _stereo=true;
				break;
			default:
				if(h.width!=_h.width || h.height!=_h.height || !_bits)
				{
					if(_bits) delete [] _bits;
					newcheck(_bits=new unsigned char[TJBUFSIZE(h.width, h.height)]);
				}
				_h=h;  _h.flags=0;  _stereo=false;
				break;
		}
		if(!_stereo && _rbits)
		{
			delete [] _rbits;  _rbits=NULL;
			memset(&_rh, 0, sizeof(rrframeheader));
		}
	}

	rrframeheader _rh;

	private:

	tjhandle _tjhnd;
	friend class rrfb;
};

// Bitmap created from shared graphics memory

class rrfb : public rrframe
{
	public:

	rrfb(Display *dpy, Window win) : rrframe()
	{
		if(!dpy || !win) throw(rrerror("rrfb::rrfb", "Invalid argument"));
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		XFlush(dpy);
		init(DisplayString(dpy), win);
	}

	rrfb(char *dpystring, Window win) : rrframe()
	{
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		init(dpystring, win);
	}

	void init(char *dpystring, Window win)
	{
		_tjhnd=NULL;
		memset(&_fb, 0, sizeof(fbx_struct));
		if(!dpystring || !win) throw(rrerror("rrfb::init", "Invalid argument"));
		if(!(_wh.dpy=XOpenDisplay(dpystring)))
			throw(rrerror("rrfb::init", "Could not open display"));
		_wh.win=win;
	}

	~rrfb(void)
	{
		#ifdef XDK
		_Mutex.lock(false);
		#endif
		if(_fb.bits) fbx_term(&_fb);  if(_bits) _bits=NULL;
		if(_tjhnd) tjDestroy(_tjhnd);
		if(_wh.dpy) XCloseDisplay(_wh.dpy);
		#ifdef XDK
		_Mutex.unlock(false);
		#endif
	}

	void init(rrframeheader &h, bool usedbe=true)
	{
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		checkheader(h);
		fbx(fbx_init(&_fb, _wh, h.framew, h.frameh, 1, usedbe?1:0));
		if(h.framew>_fb.width || h.frameh>_fb.height)
		{
			XSync(_wh.dpy, False);
			fbx(fbx_init(&_fb, _wh, h.framew, h.frameh, 1, usedbe?1:0));
		}
		_h=h;
		if(_h.framew>_fb.width) _h.framew=_fb.width;
		if(_h.frameh>_fb.height) _h.frameh=_fb.height;
		_pixelsize=fbx_ps[_fb.format];  _pitch=_fb.pitch;  _bits=(unsigned char *)_fb.bits;
		_flags=0;
		if(fbx_bgr[_fb.format]) _flags|=RRBMP_BGR;
		if(fbx_alphafirst[_fb.format]) _flags|=RRBMP_ALPHAFIRST;
	}

	rrfb& operator= (rrjpeg& f)
	{
		int tjflags=0;
		if(!f._bits || f._h.size<1)
			_throw("JPEG not initialized");
		init(f._h);
		if(!_fb.xi) _throw("Bitmap not initialized");
		if(fbx_bgr[_fb.format]) tjflags|=TJ_BGR;
		if(fbx_alphafirst[_fb.format]) tjflags|=TJ_ALPHAFIRST;
		int width=min(f._h.width, _fb.width-f._h.x);
		int height=min(f._h.height, _fb.height-f._h.y);
		if(width>0 && height>0 && f._h.width<=width && f._h.height<=height)
		{
			if(!_tjhnd)
			{
				if((_tjhnd=tjInitDecompress())==NULL) throw(rrerror("rrfb::decompressor", tjGetErrorStr()));
			}
			tj(tjDecompress(_tjhnd, f._bits, f._h.size, (unsigned char *)&_fb.bits[_fb.pitch*f._h.y+f._h.x*fbx_ps[_fb.format]],
				width, _fb.pitch, height, fbx_ps[_fb.format], tjflags));
		}
		return *this;
	}

	void redraw(void)
	{
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		if(_flags&RRBMP_BOTTOMUP) fbx(fbx_flip(&_fb, 0, 0, 0, 0));
		fbx(fbx_write(&_fb, 0, 0, 0, 0, _fb.width, _fb.height));
	}

	void draw(void)
	{
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		int w=_h.width, h=_h.height;
		XWindowAttributes xwa;
		if(!XGetWindowAttributes(_wh.dpy, _wh.win, &xwa))
		{
			rrout.print("Failed to get window attributes\n");
			return;
		}
		if(_h.x+_h.width>xwa.width || _h.y+_h.height>xwa.height)
		{
			rrout.print("WARNING: bitmap (%d,%d) at (%d,%d) extends beyond window (%d,%d)\n",
				_h.width, _h.height, _h.x, _h.y, xwa.width, xwa.height);
			w=min(_h.width, xwa.width-_h.x);
			h=min(_h.height, xwa.height-_h.y);
		}
		if(_h.x+_h.width<=_fb.width && _h.y+_h.height<=_fb.height)
		fbx(fbx_write(&_fb, _h.x, _h.y, _h.x, _h.y, w, h));
	}

	void drawtile(int x, int y, int w, int h)
	{
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		if(x<0 || w<1 || (x+w)>_h.framew || y<0 || h<1 || (y+h)>_h.frameh)
			return;
		if(_flags&RRBMP_BOTTOMUP) fbx(fbx_flip(&_fb, x, y, w, h));
		fbx(fbx_awrite(&_fb, x, y, x, y, w, h));
	}

	void sync(void)
	{
		#ifdef XDK
		rrcs::safelock l(_Mutex);
		#endif
		fbx(fbx_sync(&_fb));
	}

	private:

	fbx_wh _wh;
	fbx_struct _fb;
	tjhandle _tjhnd;
};

#endif
