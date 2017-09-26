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

#ifndef __PF_H__
#define __PF_H__

/* Pixel formats */
#define PIXELFORMATS 7
enum { PF_RGB, PF_RGBX, PF_BGR, PF_BGRX, PF_XBGR, PF_XRGB, PF_COMP };


typedef struct _PF
{
	unsigned char id;
	const char *name;
	unsigned char size;
	unsigned int rmask, gmask, bmask;
	unsigned char rshift, gshift, bshift, rindex, gindex, bindex;
	/* These are useful mainly for initializing and comparing test patterns in
	   unit tests.  They are not particularly fast. */
	void (*getRGB)(unsigned char *, int *, int *, int *);
	void (*setRGB)(unsigned char *, int, int, int);
	void (*convert)(unsigned char *srcBuf, int width, int srcStride, int height,
		unsigned char *dstBuf, int dstStride, struct _PF dstpf);
} PF;


#ifdef __cplusplus
extern "C" {
#endif

PF pf_get(int id);

#ifdef __cplusplus
}
#endif

#endif /* __PF_H__ */
