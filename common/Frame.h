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

#ifndef __FRAME_H__
#define __FRAME_H__

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


// Flags
#define FRAME_BOTTOMUP   1  // Bottom-up bitmap (as opposed to top-down)
#define FRAME_BGR        2  // BGR or BGRA pixel order
#define FRAME_ALPHAFIRST 4  // BGR buffer is really ABGR, and RGB buffer is
                            // really ARGB


// Uncompressed frame

namespace vglcommon
{
	using namespace vglutil;

	class Frame
	{
		public:
			Frame(bool primary=true);
			virtual ~Frame(void);
			void init(rrframeheader &h, int pixelSize, int flags, bool stereo=false);
			void init(unsigned char *bits, int w, int pitch, int h, int pixelSize,
				int flags);
			void deInit(void);
			Frame *getTile(int x, int y, int w, int h);
			bool tileEquals(Frame *last, int x, int y, int w, int h);
			void makeAnaglyph(Frame &r, Frame &g, Frame &b);
			void makePassive(Frame &stf, int mode);
			void signalReady(void) { ready.signal(); }
			void waitUntilReady(void) { ready.wait(); }
			void signalComplete(void) { complete.signal(); }
			void waitUntilComplete(void) { complete.wait(); }
			bool isComplete(void) { return !complete.isLocked(); }
			void decompressRGB(Frame &f, int w, int h, bool rightEye);
			void addLogo(void);

			rrframeheader hdr;
			unsigned char *bits;
			unsigned char *rbits;
			int pitch, pixelSize, flags;
			bool isGL, isXV, stereo;

		protected:
			void dumpHeader(rrframeheader &);
			void checkHeader(rrframeheader &);

			#ifdef XDK
			static CS mutex;
			#endif
			Event ready;
			Event complete;
			friend class CompressedFrame;
			bool primary;
	};
}


// Compressed frame

namespace vglcommon
{
	class CompressedFrame : public Frame
	{
		public:
			CompressedFrame(void);
			~CompressedFrame(void);
			CompressedFrame& operator= (Frame &f);
			void compressYUV(Frame &f);
			void compressJPEG(Frame &f);
			void compressRGB(Frame &f);
			void init(rrframeheader &h, int buffer);

			rrframeheader rhdr;

		private:
			tjhandle tjhnd;
			friend class FBXFrame;
	};
}


// Frame created from shared graphics memory

namespace vglcommon
{
	class FBXFrame : public Frame
	{
		public:
			FBXFrame(Display *dpy, Drawable d, Visual *v=NULL);
			FBXFrame(char *dpystring, Window win);
			void init(char *dpystring, Drawable d, Visual *v=NULL);
			~FBXFrame(void);
			void init(rrframeheader &h);
			FBXFrame& operator= (CompressedFrame &cf);
			void redraw(void);

		private:
			fbx_wh wh;
			fbx_struct fb;
			tjhandle tjhnd;
	};
}


#ifdef USEXV

// Frame created using X Video

namespace vglcommon
{
	class XVFrame : public Frame
	{
		public:
			XVFrame(Display *dpy, Window win);
			XVFrame(char *dpystring, Window win);
			void init(char *dpystring, Window win);
			~XVFrame(void);
			XVFrame &operator= (Frame &f);
			void init(rrframeheader &h);
			void redraw(void);

		private:
			fbxv_struct fb;
			Display *dpy;  Window win;
			tjhandle tjhnd;
	};
}

#endif


#endif // __FRAME_H__
