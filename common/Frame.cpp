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

#include "Log.h"
#include "Error.h"
#include "vglutil.h"
#include <string.h>
#include "vgllogo.h"
#include "Frame.h"

using namespace vglutil;
using namespace vglcommon;


#define TJSUBSAMP(s) \
	(s>=4? TJ_420 : s==2? TJ_422 : s==1? TJ_444 : s==0? TJ_GRAYSCALE : TJ_444)


// Uncompressed frame

Frame::Frame(bool primary_) : bits(NULL), rbits(NULL), pitch(0),
	pixelSize(0), flags(0), isGL(false), isXV(false), stereo(false),
	primary(primary_)
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
	if(bits && primary)
	{
		delete [] bits;  bits=NULL;
	}
	if(rbits && primary)
	{
		delete [] rbits;  rbits=NULL;
	}
}


void Frame::init(rrframeheader &h, int ps, int flags_, bool stereo_)
{
	if(ps<1) throw(Error("Frame::init", "Invalid argument"));

	flags=flags_;
	if(h.size==0) h.size=h.framew*h.frameh*ps;
	checkHeader(h);
	if(h.framew!=hdr.framew || h.frameh!=hdr.frameh || ps!=pixelSize || !bits)
	{
		if(bits) delete [] bits;
		_newcheck(bits=new unsigned char[h.framew*h.frameh*ps+1]);
	}
	if(stereo_)
	{
		if(h.framew!=hdr.framew || h.frameh!=hdr.frameh || ps!=pixelSize
			|| !rbits)
		{
			if(rbits) delete [] rbits;
			_newcheck(rbits=new unsigned char[h.framew*h.frameh*ps+1]);
		}
	}
	else
	{
		if(rbits)
		{
			delete [] rbits;  rbits=NULL;
		}
	}
	pixelSize=ps;  pitch=pixelSize*h.framew;  stereo=stereo_;
	hdr=h;
}


void Frame::init(unsigned char *bits_, int width, int pitch_, int height,
	int pixelSize_, int flags_)
{
	bits=bits_;
	hdr.x=hdr.y=0;
	hdr.framew=hdr.width=width;
	hdr.frameh=hdr.height=height;
	pixelSize=pixelSize_;
	hdr.size=hdr.framew*hdr.frameh*pixelSize;
	checkHeader(hdr);
	pitch=pitch_;
	flags=flags_;
	primary=false;
}


Frame *Frame::getTile(int x, int y, int width, int height)
{
	Frame *f;

	if(!bits || !pitch || !pixelSize) _throw("Frame not initialized");
	if(x<0 || y<0 || width<1 || height<1 || (x+width)>hdr.width
		|| (y+height)>hdr.height)
		throw Error("Frame::getTile", "Argument out of range");

	_newcheck(f=new Frame(false));
	f->hdr=hdr;
	f->hdr.x=x;
	f->hdr.y=y;
	f->hdr.width=width;
	f->hdr.height=height;
	f->pixelSize=pixelSize;
	f->flags=flags;
	f->pitch=pitch;
	f->stereo=stereo;
	f->isGL=isGL;
	bool bu=(flags&FRAME_BOTTOMUP);
	f->bits=&bits[pitch*(bu? hdr.height-y-height:y)+pixelSize*x];
	if(stereo && rbits)
		f->rbits=&rbits[pitch*(bu? hdr.height-y-height:y)+pixelSize*x];
	return f;
}


bool Frame::tileEquals(Frame *last, int x, int y, int width, int height)
{
	bool bu=(flags&FRAME_BOTTOMUP);

	if(x<0 || y<0 || width<1 || height<1 || (x+width)>hdr.width
		|| (y+height)>hdr.height)
		throw Error("Frame::tileEquals", "Argument out of range");

	if(last && hdr.width==last->hdr.width && hdr.height==last->hdr.height
		&& hdr.framew==last->hdr.framew && hdr.frameh==last->hdr.frameh
		&& hdr.qual==last->hdr.qual && hdr.subsamp==last->hdr.subsamp
		&& pixelSize==last->pixelSize && hdr.winid==last->hdr.winid
		&& hdr.dpynum==last->hdr.dpynum)
	{
		if(bits && last->bits)
		{
			unsigned char *newBits=
				&bits[pitch*(bu? hdr.height-y-height:y)+pixelSize*x];
			unsigned char *oldBits=
				&last->bits[last->pitch*(bu? hdr.height-y-height:y)+pixelSize*x];
			for(int i=0; i<height; i++)
			{
				if(memcmp(&newBits[pitch*i], &oldBits[last->pitch*i], pixelSize*width))
					return false;
			}
		}
		if(stereo && rbits && last->rbits)
		{
			unsigned char *newBits=
				&rbits[pitch*(bu? hdr.height-y-height:y)+pixelSize*x];
			unsigned char *oldBits=
				&last->rbits[last->pitch*(bu? hdr.height-y-height:y)+pixelSize*x];
			for(int i=0; i<height; i++)
			{
				if(memcmp(&newBits[pitch*i], &oldBits[last->pitch*i], pixelSize*width))
					return false;
			}
		}
		return true;
	}
	return false;
}


