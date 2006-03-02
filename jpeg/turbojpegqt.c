/* Copyright (C)2004 Landmark Graphics
 * Copyright (C)2005 Sun Microsystems, Inc.
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

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <QuickTime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#include <mach/mach.h>
#define STRIDE_BOUNDARY 16384
#else
#include <ImageCompression.h>
#include <QuickTimeComponents.h>
#include <PictUtils.h>
#include <QTML.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define GetPixRowBytes QTGetPixMapHandleRowBytes
#endif

#include "turbojpeg.h"

static char lasterror[96]="No error";

#define tryqt(f) {QDErr __err;  if((__err=(f))!=noErr) {  \
	sprintf(lasterror, "%d: Quicktime error: %d", __LINE__, __err);  return -1;}}
#define _throw(c) {sprintf(lasterror, "%d: %s", __LINE__, c);  goto bail;}


/******************************************************************
 *                        Compressor                              *
 ******************************************************************/


DLLEXPORT tjhandle DLLCALL tjInitCompress(void)
{
	#ifdef _WIN32
	InitializeQTML(0);
	#endif
	return (tjhandle)1;
}


/* Quicktime seems to use the output buffer for temporary MCU storage
   and thus requires 384 bytes per 8x8 block */
DLLEXPORT unsigned long DLLCALL TJBUFSIZE(int width, int height)
{
	return ((width/8+1) * (height/8+1) * 384 + 2048);
}


