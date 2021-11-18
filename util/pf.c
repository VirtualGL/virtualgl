/* Copyright (C)2017-2019, 2021 D. R. Commander
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

#include "pf.h"
#include "boost/endian.h"
#include "vglutil.h"
#include <string.h>


#define PF_RGB_SIZE          3
#define PF_RGB_RINDEX        0
#define PF_RGB_GINDEX        1
#define PF_RGB_BINDEX        2

#define PF_RGBX_SIZE         4
#ifdef BOOST_BIG_ENDIAN
#define PF_RGBX_RMASK        0xFF000000
#define PF_RGBX_GMASK        0x00FF0000
#define PF_RGBX_BMASK        0x0000FF00
#define PF_RGBX_RSHIFT       24
#define PF_RGBX_GSHIFT       16
#define PF_RGBX_BSHIFT       8
#else
#define PF_RGBX_RMASK        0x000000FF
#define PF_RGBX_GMASK        0x0000FF00
#define PF_RGBX_BMASK        0x00FF0000
#define PF_RGBX_RSHIFT       0
#define PF_RGBX_GSHIFT       8
#define PF_RGBX_BSHIFT       16
#endif
#define PF_RGBX_RINDEX       PF_RGB_RINDEX
#define PF_RGBX_GINDEX       PF_RGB_GINDEX
#define PF_RGBX_BINDEX       PF_RGB_BINDEX

#define PF_RGB10_X2_SIZE     PF_RGBX_SIZE
#ifdef BOOST_BIG_ENDIAN
#define PF_RGB10_X2_RMASK    0xFFC00000
#define PF_RGB10_X2_GMASK    0x003FF000
#define PF_RGB10_X2_BMASK    0x00000FFC
#define PF_RGB10_X2_RSHIFT   22
#define PF_RGB10_X2_GSHIFT   12
#define PF_RGB10_X2_BSHIFT   2
#else
#define PF_RGB10_X2_RMASK    0x000003FF
#define PF_RGB10_X2_GMASK    0x000FFC00
#define PF_RGB10_X2_BMASK    0x3FF00000
#define PF_RGB10_X2_RSHIFT   0
#define PF_RGB10_X2_GSHIFT   10
#define PF_RGB10_X2_BSHIFT   20
#endif
#define PF_RGB10_X2_RINDEX   0
#define PF_RGB10_X2_GINDEX   0
#define PF_RGB10_X2_BINDEX   0

#define PF_BGR_SIZE          PF_RGB_SIZE
#define PF_BGR_RINDEX        PF_RGB_BINDEX
#define PF_BGR_GINDEX        PF_RGB_GINDEX
#define PF_BGR_BINDEX        PF_RGB_RINDEX

#define PF_BGRX_SIZE         PF_RGBX_SIZE
#define PF_BGRX_RMASK        PF_RGBX_BMASK
#define PF_BGRX_GMASK        PF_RGBX_GMASK
#define PF_BGRX_BMASK        PF_RGBX_RMASK
#define PF_BGRX_RSHIFT       PF_RGBX_BSHIFT
#define PF_BGRX_GSHIFT       PF_RGBX_GSHIFT
#define PF_BGRX_BSHIFT       PF_RGBX_RSHIFT
#define PF_BGRX_RINDEX       PF_RGBX_BINDEX
#define PF_BGRX_GINDEX       PF_RGBX_GINDEX
#define PF_BGRX_BINDEX       PF_RGBX_RINDEX

#define PF_BGR10_X2_SIZE     PF_RGB10_X2_SIZE
#define PF_BGR10_X2_RMASK    PF_RGB10_X2_BMASK
#define PF_BGR10_X2_GMASK    PF_RGB10_X2_GMASK
#define PF_BGR10_X2_BMASK    PF_RGB10_X2_RMASK
#define PF_BGR10_X2_RSHIFT   PF_RGB10_X2_BSHIFT
#define PF_BGR10_X2_GSHIFT   PF_RGB10_X2_GSHIFT
#define PF_BGR10_X2_BSHIFT   PF_RGB10_X2_RSHIFT
#define PF_BGR10_X2_RINDEX   PF_RGB10_X2_BINDEX
#define PF_BGR10_X2_GINDEX   PF_RGB10_X2_GINDEX
#define PF_BGR10_X2_BINDEX   PF_RGB10_X2_RINDEX

#define PF_XBGR_SIZE         PF_RGBX_SIZE
#ifdef BOOST_BIG_ENDIAN
#define PF_XBGR_RMASK        0x000000FF
#define PF_XBGR_GMASK        0x0000FF00
#define PF_XBGR_BMASK        0x00FF0000
#define PF_XBGR_RSHIFT       0
#define PF_XBGR_GSHIFT       8
#define PF_XBGR_BSHIFT       16
#else
#define PF_XBGR_RMASK        0xFF000000
#define PF_XBGR_GMASK        0x00FF0000
#define PF_XBGR_BMASK        0x0000FF00
#define PF_XBGR_RSHIFT       24
#define PF_XBGR_GSHIFT       16
#define PF_XBGR_BSHIFT       8
#endif
#define PF_XBGR_RINDEX       3
#define PF_XBGR_GINDEX       2
#define PF_XBGR_BINDEX       1

#define PF_X2_BGR10_SIZE     PF_RGB10_X2_SIZE
#ifdef BOOST_BIG_ENDIAN
#define PF_X2_BGR10_RMASK    0x000003FF
#define PF_X2_BGR10_GMASK    0x000FFC00
#define PF_X2_BGR10_BMASK    0x3FF00000
#define PF_X2_BGR10_RSHIFT   0
#define PF_X2_BGR10_GSHIFT   10
#define PF_X2_BGR10_BSHIFT   20
#else
#define PF_X2_BGR10_RMASK    0xFFC00000
#define PF_X2_BGR10_GMASK    0x003FF000
#define PF_X2_BGR10_BMASK    0x00000FFC
#define PF_X2_BGR10_RSHIFT   22
#define PF_X2_BGR10_GSHIFT   12
#define PF_X2_BGR10_BSHIFT   2
#endif
#define PF_X2_BGR10_RINDEX   0
#define PF_X2_BGR10_GINDEX   0
#define PF_X2_BGR10_BINDEX   0

#define PF_XRGB_SIZE         PF_XBGR_SIZE
#define PF_XRGB_RMASK        PF_XBGR_BMASK
#define PF_XRGB_GMASK        PF_XBGR_GMASK
#define PF_XRGB_BMASK        PF_XBGR_RMASK
#define PF_XRGB_RSHIFT       PF_XBGR_BSHIFT
#define PF_XRGB_GSHIFT       PF_XBGR_GSHIFT
#define PF_XRGB_BSHIFT       PF_XBGR_RSHIFT
#define PF_XRGB_RINDEX       PF_XBGR_BINDEX
#define PF_XRGB_GINDEX       PF_XBGR_GINDEX
#define PF_XRGB_BINDEX       PF_XBGR_RINDEX

#define PF_X2_RGB10_SIZE     PF_X2_BGR10_SIZE
#define PF_X2_RGB10_RMASK    PF_X2_BGR10_BMASK
#define PF_X2_RGB10_GMASK    PF_X2_BGR10_GMASK
#define PF_X2_RGB10_BMASK    PF_X2_BGR10_RMASK
#define PF_X2_RGB10_RSHIFT   PF_X2_BGR10_BSHIFT
#define PF_X2_RGB10_GSHIFT   PF_X2_BGR10_GSHIFT
#define PF_X2_RGB10_BSHIFT   PF_X2_BGR10_RSHIFT
#define PF_X2_RGB10_RINDEX   PF_X2_BGR10_BINDEX
#define PF_X2_RGB10_GINDEX   PF_X2_BGR10_GINDEX
#define PF_X2_RGB10_BINDEX   PF_X2_BGR10_RINDEX


#define CONVERT_FAST(id) \
{ \
	int wps = width * PF_##id##_SIZE; \
	while(height--) \
	{ \
		memcpy(dstBuf, srcBuf, wps); \
		srcBuf += srcStride;  dstBuf += dstStride; \
	} \
	return; \
}

#define CONVERT_PF4I(srcid, dstid, sshift, dshift) \
{ \
	while(height--) \
	{ \
		int w = width; \
		unsigned int *srcPixeli = (unsigned int *)srcBuf; \
		unsigned int *dstPixeli = (unsigned int *)dstBuf; \
		while(w--) \
		{ \
			*dstPixeli = \
				(((*srcPixeli & PF_##srcid##_RMASK) >> \
					(PF_##srcid##_RSHIFT + sshift)) << \
					(PF_##dstid##_RSHIFT + dshift)) | \
				(((*srcPixeli & PF_##srcid##_GMASK) >> \
					(PF_##srcid##_GSHIFT + sshift)) << \
					(PF_##dstid##_GSHIFT + dshift)) | \
				(((*srcPixeli & PF_##srcid##_BMASK) >> \
					(PF_##srcid##_BSHIFT + sshift)) << \
					(PF_##dstid##_BSHIFT + dshift)); \
				srcPixeli++;  dstPixeli++; \
		} \
		srcBuf += srcStride;  dstBuf += dstStride; \
	} \
	return; \
}

#define CONVERT_PF4I2C(srcid, dstid) \
{ \
	while(height--) \
	{ \
		int w = width; \
		unsigned int *srcPixeli = (unsigned int *)srcBuf; \
		unsigned char *dstPixel = dstBuf; \
		while(w--) \
		{ \
			dstPixel[PF_##dstid##_RINDEX] = \
				(*srcPixeli >> (PF_##srcid##_RSHIFT + 2)); \
			dstPixel[PF_##dstid##_GINDEX] = \
				(*srcPixeli >> (PF_##srcid##_GSHIFT + 2)); \
			dstPixel[PF_##dstid##_BINDEX] = \
				(*srcPixeli >> (PF_##srcid##_BSHIFT + 2)); \
			srcPixeli++;  dstPixel += PF_##dstid##_SIZE; \
		} \
		srcBuf += srcStride;  dstBuf += dstStride; \
	} \
	return; \
}

#define CONVERT_PF4C2I(srcid, dstid) \
{ \
	while(height--) \
	{ \
		int w = width; \
		unsigned int *dstPixeli = (unsigned int *)dstBuf; \
		unsigned char *srcPixel = srcBuf; \
		while(w--) \
		{ \
			*dstPixeli = \
				(unsigned int)srcPixel[PF_##srcid##_RINDEX] << \
				(PF_##dstid##_RSHIFT + 2); \
			*dstPixeli |= \
				(unsigned int)srcPixel[PF_##srcid##_GINDEX] << \
				(PF_##dstid##_GSHIFT + 2); \
			*dstPixeli |= \
				(unsigned int)srcPixel[PF_##srcid##_BINDEX] << \
				(PF_##dstid##_BSHIFT + 2); \
			srcPixel += PF_##srcid##_SIZE;  dstPixeli++; \
		} \
		srcBuf += srcStride;  dstBuf += dstStride; \
	} \
	return; \
}

#if __BITS == 64
#define CONVERT_RGB(srcid, dstid) \
{ \
	while(height--) \
	{ \
		int w = width; \
		unsigned char *srcPixel = srcBuf + \
			min(PF_##srcid##_RINDEX, PF_##srcid##_BINDEX); \
		unsigned char *dstPixel = dstBuf + \
			min(PF_##dstid##_RINDEX, PF_##dstid##_BINDEX); \
		while(w--) \
		{ \
			memcpy(dstPixel, srcPixel, 3); \
			srcPixel += PF_##srcid##_SIZE;  dstPixel += PF_##dstid##_SIZE; \
		} \
		srcBuf += srcStride;  dstBuf += dstStride; \
	} \
	return; \
}
#else
#define CONVERT_RGB  CONVERT_BGR
#endif

#define CONVERT_BGR(srcid, dstid) \
{ \
	while(height--) \
	{ \
		int w = width; \
		unsigned char *srcPixel = srcBuf; \
		unsigned char *dstPixel = dstBuf; \
		while(w--) \
		{ \
			dstPixel[PF_##dstid##_RINDEX] = srcPixel[PF_##srcid##_RINDEX]; \
			dstPixel[PF_##dstid##_GINDEX] = srcPixel[PF_##srcid##_GINDEX]; \
			dstPixel[PF_##dstid##_BINDEX] = srcPixel[PF_##srcid##_BINDEX]; \
			srcPixel += PF_##srcid##_SIZE;  dstPixel += PF_##dstid##_SIZE; \
		} \
		srcBuf += srcStride;  dstBuf += dstStride; \
	} \
	return; \
}

#if __BITS == 64
#define CONVERT_PF4CRGB(srcid, dstid)  CONVERT_PF4I(srcid, dstid, 0, 0)
#define CONVERT_PF4CBGR(srcid, dstid)  CONVERT_PF4I(srcid, dstid, 0, 0)
#else
#define CONVERT_PF4CRGB  CONVERT_RGB
#define CONVERT_PF4CBGR  CONVERT_BGR
#endif

static INLINE void convert_RGB(unsigned char *srcBuf, int width, int srcStride,
	int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_FAST(RGB)
		case PF_RGBX:      CONVERT_RGB(RGB, RGBX)
		case PF_RGB10_X2:  CONVERT_PF4C2I(RGB, RGB10_X2)
		case PF_BGR:       CONVERT_BGR(RGB, BGR)
		case PF_BGRX:      CONVERT_BGR(RGB, BGRX)
		case PF_BGR10_X2:  CONVERT_PF4C2I(RGB, BGR10_X2)
		case PF_XBGR:      CONVERT_BGR(RGB, XBGR)
		case PF_X2_BGR10:  CONVERT_PF4C2I(RGB, X2_BGR10)
		case PF_XRGB:      CONVERT_RGB(RGB, XRGB)
		case PF_X2_RGB10:  CONVERT_PF4C2I(RGB, X2_RGB10)
	}
}

static INLINE void convert_RGBX(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_RGB(RGBX, RGB)
		case PF_RGBX:      CONVERT_FAST(RGBX)
		case PF_RGB10_X2:  CONVERT_PF4I(RGBX, RGB10_X2, 0, 2)
		case PF_BGR:       CONVERT_BGR(RGBX, BGR)
		case PF_BGRX:      CONVERT_PF4CBGR(RGBX, BGRX)
		case PF_BGR10_X2:  CONVERT_PF4I(RGBX, BGR10_X2, 0, 2)
		case PF_XBGR:      CONVERT_PF4CBGR(RGBX, XBGR)
		case PF_X2_BGR10:  CONVERT_PF4I(RGBX, X2_BGR10, 0, 2)
		case PF_XRGB:      CONVERT_PF4CRGB(RGBX, XRGB)
		case PF_X2_RGB10:  CONVERT_PF4I(RGBX, X2_RGB10, 0, 2)
	}
}

static INLINE void convert_RGB10_X2(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_PF4I2C(RGB10_X2, RGB)
		case PF_RGBX:      CONVERT_PF4I(RGB10_X2, RGBX, 2, 0)
		case PF_RGB10_X2:  CONVERT_FAST(RGB10_X2)
		case PF_BGR:       CONVERT_PF4I2C(RGB10_X2, BGR)
		case PF_BGRX:      CONVERT_PF4I(RGB10_X2, BGRX, 2, 0)
		case PF_BGR10_X2:  CONVERT_PF4I(RGB10_X2, BGR10_X2, 0, 0)
		case PF_XBGR:      CONVERT_PF4I(RGB10_X2, XBGR, 2, 0)
		case PF_X2_BGR10:  CONVERT_PF4I(RGB10_X2, X2_BGR10, 0, 0)
		case PF_XRGB:      CONVERT_PF4I(RGB10_X2, XRGB, 2, 0)
		case PF_X2_RGB10:  CONVERT_PF4I(RGB10_X2, X2_RGB10, 0, 0)
	}
}

static INLINE void convert_BGR(unsigned char *srcBuf, int width, int srcStride,
	int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_BGR(BGR, RGB)
		case PF_RGBX:      CONVERT_BGR(BGR, RGBX)
		case PF_RGB10_X2:  CONVERT_PF4C2I(BGR, RGB10_X2)
		case PF_BGR:       CONVERT_FAST(BGR)
		case PF_BGRX:      CONVERT_RGB(BGR, BGRX)
		case PF_BGR10_X2:  CONVERT_PF4C2I(BGR, BGR10_X2)
		case PF_XBGR:      CONVERT_RGB(BGR, XBGR)
		case PF_X2_BGR10:  CONVERT_PF4C2I(BGR, X2_BGR10)
		case PF_XRGB:      CONVERT_BGR(BGR, XRGB)
		case PF_X2_RGB10:  CONVERT_PF4C2I(BGR, X2_RGB10)
	}
}

static INLINE void convert_BGRX(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_BGR(BGRX, RGB)
		case PF_RGBX:      CONVERT_PF4CBGR(BGRX, RGBX)
		case PF_RGB10_X2:  CONVERT_PF4I(BGRX, RGB10_X2, 0, 2)
		case PF_BGR:       CONVERT_RGB(BGRX, BGR)
		case PF_BGRX:      CONVERT_FAST(BGRX)
		case PF_BGR10_X2:  CONVERT_PF4I(BGRX, BGR10_X2, 0, 2)
		case PF_XBGR:      CONVERT_PF4CRGB(BGRX, XBGR)
		case PF_X2_BGR10:  CONVERT_PF4I(BGRX, X2_BGR10, 0, 2)
		case PF_XRGB:      CONVERT_PF4CBGR(BGRX, XRGB)
		case PF_X2_RGB10:  CONVERT_PF4I(BGRX, X2_RGB10, 0, 2)
	}
}

static INLINE void convert_BGR10_X2(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_PF4I2C(BGR10_X2, RGB)
		case PF_RGBX:      CONVERT_PF4I(BGR10_X2, RGBX, 2, 0)
		case PF_RGB10_X2:  CONVERT_PF4I(BGR10_X2, RGB10_X2, 0, 0)
		case PF_BGR:       CONVERT_PF4I2C(BGR10_X2, BGR)
		case PF_BGRX:      CONVERT_PF4I(BGR10_X2, BGRX, 2, 0)
		case PF_BGR10_X2:  CONVERT_FAST(BGR10_X2)
		case PF_XBGR:      CONVERT_PF4I(BGR10_X2, XBGR, 2, 0)
		case PF_X2_BGR10:  CONVERT_PF4I(BGR10_X2, X2_BGR10, 0, 0)
		case PF_XRGB:      CONVERT_PF4I(BGR10_X2, XRGB, 2, 0)
		case PF_X2_RGB10:  CONVERT_PF4I(BGR10_X2, X2_RGB10, 0, 0)
	}
}

static INLINE void convert_XBGR(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_BGR(XBGR, RGB)
		case PF_RGBX:      CONVERT_PF4CBGR(XBGR, RGBX)
		case PF_RGB10_X2:  CONVERT_PF4I(XBGR, RGB10_X2, 0, 2)
		case PF_BGR:       CONVERT_RGB(XBGR, BGR)
		case PF_BGRX:      CONVERT_PF4CRGB(XBGR, BGRX)
		case PF_BGR10_X2:  CONVERT_PF4I(XBGR, BGR10_X2, 0, 2)
		case PF_XBGR:      CONVERT_FAST(XBGR)
		case PF_X2_BGR10:  CONVERT_PF4I(XBGR, X2_BGR10, 0, 2)
		case PF_XRGB:      CONVERT_PF4CBGR(XBGR, XRGB)
		case PF_X2_RGB10:  CONVERT_PF4I(XBGR, X2_RGB10, 0, 2)
	}
}

static INLINE void convert_X2_BGR10(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_PF4I2C(X2_BGR10, RGB)
		case PF_RGBX:      CONVERT_PF4I(X2_BGR10, RGBX, 2, 0)
		case PF_RGB10_X2:  CONVERT_PF4I(X2_BGR10, RGB10_X2, 0, 0)
		case PF_BGR:       CONVERT_PF4I2C(X2_BGR10, BGR)
		case PF_BGRX:      CONVERT_PF4I(X2_BGR10, BGRX, 2, 0)
		case PF_BGR10_X2:  CONVERT_PF4I(X2_BGR10, BGR10_X2, 0, 0)
		case PF_XBGR:      CONVERT_PF4I(X2_BGR10, XBGR, 2, 0)
		case PF_X2_BGR10:  CONVERT_FAST(X2_BGR10)
		case PF_XRGB:      CONVERT_PF4I(X2_BGR10, XRGB, 2, 0)
		case PF_X2_RGB10:  CONVERT_PF4I(X2_BGR10, X2_RGB10, 0, 0)
	}
}

static INLINE void convert_XRGB(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_RGB(XRGB, RGB)
		case PF_RGBX:      CONVERT_PF4CRGB(XRGB, RGBX)
		case PF_RGB10_X2:  CONVERT_PF4I(XRGB, RGB10_X2, 0, 2)
		case PF_BGR:       CONVERT_BGR(XRGB, BGR)
		case PF_BGRX:      CONVERT_PF4CBGR(XRGB, BGRX)
		case PF_BGR10_X2:  CONVERT_PF4I(XRGB, BGR10_X2, 0, 2)
		case PF_XBGR:      CONVERT_PF4CBGR(XRGB, XBGR)
		case PF_X2_BGR10:  CONVERT_PF4I(XRGB, X2_BGR10, 0, 2)
		case PF_XRGB:      CONVERT_FAST(XRGB)
		case PF_X2_RGB10:  CONVERT_PF4I(XRGB, X2_RGB10, 0, 2)
	}
}

static INLINE void convert_X2_RGB10(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	if(dstpf) switch(dstpf->id)
	{
		case PF_RGB:       CONVERT_PF4I2C(X2_RGB10, RGB)
		case PF_RGBX:      CONVERT_PF4I(X2_RGB10, RGBX, 2, 0)
		case PF_RGB10_X2:  CONVERT_PF4I(X2_RGB10, RGB10_X2, 0, 0)
		case PF_BGR:       CONVERT_PF4I2C(X2_RGB10, BGR)
		case PF_BGRX:      CONVERT_PF4I(X2_RGB10, BGRX, 2, 0)
		case PF_BGR10_X2:  CONVERT_PF4I(X2_RGB10, BGR10_X2, 0, 0)
		case PF_XBGR:      CONVERT_PF4I(X2_RGB10, XBGR, 2, 0)
		case PF_X2_BGR10:  CONVERT_PF4I(X2_RGB10, X2_BGR10, 0, 0)
		case PF_XRGB:      CONVERT_PF4I(X2_RGB10, XRGB, 2, 0)
		case PF_X2_RGB10:  CONVERT_FAST(X2_RGB10)
	}
}


#define DEFINE_PF4C(id) \
static INLINE void getRGB_##id(unsigned char *pixel, int *r, int *g, int *b) \
{ \
	*r = pixel[PF_##id##_RINDEX];  *g = pixel[PF_##id##_GINDEX]; \
	*b = pixel[PF_##id##_BINDEX]; \
} \
\
static INLINE void setRGB_##id(unsigned char *pixel, int r, int g, int b) \
{ \
	unsigned int *p = (unsigned int *)pixel; \
	*p = ((unsigned int)r << PF_##id##_RSHIFT) | \
		((unsigned int)g << PF_##id##_GSHIFT) | \
		((unsigned int)b << PF_##id##_BSHIFT); \
} \
\
static PF __format_##id = \
{ \
	PF_##id, #id, PF_##id##_SIZE, 8, PF_##id##_RMASK, PF_##id##_GMASK, \
		PF_##id##_BMASK, PF_##id##_RSHIFT, PF_##id##_GSHIFT, PF_##id##_BSHIFT, \
		PF_##id##_RINDEX, PF_##id##_GINDEX, PF_##id##_BINDEX, getRGB_##id, \
		setRGB_##id, convert_##id \
};

DEFINE_PF4C(RGBX)
DEFINE_PF4C(BGRX)
DEFINE_PF4C(XBGR)
DEFINE_PF4C(XRGB)


#define DEFINE_PF4(id) \
static INLINE void getRGB_##id(unsigned char *pixel, int *r, int *g, int *b) \
{ \
	unsigned int *p = (unsigned int *)pixel; \
	*r = ((*p) & PF_##id##_RMASK) >> PF_##id##_RSHIFT; \
	*g = ((*p) & PF_##id##_GMASK) >> PF_##id##_GSHIFT; \
	*b = ((*p) & PF_##id##_BMASK) >> PF_##id##_BSHIFT; \
} \
\
static INLINE void setRGB_##id(unsigned char *pixel, int r, int g, int b) \
{ \
	unsigned int *p = (unsigned int *)pixel; \
	*p = ((unsigned int)r << PF_##id##_RSHIFT) | \
		((unsigned int)g << PF_##id##_GSHIFT) | \
		((unsigned int)b << PF_##id##_BSHIFT); \
} \
\
static PF __format_##id = \
{ \
	PF_##id, #id, PF_##id##_SIZE, 10, PF_##id##_RMASK, PF_##id##_GMASK, \
		PF_##id##_BMASK, PF_##id##_RSHIFT, PF_##id##_GSHIFT, PF_##id##_BSHIFT, \
		PF_##id##_RINDEX, PF_##id##_GINDEX, PF_##id##_BINDEX, getRGB_##id, \
		setRGB_##id, convert_##id \
};

DEFINE_PF4(RGB10_X2)
DEFINE_PF4(BGR10_X2)
DEFINE_PF4(X2_BGR10)
DEFINE_PF4(X2_RGB10)


#define DEFINE_PF3(id) \
static INLINE void setRGB_##id(unsigned char *pixel, int r, int g, int b) \
{ \
	pixel[PF_##id##_RINDEX] = r;  pixel[PF_##id##_GINDEX] = g; \
	pixel[PF_##id##_BINDEX] = b; \
} \
\
static PF __format_##id = \
{ \
	PF_##id, #id, 3, 8, 0, 0, 0, 0, 0, 0, PF_##id##_RINDEX, PF_##id##_GINDEX, \
		PF_##id##_BINDEX, getRGB_##id##X, setRGB_##id, convert_##id \
};

DEFINE_PF3(RGB)
DEFINE_PF3(BGR)


static INLINE void getRGB_COMP(unsigned char *pixel, int *r, int *g, int *b)
{
	*r = *g = *b = *pixel;
}

static INLINE void setRGB_COMP(unsigned char *pixel, int r, int g, int b)
{
	*pixel = r;
}

static INLINE void convert_COMP(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF *dstpf)
{
	/* VirtualGL doesn't ever need to convert between component and RGB */
}

static PF __format_COMP =
{
	PF_COMP, "COMP", 1, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, getRGB_COMP, setRGB_COMP,
		convert_COMP
};


static PF __format_NONE =
{
	0, "Invalid", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, getRGB_COMP, setRGB_COMP,
		convert_COMP
};


PF *pf_get(int id)
{
	switch(id)
	{
		case PF_RGB:  return &__format_RGB;
		case PF_RGBX:  return &__format_RGBX;
		case PF_RGB10_X2:  return &__format_RGB10_X2;
		case PF_BGR:  return &__format_BGR;
		case PF_BGRX:  return &__format_BGRX;
		case PF_BGR10_X2:  return &__format_BGR10_X2;
		case PF_XBGR:  return &__format_XBGR;
		case PF_X2_BGR10:  return &__format_X2_BGR10;
		case PF_XRGB:  return &__format_XRGB;
		case PF_X2_RGB10:  return &__format_X2_RGB10;
		case PF_COMP:  return &__format_COMP;
		default:  return &__format_NONE;
	}
}
