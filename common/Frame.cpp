// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005-2007 Sun Microsystems, Inc.
// Copyright (C)2009-2012, 2014, 2017-2021 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#include "Log.h"
#include "Error.h"
#include "vglutil.h"
#include <string.h>
#include "vgllogo.h"
#include "Frame.h"

using namespace util;
using namespace common;


#define TJSUBSAMP(s) \
	(s >= 4 ? TJ_420 : \
		(s == 2 ? TJ_422 : \
			(s == 1 ? TJ_444 : \
				(s == 0 ? TJ_GRAYSCALE : TJ_444))))

static int tjpf[PIXELFORMATS] =
{
	TJPF_RGB, TJPF_RGBX, -1, TJPF_BGR, TJPF_BGRX, -1, TJPF_XBGR, -1, TJPF_XRGB,
	-1, TJPF_GRAY
};


// Uncompressed frame

Frame::Frame(bool primary_) : bits(NULL), rbits(NULL), pitch(0), flags(0),
	pf(pf_get(-1)), isGL(false), isXV(false), stereo(false), primary(primary_)
{
	memset(&hdr, 0, sizeof(rrframeheader));
	ready.wait();
}


Frame::~Frame(void)
{
	deInit();
}


void Frame::deInit(void)
{
	if(primary)
	{
		delete [] bits;  bits = NULL;
		delete [] rbits;  rbits = NULL;
	}
}


void Frame::init(rrframeheader &h, int pixelFormat, int flags_, bool stereo_)
{
	if(pixelFormat < 0 || pixelFormat >= PIXELFORMATS)
		throw(Error("Frame::init", "Invalid argument"));

	flags = flags_;
	PF *newpf = pf_get(pixelFormat);
	if(h.size == 0) h.size = h.framew * h.frameh * newpf->size;
	checkHeader(h);
	if(h.framew != hdr.framew || h.frameh != hdr.frameh
		|| newpf->size != pf->size || !bits)
	{
		delete [] bits;
		bits = new unsigned char[h.framew * h.frameh * newpf->size + 1];
	}
	if(stereo_)
	{
		if(h.framew != hdr.framew || h.frameh != hdr.frameh
			|| newpf->size != pf->size || !rbits)
		{
			delete [] rbits;
			rbits = new unsigned char[h.framew * h.frameh * newpf->size + 1];
		}
	}
	else
	{
		delete [] rbits;  rbits = NULL;
	}
	pf = newpf;  pitch = pf->size * h.framew;  stereo = stereo_;  hdr = h;
}


void Frame::init(unsigned char *bits_, int width, int pitch_, int height,
	int pixelFormat, int flags_)
{
	if(!bits_ || width < 1 || pitch_ < 1 || height < 1 || pixelFormat < 0
		|| pixelFormat >= PIXELFORMATS)
		THROW("Invalid argument");

	bits = bits_;
	hdr.x = hdr.y = 0;
	hdr.framew = hdr.width = width;
	hdr.frameh = hdr.height = height;
	pf = pf_get(pixelFormat);
	hdr.size = hdr.framew * hdr.frameh * pf->size;
	checkHeader(hdr);
	pitch = pitch_;
	flags = flags_;
	primary = false;
}


Frame *Frame::getTile(int x, int y, int width, int height)
{
	Frame *f;

	if(!bits || !pitch || !pf->size) THROW("Frame not initialized");
	if(x < 0 || y < 0 || width < 1 || height < 1 || (x + width) > hdr.width
		|| (y + height) > hdr.height)
		throw Error("Frame::getTile", "Argument out of range");

	f = new Frame(false);
	f->hdr = hdr;
	f->hdr.x = x;
	f->hdr.y = y;
	f->hdr.width = width;
	f->hdr.height = height;
	f->pf = pf;
	f->flags = flags;
	f->pitch = pitch;
	f->stereo = stereo;
	f->isGL = isGL;
	bool bu = (flags & FRAME_BOTTOMUP);
	f->bits = &bits[pitch * (bu ? hdr.height - y - height : y) + pf->size * x];
	if(stereo && rbits)
		f->rbits =
			&rbits[pitch * (bu ? hdr.height - y - height : y) + pf->size * x];
	return f;
}