void Frame::makeAnaglyph(Frame &r, Frame &g, Frame &b)
{
	int rindex=flags&FRAME_BGR? 2:0, gindex=1, bindex=flags&FRAME_BGR? 0:2,
		i, j;
	unsigned char *srcrptr=r.bits, *srcgptr=g.bits, *srcbptr=b.bits,
		*dstptr=bits, *dstrptr, *dstgptr, *dstbptr;

	if(flags&FRAME_ALPHAFIRST) { rindex++;  gindex++;  bindex++; }

	for(j=0; j<hdr.frameh; j++, srcrptr+=r.pitch, srcgptr+=g.pitch,
		srcbptr+=b.pitch, dstptr+=pitch)
	{
		for(i=0, dstrptr=&dstptr[rindex], dstgptr=&dstptr[gindex],
			dstbptr=&dstptr[bindex]; i<hdr.framew; i++, dstrptr+=pixelSize,
			dstgptr+=pixelSize, dstbptr+=pixelSize)
		{
			*dstrptr=srcrptr[i];  *dstgptr=srcgptr[i];  *dstbptr=srcbptr[i];
		}
	}
}


void Frame::makePassive(Frame &stf, int mode)
{
	unsigned char *srclptr=stf.bits, *srcrptr=stf.rbits;
	unsigned char *dstptr=bits;

	if(hdr.framew != stf.hdr.framew || hdr.frameh != stf.hdr.frameh
		|| pitch != stf.pitch)
		_throw("Frames are not the same size");

	if(mode==RRSTEREO_INTERLEAVED)
	{
		int rowSize=pixelSize*hdr.framew;
		for(int j=0; j<hdr.frameh; j++)
		{
			if(j%2==0) memcpy(dstptr, srclptr, rowSize);
			else memcpy(dstptr, srcrptr, rowSize);
			srclptr+=pitch;  srcrptr+=pitch;  dstptr+=pitch;
		}
	}
	else if(mode==RRSTEREO_TOPBOTTOM)
	{
		int rowSize=pixelSize*hdr.framew;
		srcrptr+=pitch;
		for(int j=0; j<(hdr.frameh+1)/2; j++)
		{
			memcpy(dstptr, srclptr, rowSize);
			srclptr+=pitch*2;  dstptr+=pitch;
		}
		for(int j=(hdr.frameh+1)/2; j<hdr.frameh; j++)
		{
			memcpy(dstptr, srcrptr, rowSize);
			srcrptr+=pitch*2;  dstptr+=pitch;
		}
	}
	else if(mode==RRSTEREO_SIDEBYSIDE)
	{
		int pad=pitch-hdr.framew*pixelSize;
		int h=hdr.frameh;
		while(h>0)
		{
			unsigned char *srclptr2=srclptr;
			unsigned char *srcrptr2=srcrptr+pixelSize;
			for(int i=0; i<(hdr.framew+1)/2; i++)
			{
				*(int *)dstptr=*(int *)srclptr2;
				srclptr2+=pixelSize*2;  dstptr+=pixelSize;
			}
			for(int i=(hdr.framew+1)/2; i<hdr.framew-1; i++)
			{
				*(int *)dstptr=*(int *)srcrptr2;
				srcrptr2+=pixelSize*2;  dstptr+=pixelSize;
			}
			if(hdr.framew>1)
			{
				memcpy(dstptr, srcrptr2, pixelSize);
				dstptr+=pixelSize;
			}
			srclptr+=pitch;  srcrptr+=pitch;  dstptr+=pad;
			h--;
		}
	}
}


