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

// This implements a JPEG compressor/decompressor using the Pegasus Pictools library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pic.h"
#include "errors.h"
#include "hpjpeg.h"

const int hpjsub[NUMSUBOPT]={SS_111, SS_211, SS_411};
static char lasterror[100]="No error";
#define _throw(c) {sprintf(lasterror, "%s", c);  return -1;}


LONG DeferFn(PIC_PARM* p, RESPONSE res)
{
    return ( 0 );   /* OK to continue */
}


#define _peg(p,f) {\
	RESPONSE __res;\
	if((__res=Pegasus(&p,f))==RES_ERR) {\
		sprintf(lasterror, "Error #%ld returned from Pegasus(%s)\n", p.Status, #f);\
		return -1;}}

#define qtop(q) (\
	(q>=50 && q<=100)? (int)(64.-0.64*(float)q): ( (q>=7 && q<=50)? (int)(1600./(float)q): 255 ) )


// CO

DLLEXPORT hpjhandle DLLCALL hpjInitCompress(void)
{
	return (hpjhandle)1;
}

DLLEXPORT unsigned long DLLCALL HPJBUFSIZE(int width, int height)
{
	return (max((max(width,16))*(max(height,16))*3,2048));
}

DLLEXPORT int DLLCALL hpjCompress(hpjhandle h,
	unsigned char *_srcbuf, int width, int pitch, int height, int ps,
	unsigned char *dstbuf, unsigned long *size,
	int jpegsub, int qual, int flags)
{
	PIC_PARM PicParm;
	BITMAPINFOHEADER bmih;
	unsigned char *srcbuf=_srcbuf;

	if(srcbuf==NULL || width<=0 || pitch<0 || height<=0
		|| dstbuf==NULL || size==NULL
		|| jpegsub<0 || jpegsub>=NUMSUBOPT || qual<0 || qual>100)
		_throw("Invalid argument in hpjCompress()");
	if(ps!=3 && ps!=4) _throw("This JPEG codec supports only 24-bit or 32-bit true color");

	if(flags&HPJ_ALPHAFIRST) srcbuf=&_srcbuf[1];

	// general PIC_PARM initialization for all operations
	memset(&PicParm, 0, sizeof(PicParm));
	PicParm.ParmSize     = sizeof(PicParm);
	PicParm.ParmVer      = CURRENT_PARMVER; // #define’d in PIC.H
	PicParm.ParmVerMinor = 1; // a magic number for the current version

	// initialize input buffer pointers
	PicParm.Get.Start  = srcbuf;
	if(pitch==0) pitch=width*ps;
	PicParm.Get.End    = srcbuf + pitch*(jpegsub==HPJ_444? max(height,8):max(height,16));
	PicParm.Get.QFlags = Q_EOF;
	if(flags&HPJ_BOTTOMUP)
	{
		PicParm.Get.QFlags|=Q_REVERSE;
		PicParm.Get.Front  = srcbuf + pitch*height;
		PicParm.Get.Rear   = PicParm.Get.Start;
	}
	else
	{
		PicParm.Get.Front  = PicParm.Get.Start;
		PicParm.Get.Rear   = srcbuf + pitch*height;
	}

	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	bmih.biSize=sizeof(BITMAPINFOHEADER);
	bmih.biWidth=width;
	bmih.biHeight=height;
	bmih.biPlanes=1;
	bmih.biBitCount=ps*8;
	bmih.biCompression=BI_RGB;
	
	memcpy(&PicParm.Head, &bmih, sizeof(BITMAPINFOHEADER));

	// initialize parameters specific to compress JPEG
	PicParm.Op = OP_D2S; // requests DIB to Sequential JPEG operation

	// memset because PegasusQuery may set values in the same union
	memset(&PicParm.u.D2S, 0, sizeof(PicParm.u.D2S));
	PicParm.u.D2S.LumFactor = qtop(qual);
	PicParm.u.D2S.ChromFactor = qtop(qual);
	PicParm.u.D2S.SubSampling = hpjsub[jpegsub];
	PicParm.u.D2S.JpegType = JT_RAW;
	if(pitch!=width*ps)
	{
		PicParm.u.D2S.PicFlags |= PF_WidthPadKnown;
		PicParm.u.D2S.WidthPad = pitch;
	}
	else
	{
		PicParm.u.D2S.PicFlags |= PF_NoDibPad;
	}
	if(!(flags&HPJ_BGR)) PicParm.u.D2S.PicFlags |= PF_SwapRB;

	PicParm.Put.Start = dstbuf;
	PicParm.Put.End   = dstbuf + HPJBUFSIZE(width, height);
	PicParm.Put.Front = PicParm.Put.Rear = PicParm.Put.Start;
	PicParm.Put.QFlags = 0;

	PicParm.DeferFn = DeferFn;
	PicParm.Flags |= F_UseDeferFn;

	_peg(PicParm, REQ_INIT);
	_peg(PicParm, REQ_EXEC);
	_peg(PicParm, REQ_TERM);
	*size=(long)(PicParm.Put.Rear-PicParm.Put.Front);
	return 0;
}


// DEC

DLLEXPORT hpjhandle DLLCALL hpjInitDecompress(void)
{
	return (hpjhandle)1;
}