bool Frame::tileEquals(Frame *last, int x, int y, int width, int height)
{
	bool bu = (flags & FRAME_BOTTOMUP);

	if(x < 0 || y < 0 || width < 1 || height < 1 || (x + width) > hdr.width
		|| (y + height) > hdr.height)
		throw Error("Frame::tileEquals", "Argument out of range");

	if(last && hdr.width == last->hdr.width && hdr.height == last->hdr.height
		&& hdr.framew == last->hdr.framew && hdr.frameh == last->hdr.frameh
		&& hdr.qual == last->hdr.qual && hdr.subsamp == last->hdr.subsamp
		&& pf->id == last->pf->id && pf->size == last->pf->size
		&& hdr.winid == last->hdr.winid && hdr.dpynum == last->hdr.dpynum)
	{
		if(bits && last->bits)
		{
			unsigned char *newBits =
				&bits[pitch * (bu ? hdr.height - y - height : y) + pf->size * x];
			unsigned char *oldBits =
				&last->bits[last->pitch * (bu ? hdr.height - y - height : y) +
					pf->size * x];
			for(int i = 0; i < height; i++)
			{
				if(memcmp(&newBits[pitch * i], &oldBits[last->pitch * i],
					pf->size * width))
					return false;
			}
		}
		if(stereo && rbits && last->rbits)
		{
			unsigned char *newBits =
				&rbits[pitch * (bu ? hdr.height - y - height : y) + pf->size * x];
			unsigned char *oldBits =
				&last->rbits[last->pitch * (bu ? hdr.height - y - height : y) +
					pf->size * x];
			for(int i = 0; i < height; i++)
			{
				if(memcmp(&newBits[pitch * i], &oldBits[last->pitch * i],
					pf->size * width))
					return false;
			}
		}
		return true;
	}
	return false;
}


void Frame::makeAnaglyph(Frame &r, Frame &g, Frame &b)
{
	int i, j;
	unsigned char *srcrptr = r.bits, *srcgptr = g.bits, *srcbptr = b.bits,
		*dstptr = bits, *dstrptr, *dstgptr, *dstbptr;

	if(pf->bpc != 8) THROW("Anaglyphic stereo requires 8 bits per component");

	for(j = 0; j < hdr.frameh; j++, srcrptr += r.pitch, srcgptr += g.pitch,
		srcbptr += b.pitch, dstptr += pitch)
	{
		for(i = 0, dstrptr = &dstptr[pf->rindex], dstgptr = &dstptr[pf->gindex],
			dstbptr = &dstptr[pf->bindex];
			i < hdr.framew;
			i++, dstrptr += pf->size, dstgptr += pf->size, dstbptr += pf->size)
		{
			*dstrptr = srcrptr[i];  *dstgptr = srcgptr[i];  *dstbptr = srcbptr[i];
		}
	}
}