void Frame::decompressRGB(Frame &f, int width, int height, bool rightEye)
{
	if(!f.bits || f.hdr.size<1 || !bits || !hdr.size)
		_throw("Frame not initialized");

	int dstbgr=((flags&FRAME_BGR)!=0), dstbu=((flags&FRAME_BOTTOMUP)!=0),
		dstaf=((flags&FRAME_ALPHAFIRST)!=0);
	int i, j;
	int srcStride=f.pitch, dstStride=pitch;
	int startLine=dstbu? max(0, hdr.frameh-f.hdr.y-height) : f.hdr.y;
	unsigned char *srcptr=rightEye? f.rbits:f.bits,
		*dstptr=rightEye? &rbits[pitch*startLine+f.hdr.x*pixelSize]:
			&bits[pitch*startLine+f.hdr.x*pixelSize];
	unsigned char *srcptr2, *dstptr2;

	if(!dstbu)
	{
		srcptr=&srcptr[(height-1)*f.pitch];  srcStride=-srcStride;
	}
	if(!dstbgr && pixelSize==3)
	{
		int wps=width*pixelSize;
		if(dstaf)
		{
			dstptr++;  wps--;
		}
		for(i=0; i<height; i++, srcptr+=srcStride, dstptr+=dstStride)
		{
			memcpy(dstptr, srcptr, wps);
		}
	}
	else
	{
		if(dstaf) dstptr++;
		if(!dstbgr)
		{
			for(i=0; i<height; i++, srcptr+=srcStride, dstptr+=dstStride)
			{
				for(j=0, srcptr2=srcptr, dstptr2=dstptr; j<width; j++,
					srcptr2+=f.pixelSize, dstptr2+=pixelSize)
					memcpy(dstptr2, srcptr2, 3);
			}
	 	}
		else
		{
			for(i=0; i<height; i++, srcptr+=srcStride, dstptr+=dstStride)
			{
				for(j=0, srcptr2=srcptr, dstptr2=dstptr; j<width; j++,
					srcptr2+=f.pixelSize, dstptr2+=pixelSize)
				{
					dstptr2[2]=srcptr2[0];
					dstptr2[1]=srcptr2[1];
					dstptr2[0]=srcptr2[2];
			 	}
			}
		}
	}
}


void Frame::addLogo(void)
{
	unsigned char *rowptr, *colptr, *logoptr=vgllogo, *logoptr2;
	int rindex=flags&FRAME_BGR? 2:0, gindex=1, bindex=flags&FRAME_BGR? 0:2;
	if(flags&FRAME_ALPHAFIRST) { rindex++;  gindex++;  bindex++; }

	if(!bits || hdr.width<1 || hdr.height<1) return;
	int height=min(VGLLOGO_HEIGHT, hdr.height-1);
	int width=min(VGLLOGO_WIDTH, hdr.width-1);
	if(height<1 || width<1) return;
	if(flags&FRAME_BOTTOMUP)
		rowptr=&bits[pitch*height+(hdr.width-width-1)*pixelSize];
	else
		rowptr=&bits[pitch*(hdr.height-height-1)+(hdr.width-width-1)*pixelSize];
	for(int j=0; j<height; j++)
	{
		colptr=rowptr;
		logoptr2=logoptr;
		for(int i=0; i<width; i++)
		{
			if(*(logoptr2++))
			{
				colptr[rindex]^=113;  colptr[gindex]^=162;  colptr[bindex]^=117;
			}
			colptr+=pixelSize;
		}
		rowptr+=(flags&FRAME_BOTTOMUP)? -pitch:pitch;
		logoptr+=VGLLOGO_WIDTH;
	}

	if(!rbits) return;
	logoptr=vgllogo;
	if(flags&FRAME_BOTTOMUP)
		rowptr=&rbits[pitch*(VGLLOGO_HEIGHT+1)
			+(hdr.width-VGLLOGO_WIDTH-1)*pixelSize];
	else
		rowptr=&rbits[pitch*(hdr.height-VGLLOGO_HEIGHT-1)
			+(hdr.width-VGLLOGO_WIDTH-1)*pixelSize];
	for(int j=0; j<VGLLOGO_HEIGHT; j++)
	{
		colptr=rowptr;
		for(int i=0; i<VGLLOGO_WIDTH; i++)
		{
			if(*(logoptr++))
			{
				colptr[rindex]^=113;  colptr[gindex]^=162;  colptr[bindex]^=117;
			}
			colptr+=pixelSize;
		}
		rowptr+=(flags&FRAME_BOTTOMUP)? -pitch:pitch;
	}
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
	if(h.flags!=RR_EOF && (h.framew<1 || h.frameh<1 || h.width<1 || h.height<1
		|| h.x+h.width>h.framew || h.y+h.height>h.frameh))
	{
		throw(Error("Frame::checkHeader", "Invalid header"));
	}
}


