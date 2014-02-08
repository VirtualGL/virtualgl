/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005-2007 Sun Microsystems, Inc.
 * Copyright (C)2009-2012, 2014 D. R. Commander
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

#ifndef __RRFRAME_H__
#define __RRFRAME_H__

#include "rr.h"
#ifdef _WIN32
#define FBXX11
#endif
#include "fbx.h"
#include "turbojpeg.h"
#include "Mutex.h"
#ifdef USEXV
#include "fbxv.h"
#endif

using namespace vglutil;


// Flags
#define RRFRAME_BOTTOMUP   1  // Bottom-up bitmap (as opposed to top-down)
#define RRFRAME_BGR        2  // BGR or BGRA pixel order
#define RRFRAME_ALPHAFIRST 4  // BGR buffer is really ABGR, and RGB buffer is
                              // really ARGB


// Uncompressed frame

class rrframe
{
	public:

		rrframe(bool primary=true);
		virtual ~rrframe(void);
		void init(rrframeheader &, int, int, bool stereo=false);
		void init(unsigned char *, int, int, int, int, int);
		void deinit(void);
		rrframe *gettile(int, int, int, int);
		bool tileequals(rrframe *, int, int, int, int);
		void makeanaglyph(rrframe &, rrframe &, rrframe &);
		void makepassive(rrframe &, int);
		void ready(void) { _ready.signal(); }
		void waituntilready(void) { _ready.wait(); }
		void complete(void) { _complete.signal(); }
		void waituntilcomplete(void) { _complete.wait(); }
		bool iscomplete(void) { return !_complete.isLocked(); }
		void decompressrgb(rrframe &, int, int, bool);
		void addlogo(void);

		rrframeheader _h;
		unsigned char *_bits;
		unsigned char *_rbits;
		int _pitch, _pixelsize, _flags;
		bool _isgl, _isxv, _stereo;

	protected:

		void dumpheader(rrframeheader &);
		void checkheader(rrframeheader &);

		#ifdef XDK
		static CS _Mutex;
		#endif
		Event _ready;
		Event _complete;
		friend class rrcompframe;
		bool _primary;
};


// Compressed frame

class rrcompframe : public rrframe
{
	public:

		rrcompframe(void);
		~rrcompframe(void);
		rrcompframe& operator= (rrframe &);
		void compressyuv(rrframe &);
		void compressjpeg(rrframe &);
		void compressrgb(rrframe &);
		void init(rrframeheader &, int);

		rrframeheader _rh;

	private:

		tjhandle _tjhnd;
		friend class rrfb;
};


// Frame created from shared graphics memory

class rrfb : public rrframe
{
	public:

		rrfb(Display *dpy, Drawable d, Visual *v=NULL);
		rrfb(char *, Window);
		void init(char *, Drawable, Visual *v=NULL);
		~rrfb(void);
		void init(rrframeheader &);
		rrfb& operator= (rrcompframe &);
		void redraw(void);

	private:

		fbx_wh _wh;
		fbx_struct _fb;
		tjhandle _tjhnd;
};


#ifdef USEXV

// Frame created using X Video

class rrxvframe : public rrframe
{
	public:

		rrxvframe(Display *, Window);
		rrxvframe(char *, Window);
		void init(char *, Window);
		~rrxvframe(void);
		rrxvframe &operator= (rrframe &b);
		void init(rrframeheader &);
		void redraw(void);

	private:

		fbxv_struct _fb;
		Display *_dpy;  Window _win;
		tjhandle _tjhnd;
};

#endif


#endif // __RRFRAME_H__