void Frame::makePassive(Frame &stf, int mode)
{
	unsigned char *srclptr = stf.bits, *srcrptr = stf.rbits;
	unsigned char *dstptr = bits;

	if(hdr.framew != stf.hdr.framew || hdr.frameh != stf.hdr.frameh
		|| pitch != stf.pitch)
		THROW("Frames are not the same size");

	if(mode == RRSTEREO_INTERLEAVED)
	{
		int rowSize = pf->size * hdr.framew;
		for(int j = 0; j < hdr.frameh; j++)
		{
			if(j % 2 == 0) memcpy(dstptr, srclptr, rowSize);
			else memcpy(dstptr, srcrptr, rowSize);
			srclptr += pitch;  srcrptr += pitch;  dstptr += pitch;
		}
	}
	else if(mode == RRSTEREO_TOPBOTTOM)
	{
		int rowSize = pf->size * hdr.framew;
		srcrptr += pitch;
		for(int j = 0; j < (hdr.frameh + 1) / 2; j++)
		{
			memcpy(dstptr, srclptr, rowSize);
			srclptr += pitch * 2;  dstptr += pitch;
		}
		for(int j = (hdr.frameh + 1) / 2; j < hdr.frameh; j++)
		{
			memcpy(dstptr, srcrptr, rowSize);
			srcrptr += pitch * 2;  dstptr += pitch;
		}
	}
	else if(mode == RRSTEREO_SIDEBYSIDE)
	{
		int pad = pitch - hdr.framew * pf->size, h = hdr.frameh;
		while(h > 0)
		{
			unsigned char *srclptr2 = srclptr;
			unsigned char *srcrptr2 = srcrptr + pf->size;
			for(int i = 0; i < (hdr.framew + 1) / 2; i++)
			{
				*(int *)dstptr = *(int *)srclptr2;
				srclptr2 += pf->size * 2;  dstptr += pf->size;
			}
			for(int i = (hdr.framew + 1) / 2; i < hdr.framew - 1; i++)
			{
				*(int *)dstptr = *(int *)srcrptr2;
				srcrptr2 += pf->size * 2;  dstptr += pf->size;
			}
			if(hdr.framew > 1)
			{
				memcpy(dstptr, srcrptr2, pf->size);
				dstptr += pf->size;
			}
			srclptr += pitch;  srcrptr += pitch;  dstptr += pad;
			h--;
		}
	}
}


void Frame::decompressRGB(Frame &f, int width, int height, bool rightEye)
{
	if(!f.bits || f.hdr.size < 1 || !bits || !hdr.size)
		THROW("Frame not initialized");
	if(pf->bpc < 8)
		throw(Error("RGB decompressor",
			"Destination frame has the wrong pixel format"));

	bool dstbu = (flags & FRAME_BOTTOMUP);
	int srcStride = f.pitch, dstStride = pitch;
	int startLine = dstbu ? max(0, hdr.frameh - f.hdr.y - height) : f.hdr.y;
	unsigned char *srcptr = rightEye ? f.rbits : f.bits,
		*dstptr = rightEye ? &rbits[pitch * startLine + f.hdr.x * pf->size] :
			&bits[pitch * startLine + f.hdr.x * pf->size];

	if(!dstbu)
	{
		srcptr = &srcptr[(height - 1) * f.pitch];  srcStride = -srcStride;
	}
	pf_get(PF_RGB)->convert(srcptr, width, srcStride, height, dstptr, dstStride,
		pf);
}


#define DRAWLOGO() \
	switch(pf->size) \
	{ \
		case 3: \
		{ \
			for(int j = 0; j < height; j++, rowptr += stride) \
			{ \
				unsigned char *pixel = rowptr; \
				logoptr2 = logoptr; \
				for(int i = 0; i < width; i++, pixel += pf->size) \
				{ \
					if(*(logoptr2++)) \
					{ \
						pixel[pf->rindex] ^= 113;  pixel[pf->gindex] ^= 162; \
						pixel[pf->bindex] ^= 117; \
					} \
				} \
				logoptr += VGLLOGO_WIDTH; \
			} \
			break; \
		} \
		case 4: \
		{ \
			unsigned int mask; \
			pf->setRGB((unsigned char *)&mask, 113, 162, 117); \
			for(int j = 0; j < height; j++, rowptr += stride) \
			{ \
				unsigned int *pixel = (unsigned int *)rowptr; \
				logoptr2 = logoptr; \
				for(int i = 0; i < width; i++, pixel++) \
				{ \
					if(*(logoptr2++)) *pixel ^= mask; \
				} \
				logoptr += VGLLOGO_WIDTH; \
			} \
			break; \
		} \
		default: \
			THROW("Invalid pixel format"); \
	}