// Compressed frame

CompressedFrame::CompressedFrame(void) : Frame(), tjhnd(NULL)
{
	if(!(tjhnd=tjInitCompress())) _throw(tjGetErrorStr());
	pixelSize=3;
	memset(&rhdr, 0, sizeof(rrframeheader));
}


CompressedFrame::~CompressedFrame(void)
{
	if(tjhnd) tjDestroy(tjhnd);
}

CompressedFrame &CompressedFrame::operator= (Frame &f)
{
	if(!f.bits) _throw("Frame not initialized");
	if(f.pixelSize<3 || f.pixelSize>4)
		_throw("Only true color frames are supported");
	switch(f.hdr.compress)
	{
		case RRCOMP_RGB:  compressRGB(f);  break;
		case RRCOMP_JPEG:  compressJPEG(f);  break;
		#ifdef USEXV
		case RRCOMP_YUV:  compressYUV(f);  break;
		#endif
		default:  _throw("Invalid compression type");
	}
	return *this;
}


#ifdef USEXV

void CompressedFrame::compressYUV(Frame &f)
{
	int tjflags=0;

	if(f.hdr.subsamp!=4) throw(Error("YUV encoder", "Invalid argument"));
	init(f.hdr, 0);
	if(f.flags&FRAME_BOTTOMUP) tjflags|=TJ_BOTTOMUP;
	if(f.flags&FRAME_BGR) tjflags|=TJ_BGR;
	_tj(tjEncodeYUV(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		f.pixelSize, bits, TJSUBSAMP(f.hdr.subsamp), tjflags));
	hdr.size=(unsigned int)tjBufSizeYUV(f.hdr.width, f.hdr.height,
		TJSUBSAMP(f.hdr.subsamp));
}

#endif


void CompressedFrame::compressJPEG(Frame &f)
{
	int tjflags=0;

	if(f.hdr.qual>100 || f.hdr.subsamp>16 || !isPow2(f.hdr.subsamp))
		throw(Error("JPEG compressor", "Invalid argument"));

	init(f.hdr, f.stereo? RR_LEFT:0);
	if(f.flags&FRAME_BOTTOMUP) tjflags|=TJ_BOTTOMUP;
	if(f.flags&FRAME_BGR) tjflags|=TJ_BGR;
	unsigned long size;
	_tj(tjCompress(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		f.pixelSize, bits, &size, TJSUBSAMP(f.hdr.subsamp), f.hdr.qual, tjflags));
	hdr.size=(unsigned int)size;
	if(f.stereo && f.rbits)
	{
		init(f.hdr, RR_RIGHT);
		if(rbits)
			_tj(tjCompress(tjhnd, f.rbits, f.hdr.width, f.pitch, f.hdr.height,
				f.pixelSize, rbits, &size, TJSUBSAMP(f.hdr.subsamp), f.hdr.qual,
				tjflags));
		rhdr.size=(unsigned int)size;
	}
}


void CompressedFrame::compressRGB(Frame &f)
{
	int i;  unsigned char *srcptr, *dstptr;
	int bu=(f.flags&FRAME_BOTTOMUP)? 1:0;
	if(f.flags&FRAME_BGR || f.flags&FRAME_ALPHAFIRST || f.pixelSize!=3)
		throw(Error("RGB compressor", "Source frame is not RGB"));
	int dstPitch=f.hdr.width*f.pixelSize;
	int srcStride=bu? f.pitch:-f.pitch;

	init(f.hdr, f.stereo? RR_LEFT:0);
	srcptr=bu? f.bits:&f.bits[f.pitch*(f.hdr.height-1)];
	for(i=0, dstptr=bits; i<f.hdr.height; i++, srcptr+=srcStride,
		dstptr+=dstPitch)
		memcpy(dstptr, srcptr, dstPitch);
	hdr.size=dstPitch*f.hdr.height;

	if(f.stereo && f.rbits)
	{
		init(f.hdr, RR_RIGHT);
		if(rbits)
		{
			srcptr=bu? f.rbits:&f.rbits[f.pitch*(f.hdr.height-1)];
			for(i=0, dstptr=rbits; i<f.hdr.height; i++, srcptr+=srcStride,
				dstptr+=dstPitch)
				memcpy(dstptr, srcptr, dstPitch);
			rhdr.size=dstPitch*f.hdr.height;
		}
	}
}


