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

#ifndef __RRFRAME_H
#define __RRFRAME_H

#include "rr.h"
#include "fbx.h"
#include "rrmutex.h"
#include "rrlog.h"
#include "rrutil.h"
#include <string.h>

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
		if(h.size==0) h.size=h.framew*h.frameh*pixelsize;
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
		_flags=flags;
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
	bool iscomplete(void) {return !_complete.locked();}

	void decompressrgb(rrframe &f, int w, int h, bool righteye)
	{
		if(!f._bits || f._h.size<1 || !_bits || !_h.size)
			_throw("Frame not initialized");
		int dstbgr=((_flags&RRBMP_BGR)!=0), dstbu=((_flags&RRBMP_BOTTOMUP)!=0),
			dstaf=((_flags&RRBMP_ALPHAFIRST)!=0);

		int i, j;
		int srcstride=f._pitch, dststride=_pitch;
		int startline=dstbu? max(0, _h.frameh-f._h.y-h) : f._h.y;
		unsigned char *srcptr=righteye? f._rbits:f._bits,
			*dstptr=righteye? &_rbits[_pitch*startline+f._h.x*_pixelsize]:
				&_bits[_pitch*startline+f._h.x*_pixelsize];
		unsigned char *srcptr2, *dstptr2;
		if(!dstbu)
			{srcptr=&srcptr[(h-1)*f._pitch];  srcstride=-srcstride;}

		if(!dstbgr && _pixelsize==3)
		{
			int wps=w*_pixelsize;
			if(dstaf) {dstptr++;  wps--;}
			for(i=0; i<h; i++, srcptr+=srcstride, dstptr+=dststride)
			{
				memcpy(dstptr, srcptr, wps);
			}
		}
		#ifdef USEMEDIALIB
		else if(dstaf && dstbgr && _pixelsize==4)
		{
			for(i=0; i<h; i++, srcptr+=srcstride, dstptr+=dststride)
			{
				mlib_VideoColorRGBint_to_ABGRint((mlib_u32 *)dstptr, srcptr,
					NULL, 0, w, 1, _pitch, f._pitch, 0);
			}
		}
		else if(dstaf && !dstbgr && _pixelsize==4)
		{
			for(i=0; i<h; i++, srcptr+=srcstride, dstptr+=dststride)
			{
				mlib_VideoColorBGRint_to_ABGRint((mlib_u32 *)dstptr, srcptr,
					NULL, 0, w, 1, _pitch, f._pitch, 0);
			}
		}
		#endif
		else
		{
			if(dstaf) dstptr++;
			if(!dstbgr)
			{
 				for(i=0; i<h; i++, srcptr+=srcstride, dstptr+=dststride)
				{
					for(j=0, srcptr2=srcptr, dstptr2=dstptr; j<w; j++,
						srcptr2+=f._pixelsize, dstptr2+=_pixelsize)
					{
						memcpy(dstptr2, srcptr2, 3);
				 	}
				}
		 	}
			else
			{
				for(i=0; i<h; i++, srcptr+=srcstride, dstptr+=dststride)
				{
					for(j=0, srcptr2=srcptr, dstptr2=dstptr; j<w; j++,
						srcptr2+=f._pixelsize, dstptr2+=_pixelsize)
					{
						dstptr2[2]=srcptr2[0];
						dstptr2[1]=srcptr2[1];
						dstptr2[0]=srcptr2[2];
				 	}
				}
			}
		}

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
		rrout.print("h.compress= %d\n", h.compress);
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

	rrevent _ready;
	rrevent _complete;
	bool _primary;
};


// Bitmap created from shared graphics memory

class rrfb : public rrframe
{
	public:

	rrfb(HWND hwnd) : rrframe()
	{
		if(!hwnd) throw(rrerror("rrfb::rrfb", "Invalid argument"));
		_wh=hwnd;
		memset(&_fb, 0, sizeof(fbx_struct));
	}

	~rrfb(void)
	{
		if(_fb.bits) fbx_term(&_fb);  if(_bits) _bits=NULL;
	}

	void init(rrframeheader &h)
	{
		checkheader(h);
		fbx(fbx_init(&_fb, _wh, h.framew, h.frameh, 1));
		_h=h;
		if(_h.framew>_fb.width) _h.framew=_fb.width;
		if(_h.frameh>_fb.height) _h.frameh=_fb.height;
		_pixelsize=fbx_ps[_fb.format];  _pitch=_fb.pitch;  _bits=(unsigned char *)_fb.bits;
		_flags=0;
		if(fbx_bgr[_fb.format]) _flags|=RRBMP_BGR;
		if(fbx_alphafirst[_fb.format]) _flags|=RRBMP_ALPHAFIRST;
	}

	void redraw(void)
	{
		if(_flags&RRBMP_BOTTOMUP) fbx(fbx_flip(&_fb, 0, 0, 0, 0));
		fbx(fbx_write(&_fb, 0, 0, 0, 0, _fb.width, _fb.height));
	}

	private:

	HWND _wh;
	fbx_struct _fb;
};

#endif