void Frame::addLogo(void)
{
	unsigned char *rowptr, *logoptr = vgllogo, *logoptr2;

	if(!bits || hdr.width < 1 || hdr.height < 1) return;

	int height = min(VGLLOGO_HEIGHT, hdr.height - 1);
	int width = min(VGLLOGO_WIDTH, hdr.width - 1);
	int stride = flags & FRAME_BOTTOMUP ? -pitch : pitch;
	if(height < 1 || width < 1) return;
	if(flags & FRAME_BOTTOMUP)
		rowptr = &bits[pitch * height + (hdr.width - width - 1) * pf->size];
	else
		rowptr = &bits[pitch * (hdr.height - height - 1) +
			(hdr.width - width - 1) * pf->size];
	DRAWLOGO()

	if(!rbits) return;
	logoptr = vgllogo;
	if(flags & FRAME_BOTTOMUP)
		rowptr = &rbits[pitch * height + (hdr.width - width - 1) * pf->size];
	else
		rowptr = &rbits[pitch * (hdr.height - height - 1) +
			(hdr.width - width - 1) * pf->size];
	logoptr = vgllogo;
	DRAWLOGO()
}


void Frame::dumpHeader(rrframeheader &h)
{
	vglout.print("hdr.size    = %lu\n", h.size);
	vglout.print("hdr.winid   = 0x%.8x\n", h.winid);
	vglout.print("hdr.dpynum  = %d\n", h.dpynum);
	vglout.print("hdr.compress= %d\n", h.compress);
	vglout.print("hdr.framew  = %d\n", h.framew);
	vglout.print("hdr.frameh  = %d\n", h.frameh);
	vglout.print("hdr.width   = %d\n", h.width);
	vglout.print("hdr.height  = %d\n", h.height);
	vglout.print("hdr.x       = %d\n", h.x);
	vglout.print("hdr.y       = %d\n", h.y);
	vglout.print("hdr.qual    = %d\n", h.qual);
	vglout.print("hdr.subsamp = %d\n", h.subsamp);
	vglout.print("hdr.flags   = %d\n", h.flags);
}


void Frame::checkHeader(rrframeheader &h)
{
	if(h.flags != RR_EOF && (h.framew < 1 || h.frameh < 1 || h.width < 1
		|| h.height < 1 || h.x + h.width > h.framew || h.y + h.height > h.frameh))
	{
		throw(Error("Frame::checkHeader", "Invalid header"));
	}
}


// Compressed frame

CompressedFrame::CompressedFrame(void) : Frame(), tjhnd(NULL)
{
	if(!(tjhnd = tjInitCompress())) THROW(tjGetErrorStr());
	pf = pf_get(PF_RGB);
	memset(&rhdr, 0, sizeof(rrframeheader));
}


CompressedFrame::~CompressedFrame(void)
{
	if(tjhnd) tjDestroy(tjhnd);
}

CompressedFrame &CompressedFrame::operator= (Frame &f)
{
	if(!f.bits) THROW("Frame not initialized");
	if(f.pf->size < 3 || f.pf->size > 4)
		THROW("Only true color frames are supported");

	switch(f.hdr.compress)
	{
		case RRCOMP_RGB:  compressRGB(f);  break;
		case RRCOMP_JPEG:  compressJPEG(f);  break;
		case RRCOMP_YUV:  compressYUV(f);  break;
		default:  THROW("Invalid compression type");
	}
	return *this;
}


void CompressedFrame::compressYUV(Frame &f)
{
	int tjflags = 0;

	if(f.hdr.subsamp != 4) throw(Error("YUV encoder", "Invalid argument"));
	if(f.pf->bpc != 8)
		throw(Error("YUV encoder", "YUV encoding requires 8 bits per component"));

	init(f.hdr, 0);
	if(f.flags & FRAME_BOTTOMUP) tjflags |= TJ_BOTTOMUP;
	TRY_TJ(tjEncodeYUV2(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		tjpf[f.pf->id], bits, TJSUBSAMP(f.hdr.subsamp), tjflags));
	hdr.size = (unsigned int)tjBufSizeYUV(f.hdr.width, f.hdr.height,
		TJSUBSAMP(f.hdr.subsamp));
}


