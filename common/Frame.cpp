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


#define jpegsub(s) \
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
		newcheck(bits=new unsigned char[h.framew*h.frameh*ps+1]);
	}
	if(stereo_)
	{
		if(h.framew!=hdr.framew || h.frameh!=hdr.frameh || ps!=pixelSize
			|| !rbits)
		{
			if(rbits) delete [] rbits;
			newcheck(rbits=new unsigned char[h.framew*h.frameh*ps+1]);
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


void Frame::init(unsigned char *bits_, int w, int pitch_, int h,
	int pixelSize_, int flags_)
{
	bits=bits_;
	hdr.framew=hdr.width=w;
	hdr.frameh=hdr.height=h;
	hdr.x=hdr.y=0;
	pixelSize=pixelSize_;
	hdr.size=hdr.framew*hdr.frameh*pixelSize;
	checkHeader(hdr);
	pitch=pitch_;
	flags=flags_;
	primary=false;
}


Frame *Frame::getTile(int x, int y, int w, int h)
{
	Frame *f;

	if(!bits || !pitch || !pixelSize) _throw("Frame not initialized");
	if(x<0 || y<0 || w<1 || h<1 || (x+w)>hdr.width || (y+h)>hdr.height)
		throw Error("Frame::getTile", "Argument out of range");

	newcheck(f=new Frame(false));
	f->hdr=hdr;
	f->hdr.height=h;
	f->hdr.width=w;
	f->hdr.y=y;
	f->hdr.x=x;
	f->pixelSize=pixelSize;
	f->flags=flags;
	f->pitch=pitch;
	f->stereo=stereo;
	f->isGL=isGL;
	bool bu=(flags&FRAME_BOTTOMUP);
	f->bits=&bits[pitch*(bu? hdr.height-y-h:y)+pixelSize*x];
	if(stereo && rbits)
		f->rbits=&rbits[pitch*(bu? hdr.height-y-h:y)+pixelSize*x];
	return f;
}


bool Frame::tileEquals(Frame *last, int x, int y, int w, int h)
{
	bool bu=(flags&FRAME_BOTTOMUP);

	if(x<0 || y<0 || w<1 || h<1 || (x+w)>hdr.width || (y+h)>hdr.height)
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
				&bits[pitch*(bu? hdr.height-y-h:y)+pixelSize*x];
			unsigned char *oldBits=
				&last->bits[last->pitch*(bu? hdr.height-y-h:y)+pixelSize*x];
			for(int i=0; i<h; i++)
			{
				if(memcmp(&newBits[pitch*i], &oldBits[last->pitch*i], pixelSize*w))
					return false;
			}
		}
		if(stereo && rbits && last->rbits)
		{
			unsigned char *newBits=
				&rbits[pitch*(bu? hdr.height-y-h:y)+pixelSize*x];
			unsigned char *oldBits=
				&last->rbits[last->pitch*(bu? hdr.height-y-h:y)+pixelSize*x];
			for(int i=0; i<h; i++)
			{
				if(memcmp(&newBits[pitch*i], &oldBits[last->pitch*i], pixelSize*w))
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
	unsigned char *sr=r.bits, *sg=g.bits, *sb=b.bits, *d=bits, *dr, *dg, *db;

	if(flags&FRAME_ALPHAFIRST) { rindex++;  gindex++;  bindex++; }

	for(j=0; j<hdr.frameh; j++, sr+=r.pitch, sg+=g.pitch, sb+=b.pitch,
		d+=pitch)
	{
		for(i=0, dr=&d[rindex], dg=&d[gindex],
			db=&d[bindex]; i<hdr.framew; i++, dr+=pixelSize,
			dg+=pixelSize, db+=pixelSize)
		{
			*dr=sr[i];  *dg=sg[i];  *db=sb[i];
		}
	}
}


void Frame::makePassive(Frame &stf, int mode)
{
	unsigned char *lptr=stf.bits, *rptr=stf.rbits;
	unsigned char *dstptr=bits;

	if(hdr.framew != stf.hdr.framew || hdr.frameh != stf.hdr.frameh
		|| pitch != stf.pitch)
		_throw("Frames are not the same size");

	if(mode==RRSTEREO_INTERLEAVED)
	{
		int rowSize=pixelSize*hdr.framew;
		for(int j=0; j<hdr.frameh; j++)
		{
			if(j%2==0) memcpy(dstptr, lptr, rowSize);
			else memcpy(dstptr, rptr, rowSize);
			lptr+=pitch;  rptr+=pitch;  dstptr+=pitch;
		}
	}
	else if(mode==RRSTEREO_TOPBOTTOM)
	{
		int rowSize=pixelSize*hdr.framew;
		rptr+=pitch;
		for(int j=0; j<(hdr.frameh+1)/2; j++)
		{
			memcpy(dstptr, lptr, rowSize);
			lptr+=pitch*2;  dstptr+=pitch;
		}
		for(int j=(hdr.frameh+1)/2; j<hdr.frameh; j++)
		{
			memcpy(dstptr, rptr, rowSize);
			rptr+=pitch*2;  dstptr+=pitch;
		}
	}
	else if(mode==RRSTEREO_SIDEBYSIDE)
	{
		int pad=pitch-hdr.framew*pixelSize;
		int h=hdr.frameh;
		while(h>0)
		{
			unsigned char *lptr2=lptr;
			unsigned char *rptr2=rptr+pixelSize;
			for(int i=0; i<(hdr.framew+1)/2; i++)
			{
				*(int *)dstptr=*(int *)lptr2;
				lptr2+=pixelSize*2;  dstptr+=pixelSize;
			}
			for(int i=(hdr.framew+1)/2; i<hdr.framew-1; i++)
			{
				*(int *)dstptr=*(int *)rptr2;
				rptr2+=pixelSize*2;  dstptr+=pixelSize;
			}
			if(hdr.framew>1)
			{
				memcpy(dstptr, rptr2, pixelSize);
				dstptr+=pixelSize;
			}
			lptr+=pitch;  rptr+=pitch;  dstptr+=pad;
			h--;
		}
	}
}


void Frame::decompressRGB(Frame &f, int w, int h, bool rightEye)
{
	if(!f.bits || f.hdr.size<1 || !bits || !hdr.size)
		_throw("Frame not initialized");

	int dstbgr=((flags&FRAME_BGR)!=0), dstbu=((flags&FRAME_BOTTOMUP)!=0),
		dstaf=((flags&FRAME_ALPHAFIRST)!=0);
	int i, j;
	int srcStride=f.pitch, dstStride=pitch;
	int startLine=dstbu? max(0, hdr.frameh-f.hdr.y-h) : f.hdr.y;
	unsigned char *srcptr=rightEye? f.rbits:f.bits,
		*dstptr=rightEye? &rbits[pitch*startLine+f.hdr.x*pixelSize]:
			&bits[pitch*startLine+f.hdr.x*pixelSize];
	unsigned char *srcptr2, *dstptr2;

	if(!dstbu)
	{
		srcptr=&srcptr[(h-1)*f.pitch];  srcStride=-srcStride;
	}
	if(!dstbgr && pixelSize==3)
	{
		int wps=w*pixelSize;
		if(dstaf)
		{
			dstptr++;  wps--;
		}
		for(i=0; i<h; i++, srcptr+=srcStride, dstptr+=dstStride)
		{
			memcpy(dstptr, srcptr, wps);
		}
	}
	else
	{
		if(dstaf) dstptr++;
		if(!dstbgr)
		{
			for(i=0; i<h; i++, srcptr+=srcStride, dstptr+=dstStride)
			{
				for(j=0, srcptr2=srcptr, dstptr2=dstptr; j<w; j++,
					srcptr2+=f.pixelSize, dstptr2+=pixelSize)
					memcpy(dstptr2, srcptr2, 3);
			}
	 	}
		else
		{
			for(i=0; i<h; i++, srcptr+=srcStride, dstptr+=dstStride)
			{
				for(j=0, srcptr2=srcptr, dstptr2=dstptr; j<w; j++,
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
	int h=min(VGLLOGO_HEIGHT, hdr.height-1);
	int w=min(VGLLOGO_WIDTH, hdr.width-1);
	if(h<1 || w<1) return;
	if(flags&FRAME_BOTTOMUP)
		rowptr=&bits[pitch*h+(hdr.width-w-1)*pixelSize];
	else
		rowptr=&bits[pitch*(hdr.height-h-1)+(hdr.width-w-1)*pixelSize];
	for(int j=0; j<h; j++)
	{
		colptr=rowptr;
		logoptr2=logoptr;
		for(int i=0; i<w; i++)
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
	vglout.print("hdr.size    = %lu\n", hdr.size);
	vglout.print("hdr.winid   = 0x%.8x\n", hdr.winid);
	vglout.print("hdr.dpynum  = %d\n", hdr.dpynum);
	vglout.print("hdr.compress= %d\n", hdr.compress);
	vglout.print("hdr.framew  = %d\n", hdr.framew);
	vglout.print("hdr.frameh  = %d\n", hdr.frameh);
	vglout.print("hdr.width   = %d\n", hdr.width);
	vglout.print("hdr.height  = %d\n", hdr.height);
	vglout.print("hdr.x       = %d\n", hdr.x);
	vglout.print("hdr.y       = %d\n", hdr.y);
	vglout.print("hdr.qual    = %d\n", hdr.qual);
	vglout.print("hdr.subsamp = %d\n", hdr.subsamp);
	vglout.print("hdr.flags   = %d\n", hdr.flags);
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
	tj(tjEncodeYUV(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		f.pixelSize, bits, jpegsub(f.hdr.subsamp), tjflags));
	hdr.size=(unsigned int)TJBUFSIZEYUV(f.hdr.width, f.hdr.height,
		jpegsub(f.hdr.subsamp));
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
	tj(tjCompress(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		f.pixelSize, bits, &size, jpegsub(f.hdr.subsamp), f.hdr.qual, tjflags));
	hdr.size=(unsigned int)size;
	if(f.stereo && f.rbits)
	{
		init(f.hdr, RR_RIGHT);
		if(rbits)
			tj(tjCompress(tjhnd, f.rbits, f.hdr.width, f.pitch, f.hdr.height,
				f.pixelSize, rbits, &size, jpegsub(f.hdr.subsamp), f.hdr.qual,
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
	int pitch=f.hdr.width*f.pixelSize;
	int srcStride=bu? f.pitch:-f.pitch;

	init(f.hdr, f.stereo? RR_LEFT:0);
	srcptr=bu? f.bits:&f.bits[f.pitch*(f.hdr.height-1)];
	for(i=0, dstptr=bits; i<f.hdr.height; i++, srcptr+=srcStride,
		dstptr+=pitch)
		memcpy(dstptr, srcptr, pitch);
	hdr.size=pitch*f.hdr.height;

	if(f.stereo && f.rbits)
	{
		init(f.hdr, RR_RIGHT);
		if(rbits)
		{
			srcptr=bu? f.rbits:&f.rbits[f.pitch*(f.hdr.height-1)];
			for(i=0, dstptr=rbits; i<f.hdr.height; i++, srcptr+=srcStride,
				dstptr+=pitch)
				memcpy(dstptr, srcptr, pitch);
			rhdr.size=pitch*f.hdr.height;
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
				newcheck(bits=new unsigned char[TJBUFSIZE(h.width, h.height)]);
			}
			hdr=h;  hdr.flags=RR_LEFT;  stereo=true;
			break;
		case RR_RIGHT:
			if(h.width!=rhdr.width || h.height!=rhdr.height || !rbits)
			{
				if(rbits) delete [] rbits;
				newcheck(rbits=new unsigned char[TJBUFSIZE(h.width, h.height)]);
			}
			rhdr=h;  rhdr.flags=RR_RIGHT;  stereo=true;
			break;
		default:
			if(h.width!=hdr.width || h.height!=hdr.height || !bits)
			{
				if(bits) delete [] bits;
				newcheck(bits=new unsigned char[TJBUFSIZE(h.width, h.height)]);
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

FBXFrame::FBXFrame(Display *dpy, Drawable d, Visual *v) : Frame()
{
	if(!dpy || !d) throw(Error("FBXFrame::FBXFrame", "Invalid argument"));
	XFlush(dpy);
	init(DisplayString(dpy), d, v);
}


FBXFrame::FBXFrame(char *dpystring, Window win) : Frame()
{
	init(dpystring, win);
}


void FBXFrame::init(char *dpystring, Drawable d, Visual *v)
{
	tjhnd=NULL;
	memset(&fb, 0, sizeof(fbx_struct));
	if(!dpystring || !d) throw(Error("FBXFrame::init", "Invalid argument"));
	if(!(wh.dpy=XOpenDisplay(dpystring)))
		throw(Error("FBXFrame::init", "Could not open display"));
	wh.d=d;  wh.v=v;
}


FBXFrame::~FBXFrame(void)
{
	if(fb.bits) fbx_term(&fb);  if(bits) bits=NULL;
	if(tjhnd) tjDestroy(tjhnd);
	if(wh.dpy) XCloseDisplay(wh.dpy);
}


void FBXFrame::init(rrframeheader &h)
{
	checkHeader(h);
	int usexshm=1;  char *env=NULL;
	if((env=getenv("VGL_USEXSHM"))!=NULL && strlen(env)>0 && !strcmp(env, "0"))
		usexshm=0;
	fbx(fbx_init(&fb, wh, h.framew, h.frameh, usexshm));
	if(h.framew>fb.width || h.frameh>fb.height)
	{
		XSync(wh.dpy, False);
		fbx(fbx_init(&fb, wh, h.framew, h.frameh, usexshm));
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
			tj(tjDecompress(tjhnd, cf.bits, cf.hdr.size,
				(unsigned char *)&fb.bits[fb.pitch*cf.hdr.y+cf.hdr.x*fbx_ps[fb.format]],
				width, fb.pitch, height, fbx_ps[fb.format], tjflags));
		}
	}
	return *this;
}


void FBXFrame::redraw(void)
{
	if(flags&FRAME_BOTTOMUP) fbx(fbx_flip(&fb, 0, 0, 0, 0));
	fbx(fbx_write(&fb, 0, 0, 0, 0, fb.width, fb.height));
}


#ifdef USEXV

// Frame created using X Video

XVFrame::XVFrame(Display *dpy, Window win) : Frame()
{
	if(!dpy || !win) throw(Error("XVFrame::XVFrame", "Invalid argument"));
	XFlush(dpy);
	init(DisplayString(dpy), win);
}


XVFrame::XVFrame(char *dpystring, Window win) : Frame()
{
	init(dpystring, win);
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
	tj(tjEncodeYUV(tjhnd, f.bits, f.hdr.width, f.pitch, f.hdr.height,
		f.pixelSize, bits, TJ_420, tjflags));
	hdr.size=(unsigned int)TJBUFSIZEYUV(f.hdr.width, f.hdr.height, TJ_420);
	if(hdr.size!=(unsigned long)fb.xvi->data_size)
		_throw("Image size mismatch in YUV encoder");
	return *this;
}


void XVFrame::init(rrframeheader &h)
{
	checkHeader(h);
	fbxv(fbxv_init(&fb, dpy, win, h.framew, h.frameh, I420_PLANAR, 0));
	if(h.framew>fb.xvi->width || h.frameh>fb.xvi->height)
	{
		XSync(dpy, False);
		fbx(fbxv_init(&fb, dpy, win, h.framew, h.frameh, I420_PLANAR, 0));
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
	fbxv(fbxv_write(&fb, 0, 0, 0, 0, 0, 0, hdr.framew, hdr.frameh));
}

#endif