void CompressedFrame::init(rrframeheader &h, int buffer)
{
	checkHeader(h);
	if(h.flags==RR_EOF) { hdr=h;  return; }
	switch(buffer)
	{
		case RR_LEFT:
			if(h.width!=hdr.width || h.height!=hdr.height || !bits)
			{
				if(bits) delete [] bits;
				_newcheck(bits=new unsigned char[tjBufSize(h.width, h.height,
					h.subsamp)]);
			}
			hdr=h;  hdr.flags=RR_LEFT;  stereo=true;
			break;
		case RR_RIGHT:
			if(h.width!=rhdr.width || h.height!=rhdr.height || !rbits)
			{
				if(rbits) delete [] rbits;
				_newcheck(rbits=new unsigned char[tjBufSize(h.width, h.height,
					h.subsamp)]);
			}
			rhdr=h;  rhdr.flags=RR_RIGHT;  stereo=true;
			break;
		default:
			if(h.width!=hdr.width || h.height!=hdr.height || !bits)
			{
				if(bits) delete [] bits;
				_newcheck(bits=new unsigned char[tjBufSize(h.width, h.height,
					h.subsamp)]);
			}
			hdr=h;  hdr.flags=0;  stereo=false;
			break;
	}
	if(!stereo && rbits)
	{
		delete [] rbits;  rbits=NULL;
		memset(&rhdr, 0, sizeof(rrframeheader));
	}
	pitch=hdr.width*pixelSize;
}


// Frame created from shared graphics memory

FBXFrame::FBXFrame(Display *dpy, Drawable draw, Visual *vis,
	bool reuseConn) : Frame()
{
	if(!dpy || !draw) throw(Error("FBXFrame::FBXFrame", "Invalid argument"));
	XFlush(dpy);
	if(reuseConn) init(dpy, draw, vis);
	else init(DisplayString(dpy), draw, vis);
}


FBXFrame::FBXFrame(char *dpystring, Window win) : Frame()
{
	init(dpystring, win);
}


void FBXFrame::init(char *dpystring, Drawable draw, Visual *vis)
{
	tjhnd=NULL;  reuseConn=false;
	memset(&fb, 0, sizeof(fbx_struct));
	if(!dpystring || !draw) throw(Error("FBXFrame::init", "Invalid argument"));
	if(!(wh.dpy=XOpenDisplay(dpystring)))
		throw(Error("FBXFrame::init", "Could not open display"));
	wh.d=draw;  wh.v=vis;
}


void FBXFrame::init(Display *dpy, Drawable draw, Visual *vis)
{
	tjhnd=NULL;  reuseConn=true;
	memset(&fb, 0, sizeof(fbx_struct));
	if(!dpy || !draw) throw(Error("FBXFrame::init", "Invalid argument"));
	wh.dpy=dpy;
	wh.d=draw;  wh.v=vis;
}


FBXFrame::~FBXFrame(void)
{
	if(fb.bits) fbx_term(&fb);  if(bits) bits=NULL;
	if(tjhnd) tjDestroy(tjhnd);
	if(wh.dpy && !reuseConn) XCloseDisplay(wh.dpy);
}


void FBXFrame::init(rrframeheader &h)
{
	checkHeader(h);
	int usexshm=1;  char *env=NULL;
	if((env=getenv("VGL_USEXSHM"))!=NULL && strlen(env)>0 && !strcmp(env, "0"))
		usexshm=0;
	_fbx(fbx_init(&fb, wh, h.framew, h.frameh, usexshm));
	if(h.framew>fb.width || h.frameh>fb.height)
	{
		XSync(wh.dpy, False);
		_fbx(fbx_init(&fb, wh, h.framew, h.frameh, usexshm));
	}
	hdr=h;
	if(hdr.framew>fb.width) hdr.framew=fb.width;
	if(hdr.frameh>fb.height) hdr.frameh=fb.height;
	pixelSize=fbx_ps[fb.format];  pitch=fb.pitch;
	bits=(unsigned char *)fb.bits;
	flags=0;
	if(fbx_bgr[fb.format]) flags|=FRAME_BGR;
	if(fbx_alphafirst[fb.format]) flags|=FRAME_ALPHAFIRST;
}