void CompressedFrame::compressJPEG(Frame &f)
{
	int tjflags = 0;

	if(f.hdr.qual > 100 || f.hdr.subsamp > 16 || !IS_POW2(f.hdr.subsamp))
		throw(Error("JPEG compressor", "Invalid argument"));
	if(f.pf->bpc != 8)
		throw(Error("JPEG compressor",
			"JPEG compression requires 8 bits per component"));

	init(f.hdr, f.stereo ? RR_LEFT : 0);
	if(f.flags & FRAME_BOTTOMUP) tjflags |= TJ_BOTTOMUP;
	unsigned long size;
	TRY_TJ(tjCompress2(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		tjpf[f.pf->id], &bits, &size, TJSUBSAMP(f.hdr.subsamp), f.hdr.qual,
		tjflags | TJFLAG_NOREALLOC));
	hdr.size = (unsigned int)size;
	if(f.stereo && f.rbits)
	{
		init(f.hdr, RR_RIGHT);
		if(rbits)
			TRY_TJ(tjCompress2(tjhnd, f.rbits, f.hdr.width, f.pitch, f.hdr.height,
				tjpf[f.pf->id], &rbits, &size, TJSUBSAMP(f.hdr.subsamp), f.hdr.qual,
				tjflags | TJFLAG_NOREALLOC));
		rhdr.size = (unsigned int)size;
	}
}


void CompressedFrame::compressRGB(Frame &f)
{
	unsigned char *srcptr;
	bool bu = (f.flags & FRAME_BOTTOMUP);

	if(f.pf->bpc != 8)
		throw(Error("RGB compressor",
			"RGB encoding requires 8 bits per component"));

	int dstPitch = f.hdr.width * 3;
	int srcStride = bu ? f.pitch : -f.pitch;
	init(f.hdr, f.stereo ? RR_LEFT : 0);
	srcptr = bu ? f.bits : &f.bits[f.pitch * (f.hdr.height - 1)];
	f.pf->convert(srcptr, f.hdr.width, srcStride, f.hdr.height, bits, dstPitch,
		pf_get(PF_RGB));
	hdr.size = dstPitch * f.hdr.height;

	if(f.stereo && f.rbits)
	{
		init(f.hdr, RR_RIGHT);
		if(rbits)
		{
			srcptr = bu ? f.rbits : &f.rbits[f.pitch * (f.hdr.height - 1)];
			f.pf->convert(srcptr, f.hdr.width, srcStride, f.hdr.height, rbits,
				dstPitch, pf_get(PF_RGB));
			rhdr.size = dstPitch * f.hdr.height;
		}
	}
}


void CompressedFrame::init(rrframeheader &h, int buffer)
{
	checkHeader(h);
	if(h.flags == RR_EOF) { hdr = h;  return; }
	switch(buffer)
	{
		case RR_LEFT:
			if(h.width != hdr.width || h.height != hdr.height || !bits)
			{
				delete [] bits;
				bits = new unsigned char[tjBufSize(h.width, h.height, h.subsamp)];
			}
			hdr = h;  hdr.flags = RR_LEFT;  stereo = true;
			break;
		case RR_RIGHT:
			if(h.width != rhdr.width || h.height != rhdr.height || !rbits)
			{
				delete [] rbits;
				rbits = new unsigned char[tjBufSize(h.width, h.height, h.subsamp)];
			}
			rhdr = h;  rhdr.flags = RR_RIGHT;  stereo = true;
			break;
		default:
			if(h.width != hdr.width || h.height != hdr.height || !bits)
			{
				delete [] bits;
				bits = new unsigned char[tjBufSize(h.width, h.height, h.subsamp)];
			}
			hdr = h;  hdr.flags = 0;  stereo = false;
			break;
	}
	if(!stereo && rbits)
	{
		delete [] rbits;  rbits = NULL;
		memset(&rhdr, 0, sizeof(rrframeheader));
	}
	pitch = hdr.width * pf->size;
}