DLLEXPORT int DLLCALL tjCompress(tjhandle h,
	unsigned char *srcbuf, int width, int pitch, int height, int ps,
	unsigned char *dstbuf, unsigned long *size,
	int jpegsub, int qual, int flags)
{
	ImageDescriptionHandle myDesc = NULL;
	Rect                   rect;
	GWorldPtr              worldPtr = NULL;
	PixMapHandle           pixMapHandle = NULL;
	unsigned char          *rowptr;
	CodecQ myCodecQ;
	long needSize;
	int rowbytes, fastpath, qt_roffset, qt_goffset, qt_boffset;

	if(srcbuf==NULL || width<=0 || pitch<0 || height<=0
		|| dstbuf==NULL || size==NULL
		|| jpegsub<0 || jpegsub>=NUMSUBOPT || qual<0 || qual>100)
		_throw("Invalid argument in tjCompress()");
	if(ps!=3 && ps!=4) _throw("This compressor can only take 24-bit or 32-bit RGB input");

	if(pitch==0) pitch=width*ps;
	*size = 0;

	rect.left = rect.top = 0;
	rect.bottom = (short)height;
	rect.right = (short)width;
	tryqt( NewGWorld(&worldPtr, 32, &rect, NULL, NULL, 0) );

	pixMapHandle = GetGWorldPixMap(worldPtr);
	if(!pixMapHandle) _throw("GetGWorldPixMap() failed");

	LockPixels(pixMapHandle);

	fastpath = 0;
	switch((**pixMapHandle).pixelFormat)
	{
		case k24RGBPixelFormat:
			if(!(flags&TJ_BGR) && ps==3) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k32RGBAPixelFormat:
			if(!(flags&TJ_BGR) && !(flags&TJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k24BGRPixelFormat:
			if(flags&TJ_BGR && ps==3) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32BGRAPixelFormat:
			if(flags&TJ_BGR && !(flags&TJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32ABGRPixelFormat:
			if(flags&TJ_BGR && flags&TJ_ALPHAFIRST && ps==4) fastpath=1;
			qt_roffset=3;  qt_goffset=2;  qt_boffset=1;
			break;
		case k32ARGBPixelFormat:
			if(!(flags&TJ_BGR) && flags&TJ_ALPHAFIRST && ps==4) fastpath=1;
			qt_roffset=1;  qt_goffset=2;  qt_boffset=3;
			break;
		default:
			_throw("Quicktime returned a pixmap which isn't 24-bit or 32-bit");
	}

	rowptr = (unsigned char *)GetPixBaseAddr(pixMapHandle);
	rowbytes = GetPixRowBytes(pixMapHandle);

	/* Wish there was a way to do this without a buffer copy */
	if(fastpath)
	{
		int i, srcstride=pitch;
		unsigned char *srcptr=srcbuf;
		if(flags&TJ_BOTTOMUP) {srcptr=&srcbuf[pitch*(height-1)];  srcstride=-pitch;}
		for(i=0; i<height; ++i)
		{
			#if defined(__APPLE__)

			/* 
			 * Use the mach kernel to execute a copy-on-write operation.
			 * Since we're only going to be reading from the buffer, this
			 * should be *far* less expensive than memcpy, if that copy
			 * is large (>16 Kbytes)
			 */

			if(pitch > STRIDE_BOUNDARY)
				vm_copy(mach_task_self(), (vm_address_t)srcptr, pitch,
					(vm_address_t)rowptr);
		    else
			#else
			memcpy(rowptr, srcptr, pitch); 
			#endif
			rowptr += rowbytes;
			srcptr += srcstride;
		}
	}
	else
	{
		int roffset=0, goffset=1, boffset=2;
		int i, j, srcstride=pitch;
		unsigned char *srcptr=srcbuf;
		int rowps=(**pixMapHandle).pixelSize/8;

		if(flags&TJ_BGR) {roffset=2;  goffset=1;  boffset=0;}
		if(flags&TJ_ALPHAFIRST) {roffset++;  goffset++;  boffset++;}
		if(flags&TJ_BOTTOMUP) {srcptr=&srcbuf[pitch*(height-1)];  srcstride=-pitch;}
		for(i=0; i<height; ++i)
		{
			for(j=0; j<width; ++j)
			{
				rowptr[(rowps*j)+qt_roffset] = srcptr[(ps*j)+roffset];
				rowptr[(rowps*j)+qt_goffset] = srcptr[(ps*j)+goffset];
				rowptr[(rowps*j)+qt_boffset] = srcptr[(ps*j)+boffset];
			}
			rowptr += rowbytes;
			srcptr += srcstride;
		}
	}

	myDesc = (ImageDescriptionHandle)NewHandle(sizeof(ImageDescription));
	if(!myDesc) _throw("Could not create image description");

	myCodecQ = codecMinQuality;
	if(qual > 25) myCodecQ = codecLowQuality;
	if(qual > 50) myCodecQ = codecNormalQuality;
	if(qual > 75) myCodecQ = codecHighQuality;
	if(qual > 90) myCodecQ = codecMaxQuality;
	if(qual > 95) myCodecQ = codecLosslessQuality;
    
	/*
	 * Check to make sure that the dstbuf is big enough
	 * to receive our image data
	 */
	needSize = 0;
	GetMaxCompressionSize(pixMapHandle, &rect, (**pixMapHandle).pixelSize,
		myCodecQ, kJPEGCodecType, bestSpeedCodec, &needSize);

	if(needSize > (long)TJBUFSIZE(width, height))
	{
		sprintf(lasterror, "Output buffer needs to be %d bytes", needSize);
		goto bail;
	}

	tryqt( FCompressImage(pixMapHandle, &rect, (**pixMapHandle).pixelSize,
		myCodecQ, kJPEGCodecType, bestSpeedCodec, NULL, 0, 0, NULL, NULL, myDesc,
		(Ptr)dstbuf) );

	*size = (**myDesc).dataSize;

	if(myDesc) DisposeHandle((Handle)myDesc);
	if(pixMapHandle) UnlockPixels(pixMapHandle);
	if(worldPtr) DisposeGWorld(worldPtr);
	return 0;

	bail:
	if(myDesc) DisposeHandle((Handle)myDesc);
	if(pixMapHandle) UnlockPixels(pixMapHandle);
	if(worldPtr) DisposeGWorld(worldPtr);
	return -1;
}


/******************************************************************
 *                        Decompressor                            *
 ******************************************************************/


DLLEXPORT tjhandle DLLCALL tjInitDecompress(void) 
{
	#ifdef _WIN32
	InitializeQTML(0);
	#endif
	return (tjhandle)1;
}

typedef struct
{
	unsigned long bytesleft, bytesprocessed;
	unsigned char *jpgptr;
} jpgstruct;

#define read_byte(j, b) {if((j)->bytesleft<=0) _throw("Unexpected end of image");  \
	b=*(j)->jpgptr;  (j)->jpgptr++;  (j)->bytesleft--;  (j)->bytesprocessed++;}
#define read_word(j, w) {unsigned char __b;  read_byte(j, __b);  w=(__b<<8);  \
	read_byte(j, __b);  w|=(__b&0xff);}

int find_marker(jpgstruct *jpg, unsigned char *marker)
{
	unsigned char b;
	while(1)
	{
		do {read_byte(jpg, b);} while(b!=0xff);
		read_byte(jpg, b);
		if(b!=0 && b!=0xff) {*marker=b;  return 0;}
		else {jpg->jpgptr--;  jpg->bytesleft++;  jpg->bytesprocessed--;}
	}
	return 0;

	bail:
	return -1;
}

#define check_byte(j, b) {unsigned char __b;  read_byte(j, __b);  \
	if(__b!=(b)) _throw("JPEG bitstream error");}

DLLEXPORT int DLLCALL tjDecompressHeader(tjhandle h,
	unsigned char *srcbuf, unsigned long size,
	int *width, int *height)
{
	int i;  unsigned char tempbyte, marker;  unsigned short tempword, length;
	int markerread=0;  jpgstruct j;

	if(srcbuf==NULL || size<=0 || width==NULL || height==NULL)
		_throw("Invalid argument in tjDecompressHeader()");

	// This is nasty, but I don't know of another way
	j.bytesprocessed=0;  j.bytesleft=size;  j.jpgptr=srcbuf;
	*width=*height=0;

	check_byte(&j, 0xff);  check_byte(&j, 0xd8);  // SOI

	while(1)
	{
		if(find_marker(&j, &marker)==-1) goto bail;

		switch(marker)
		{
			case 0xe0:  // JFIF
				read_word(&j, length);
				if(length<8) _throw("JPEG bitstream error");
				check_byte(&j, 'J');  check_byte(&j, 'F');
				check_byte(&j, 'I');  check_byte(&j, 'F');  check_byte(&j, 0);
				for(i=7; i<length; i++) read_byte(&j, tempbyte) // We don't care about the rest
				markerread+=1;
				break;
			case 0xc0:  // SOF
				read_word(&j, length);
				if(length<11) _throw("JPEG bitstream error");
				read_byte(&j, tempbyte);  // precision
				read_word(&j, tempword);  // height
				if(tempword) *height=(int)tempword;
				read_word(&j, tempword);  // width
				if(tempword) *width=(int)tempword;
				goto done;
				break;
		}
	}

	done:
	if(*width<1 || *height<1) _throw("Invalid data returned in header");
	return 0;

	bail:
	return -1;
}

DLLEXPORT int DLLCALL tjDecompress(tjhandle h,
	unsigned char *srcbuf, unsigned long size,
	unsigned char *dstbuf, int width, int pitch, int height, int ps,
	int flags)
{
	ImageDescriptionHandle myDesc = NULL;
	Rect                   rect;
	GWorldPtr              worldPtr = NULL;
	PixMapHandle           pixMapHandle = NULL;
	unsigned char          *rowptr;
	int rowbytes, fastpath, qt_roffset, qt_goffset, qt_boffset;

	if(srcbuf==NULL || size<=0
		|| dstbuf==NULL || width<=0 || pitch<0 || height<=0)
		_throw("Invalid argument in tjDecompress()");
	if(ps!=3 && ps!=4) _throw("This decompressor can only take 24-bit or 32-bit RGB input");

	if(pitch==0) pitch = width * ps;

	rect.left = rect.top = 0;
	rect.bottom = (short)height;
	rect.right = (short)width;
	tryqt( NewGWorld(&worldPtr, 32, &rect, NULL, NULL, 0) );

	pixMapHandle = GetGWorldPixMap(worldPtr);
	if(!pixMapHandle) _throw("GetGWorldPixMap() failed");

	LockPixels(pixMapHandle);

	myDesc = (ImageDescriptionHandle)NewHandle(sizeof(ImageDescription));
	if(!myDesc) _throw("Could not create image description");

	HLock((Handle)myDesc);

	/*
	 * Fill in the description information
	 */
	(**myDesc).idSize           = sizeof(ImageDescription);
	(**myDesc).cType            = kJPEGCodecType;
	(**myDesc).resvd1           = 0;
	(**myDesc).resvd2           = 0;
	(**myDesc).dataRefIndex     = 0;
	(**myDesc).version          = 1;
	(**myDesc).revisionLevel    = 1;
	(**myDesc).vendor           = 28780;
	(**myDesc).temporalQuality  = 0;
	(**myDesc).spatialQuality   = codecLowQuality;
	(**myDesc).width            = (short)width;
	(**myDesc).height           = (short)height;
	(**myDesc).hRes             = 4718592;
	(**myDesc).vRes             = 4718592;
	(**myDesc).dataSize         = size;
	(**myDesc).frameCount       = 1;
	(**myDesc).depth            = 24;
	(**myDesc).clutID           = -1;

	rect.top = rect.left = 0;
	rect.bottom = (short)height;
	rect.right = (short)width;

	SetGWorld(worldPtr, NULL);

	tryqt( FDecompressImage( (Ptr)srcbuf, myDesc, pixMapHandle, &rect, NULL, srcCopy,
		NULL, NULL, NULL, codecLowQuality, bestSpeedCodec, 0, NULL, NULL) );

	fastpath = 0;
	switch((**pixMapHandle).pixelFormat)
	{
		case k24RGBPixelFormat:
			if(!(flags&TJ_BGR) && ps==3) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k32RGBAPixelFormat:
			if(!(flags&TJ_BGR) && !(flags&TJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k24BGRPixelFormat:
			if(flags&TJ_BGR && ps==3) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32BGRAPixelFormat:
			if(flags&TJ_BGR && !(flags&TJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32ABGRPixelFormat:
			if(flags&TJ_BGR && flags&TJ_ALPHAFIRST && ps==4) fastpath=1;
			qt_roffset=3;  qt_goffset=2;  qt_boffset=1;
			break;
		case k32ARGBPixelFormat:
			if(!(flags&TJ_BGR) && flags&TJ_ALPHAFIRST && ps==4) fastpath=1;
			qt_roffset=1;  qt_goffset=2;  qt_boffset=3;
			break;
		default:
			_throw("Quicktime returned a pixmap which isn't 24-bit or 32-bit");
	}
  
	rowptr = (unsigned char *)GetPixBaseAddr(pixMapHandle);
	rowbytes = GetPixRowBytes(pixMapHandle);

	/* Wish there was a way to do this without a buffer copy */
	if(fastpath)
	{
		int i, dststride=pitch;
		unsigned char *dstptr=dstbuf;
		if(flags&TJ_BOTTOMUP) {dstptr=&dstbuf[pitch*(height-1)];  dststride=-pitch;}
		for(i=0; i<height; ++i)
		{
			#if defined (__APPLE__)	

			/* 
			 * Use the mach kernel to execute a copy-on-write operation.
			 * Since we're only going to be reading from the buffer, this
			 * should be *far* less expensive than memcpy, if that copy 
		     * is large (>16 Kbytes)
			 */

			if(pitch > STRIDE_BOUNDARY)
				vm_copy(mach_task_self(), (vm_address_t)rowptr, pitch,
					(vm_address_t)dstptr);
			else
			#else
			memcpy(dstptr, rowptr, pitch); 
			#endif
			rowptr += rowbytes;
			dstptr += dststride;
		}
	}
	else
	{
		int roffset=0, goffset=1, boffset=2;
		int i, j, dststride=pitch;
		unsigned char *dstptr=dstbuf;
		int rowps=(**pixMapHandle).pixelSize/8;

		if(flags&TJ_BGR) {roffset=2;  goffset=1;  boffset=0;}
		if(flags&TJ_ALPHAFIRST) {roffset++;  goffset++;  boffset++;}
		if(flags&TJ_BOTTOMUP) {dstptr=&dstbuf[pitch*(height-1)];  dststride=-pitch;}
		for(i=0; i<height; ++i)
		{
			for(j=0; j<width; ++j)
			{
				dstptr[(ps*j)+roffset] = rowptr[(rowps*j)+qt_roffset];
				dstptr[(ps*j)+goffset] = rowptr[(rowps*j)+qt_goffset];
				dstptr[(ps*j)+boffset] = rowptr[(rowps*j)+qt_boffset];
			}
			rowptr += rowbytes;
			dstptr += dststride;
		}
	}

	if(myDesc)
	{
		HUnlock((Handle)myDesc);
		DisposeHandle((Handle)myDesc);
	}
	if(pixMapHandle) UnlockPixels(pixMapHandle);
	if(worldPtr) DisposeGWorld(worldPtr);
	return 0;

	bail:
	if(myDesc)
	{
		HUnlock((Handle)myDesc);
		DisposeHandle((Handle)myDesc);
	}
	if(pixMapHandle) UnlockPixels(pixMapHandle);
	if(worldPtr) DisposeGWorld(worldPtr);
	return -1;
    
}

DLLEXPORT int DLLCALL tjDestroy(tjhandle h)
{
	return 0;
}

DLLEXPORT char* DLLCALL tjGetErrorStr(void) 
{
	return lasterror;
}