FBXFrame &FBXFrame::operator= (CompressedFrame &cf)
{
	int tjflags=0;
	if(!cf.bits || cf.hdr.size<1)
		_throw("JPEG not initialized");
	init(cf.hdr);
	if(!fb.xi) _throw("Frame not initialized");
	if(fbx_bgr[fb.format]) tjflags|=TJ_BGR;
	if(fbx_alphafirst[fb.format]) tjflags|=TJ_ALPHAFIRST;
	int width=min(cf.hdr.width, fb.width-cf.hdr.x);
	int height=min(cf.hdr.height, fb.height-cf.hdr.y);
	if(width>0 && height>0 && cf.hdr.width<=width && cf.hdr.height<=height)
	{
		if(cf.hdr.compress==RRCOMP_RGB) decompressRGB(cf, width, height, false);
		else
		{
			if(!tjhnd)
			{
				if((tjhnd=tjInitDecompress())==NULL)
					throw(Error("FBXFrame::decompressor", tjGetErrorStr()));
			}
			_tj(tjDecompress(tjhnd, cf.bits, cf.hdr.size,
				(unsigned char *)&fb.bits[fb.pitch*cf.hdr.y+cf.hdr.x*fbx_ps[fb.format]],
				width, fb.pitch, height, fbx_ps[fb.format], tjflags));
		}
	}
	return *this;
}


void FBXFrame::redraw(void)
{
	if(flags&FRAME_BOTTOMUP) _fbx(fbx_flip(&fb, 0, 0, 0, 0));
	_fbx(fbx_write(&fb, 0, 0, 0, 0, fb.width, fb.height));
}


#ifdef USEXV

// Frame created using X Video

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
	tjhnd=NULL;  isXV=true;
	memset(&fb, 0, sizeof(fbxv_struct));
	if(!dpystring || !win_) throw(Error("XVFrame::init", "Invalid argument"));
	if(!(dpy=XOpenDisplay(dpystring)))
		throw(Error("XVFrame::init", "Could not open display"));
	win=win_;
}


XVFrame::~XVFrame(void)
{
	fbxv_term(&fb);  if(bits) bits=NULL;
	if(tjhnd) tjDestroy(tjhnd);
	if(dpy) XCloseDisplay(dpy);
}


XVFrame &XVFrame::operator= (Frame &f)
{
	if(!f.bits) _throw("Frame not initialized");
	if(f.pixelSize<3 || f.pixelSize>4)
		_throw("Only true color frames are supported");
	int tjflags=0;
	init(f.hdr);
	if(f.flags&FRAME_BOTTOMUP) tjflags|=TJ_BOTTOMUP;
	if(f.flags&FRAME_BGR) tjflags|=TJ_BGR;
	if(!tjhnd)
	{
		if((tjhnd=tjInitCompress())==NULL)
			throw(Error("XVFrame::compressor", tjGetErrorStr()));
	}
	_tj(tjEncodeYUV(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		f.pixelSize, bits, TJ_420, tjflags));
	hdr.size=(unsigned int)tjBufSizeYUV(f.hdr.width, f.hdr.height, TJ_420);
	if(hdr.size!=(unsigned long)fb.xvi->data_size)
		_throw("Image size mismatch in YUV encoder");
	return *this;
}


void XVFrame::init(rrframeheader &h)
{
	checkHeader(h);
	_fbxv(fbxv_init(&fb, dpy, win, h.framew, h.frameh, I420_PLANAR, 0));
	if(h.framew>fb.xvi->width || h.frameh>fb.xvi->height)
	{
		XSync(dpy, False);
		_fbx(fbxv_init(&fb, dpy, win, h.framew, h.frameh, I420_PLANAR, 0));
	}
	hdr=h;
	if(hdr.framew>fb.xvi->width) hdr.framew=fb.xvi->width;
	if(hdr.frameh>fb.xvi->height) hdr.frameh=fb.xvi->height;
	bits=(unsigned char *)fb.xvi->data;
	flags=pixelSize=pitch=0;
	hdr.size=fb.xvi->data_size;
}


void XVFrame::redraw(void)
{
	_fbxv(fbxv_write(&fb, 0, 0, 0, 0, 0, 0, hdr.framew, hdr.frameh));
}

#endif