// Frame created from shared graphics memory

CriticalSection FBXFrame::mutex;


FBXFrame::FBXFrame(Display *dpy, Drawable draw, Visual *vis,
	bool reuseConn_) : Frame()
{
	if(!dpy || !draw) throw(Error("FBXFrame::FBXFrame", "Invalid argument"));

	XFlush(dpy);
	if(reuseConn_) init(dpy, draw, vis);
	else init(DisplayString(dpy), draw, vis);
}


FBXFrame::FBXFrame(char *dpystring, Window win) : Frame()
{
	init(dpystring, win);
}


void FBXFrame::init(char *dpystring, Drawable draw, Visual *vis)
{
	tjhnd = NULL;  reuseConn = false;
	memset(&fb, 0, sizeof(fbx_struct));

	if(!dpystring || !draw) throw(Error("FBXFrame::init", "Invalid argument"));
	CriticalSection::SafeLock l(mutex);
	if(!(wh.dpy = XOpenDisplay(dpystring)))
		throw(Error("FBXFrame::init", "Could not open display"));

	wh.d = draw;  wh.v = vis;
}


void FBXFrame::init(Display *dpy, Drawable draw, Visual *vis)
{
	tjhnd = NULL;  reuseConn = true;
	memset(&fb, 0, sizeof(fbx_struct));

	if(!dpy || !draw) throw(Error("FBXFrame::init", "Invalid argument"));

	wh.dpy = dpy;
	wh.d = draw;  wh.v = vis;
}


FBXFrame::~FBXFrame(void)
{
	if(fb.bits) fbx_term(&fb);
	if(bits) bits = NULL;
	if(tjhnd) tjDestroy(tjhnd);
	if(wh.dpy && !reuseConn) XCloseDisplay(wh.dpy);
}


void FBXFrame::init(rrframeheader &h)
{
	checkHeader(h);
	int usexshm = 1;  char *env = NULL;
	if((env = getenv("VGL_USEXSHM")) != NULL && strlen(env) > 0
		&& !strcmp(env, "0"))
		usexshm = 0;
	{
		CriticalSection::SafeLock l(mutex);
		TRY_FBX(fbx_init(&fb, wh, h.framew, h.frameh, usexshm));
	}
	if(h.framew > fb.width || h.frameh > fb.height)
	{
		XSync(wh.dpy, False);
		CriticalSection::SafeLock l(mutex);
		TRY_FBX(fbx_init(&fb, wh, h.framew, h.frameh, usexshm));
	}
	hdr = h;
	if(hdr.framew > fb.width) hdr.framew = fb.width;
	if(hdr.frameh > fb.height) hdr.frameh = fb.height;
	pf = fb.pf;  pitch = fb.pitch;
	bits = (unsigned char *)fb.bits;
	flags = 0;
}


FBXFrame &FBXFrame::operator= (CompressedFrame &cf)
{
	int tjflags = 0;

	if(!cf.bits || cf.hdr.size < 1)
		THROW("JPEG not initialized");
	init(cf.hdr);
	if(!fb.xi) THROW("Frame not initialized");

	int width = min(cf.hdr.width, fb.width - cf.hdr.x);
	int height = min(cf.hdr.height, fb.height - cf.hdr.y);
	if(width > 0 && height > 0 && cf.hdr.width <= width
		&& cf.hdr.height <= height)
	{
		if(cf.hdr.compress == RRCOMP_RGB) decompressRGB(cf, width, height, false);
		else
		{
			if(pf->bpc != 8)
				throw(Error("JPEG decompressor",
					"JPEG decompression requires 8 bits per component"));
			if(!tjhnd)
			{
				if((tjhnd = tjInitDecompress()) == NULL)
					throw(Error("FBXFrame::decompressor", tjGetErrorStr()));
			}
			TRY_TJ(tjDecompress2(tjhnd, cf.bits, cf.hdr.size,
				(unsigned char *)&fb.bits[fb.pitch * cf.hdr.y + cf.hdr.x * pf->size],
				width, fb.pitch, height, tjpf[pf->id], tjflags));
		}
	}
	return *this;
}


