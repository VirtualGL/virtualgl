#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <QuickTime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
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

#include "hpjpeg.h"

static char lasterror[96]="No error";

#define tryqt(f) {QDErr __err;  if((__err=(f))!=noErr) {  \
	sprintf(lasterror, "%d: Quicktime error: %d", __LINE__, __err);  return -1;}}
#define _throw(c) {sprintf(lasterror, "%d: %s", __LINE__, c);  goto bail;}


/******************************************************************
 *                        Compressor                              *
 ******************************************************************/


DLLEXPORT hpjhandle DLLCALL hpjInitCompress(void)
{
	#ifdef _WIN32
	InitializeQTML(0);
	#endif
	return (hpjhandle)1;
}


DLLEXPORT int DLLCALL hpjCompress(hpjhandle h,
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
		_throw("Invalid argument in hpjCompress()");
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
			if(!(flags&HPJ_BGR) && ps==3) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k32RGBAPixelFormat:
			if(!(flags&HPJ_BGR) && !(flags&HPJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k24BGRPixelFormat:
			if(flags&HPJ_BGR && ps==3) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32BGRAPixelFormat:
			if(flags&HPJ_BGR && !(flags&HPJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32ABGRPixelFormat:
			if(flags&HPJ_BGR && flags&HPJ_ALPHAFIRST && ps==4) fastpath=1;
			qt_roffset=3;  qt_goffset=2;  qt_boffset=1;
			break;
		case k32ARGBPixelFormat:
			if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4) fastpath=1;
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
		if(flags&HPJ_BOTTOMUP) {srcptr=&srcbuf[pitch*(height-1)];  srcstride=-pitch;}
		for(i=0; i<height; ++i)
		{
			memcpy(rowptr, srcptr, pitch); 
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

		if(flags&HPJ_BGR) {roffset=2;  goffset=1;  boffset=0;}
		if(flags&HPJ_ALPHAFIRST) {roffset++;  goffset++;  boffset++;}
		if(flags&HPJ_BOTTOMUP) {srcptr=&srcbuf[pitch*(height-1)];  srcstride=-pitch;}
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

	if(needSize > (long)HPJBUFSIZE(width, height))
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


DLLEXPORT hpjhandle DLLCALL hpjInitDecompress(void) 
{
	#ifdef _WIN32
	InitializeQTML(0);
	#endif
	return (hpjhandle)1;
}


DLLEXPORT int DLLCALL hpjDecompress(hpjhandle h,
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
		_throw("Invalid argument in hpjDecompress()");
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

	tryqt( DecompressImage( (Ptr)srcbuf, myDesc, pixMapHandle, &rect, &rect, srcCopy,
		NULL ) );

	fastpath = 0;
	switch((**pixMapHandle).pixelFormat)
	{
		case k24RGBPixelFormat:
			if(!(flags&HPJ_BGR) && ps==3) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k32RGBAPixelFormat:
			if(!(flags&HPJ_BGR) && !(flags&HPJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=0;  qt_goffset=1;  qt_boffset=2;
			break;
		case k24BGRPixelFormat:
			if(flags&HPJ_BGR && ps==3) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32BGRAPixelFormat:
			if(flags&HPJ_BGR && !(flags&HPJ_ALPHAFIRST) && ps==4) fastpath=1;
			qt_roffset=2;  qt_goffset=1;  qt_boffset=0;
			break;
		case k32ABGRPixelFormat:
			if(flags&HPJ_BGR && flags&HPJ_ALPHAFIRST && ps==4) fastpath=1;
			qt_roffset=3;  qt_goffset=2;  qt_boffset=1;
			break;
		case k32ARGBPixelFormat:
			if(!(flags&HPJ_BGR) && flags&HPJ_ALPHAFIRST && ps==4) fastpath=1;
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
		if(flags&HPJ_BOTTOMUP) {dstptr=&dstbuf[pitch*(height-1)];  dststride=-pitch;}
		for(i=0; i<height; ++i)
		{
			memcpy(dstptr, rowptr, pitch); 
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

		if(flags&HPJ_BGR) {roffset=2;  goffset=1;  boffset=0;}
		if(flags&HPJ_ALPHAFIRST) {roffset++;  goffset++;  boffset++;}
		if(flags&HPJ_BOTTOMUP) {dstptr=&dstbuf[pitch*(height-1)];  dststride=-pitch;}
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

DLLEXPORT int DLLCALL hpjDestroy(hpjhandle h)
{
	return 0;
}

DLLEXPORT char* DLLCALL hpjGetErrorStr(void) 
{
	return lasterror;
}
