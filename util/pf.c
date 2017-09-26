/* Copyright (C)2017 D. R. Commander
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
#include "boost/endian.hpp"
#include "vglutil.h"
#include <string.h>


#ifdef sun
#define __inline inline
#endif


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


#define CONVERT_FAST(id)  \
{  \
	int wps=width*PF_##id##_SIZE;  \
	while(height--)  \
	{  \
		memcpy(dstBuf, srcBuf, wps);  \
		srcBuf+=srcStride;  dstBuf+=dstStride;  \
	}  \
	return;  \
}

#define CONVERT_PF4I(srcid, dstid)  \
{  \
	while(height--)  \
	{  \
		int w=width;  \
		unsigned int *srcPixeli=(unsigned int *)srcBuf;  \
		unsigned int *dstPixeli=(unsigned int *)dstBuf;  \
		while(w--)  \
		{  \
			*dstPixeli=  \
				(((*srcPixeli&PF_##srcid##_RMASK)>>PF_##srcid##_RSHIFT)<<  \
					PF_##dstid##_RSHIFT) |  \
				(((*srcPixeli&PF_##srcid##_GMASK)>>PF_##srcid##_GSHIFT)<<  \
					PF_##dstid##_GSHIFT) |  \
				(((*srcPixeli&PF_##srcid##_BMASK)>>PF_##srcid##_BSHIFT)<<  \
					PF_##dstid##_BSHIFT);  \
				srcPixeli++;  dstPixeli++;  \
		}  \
		srcBuf+=srcStride;  dstBuf+=dstStride;  \
	}  \
	return;  \
}

#if __BITS==64
#define CONVERT_RGB(srcid, dstid)  \
{  \
	while(height--)  \
	{  \
		int w=width;  \
		unsigned char *srcPixel=srcBuf+  \
			min(PF_##srcid##_RINDEX, PF_##srcid##_BINDEX);  \
		unsigned char *dstPixel=dstBuf+  \
			min(PF_##dstid##_RINDEX, PF_##dstid##_BINDEX);  \
		while(w--)  \
		{  \
			memcpy(dstPixel, srcPixel, 3);  \
			srcPixel+=PF_##srcid##_SIZE;  dstPixel+=PF_##dstid##_SIZE;  \
		}  \
		srcBuf+=srcStride;  dstBuf+=dstStride;  \
	}  \
	return;  \
}
#else
#define CONVERT_RGB CONVERT_BGR
#endif

#define CONVERT_BGR(srcid, dstid)  \
{  \
	while(height--)  \
	{  \
		int w=width;  \
		unsigned char *srcPixel=srcBuf;  \
		unsigned char *dstPixel=dstBuf;  \
		while(w--)  \
		{  \
			dstPixel[PF_##dstid##_RINDEX]=srcPixel[PF_##srcid##_RINDEX];  \
			dstPixel[PF_##dstid##_GINDEX]=srcPixel[PF_##srcid##_GINDEX];  \
			dstPixel[PF_##dstid##_BINDEX]=srcPixel[PF_##srcid##_BINDEX];  \
			srcPixel+=PF_##srcid##_SIZE;  dstPixel+=PF_##dstid##_SIZE;  \
		}  \
		srcBuf+=srcStride;  dstBuf+=dstStride;  \
	}  \
	return;  \
}

#if __BITS==64
#define CONVERT_PF4CRGB(srcid, dstid) CONVERT_PF4I(srcid, dstid)
#define CONVERT_PF4CBGR(srcid, dstid) CONVERT_PF4I(srcid, dstid)
#else
#define CONVERT_PF4CRGB CONVERT_RGB
#define CONVERT_PF4CBGR CONVERT_BGR
#endif

static __inline void convert_RGB(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	switch(dstpf.id)
	{
		case PF_RGB:       CONVERT_FAST(RGB)
		case PF_RGBX:      CONVERT_RGB(RGB, RGBX)
		case PF_BGR:       CONVERT_BGR(RGB, BGR)
		case PF_BGRX:      CONVERT_BGR(RGB, BGRX)
		case PF_XBGR:      CONVERT_BGR(RGB, XBGR)
		case PF_XRGB:      CONVERT_RGB(RGB, XRGB)
	}
}

static __inline void convert_RGBX(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	switch(dstpf.id)
	{
		case PF_RGB:       CONVERT_RGB(RGBX, RGB)
		case PF_RGBX:      CONVERT_FAST(RGBX)
		case PF_BGR:       CONVERT_BGR(RGBX, BGR)
		case PF_BGRX:      CONVERT_PF4CBGR(RGBX, BGRX)
		case PF_XBGR:      CONVERT_PF4CBGR(RGBX, XBGR)
		case PF_XRGB:      CONVERT_PF4CRGB(RGBX, XRGB)
	}
}

static __inline void convert_BGR(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	switch(dstpf.id)
	{
		case PF_RGB:       CONVERT_BGR(BGR, RGB)
		case PF_RGBX:      CONVERT_BGR(BGR, RGBX)
		case PF_BGR:       CONVERT_FAST(BGR)
		case PF_BGRX:      CONVERT_RGB(BGR, BGRX)
		case PF_XBGR:      CONVERT_RGB(BGR, XBGR)
		case PF_XRGB:      CONVERT_BGR(BGR, XRGB)
	}
}

static __inline void convert_BGRX(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	switch(dstpf.id)
	{
		case PF_RGB:       CONVERT_BGR(BGRX, RGB)
		case PF_RGBX:      CONVERT_PF4CBGR(BGRX, RGBX)
		case PF_BGR:       CONVERT_RGB(BGRX, BGR)
		case PF_BGRX:      CONVERT_FAST(BGRX)
		case PF_XBGR:      CONVERT_PF4CRGB(BGRX, XBGR)
		case PF_XRGB:      CONVERT_PF4CBGR(BGRX, XRGB)
	}
}

static __inline void convert_XBGR(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	switch(dstpf.id)
	{
		case PF_RGB:       CONVERT_BGR(XBGR, RGB)
		case PF_RGBX:      CONVERT_PF4CBGR(XBGR, RGBX)
		case PF_BGR:       CONVERT_RGB(XBGR, BGR)
		case PF_BGRX:      CONVERT_PF4CRGB(XBGR, BGRX)
		case PF_XBGR:      CONVERT_FAST(XBGR)
		case PF_XRGB:      CONVERT_PF4CBGR(XBGR, XRGB)
	}
}

static __inline void convert_XRGB(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	switch(dstpf.id)
	{
		case PF_RGB:       CONVERT_RGB(XRGB, RGB)
		case PF_RGBX:      CONVERT_PF4CRGB(XRGB, RGBX)
		case PF_BGR:       CONVERT_BGR(XRGB, BGR)
		case PF_BGRX:      CONVERT_PF4CBGR(XRGB, BGRX)
		case PF_XBGR:      CONVERT_PF4CBGR(XRGB, XBGR)
		case PF_XRGB:      CONVERT_FAST(XRGB)
	}
}


#define DEFINE_PF4(id)  \
static __inline void getRGB_##id(unsigned char *pixel, int *r, int *g,  \
	int *b)  \
{  \
	*r=pixel[PF_##id##_RINDEX];  *g=pixel[PF_##id##_GINDEX];  \
  *b=pixel[PF_##id##_BINDEX];  \
}  \
  \
static __inline void setRGB_##id(unsigned char *pixel, int r, int g, int b)  \
{  \
	unsigned int *p=(unsigned int *)pixel;  \
	*p=(r<<PF_##id##_RSHIFT) | (g<<PF_##id##_GSHIFT) |  \
		(b<<PF_##id##_BSHIFT);  \
}  \
  \
static const PF __format_##id=  \
{  \
	PF_##id, #id, PF_##id##_SIZE, PF_##id##_RMASK, PF_##id##_GMASK,  \
		PF_##id##_BMASK, PF_##id##_RSHIFT, PF_##id##_GSHIFT, PF_##id##_BSHIFT,  \
		PF_##id##_RINDEX, PF_##id##_GINDEX, PF_##id##_BINDEX, getRGB_##id,  \
		setRGB_##id, convert_##id  \
};

DEFINE_PF4(RGBX)
DEFINE_PF4(BGRX)
DEFINE_PF4(XBGR)
DEFINE_PF4(XRGB)


#define DEFINE_PF3(id)  \
static __inline void setRGB_##id(unsigned char *pixel, int r, int g, int b)  \
{  \
	pixel[PF_##id##_RINDEX]=r;  pixel[PF_##id##_GINDEX]=g;  \
	pixel[PF_##id##_BINDEX]=b;  \
}  \
  \
static const PF __format_##id=  \
{  \
	PF_##id, #id, 3, 0, 0, 0, 0, 0, 0, PF_##id##_RINDEX, PF_##id##_GINDEX,  \
		PF_##id##_BINDEX, getRGB_##id##X, setRGB_##id, convert_##id  \
};

DEFINE_PF3(RGB)
DEFINE_PF3(BGR)


static __inline void getRGB_COMP(unsigned char *pixel, int *r, int *g, int *b)
{
	*r=*g=*b=*pixel;
}

static __inline void setRGB_COMP(unsigned char *pixel, int r, int g, int b)
{
	*pixel=r;
}

static __inline void convert_COMP(unsigned char *srcBuf, int width,
	int srcStride, int height, unsigned char *dstBuf, int dstStride, PF dstpf)
{
	/* VirtualGL doesn't ever need to convert between component and RGB */
}

static const PF __format_COMP=
{
	PF_COMP, "COMP", 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, getRGB_COMP, setRGB_COMP,
		convert_COMP
};


PF pf_get(int id)
{
	switch(id)
	{
		case PF_RGB:  return __format_RGB;  break;
		case PF_RGBX:  return __format_RGBX;  break;
		case PF_BGR:  return __format_BGR;  break;
		case PF_BGRX:  return __format_BGRX;  break;
		case PF_XBGR:  return __format_XBGR;  break;
		case PF_XRGB:  return __format_XRGB;  break;
		case PF_COMP:  return __format_COMP;  break;
		default:
		{
			PF pf;
			memset(&pf, 0, sizeof(PF));
			pf.name="Invalid";
			pf.getRGB=getRGB_COMP;  pf.setRGB=setRGB_COMP;
			return pf;
		}
	}
}