DLLEXPORT int DLLCALL hpjDecompressHeader(hpjhandle h,
	unsigned char *srcbuf, unsigned long size,
	int *width, int *height)
{
	PIC_PARM PicParm;

	if(srcbuf==NULL || size<=0 || width==NULL || height==NULL)
		_throw("Invalid argument in hpjDecompressHeader()");

	// general PIC_PARM initialization for all operations
	memset(&PicParm, 0, sizeof(PicParm));
	PicParm.ParmSize     = sizeof(PicParm);
	PicParm.ParmVer      = CURRENT_PARMVER; // #define’d in PIC.H
	PicParm.ParmVerMinor = 1; // a magic number for the current version

	// initialize input buffer pointers
	PicParm.Get.Start  = srcbuf;
	PicParm.Get.End    = srcbuf + size;
	PicParm.Get.Front  = PicParm.Get.Start;
	PicParm.Get.Rear   = PicParm.Get.End;
	PicParm.Get.QFlags = Q_EOF;

	PicParm.Op = OP_S2D;

	PicParm.Put.Start = NULL;
	PicParm.Put.End   = NULL;
	PicParm.Put.Front = PicParm.Put.Rear = PicParm.Put.Start;
	PicParm.Put.QFlags = 0;

	_peg(PicParm, REQ_INIT);
	{
		RESPONSE __res;
		if((__res=Pegasus(&PicParm, REQ_EXEC))!=RES_PUT_NEED_SPACE)
		{
			sprintf(lasterror, "Error #%ld returned from Pegasus(REQ_EXEC)\n", PicParm.Status);
			return -1;
		}
	}
	_peg(PicParm, REQ_TERM);

	*width=PicParm.Head.biWidth;  *height=PicParm.Head.biHeight;
	if(*width<1 || *height<1) _throw("Invalid data returned in header");
	return 0;
}

DLLEXPORT int DLLCALL hpjDecompress(hpjhandle h,
	unsigned char *srcbuf, unsigned long size,
	unsigned char *_dstbuf, int width, int pitch, int height, int ps,
	int flags)
{
	PIC_PARM PicParm;
	BITMAPINFOHEADER bmih;
	unsigned char *dstbuf=_dstbuf;

	if(srcbuf==NULL || size<=0
		|| dstbuf==NULL || width<=0 || pitch<0 || height<=0)
		_throw("Invalid argument in hpjDecompress()");
	if(ps!=3 && ps!=4) _throw("This JPEG codec supports only 24-bit or 32-bit true color");

	if(flags&HPJ_ALPHAFIRST) dstbuf=&_dstbuf[1];

	// general PIC_PARM initialization for all operations
	memset(&PicParm, 0, sizeof(PicParm));
	PicParm.ParmSize     = sizeof(PicParm);
	PicParm.ParmVer      = CURRENT_PARMVER; // #define’d in PIC.H
	PicParm.ParmVerMinor = 1; // a magic number for the current version

	// initialize input buffer pointers
	PicParm.Get.Start  = srcbuf;
	PicParm.Get.End    = srcbuf + size;
	PicParm.Get.Front  = PicParm.Get.Start;
	PicParm.Get.Rear   = PicParm.Get.End;
	PicParm.Get.QFlags = Q_EOF;

	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	bmih.biSize=sizeof(BITMAPINFOHEADER);
	bmih.biWidth=width;
	bmih.biHeight=height;
	bmih.biPlanes=1;
	bmih.biBitCount=ps*8;
	bmih.biCompression=BI_RGB;

	memcpy(&PicParm.Head, &bmih, sizeof(BITMAPINFOHEADER));

	PicParm.Op = OP_S2D;

	memset(&PicParm.u.S2D, 0, sizeof(PicParm.u.S2D));
	if(pitch==0) pitch=width*ps;
	if(pitch!=width*ps)
	{
		PicParm.u.S2D.PicFlags |= PF_WidthPadKnown;
		PicParm.u.S2D.WidthPad = pitch;
	}
	else
	{
		PicParm.u.S2D.PicFlags |= PF_NoDibPad;
	}
	PicParm.u.S2D.PicFlags |= PF_NoCrossBlockSmoothing;
	PicParm.u.S2D.DibSize = ps*8;
	if(!(flags&HPJ_BGR)) PicParm.u.S2D.PicFlags |= PF_SwapRB;

	PicParm.Put.Start = dstbuf;
	PicParm.Put.End   = dstbuf + pitch*height;
	PicParm.Put.QFlags = 0;
	if(flags&HPJ_BOTTOMUP)
	{
		PicParm.Put.QFlags|=Q_REVERSE;
		PicParm.Put.Front = PicParm.Put.Rear = PicParm.Put.End;
	}
	else
		PicParm.Put.Front = PicParm.Put.Rear = PicParm.Put.Start;
	PicParm.DeferFn = DeferFn;
	PicParm.Flags |= F_UseDeferFn;

	_peg(PicParm, REQ_INIT);
	_peg(PicParm, REQ_EXEC);
	_peg(PicParm, REQ_TERM);
	return 0;
}


// General

DLLEXPORT char* DLLCALL hpjGetErrorStr(void)
{
	return lasterror;
}

DLLEXPORT int DLLCALL hpjDestroy(hpjhandle h)
{
	return 0;
}