void FBXFrame::redraw(void)
{
	if(flags & FRAME_BOTTOMUP) TRY_FBX(fbx_flip(&fb, 0, 0, 0, 0));
	TRY_FBX(fbx_write(&fb, 0, 0, 0, 0, fb.width, fb.height));
}


#ifdef USEXV

// Frame created using X Video

CriticalSection XVFrame::mutex;


XVFrame::XVFrame(Display *dpy_, Window win_) : Frame()
{
	if(!dpy_ || !win_) throw(Error("XVFrame::XVFrame", "Invalid argument"));

	XFlush(dpy_);
	init(DisplayString(dpy_), win_);
}


XVFrame::XVFrame(char *dpystring, Window win_) : Frame()
{
	init(dpystring, win_);
}


void XVFrame::init(char *dpystring, Window win_)
{
	tjhnd = NULL;  isXV = true;
	memset(&fb, 0, sizeof(fbxv_struct));

	if(!dpystring || !win_) throw(Error("XVFrame::init", "Invalid argument"));
	CriticalSection::SafeLock l(mutex);
	if(!(dpy = XOpenDisplay(dpystring)))
		throw(Error("XVFrame::init", "Could not open display"));

	win = win_;
}


XVFrame::~XVFrame(void)
{
	fbxv_term(&fb);
	if(bits) bits = NULL;
	if(tjhnd) tjDestroy(tjhnd);
	if(dpy) XCloseDisplay(dpy);
}


XVFrame &XVFrame::operator= (Frame &f)
{
	if(!f.bits) THROW("Frame not initialized");
	if(f.pf->bpc != 8)
		throw(Error("YUV encoder", "YUV encoding requires 8 bits per component"));

	int tjflags = 0;
	init(f.hdr);
	if(f.flags & FRAME_BOTTOMUP) tjflags |= TJ_BOTTOMUP;
	if(!tjhnd)
	{
		if((tjhnd = tjInitCompress()) == NULL)
			throw(Error("XVFrame::compressor", tjGetErrorStr()));
	}
	TRY_TJ(tjEncodeYUV2(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		tjpf[f.pf->id], bits, TJ_420, tjflags));
	hdr.size = (unsigned int)tjBufSizeYUV(f.hdr.width, f.hdr.height, TJ_420);
	if(hdr.size != (unsigned long)fb.xvi->data_size)
		THROW("Image size mismatch in YUV encoder");
	return *this;
}


void XVFrame::init(rrframeheader &h)
{
	checkHeader(h);
	{
		CriticalSection::SafeLock l(mutex);
		TRY_FBXV(fbxv_init(&fb, dpy, win, h.framew, h.frameh, I420_PLANAR, 0));
	}
	if(h.framew > fb.xvi->width || h.frameh > fb.xvi->height)
	{
		XSync(dpy, False);
		CriticalSection::SafeLock l(mutex);
		TRY_FBXV(fbxv_init(&fb, dpy, win, h.framew, h.frameh, I420_PLANAR, 0));
	}
	hdr = h;
	if(hdr.framew > fb.xvi->width) hdr.framew = fb.xvi->width;
	if(hdr.frameh > fb.xvi->height) hdr.frameh = fb.xvi->height;
	bits = (unsigned char *)fb.xvi->data;
	flags = pitch = 0;
	hdr.size = fb.xvi->data_size;
}


void XVFrame::redraw(void)
{
	TRY_FBXV(fbxv_write(&fb, 0, 0, 0, 0, 0, 0, hdr.framew, hdr.frameh));
}

#endif
